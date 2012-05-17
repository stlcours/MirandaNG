#include "headers.h"
#include <time.h>
#include <math.h>

int __cdecl CContactCache::OnDbEventAdded(WPARAM wParam, LPARAM lParam)
{
	HANDLE hContact = (HANDLE)wParam;
	HANDLE hEvent = (HANDLE)lParam;

	DBEVENTINFO dbei = {0};
	dbei.cbSize = sizeof(dbei);
	CallService(MS_DB_EVENT_GET, (WPARAM)hEvent, (LPARAM)&dbei);
	if (dbei.eventType != EVENTTYPE_MESSAGE) return 0;

	float weight = GetEventWeight(time(NULL) - dbei.timestamp);
	float q = GetTimeWeight(time(NULL) - m_lastUpdate);
	m_lastUpdate = time(NULL);
	if (!weight) return 0;

	Lock();
	bool found = false;
	for (int i = 0; i < m_cache.getCount(); ++i)
	{
		m_cache[i].rate *= q;
		if (m_cache[i].hContact == hContact)
		{
			found = true;
			m_cache[i].rate += weight;
		}
	}

	if (!found)
	{
		TContactInfo *info = new TContactInfo;
		info->hContact = hContact;
		info->rate = weight;
		m_cache.insert(info);
	} else
	{
		qsort(m_cache.getArray(), m_cache.getCount(), sizeof(TContactInfo *), TContactInfo::cmp2);
	}

	Unlock();
	return 0;
}

float CContactCache::GetEventWeight(unsigned long age)
{
	const float ceil = 1000.f;
	const float floor = 0.0001f;
	const int depth = 60 * 60 * 24 * 30;
	if (age > depth) return 0;
	return exp(log(ceil) - age * (log(ceil) - log(floor)) / depth);
}

float CContactCache::GetTimeWeight(unsigned long age)
{
	const float ceil = 1000.f;
	const float floor = 0.0001f;
	const int depth = 60 * 60 * 24 * 30;
	if (age > depth) return 0;
	return exp(age * (log(ceil) - log(floor)) / depth);
}

CContactCache::CContactCache(): m_cache(50, TContactInfo::cmp)
{
	InitializeCriticalSection(&m_cs);

	int (__cdecl CContactCache::*pfn)(WPARAM, LPARAM);
	pfn = &CContactCache::OnDbEventAdded;
	m_hOnDbEventAdded = HookEventObj(ME_DB_EVENT_ADDED, *(MIRANDAHOOKOBJ *)&pfn, this);

	Rebuild();
}

CContactCache::~CContactCache()
{
	UnhookEvent(m_hOnDbEventAdded);
	DeleteCriticalSection(&m_cs);
}

void CContactCache::Rebuild()
{
	unsigned long timestamp = time(NULL);
	m_lastUpdate = time(NULL);

	HANDLE hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
	while (hContact)
	{
		TContactInfo *info = new TContactInfo;
		info->hContact = hContact;
		info->rate = 0;

		HANDLE hEvent = (HANDLE)CallService(MS_DB_EVENT_FINDLAST, (WPARAM)hContact, 0);
		while (hEvent)
		{
			DBEVENTINFO dbei = {0};
			dbei.cbSize = sizeof(dbei);
			if (!CallService(MS_DB_EVENT_GET, (WPARAM)hEvent, (LPARAM)&dbei))
			{
				if (float weight = GetEventWeight(timestamp - dbei.timestamp))
				{
					if (dbei.eventType == EVENTTYPE_MESSAGE)
						info->rate += weight;
				} else
				{
					break;
				}
			}
			hEvent = (HANDLE)CallService(MS_DB_EVENT_FINDPREV, (WPARAM)hEvent, 0);
		}

		m_cache.insert(info);
		hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, 0);
	}
}

HANDLE CContactCache::get(int rate)
{
	if (rate >= 0 && rate < m_cache.getCount())
		return m_cache[rate].hContact;
	return NULL;
}

float CContactCache::getWeight(int rate)
{
	if (rate >= 0 && rate < m_cache.getCount())
		return m_cache[rate].rate;
	return -1;
}

static bool AppendInfo(TCHAR *buf, int size, HANDLE hContact, int info)
{
	CONTACTINFO ci = {0};
	ci.cbSize = sizeof(ci);
	ci.hContact = hContact;
	ci.dwFlag = info;
#if defined(UNICODE) || defined(_UNICODE)
	ci.dwFlag |= CNF_UNICODE;
#endif

	bool ret = false;

	if (!CallService(MS_CONTACT_GETCONTACTINFO, 0, (LPARAM)&ci) && (ci.type == CNFT_ASCIIZ) && ci.pszVal)
	{
		if (*ci.pszVal && (lstrlen(ci.pszVal) < size-2))
		{
			lstrcpy(buf, ci.pszVal);
			ret = true;
		}
		mir_free(ci.pszVal);
	}

	return ret;
}

void CContactCache::TContactInfo::LoadInfo()
{
	if (infoLoaded) return;
	TCHAR *p = info;

	p[0] = p[1] = 0;

	static const int items[] = {
		CNF_FIRSTNAME, CNF_LASTNAME, CNF_NICK , CNF_CUSTOMNICK, CNF_EMAIL, CNF_CITY, CNF_STATE,
		CNF_COUNTRY, CNF_PHONE, CNF_HOMEPAGE, CNF_ABOUT, CNF_UNIQUEID, CNF_MYNOTES, CNF_STREET,
		CNF_CONAME, CNF_CODEPT, CNF_COCITY, CNF_COSTATE, CNF_COSTREET, CNF_COCOUNTRY
	};

	for (int i = 0; i < SIZEOF(items); ++i)
	{
		if (AppendInfo(p, SIZEOF(info) - (p - info), hContact, items[i]))
			p += lstrlen(p) + 1;
	}
	*p = 0;

	infoLoaded = true;
}

TCHAR *nb_stristr(TCHAR *str, TCHAR *substr)
{
	if (!substr || !*substr) return str;
	if (!str || !*str) return NULL;

	TCHAR *str_up = NEWTSTR_ALLOCA(str);
	TCHAR *substr_up = NEWTSTR_ALLOCA(substr);

	CharUpperBuff(str_up, lstrlen(str_up));
	CharUpperBuff(substr_up, lstrlen(substr_up));

	TCHAR* p = _tcsstr(str_up, substr_up);
	return p ? (str + (p - str_up)) : NULL;
}

bool CContactCache::filter(int rate, TCHAR *str)
{
	if (!str || !*str)
		return true;
	m_cache[rate].LoadInfo();

	HKL kbdLayoutActive = GetKeyboardLayout(GetCurrentThreadId());
	HKL kbdLayouts[10];
	int nKbdLayouts = GetKeyboardLayoutList(SIZEOF(kbdLayouts), kbdLayouts);

	TCHAR buf[256];
	BYTE keyState[256] = {0};

	for (int iLayout = 0; iLayout < nKbdLayouts; ++iLayout)
	{
		if (kbdLayoutActive == kbdLayouts[iLayout])
		{
			lstrcpy(buf, str);
		} else
		{
			int i;
			for (i = 0; str[i]; ++i)
			{
				UINT vk = VkKeyScanEx(str[i], kbdLayoutActive);
				UINT scan = MapVirtualKeyEx(vk, 0, kbdLayoutActive);
				ToUnicodeEx(vk, scan, keyState, buf+i, SIZEOF(buf)-i, 0, kbdLayouts[iLayout]);
			}
			buf[i] = 0;
		}

		for (TCHAR *p = m_cache[rate].info; p && *p; p = p + lstrlen(p) + 1)
			if (nb_stristr(p, buf))
				return true;
	}

	return false;
}
