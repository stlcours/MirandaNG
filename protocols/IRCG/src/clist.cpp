
/*
IRC plugin for Miranda IM

Copyright (C) 2003-05 Jurgen Persson
Copyright (C) 2007-09 George Hazan

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

BOOL CIrcProto::CList_AddDCCChat(const CMStringW& name, const CMStringW& hostmask, unsigned long adr, int port)
{
	MCONTACT hContact;
	wchar_t szNick[256];
	char szService[256];
	bool bFlag = false;

	CONTACT usertemp = { (wchar_t*)name.c_str(), NULL, NULL, false, false, true };
	MCONTACT hc = CList_FindContact(&usertemp);
	if (hc && db_get_b(hc, "CList", "NotOnList", 0) == 0 && db_get_b(hc, "CList", "Hidden", 0) == 0)
		bFlag = true;

	CMStringW contactname = name; contactname += DCCSTRING;

	CONTACT user = { (wchar_t*)contactname.c_str(), NULL, NULL, false, false, true };
	hContact = CList_AddContact(&user, false, false);
	setByte(hContact, "DCC", 1);

	DCCINFO* pdci = new DCCINFO;
	pdci->sHostmask = hostmask;
	pdci->hContact = hContact;
	pdci->dwAdr = (DWORD)adr;
	pdci->iPort = port;
	pdci->iType = DCC_CHAT;
	pdci->bSender = false;
	pdci->sContactName = name;

	if (m_DCCChatAccept == 3 || m_DCCChatAccept == 2 && bFlag) {
		CDccSession* dcc = new CDccSession(this, pdci);

		CDccSession* olddcc = FindDCCSession(hContact);
		if (olddcc)
			olddcc->Disconnect();

		AddDCCSession(hContact, dcc);
		dcc->Connect();
		if (getByte("MirVerAutoRequest", 1))
			PostIrcMessage(L"/PRIVMSG %s \001VERSION\001", name.c_str());
	}
	else {
		CLISTEVENT cle = {};
		cle.hContact = hContact;
		cle.hDbEvent = -100;
		cle.flags = CLEF_TCHAR;
		cle.hIcon = LoadIconEx(IDI_DCC);
		mir_snprintf(szService, "%s/DblClickEvent", m_szModuleName);
		cle.pszService = szService;
		mir_snwprintf(szNick, TranslateT("CTCP chat request from %s"), name.c_str());
		cle.ptszTooltip = szNick;
		cle.lParam = (LPARAM)pdci;

		if (pcli->pfnGetEvent(hContact, 0))
			pcli->pfnRemoveEvent(hContact, -100);
		pcli->pfnAddEvent(&cle);
	}
	return TRUE;
}

MCONTACT CIrcProto::CList_AddContact(CONTACT *user, bool InList, bool SetOnline)
{
	if (user->name == NULL)
		return 0;

	MCONTACT hContact = CList_FindContact(user);
	if (hContact) {
		if (InList)
			db_unset(hContact, "CList", "NotOnList");
		setWString(hContact, "Nick", user->name);
		db_unset(hContact, "CList", "Hidden");
		if (SetOnline && getWord(hContact, "Status", ID_STATUS_OFFLINE) == ID_STATUS_OFFLINE)
			setWord(hContact, "Status", ID_STATUS_ONLINE);
		return hContact;
	}

	// here we create a new one since no one is to be found
	hContact = (MCONTACT)CallService(MS_DB_CONTACT_ADD, 0, 0);
	if (hContact) {
		Proto_AddToContact(hContact, m_szModuleName);

		if (InList)
			db_unset(hContact, "CList", "NotOnList");
		else
			db_set_b(hContact, "CList", "NotOnList", 1);
		db_unset(hContact, "CList", "Hidden");
		setWString(hContact, "Nick", user->name);
		setWString(hContact, "Default", user->name);
		setWord(hContact, "Status", SetOnline ? ID_STATUS_ONLINE : ID_STATUS_OFFLINE);
		if (!InList && getByte("MirVerAutoRequestTemp", 0))
			PostIrcMessage(L"/PRIVMSG %s \001VERSION\001", user->name);
		return hContact;
	}
	return false;
}

MCONTACT CIrcProto::CList_SetOffline(CONTACT *user)
{
	MCONTACT hContact = CList_FindContact(user);
	if (hContact) {
		DBVARIANT dbv;
		if (!getWString(hContact, "Default", &dbv)) {
			setString(hContact, "User", "");
			setString(hContact, "Host", "");
			setWString(hContact, "Nick", dbv.ptszVal);
			setWord(hContact, "Status", ID_STATUS_OFFLINE);
			db_free(&dbv);
			return hContact;
		}
	}

	return 0;
}

bool CIrcProto::CList_SetAllOffline(BYTE ChatsToo)
{
	DBVARIANT dbv;

	DisconnectAllDCCSessions(false);

	for (MCONTACT hContact = db_find_first(m_szModuleName); hContact; hContact = db_find_next(hContact, m_szModuleName)) {
		if (isChatRoom(hContact))
			continue;

		if (getByte(hContact, "DCC", 0) != 0) {
			if (ChatsToo)
				setWord(hContact, "Status", ID_STATUS_OFFLINE);
		}
		else if (!getWString(hContact, "Default", &dbv)) {
			setWString(hContact, "Nick", dbv.ptszVal);
			setWord(hContact, "Status", ID_STATUS_OFFLINE);
			db_free(&dbv);
		}
		db_unset(hContact, m_szModuleName, "IP");
		setString(hContact, "User", "");
		setString(hContact, "Host", "");
	}
	return true;
}

MCONTACT CIrcProto::CList_FindContact(CONTACT *user)
{
	if (!user || !user->name)
		return 0;

	wchar_t* lowercasename = mir_wstrdup(user->name);
	CharLower(lowercasename);

	for (MCONTACT hContact = db_find_first(m_szModuleName); hContact; hContact = db_find_next(hContact, m_szModuleName)) {
		if (isChatRoom(hContact))
			continue;

		MCONTACT  hContact_temp = NULL;
		ptrW DBNick(getWStringA(hContact, "Nick"));
		ptrW DBUser(getWStringA(hContact, "UUser"));
		ptrW DBHost(getWStringA(hContact, "UHost"));
		ptrW DBDefault(getWStringA(hContact, "Default"));
		ptrW DBWildcard(getWStringA(hContact, "UWildcard"));

		if (DBWildcard)
			CharLower(DBWildcard);
		if (IsChannel(user->name)) {
			if (DBDefault && !mir_wstrcmpi(DBDefault, user->name))
				hContact_temp = (MCONTACT)-1;
		}
		else if (user->ExactNick && DBNick && !mir_wstrcmpi(DBNick, user->name))
			hContact_temp = hContact;

		else if (user->ExactOnly && DBDefault && !mir_wstrcmpi(DBDefault, user->name))
			hContact_temp = hContact;

		else if (user->ExactWCOnly) {
			if (DBWildcard && !mir_wstrcmpi(DBWildcard, lowercasename)
				|| (DBWildcard && !mir_wstrcmpi(DBNick, lowercasename) && !WCCmp(DBWildcard, lowercasename))
				|| (!DBWildcard && !mir_wstrcmpi(DBNick, lowercasename))) {
				hContact_temp = hContact;
			}
		}
		else if (wcschr(user->name, ' ') == 0) {
			if ((DBDefault && !mir_wstrcmpi(DBDefault, user->name) || DBNick && !mir_wstrcmpi(DBNick, user->name) ||
				DBWildcard && WCCmp(DBWildcard, lowercasename))
				&& (WCCmp(DBUser, user->user) && WCCmp(DBHost, user->host))) {
				hContact_temp = hContact;
			}
		}

		if (hContact_temp != NULL) {
			mir_free(lowercasename);
			if (hContact_temp != (MCONTACT)-1)
				return hContact_temp;
			return 0;
		}
	}
	mir_free(lowercasename);
	return 0;
}
