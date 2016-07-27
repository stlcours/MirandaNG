/*

Miranda NG: the free IM client for Microsoft* Windows*

Copyright (�) 2012-16 Miranda NG project (http://miranda-ng.org),
Copyright (c) 2000-12 Miranda IM project,
all portions of this codebase are copyrighted to the people
listed in contributors.txt.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "stdafx.h"

#define NAMEORDERCOUNT 9
static wchar_t* nameOrderDescr[ NAMEORDERCOUNT ] =
{
	LPGENW("My custom name (not movable)"),
	LPGENW("Nick"),
	LPGENW("FirstName"),
	LPGENW("E-mail"),
	LPGENW("LastName"),
	LPGENW("Username"),
	LPGENW("FirstName LastName"),
	LPGENW("LastName FirstName"),
	LPGENW("'(Unknown contact)' (not movable)")
};

BYTE nameOrder[NAMEORDERCOUNT];

static int GetDatabaseString(MCONTACT hContact, const char *szProto, const char *szSetting, DBVARIANT *dbv)
{
	if (mir_strcmp(szProto, "CList") && CallProtoService(szProto, PS_GETCAPS, PFLAGNUM_4, 0) & PF4_INFOSETTINGSVC) {
		DBCONTACTGETSETTING cgs = { szProto, szSetting, dbv };
		dbv->type = DBVT_WCHAR;

		int res = CallProtoService(szProto, PS_GETINFOSETTING, (WPARAM)hContact, (LPARAM)&cgs);
		if (res != CALLSERVICE_NOTFOUND)
			return res;
	}

	return db_get_ws(hContact, szProto, szSetting, dbv);
}

static wchar_t* ProcessDatabaseValueDefault(MCONTACT hContact, const char *szProto, const char *szSetting)
{
	DBVARIANT dbv;
	if (!GetDatabaseString(hContact, szProto, szSetting, &dbv)) {
		switch (dbv.type) {
		case DBVT_ASCIIZ:
			if (!dbv.pszVal[0]) break;
		case DBVT_WCHAR:
			if (!dbv.pwszVal[0]) break;
			return dbv.ptszVal;
		}
		db_free(&dbv);
	}

	if (db_get(hContact, szProto, szSetting, &dbv))
		return NULL;

	wchar_t buf[40];
	switch (dbv.type) {
	case DBVT_BYTE:
		return mir_wstrdup(_itow(dbv.bVal, buf, 10));
	case DBVT_WORD:
		return mir_wstrdup(_itow(dbv.wVal, buf, 10));
	case DBVT_DWORD:
		return mir_wstrdup(_itow(dbv.dVal, buf, 10));
	}

	db_free(&dbv);
	return NULL;
}

MIR_APP_DLL(wchar_t*) Contact_GetInfo(int type, MCONTACT hContact, const char *szProto)
{
	if (hContact == NULL && szProto == NULL)
		return NULL;

	if (szProto == NULL)
		szProto = Proto_GetBaseAccountName(hContact);
	if (szProto == NULL)
		return NULL;

	char *uid;
	wchar_t *res;
	DBVARIANT dbv;
	switch (type) {
	case CNF_FIRSTNAME:  return ProcessDatabaseValueDefault(hContact, szProto, "FirstName");
	case CNF_LASTNAME:   return ProcessDatabaseValueDefault(hContact, szProto, "LastName");
	case CNF_NICK:       return ProcessDatabaseValueDefault(hContact, szProto, "Nick");
	case CNF_EMAIL:      return ProcessDatabaseValueDefault(hContact, szProto, "e-mail");
	case CNF_CITY:       return ProcessDatabaseValueDefault(hContact, szProto, "City");
	case CNF_STATE:      return ProcessDatabaseValueDefault(hContact, szProto, "State");
	case CNF_PHONE:      return ProcessDatabaseValueDefault(hContact, szProto, "Phone");
	case CNF_HOMEPAGE:   return ProcessDatabaseValueDefault(hContact, szProto, "Homepage");
	case CNF_ABOUT:      return ProcessDatabaseValueDefault(hContact, szProto, "About");
	case CNF_AGE:        return ProcessDatabaseValueDefault(hContact, szProto, "Age");
	case CNF_GENDER:     return ProcessDatabaseValueDefault(hContact, szProto, "Gender");
	case CNF_FAX:        return ProcessDatabaseValueDefault(hContact, szProto, "Fax");
	case CNF_CELLULAR:	return ProcessDatabaseValueDefault(hContact, szProto, "Cellular");
	case CNF_BIRTHDAY:	return ProcessDatabaseValueDefault(hContact, szProto, "BirthDay");
	case CNF_BIRTHMONTH:	return ProcessDatabaseValueDefault(hContact, szProto, "BirthMonth");
	case CNF_BIRTHYEAR:	return ProcessDatabaseValueDefault(hContact, szProto, "BirthYear");
	case CNF_STREET:		return ProcessDatabaseValueDefault(hContact, szProto, "Street");
	case CNF_ZIP:			return ProcessDatabaseValueDefault(hContact, szProto, "ZIP");
	case CNF_LANGUAGE1:	return ProcessDatabaseValueDefault(hContact, szProto, "Language1");
	case CNF_LANGUAGE2:	return ProcessDatabaseValueDefault(hContact, szProto, "Language2");
	case CNF_LANGUAGE3:	return ProcessDatabaseValueDefault(hContact, szProto, "Language3");
	case CNF_CONAME:		return ProcessDatabaseValueDefault(hContact, szProto, "Company");
	case CNF_CODEPT:     return ProcessDatabaseValueDefault(hContact, szProto, "CompanyDepartment");
	case CNF_COPOSITION: return ProcessDatabaseValueDefault(hContact, szProto, "CompanyPosition");
	case CNF_COSTREET:   return ProcessDatabaseValueDefault(hContact, szProto, "CompanyStreet");
	case CNF_COCITY:     return ProcessDatabaseValueDefault(hContact, szProto, "CompanyCity");
	case CNF_COSTATE:    return ProcessDatabaseValueDefault(hContact, szProto, "CompanyState");
	case CNF_COZIP:      return ProcessDatabaseValueDefault(hContact, szProto, "CompanyZIP");
	case CNF_COHOMEPAGE: return ProcessDatabaseValueDefault(hContact, szProto, "CompanyHomepage");

	case CNF_CUSTOMNICK:
		{
			const char* saveProto = szProto; szProto = "CList";
			if (hContact != NULL && !ProcessDatabaseValueDefault(hContact, szProto, "MyHandle")) {
				szProto = saveProto;
				return 0;
			}
			szProto = saveProto;
		}
		break;

	case CNF_COUNTRY:
	case CNF_COCOUNTRY:
		if (!GetDatabaseString(hContact, szProto, type == CNF_COUNTRY ? "CountryName" : "CompanyCountryName", &dbv))
			return dbv.ptszVal;

		if (!db_get(hContact, szProto, type == CNF_COUNTRY ? "Country" : "CompanyCountry", &dbv)) {
			if (dbv.type == DBVT_WORD) {
				int countryCount;
				struct CountryListEntry *countries;
				CallService(MS_UTILS_GETCOUNTRYLIST, (WPARAM)&countryCount, (LPARAM)&countries);
				for (int i = 0; i < countryCount; i++)
					if (countries[i].id == dbv.wVal)
						return mir_a2u(countries[i].szName);
			}
			else {
				db_free(&dbv);
				return ProcessDatabaseValueDefault(hContact, szProto, type == CNF_COUNTRY ? "Country" : "CompanyCountry");
			}
		}
		break;

	case CNF_FIRSTLAST:
		if (!GetDatabaseString(hContact, szProto, "FirstName", &dbv)) {
			DBVARIANT dbv2;
			if (!GetDatabaseString(hContact, szProto, "LastName", &dbv2)) {
				size_t len = mir_wstrlen(dbv.pwszVal) + mir_wstrlen(dbv2.pwszVal) + 2;
				WCHAR* buf = (WCHAR*)mir_alloc(sizeof(WCHAR)*len);
				if (buf != NULL)
					mir_wstrcat(mir_wstrcat(mir_wstrcpy(buf, dbv.pwszVal), L" "), dbv2.pwszVal);
				db_free(&dbv);
				db_free(&dbv2);
				return buf;
			}
			db_free(&dbv);
		}
		break;

	case CNF_UNIQUEID:
		if (db_mc_isMeta(hContact)) {
			wchar_t buf[40];
			_itow(hContact, buf, 10);
			return mir_wstrdup(buf);
		}
	
		uid = (char*)CallProtoService(szProto, PS_GETCAPS, PFLAG_UNIQUEIDSETTING, 0);
		if ((INT_PTR)uid != CALLSERVICE_NOTFOUND && uid)
			return ProcessDatabaseValueDefault(hContact, szProto, uid);
		break;

	case CNF_DISPLAYUID:
		if (res = ProcessDatabaseValueDefault(hContact, szProto, "display_uid"))
			return res;
			
		uid = (char*)CallProtoService(szProto, PS_GETCAPS, PFLAG_UNIQUEIDSETTING, 0);
		if ((INT_PTR)uid != CALLSERVICE_NOTFOUND && uid)
			return ProcessDatabaseValueDefault(hContact, szProto, uid);
		break;

	case CNF_DISPLAYNC:
	case CNF_DISPLAY:
		for (int i = 0; i < NAMEORDERCOUNT; i++) {
			switch (nameOrder[i]) {
			case 0: // custom name
				// make sure we aren't in CNF_DISPLAYNC mode
				// don't get custom name for NULL contact
				if (hContact != NULL && type == CNF_DISPLAY && (res = ProcessDatabaseValueDefault(hContact, "CList", "MyHandle")) != NULL)
					return res;
				break;

			case 1:
				if (res = ProcessDatabaseValueDefault(hContact, szProto, "Nick")) // nick
					return res;
				break;
			case 2:
				if (res = ProcessDatabaseValueDefault(hContact, szProto, "FirstName")) // First Name
					return res;
				break;
			case 3:
				if (res = ProcessDatabaseValueDefault(hContact, szProto, "e-mail")) // E-mail
					return res;
				break;
			case 4:
				if (res = ProcessDatabaseValueDefault(hContact, szProto, "LastName")) // Last Name
					return res;
				break;

			case 5: // Unique id
				// protocol must define a PFLAG_UNIQUEIDSETTING
				uid = (char*)CallProtoService(szProto, PS_GETCAPS, PFLAG_UNIQUEIDSETTING, 0);
				if ((INT_PTR)uid != CALLSERVICE_NOTFOUND && uid) {
					if (!GetDatabaseString(hContact, szProto, uid, &dbv)) {
						if (dbv.type == DBVT_BYTE || dbv.type == DBVT_WORD || dbv.type == DBVT_DWORD) {
							long value = (dbv.type == DBVT_BYTE) ? dbv.bVal : (dbv.type == DBVT_WORD ? dbv.wVal : dbv.dVal);
							WCHAR buf[40];
							_ltow(value, buf, 10);
							return mir_wstrdup(buf);
						}
						return dbv.ptszVal;
					}
				}
				break;

			case 6: // first + last name
			case 7: // last + first name
				if (!GetDatabaseString(hContact, szProto, nameOrder[i] == 6 ? "FirstName" : "LastName", &dbv)) {
					DBVARIANT dbv2;
					if (!GetDatabaseString(hContact, szProto, nameOrder[i] == 6 ? "LastName" : "FirstName", &dbv2)) {
						size_t len = mir_wstrlen(dbv.pwszVal) + mir_wstrlen(dbv2.pwszVal) + 2;
						WCHAR* buf = (WCHAR*)mir_alloc(sizeof(WCHAR)*len);
						if (buf != NULL)
							mir_wstrcat(mir_wstrcat(mir_wstrcpy(buf, dbv.pwszVal), L" "), dbv2.pwszVal);

						db_free(&dbv);
						db_free(&dbv2);
						return buf;
					}
					db_free(&dbv);
				}
				break;

			case 8:
				return mir_wstrdup(TranslateT("'(Unknown contact)'"));
			}
		}
		break;

	case CNF_MYNOTES:
		return ProcessDatabaseValueDefault(hContact, "UserInfo", "MyNotes");

	case CNF_TIMEZONE:
		HANDLE hTz = TimeZone_CreateByContact(hContact, 0, TZF_KNOWNONLY);
		if (hTz) {
			LPTIME_ZONE_INFORMATION tzi = TimeZone_GetInfo(hTz);
			int offset = tzi->Bias + tzi->StandardBias;

			char str[80];
			mir_snprintf(str, offset ? "UTC%+d:%02d" : "UTC", offset / -60, abs(offset % 60));
			return mir_a2u(str);
		}
		break;
	}

	return NULL;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Options dialog

class CContactOptsDlg : public CDlgBase
{
	CCtrlTreeView m_nameOrder;

public:
	CContactOptsDlg() :
		CDlgBase(g_hInst, IDD_OPT_CONTACT),
		m_nameOrder(this, IDC_NAMEORDER)
	{
		m_nameOrder.SetFlags(MTREE_DND);
		m_nameOrder.OnBeginDrag = Callback(this, &CContactOptsDlg::OnBeginDrag);
	}

	virtual void OnInitDialog()
	{
		TVINSERTSTRUCT tvis;
		tvis.hParent = NULL;
		tvis.hInsertAfter = TVI_LAST;
		tvis.item.mask = TVIF_TEXT | TVIF_PARAM;
		for (int i = 0; i < _countof(nameOrderDescr); i++) {
			tvis.item.lParam = nameOrder[i];
			tvis.item.pszText = TranslateTS(nameOrderDescr[nameOrder[i]]);
			m_nameOrder.InsertItem(&tvis);
		}
	}

	virtual void OnApply()
	{
		TVITEMEX tvi;
		tvi.hItem = m_nameOrder.GetRoot();
		int i = 0;
		while (tvi.hItem != NULL) {
			tvi.mask = TVIF_PARAM | TVIF_HANDLE;
			m_nameOrder.GetItem(&tvi);
			nameOrder[i++] = (BYTE)tvi.lParam;
			tvi.hItem = m_nameOrder.GetNextSibling(tvi.hItem);
		}
		db_set_blob(NULL, "Contact", "NameOrder", nameOrder, _countof(nameOrderDescr));
		cli.pfnInvalidateDisplayNameCacheEntry(INVALID_CONTACT_ID);
	}

	void OnBeginDrag(CCtrlTreeView::TEventInfo *evt)
	{
		LPNMTREEVIEW pNotify = evt->nmtv;
		if (pNotify->itemNew.lParam == 0 || pNotify->itemNew.lParam == _countof(nameOrderDescr) - 1)
			pNotify->hdr.code = 0; // deny dragging
	}
};

static int ContactOptInit(WPARAM wParam, LPARAM)
{
	OPTIONSDIALOGPAGE odp = { 0 };
	odp.position = -1000000000;
	odp.pszGroup = LPGEN("Contact list");
	odp.pszTitle = LPGEN("Contact names");
	odp.pDialog = new CContactOptsDlg();
	odp.flags = ODPF_BOLDGROUPS;
	Options_AddPage(wParam, &odp);
	return 0;
}

int LoadContactsModule(void)
{
	for (BYTE i = 0; i < NAMEORDERCOUNT; i++)
		nameOrder[i] = i;

	DBVARIANT dbv;
	if (!db_get(NULL, "Contact", "NameOrder", &dbv)) {
		memcpy(nameOrder, dbv.pbVal, dbv.cpbVal);
		db_free(&dbv);
	}

	HookEvent(ME_OPT_INITIALISE, ContactOptInit);
	return 0;
}
