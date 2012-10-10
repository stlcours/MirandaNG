/*
Weather Protocol plugin for Miranda IM
Copyright (C) 2005-2011 Boris Krasnovskiy All Rights Reserved
Copyright (C) 2002-2005 Calvin Che

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; version 2
of the License.

This program is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/* This file contain the source related to search and add a weather station
to the contact list.  Contain code for both name and ID search.
*/

#include "weather.h"

// variables used for weather_addstn.c
static int searchId = -1;
static TCHAR name1[256];

// ============ ADDING NEW STATION  ============

// protocol service function for adding a new contact onto contact list
// lParam = PROTOSEARCHRESULT
INT_PTR WeatherAddToList(WPARAM wParam, LPARAM lParam) 
{
	PROTOSEARCHRESULT *psr = (PROTOSEARCHRESULT*)lParam;
	WIDATA *sData;

	// search for existing contact
	HANDLE hContact = db_find_first();
	while (hContact != NULL) {
		// check if it is a weather contact
		if ( IsMyContact(hContact)) {
			DBVARIANT dbv;
			// check ID to see if the contact already exist in the database
			if (!DBGetContactSettingTString(hContact, WEATHERPROTONAME, "ID", &dbv)) {
				if (!_tcsicmp(psr->email, dbv.ptszVal)) {
					// remove the flag for not on list and hidden, thus make the contact visible
					// and add them on the list
					if (DBGetContactSettingByte(hContact, "CList", "NotOnList", 1)) {
						DBDeleteContactSetting(hContact, "CList", "NotOnList");
						DBDeleteContactSetting(hContact, "CList", "Hidden");						
					}
					DBFreeVariant(&dbv);
					// contact is added, function quitting
					return (INT_PTR)hContact;
				}
				DBFreeVariant(&dbv);
			}
		}
		hContact = db_find_next(hContact);
	}

	// if contact with the same ID was not found, add it
	if (psr->cbSize < sizeof(PROTOSEARCHRESULT)) return 0;
	hContact = (HANDLE) CallService(MS_DB_CONTACT_ADD, 0, 0);
	CallService(MS_PROTO_ADDTOCONTACT, (WPARAM)hContact, (LPARAM)WEATHERPROTONAME);
	// suppress online notification for the new contact
	CallService(MS_IGNORE_IGNORE, (WPARAM)hContact, IGNOREEVENT_USERONLINE);

	// set contact info and settings
	TCHAR svc[256];
	_tcsncpy(svc, psr->email, SIZEOF(svc)); svc[SIZEOF(svc)-1] = 0;
	GetSvc(svc);
	// set settings by obtaining the default for the service 
	if (psr->lastName[0] != 0) {
		sData = GetWIData(svc);
		DBWriteContactSettingTString(hContact, WEATHERPROTONAME, "MapURL", sData->DefaultMap);
		DBWriteContactSettingString(hContact, WEATHERPROTONAME, "InfoURL", sData->DefaultURL);
	}
	else { // if no valid service is found, create empty strings for MapURL and InfoURL
		DBWriteContactSettingString(hContact, WEATHERPROTONAME, "MapURL", "");
		DBWriteContactSettingString(hContact, WEATHERPROTONAME, "InfoURL", "");
	}
	// write the other info and settings to the database
	DBWriteContactSettingTString(hContact, WEATHERPROTONAME, "ID", psr->email);
	DBWriteContactSettingTString(hContact, WEATHERPROTONAME, "Nick", psr->nick);
	DBWriteContactSettingWord(hContact, WEATHERPROTONAME, "Status", ID_STATUS_OFFLINE);

	AvatarDownloaded(hContact);

	TCHAR str[256];
	mir_sntprintf(str, SIZEOF(str), TranslateT("Current weather information for %s."), psr->nick);
	DBWriteContactSettingTString(hContact, WEATHERPROTONAME, "About", str);

	// make the last update tags to something invalid
	DBWriteContactSettingString(hContact, WEATHERPROTONAME, "LastLog", "never");
	DBWriteContactSettingString(hContact, WEATHERPROTONAME, "LastCondition", "None");
	DBWriteContactSettingString(hContact, WEATHERPROTONAME, "LastTemperature", "None");

	// ignore status change
	DBWriteContactSettingDword(hContact, "Ignore", "Mask", 8);

	// if no default station is found, set the new contact as default station
	if (opt.Default[0] == 0) {
		DBVARIANT dbv;
		GetStationID(hContact, opt.Default, SIZEOF(opt.Default));

		opt.DefStn = hContact;
		if (!DBGetContactSettingTString(hContact, WEATHERPROTONAME, "Nick", &dbv)) {
			// notification message box
			wsprintf(str, TranslateT("%s is now the default weather station"), dbv.ptszVal);
			DBFreeVariant(&dbv);
			MessageBox(NULL, str, TranslateT("Weather Protocol"), MB_OK|MB_ICONINFORMATION);
		}
		DBWriteContactSettingTString(NULL, WEATHERPROTONAME, "Default", opt.Default);
	}
	// display the Edit Settings dialog box
	EditSettings((WPARAM)hContact, 0);
	return (INT_PTR)hContact;
}

// ============ WARNING DIALOG  ============

// show a message box and cancel search if update is in process
BOOL CheckSearch() {
	if (UpdateListHead != NULL) {
		MessageBox(NULL, TranslateT("Please try again after weather update is completed."), TranslateT("Weather Protocol"), MB_OK|MB_ICONERROR);
		return FALSE;
	}
	return TRUE;
}

// ============ BASIC ID SEARCH  ============

static TCHAR sID[32];

// A timer process for the ID search (threaded)
static void __cdecl BasicSearchTimerProc(LPVOID hWnd) 
{
	int result;
	// search only when it's not current updating weather.
	if (CheckSearch())	
		result = IDSearch(sID, searchId);

	// broadcast the search result
	ProtoBroadcastAck(WEATHERPROTONAME, NULL, ACKTYPE_SEARCH, ACKRESULT_SUCCESS, (HANDLE)searchId, 0);

	// exit the search
	searchId = -1;
}

// the service function for ID search
// lParam = ID search string
INT_PTR WeatherBasicSearch(WPARAM wParam, LPARAM lParam) 
{
	if (searchId != -1) return 0;   //only one search at a time
	_tcsncpy(sID, ( TCHAR* )lParam, SIZEOF(sID));
	sID[SIZEOF(sID)-1] = 0;
	searchId = 1;
	// create a thread for the ID search
	mir_forkthread(BasicSearchTimerProc, NULL);
	return searchId;
}

// ============ NAME SEARCH  ============

// name search timer process (threaded)
static void __cdecl NameSearchTimerProc(LPVOID hWnd) 
{
	// search only when it's not current updating weather.
	if (CheckSearch())
		if (name1[0] != 0)
			NameSearch(name1, searchId);	// search nickname field

	// broadcast the result
	ProtoBroadcastAck(WEATHERPROTONAME, NULL, ACKTYPE_SEARCH, ACKRESULT_SUCCESS, (HANDLE)searchId, 0);

	// exit the search
	searchId = -1;
}

static INT_PTR CALLBACK WeatherSearchAdvancedDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_INITDIALOG:
		TranslateDialogDefault(hwndDlg);
		SetFocus(GetDlgItem(hwndDlg, IDC_SEARCHCITY));
		return TRUE;

	case WM_COMMAND:
		if (HIWORD(wParam) == EN_SETFOCUS)
			PostMessage(GetParent(hwndDlg), WM_COMMAND, MAKEWPARAM(0, EN_SETFOCUS), (LPARAM)hwndDlg);
	}
	return FALSE;
}

INT_PTR WeatherCreateAdvancedSearchUI(WPARAM wParam, LPARAM lParam)
{
	HWND parent = (HWND)lParam;
	if (parent)
		return (INT_PTR)CreateDialogParam(hInst, MAKEINTRESOURCE(IDD_SEARCHCITY), parent, WeatherSearchAdvancedDlgProc, 0);

	return 0;
}

// service function for name search
INT_PTR WeatherAdvancedSearch(WPARAM wParam, LPARAM lParam)
{
	if (searchId != -1) return 0;   //only one search at a time

	searchId = 1;
	GetDlgItemText((HWND)lParam, IDC_SEARCHCITY, name1, 256);

	// search for the weather station using a thread
	mir_forkthread(NameSearchTimerProc, NULL);
	return searchId;
}

// ============ SEARCH FOR A WEATHER STATION USING ID  ============

// Seaching station ID from a single weather service (Threaded)
// sID = search string for the station ID
// searchId = -1
// sData = the ID search data for that particular weather service
// svcname = the name of the weather service that is currently searching (ie. Yahoo Weather)
int IDSearchProc(TCHAR *sID, const int searchId, WIIDSEARCH *sData, TCHAR *svc, TCHAR *svcname) 
{
	TCHAR str[MAX_DATA_LEN], newID[MAX_DATA_LEN];

	if (sData->Available) {
		char loc[255];
		TCHAR *szData = NULL;

		// load the page
		mir_snprintf(loc, SIZEOF(loc), sData->SearchURL, sID);
		if (InternetDownloadFile(loc, NULL, &szData) == 0) {
			TCHAR* szInfo = szData;

			// not found
			if ( _tcsstr(szInfo, sData->NotFoundStr) == NULL) 
				GetDataValue(&sData->Name, str, &szInfo);
		}
		mir_free(szData);
		// Station not found exit
		if (str[0] == 0) return 1;
	}

	// give no station name but only ID if the search is unavailable
	else _tcscpy(str, TranslateT("<Enter station name here>"));
	mir_sntprintf(newID, SIZEOF(newID), _T("%s/%s"), svc, sID);

	// set the search result and broadcast it
	PROTOSEARCHRESULT psr = { sizeof(psr) };
	psr.flags = PSR_TCHAR;
	psr.nick = str;
	psr.firstName = _T(" ");
	psr.lastName = svcname;
	psr.email = newID;
	ProtoBroadcastAck(WEATHERPROTONAME, NULL, ACKTYPE_SEARCH, ACKRESULT_DATA, (HANDLE)searchId, (LPARAM)&psr);

	return 0;
}

// ID search	 (Threaded)
//  sID:		the ID to search for
//  searchId:	don't change
// return 0 if no error
int IDSearch(TCHAR *sID, const int searchId) 
{
	// for a normal ID search (ID != #)
	if ( _tcscmp(sID, _T("#"))) {
		WIDATALIST *Item = WIHead;

		// search every weather service using the search station ID
		while (Item != NULL) {
			IDSearchProc(sID, searchId, &Item->Data.IDSearch, Item->Data.InternalName, Item->Data.DisplayName);
			Item = Item->next;
		}
		NetlibHttpDisconnect();
	}
	// if the station ID is #, return a dummy result and quit the funciton
	else {
		// return an empty contact on "#"
		PROTOSEARCHRESULT psr = { sizeof(psr) };
		psr.flags = PSR_TCHAR;
		psr.nick = TranslateT("<Enter station name here>");	// to be entered
		psr.firstName = _T(" ");
		psr.lastName = _T("");
		psr.email = TranslateT("<Enter station ID here>");		// to be entered
		ProtoBroadcastAck(WEATHERPROTONAME, NULL, ACKTYPE_SEARCH, ACKRESULT_DATA, (HANDLE)searchId, (LPARAM)&psr);
	}

	return 0;
}

// ============ SEARCH FOR A WEATHER STATION BY NAME  ============

// Seaching station name from a single weather service (Threaded)
// name = the name of the weather station to be searched
// searchId = -1
// sData = the name search data for that particular weather service
// svcname = the name of the weather service that is currently searching (ie. Yahoo Weather)
int NameSearchProc(TCHAR *name, const int searchId, WINAMESEARCH *sData, TCHAR *svc, TCHAR *svcname)
{
	char loc[256];
	TCHAR Name[MAX_DATA_LEN], str[MAX_DATA_LEN], sID[MAX_DATA_LEN], *szData = NULL, *search;

	// replace spaces with %20
	char *pstr = (char*)CallService(MS_NETLIB_URLENCODE, 0, (LPARAM)(char*)_T2A(name));
	wsprintfA(loc, sData->SearchURL, pstr);
	HeapFree(GetProcessHeap(), 0, pstr);

	if (InternetDownloadFile(loc, NULL, &szData) == 0) {
		TCHAR* szInfo = szData;
		search = _tcsstr(szInfo, sData->NotFoundStr);	// determine if data is available
		if (search == NULL) { // if data is found
			// test if it is single result
			if (sData->Single.Available && sData->Multiple.Available)
				search = _tcsstr(szInfo, sData->SingleStr);
			// for single result
			if (sData->Single.Available && (search != NULL || !sData->Multiple.Available)) { // single result
				// if station ID appears first in the downloaded data
				if ( !_tcsicmp(sData->Single.First, _T("ID"))) {
					GetDataValue(&sData->Single.ID, str, &szInfo);
					wsprintf(sID, _T("%s/%s"), svc, str);
					GetDataValue(&sData->Single.Name, Name, &szInfo);
				}
				// if station name appears first in the downloaded data
				else if (!_tcsicmp(sData->Single.First, _T("NAME"))) {
					GetDataValue(&sData->Single.Name, Name, &szInfo);
					GetDataValue(&sData->Single.ID, str, &szInfo);
					wsprintf(sID, _T("%s/%s"), svc, str);
				}
				// if no station ID is obtained, quit the search
				if (str[0] == 0) {
					mir_free(szData);
					return 1;
				}
				
				// if can't get the name, use the search string as name
				if (Name[0] == 0)
					_tcscpy(Name, name);

				// set the data and broadcast it
				PROTOSEARCHRESULT psr = { sizeof(psr) };
				psr.flags = PSR_TCHAR;
				psr.nick = Name;
				psr.firstName = _T(" ");
				psr.lastName = svcname;
				psr.email = sID;
				psr.id = sID;
				ProtoBroadcastAck(WEATHERPROTONAME, NULL, ACKTYPE_SEARCH, ACKRESULT_DATA, (HANDLE)searchId, (LPARAM)&psr);
				mir_free(szData);
				return 0;
			}
			// for multiple result
			else if (sData->Multiple.Available) { // multiple results
				// search for the next occurrence of the string
				for (;;) {
					// if station ID appears first in the downloaded data
					if ( !_tcsicmp(sData->Multiple.First, _T("ID"))) {
						GetDataValue(&sData->Multiple.ID, str, &szInfo);
						wsprintf(sID, _T("%s/%s"), svc, str);
						GetDataValue(&sData->Multiple.Name, Name, &szInfo);
					}
					// if station name appears first in the downloaded data
					else if ( !_tcsicmp(sData->Multiple.First, _T("NAME"))) {
						GetDataValue(&sData->Multiple.Name, Name, &szInfo);
						GetDataValue(&sData->Multiple.ID, str, &szInfo);
						wsprintf(sID, _T("%s/%s"), svc, str);
					}
					// if no station ID is obtained, search completed and quit the search
					if (str[0] == 0)	break;
					// if can't get the name, use the search string as name
					if (Name[0] == 0)	
						_tcscpy(Name, name);

					PROTOSEARCHRESULT psr = { sizeof(psr) };
					psr.flags = PSR_TCHAR;
					psr.nick = Name;
					psr.firstName = _T("");
					psr.lastName = svcname;
					psr.email = sID;
					psr.id = sID;
					ProtoBroadcastAck(WEATHERPROTONAME, NULL, ACKTYPE_SEARCH, ACKRESULT_DATA, (HANDLE)searchId, (LPARAM)&psr);
		}	}	}

		mir_free(szData);
		return 0;
	}

	mir_free(szData);
	return 1;  
}

// name search	(Threaded)
//  name:		the station name to search for
//  searchId:	don't change
// return 0 if no error
int NameSearch(TCHAR *name, const int searchId)
{
	WIDATALIST *Item = WIHead;

	// search every weather service using the search station name
	while (Item != NULL) {
		if (Item->Data.NameSearch.Single.Available || Item->Data.NameSearch.Multiple.Available)
			NameSearchProc(name, searchId, &Item->Data.NameSearch, Item->Data.InternalName, Item->Data.DisplayName);
		Item = Item->next;
	}

	NetlibHttpDisconnect();
	return 0;
}

// ======================MENU ITEM FUNCTION ============

// add a new weather station via find/add dialog
int WeatherAdd(WPARAM wParam, LPARAM lParam) 
{
	DBWriteContactSettingString(NULL, "FindAdd", "LastSearched", "Weather");
	CallService(MS_FINDADD_FINDADD, 0, 0);
	return 0;
}
