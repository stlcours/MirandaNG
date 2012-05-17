#ifndef __contact_cache__
#define __contact_cache__

class CContactCache
{
public:
	enum { INFOSIZE = 1024 };

private:

	struct TContactInfo
	{
		HANDLE hContact;
		float rate;
		TCHAR info[INFOSIZE];
		bool infoLoaded;

		static int cmp(const TContactInfo *p1, const TContactInfo *p2)
		{
			if (p1->rate > p2->rate) return -1;
			if (p1->rate < p2->rate) return 1;
			return 0;
		}

		static int cmp2(const void *a1, const void *a2)
		{
			return cmp(*(const TContactInfo **)a1, *(const TContactInfo **)a2);
		}

		TContactInfo()
		{
			info[0] = info[1] = 0;
			infoLoaded = false;
		}

		void LoadInfo();
	};

	OBJLIST<TContactInfo> m_cache;
	unsigned long m_lastUpdate;
	CRITICAL_SECTION m_cs;
	HANDLE m_hOnDbEventAdded;

	int __cdecl OnDbEventAdded(WPARAM wParam, LPARAM lParam);
	float GetEventWeight(unsigned long age);
	float GetTimeWeight(unsigned long age);

public:
	CContactCache();
	~CContactCache();

	void Lock() { EnterCriticalSection(&m_cs); }
	void Unlock() { LeaveCriticalSection(&m_cs); }
	void Rebuild();

	HANDLE get(int rate);
	float getWeight(int rate);
	bool filter(int rate, TCHAR *str);
};

#endif // __contact_cache__
