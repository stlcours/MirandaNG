#ifndef favlist_h__
#define favlist_h__

struct TContactInfo
{
private:
	HANDLE hContact;
	DWORD status;
	TCHAR *name;
	TCHAR *group;
	bool bManual;
	float fRate;

public:
	TContactInfo(HANDLE hContact, bool bManual, float fRate = 0)
	{
		DBVARIANT dbv = {0};

		this->hContact = hContact;
		this->bManual = bManual;
		this->fRate = fRate;
		name = mir_tstrdup((TCHAR *)CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM)hContact, GCDNF_TCHAR));
		if (g_Options.bUseGroups && !DBGetContactSettingTString(hContact, "CList", "Group", &dbv))
		{
			group = mir_tstrdup(dbv.ptszVal);
			DBFreeVariant(&dbv);
		} else
		if (g_Options.bUseGroups)
		{
			group = mir_tstrdup(TranslateT("<no group>"));
		} else
		{
			group = mir_tstrdup(TranslateT("Favourite Contacts"));
		}
		status = DBGetContactSettingWord(hContact, (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0), "Status", ID_STATUS_OFFLINE);
	}

	~TContactInfo()
	{
		mir_free(name);
		mir_free(group);
	}

	HANDLE getHandle() const
	{
		return hContact;
	}

	TCHAR *getGroup() const
	{
		return group;
	}

	static int cmp(const TContactInfo *p1, const TContactInfo *p2)
	{
		if (p1->bManual && !p2->bManual) return -1;
		if (!p1->bManual && p2->bManual) return 1;

		if (!p1->bManual)
		{
			if (p1->fRate > p2->fRate) return -1;
			if (p1->fRate < p2->fRate) return 1;
		}

		int res = 0;
		if (res = lstrcmp(p1->group, p2->group)) return res;
		//if (p1->status < p2->status) return -1;
		//if (p1->status < p2->status) return +1;
		if (res = lstrcmp(p1->name, p2->name)) return res;
		return 0;
	}
};

class TFavContacts: public LIST<TContactInfo>
{
private:
	int nGroups;

public:
	TFavContacts(): LIST<TContactInfo>(5, TContactInfo::cmp) {}
	~TFavContacts()
	{
		for (int i = 0; i < this->getCount(); ++i)
			delete (*this)[i];
	}

	int groupCount()
	{
		return nGroups;
	}

	TContactInfo *addContact(HANDLE hContact, bool bManual)
	{
		TContactInfo *info = new TContactInfo(hContact, bManual);
		this->insert(info);
		return info;
	}

	void build()
	{
		TCHAR *prevGroup = NULL;
		int i;
		
		nGroups = 1;

		HANDLE hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
		for ( ; hContact; hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, 0))
			if (DBGetContactSettingByte(hContact, "FavContacts", "IsFavourite", 0))
			{
				TCHAR *group = addContact(hContact, true)->getGroup();
				if (prevGroup && lstrcmp(prevGroup, group))
					++nGroups;
				prevGroup = group;
			}

		int nRecent = 0;
		for (i = 0; nRecent < g_Options.wMaxRecent; ++i)
		{
			hContact = g_contactCache->get(i);
			if (!hContact) break;
			if (!DBGetContactSettingByte(hContact, "FavContacts", "IsFavourite", 0))
			{
				TCHAR *group = addContact(hContact, false)->getGroup();
				if (prevGroup && lstrcmp(prevGroup, group))
					++nGroups;
				prevGroup = group;

				++nRecent;
			}
		}
	}
};

#endif // favlist_h__
