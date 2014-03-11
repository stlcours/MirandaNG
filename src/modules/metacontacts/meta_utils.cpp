/*
former MetaContacts Plugin for Miranda IM.

Copyright � 2014 Miranda NG Team
Copyright � 2004-07 Scott Ellis
Copyright � 2004 Universite Louis PASTEUR, STRASBOURG.

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

#include "..\..\core\commonheaders.h"

#include "metacontacts.h"

HANDLE invisiGroup;
POINT menuMousePoint;

/** Update the MetaContact login, depending on the protocol desired
*
* The login of the "MetaContacts" protocol will be copied from the login
* of the specified protocol.
*
* @param szProto :	The name of the protocol used to get the login that will be
*					affected to the "MetaContacts" protocol.
*
* @return			O on success, 1 otherwise.
*/

int Meta_SetNick(char *szProto)
{
	CONTACTINFO ci = { sizeof(ci) };
	ci.dwFlag = CNF_DISPLAY | CNF_TCHAR;
	ci.szProto = szProto;
	if (CallService(MS_CONTACT_GETCONTACTINFO, 0, (LPARAM)&ci))
		return 1;

	switch (ci.type) {
	case CNFT_BYTE:
		if (db_set_b(NULL, META_PROTO, "Nick", ci.bVal))
			return 1;
		break;
	case CNFT_WORD:
		if (db_set_w(NULL, META_PROTO, "Nick", ci.wVal))
			return 1;
		break;
	case CNFT_DWORD:
		if (db_set_dw(NULL, META_PROTO, "Nick", ci.dVal))
			return 1;
		break;
	case CNFT_ASCIIZ:
		if (db_set_ts(NULL, META_PROTO, "Nick", ci.pszVal))
			return 1;
		mir_free(ci.pszVal);
		break;
	default:
		if (db_set_s(NULL, META_PROTO, "Nick", (char *)TranslateT("Sender")))
			return 1;
		break;
	}
	return 0;
}

/** Assign a contact (hSub) to a metacontact (hMeta)
*
* @param hSub : HANDLE to a contact that should be assigned
* @param hMeta : HANDLE to a metacontact that will host the contact
* @param set_as_default : bool flag to indicate whether the new contact becomes the default
*
* @return TRUE on success, FALSE otherwise
*/

BOOL Meta_Assign(MCONTACT hSub, MCONTACT hMeta, BOOL set_as_default)
{
	DBCachedContact *ccDest = CheckMeta(hMeta), *ccSub = currDb->m_cache->GetCachedContact(hSub);
	if (ccDest == NULL || ccSub == NULL)
		return FALSE;

	char *szProto = GetContactProto(hSub);
	if (szProto == NULL) {
		MessageBox(0, TranslateT("Could not retreive contact protocol"), TranslateT("Assignment error"), MB_OK | MB_ICONWARNING);
		return FALSE;
	}

	// Get the login of the subcontact
	char *field = (char *)CallProtoService(szProto, PS_GETCAPS, PFLAG_UNIQUEIDSETTING, 0);
	DBVARIANT dbv;
	if (db_get(hSub, szProto, field, &dbv)) {
		MessageBox(0, TranslateT("Could not get unique ID of contact"), TranslateT("Assignment error"), MB_OK | MB_ICONWARNING);
		return FALSE;
	}

	// Check that is is 'on the list'
	if (db_get_b(hSub, "CList", "NotOnList", 0) == 1) {
		MessageBox(0, TranslateT("Contact is 'Not on List' - please add the contact to your contact list before assigning."), TranslateT("Assignment error"), MB_OK | MB_ICONWARNING);
		db_free(&dbv);
		return FALSE;
	}

	char szId[40];
	_itoa(ccDest->nSubs++, szId, 10);
	if (ccDest->nSubs >= MAX_CONTACTS) {
		MessageBox(0, TranslateT("MetaContact is full"), TranslateT("Assignment error"), MB_OK | MB_ICONWARNING);
		db_free(&dbv);
		return FALSE;
	}

	// write the contact's protocol
	char buffer[512];
	strcpy(buffer, "Protocol"); strcat(buffer, szId);
	if (db_set_s(hMeta, META_PROTO, buffer, szProto)) {
		MessageBox(0, TranslateT("Could not write contact protocol to MetaContact"), TranslateT("Assignment error"), MB_OK | MB_ICONWARNING);
		db_free(&dbv);
		return FALSE;
	}

	// write the login
	strcpy(buffer, "Login"); strcat(buffer, szId);
	if (db_set(hMeta, META_PROTO, buffer, &dbv)) {
		MessageBox(0, TranslateT("Could not write unique ID of contact to MetaContact"), TranslateT("Assignment error"), MB_OK | MB_ICONWARNING);
		db_free(&dbv);
		return FALSE;
	}

	db_free(&dbv);

	// If we can get the nickname of the subcontact...
	if (!db_get(hSub, szProto, "Nick", &dbv)) {
		// write the nickname
		strcpy(buffer, "Nick");
		strcat(buffer, szId);
		if (db_set(hMeta, META_PROTO, buffer, &dbv)) {
			MessageBox(0, TranslateT("Could not write nickname of contact to MetaContact"), TranslateT("Assignment error"), MB_OK | MB_ICONWARNING);
			return FALSE;
		}

		db_free(&dbv);
	}

	// write the display name
	strcpy(buffer, "CListName");
	strcat(buffer, szId);
	db_set_ts(hMeta, META_PROTO, buffer, cli.pfnGetContactDisplayName(hSub, 0));

	// Get the status
	WORD status = (!szProto) ? ID_STATUS_OFFLINE : db_get_w(hSub, szProto, "Status", ID_STATUS_OFFLINE);

	// write the status
	strcpy(buffer, "Status"); strcat(buffer, szId);
	db_set_w(hMeta, META_PROTO, buffer, status);

	// write the handle
	strcpy(buffer, "Handle"); strcat(buffer, szId);
	db_set_dw(hMeta, META_PROTO, buffer, hSub);

	// write status string
	strcpy(buffer, "StatusString"); strcat(buffer, szId);

	TCHAR *szStatus = cli.pfnGetStatusModeDescription(status, 0);
	db_set_ts(hMeta, META_PROTO, buffer, szStatus);

	// Write the link in the contact
	db_set_dw(hSub, META_PROTO, "ParentMeta", hMeta);
	db_set_b(hSub, META_PROTO, "IsSubcontact", true);

	// update count of contacts
	db_set_dw(hMeta, META_PROTO, "NumContacts", ccDest->nSubs);
	ccDest->pSubs = (MCONTACT*)mir_realloc(ccDest->pSubs, sizeof(MCONTACT)*ccDest->nSubs);
	ccDest->pSubs[ccDest->nSubs - 1] = hSub;
	ccSub->parentID = hMeta;

	if (set_as_default)
		db_mc_setDefaultNum(hMeta, ccDest->nSubs-1);

	// set nick to most online contact that can message
	MCONTACT most_online = Meta_GetMostOnline(ccDest);
	Meta_CopyContactNick(ccDest, most_online);

	// set status to that of most online contact
	Meta_FixStatus(ccDest);

	// if the new contact is the most online contact with avatar support, get avatar info
	most_online = Meta_GetMostOnlineSupporting(ccDest, PFLAGNUM_4, PF4_AVATARS);
	if (most_online == hSub) {
		PROTO_AVATAR_INFORMATIONT AI;
		AI.cbSize = sizeof(AI);
		AI.hContact = hMeta;
		AI.format = PA_FORMAT_UNKNOWN;
		_tcscpy(AI.filename, _T("X"));

		if (CallProtoService(META_PROTO, PS_GETAVATARINFOT, 0, (LPARAM)&AI) == GAIR_SUCCESS)
			db_set_ts(hMeta, "ContactPhoto", "File", AI.filename);
	}

	// merge sub's events to the meta-history
	currDb->MetaMergeHistory(ccDest, ccSub);

	// Ignore status if the option is on
	if (options.bSuppressStatus)
		CallService(MS_IGNORE_IGNORE, hSub, IGNOREEVENT_USERONLINE);

	NotifyEventHooks(hSubcontactsChanged, hMeta, 0);
	return TRUE;
}

/**
*	Convenience method - get most online contact supporting messaging
*
*/

MCONTACT Meta_GetMostOnline(DBCachedContact *cc)
{
	return Meta_GetMostOnlineSupporting(cc, PFLAGNUM_1, PF1_IM);
}

/** Get the 'most online' contact for a meta contact (according to above order) which supports the specified
* protocol service, and copies nick from that contact
*
* @param hMetaHANDLE to a metacontact
*
* @return HANDLE to a contact
*/

MCONTACT Meta_GetMostOnlineSupporting(DBCachedContact *cc, int pflagnum, unsigned long capability)
{
	if (cc == NULL || cc->nDefault == -1)
		return 0;

	int most_online_status = ID_STATUS_OFFLINE;
	MCONTACT hContact;
	int i;

	// if the default is beyond the end of the list (eek!) return null
	if (cc->nDefault >= cc->nSubs)
		return NULL;

	MCONTACT most_online_contact = Meta_GetContactHandle(cc, cc->nDefault);
	char *szProto = GetContactProto(most_online_contact);
	DWORD caps = szProto ? CallProtoService(szProto, PS_GETCAPS, pflagnum, 0) : 0;
	if (szProto && strcmp(szProto, "IRC") == 0) caps |= PF1_IM;
	// we are forced to do use default for sending - '-1' capability indicates no specific capability, but respect 'Force Default'
	if (szProto && db_get_b(cc->contactID, META_PROTO, "ForceDefault", 0) && capability != 0 && (capability == -1 || (caps & capability) == capability)) // capability is 0 when we're working out status
		return most_online_contact;

	// a subcontact is being temporarily 'forced' to do sending
	if ((most_online_contact = db_get_dw(cc->contactID, META_PROTO, "ForceSend", 0))) {
		caps = szProto ? CallProtoService(szProto, PS_GETCAPS, pflagnum, 0) : 0;
		if (szProto && strcmp(szProto, "IRC") == 0) caps |= PF1_IM;
		if (szProto && (caps & capability) == capability && capability != 0) // capability is 0 when we're working out status
			return most_online_contact;
	}

	most_online_contact = Meta_GetContactHandle(cc, cc->nDefault);
	szProto = GetContactProto(most_online_contact);
	if (szProto && CallProtoService(szProto, PS_GETSTATUS, 0, 0) >= ID_STATUS_ONLINE) {
		caps = szProto ? CallProtoService(szProto, PS_GETCAPS, pflagnum, 0) : 0;
		if (szProto && strcmp(szProto, "IRC") == 0) caps |= PF1_IM;
		if (szProto && (capability == -1 || (caps & capability) == capability)) {
			most_online_status = db_get_w(most_online_contact, szProto, "Status", ID_STATUS_OFFLINE);

			// if our default is not offline, and option to use default is set - return default
			// and also if our default is online, return it
			if (most_online_status == ID_STATUS_ONLINE)
				return most_online_contact;
		}
		else most_online_status = ID_STATUS_OFFLINE;
	}
	else most_online_status = ID_STATUS_OFFLINE;

	char *most_online_proto = szProto;
	// otherwise, check all the subcontacts for the one closest to the ONLINE state which supports the required capability
	for (i = 0; i < cc->nSubs; i++) {
		if (i == cc->nDefault) // already checked that (i.e. initial value of most_online_contact and most_online_status are those of the default contact)
			continue;

		hContact = Meta_GetContactHandle(cc, i);
		szProto = GetContactProto(hContact);

		if (!szProto || CallProtoService(szProto, PS_GETSTATUS, 0, 0) < ID_STATUS_ONLINE) // szProto offline or connecting
			continue;

		caps = szProto ? CallProtoService(szProto, PS_GETCAPS, pflagnum, 0) : 0;
		if (szProto && strcmp(szProto, "IRC") == 0) caps |= PF1_IM;
		if (szProto && (capability == -1 || (caps & capability) == capability)) {
			int status = db_get_w(hContact, szProto, "Status", ID_STATUS_OFFLINE);
			if (status == ID_STATUS_ONLINE) {
				most_online_contact = hContact;
				most_online_proto = szProto;
				return most_online_contact;
			}
			if (status <= ID_STATUS_OFFLINE) // status below ID_STATUS_OFFLINE is a connecting status
				continue;

			if (GetRealPriority(szProto, status) < GetRealPriority(most_online_proto, most_online_status)) {
				most_online_status = status;
				most_online_contact = hContact;
				most_online_proto = szProto;
			}
		}
	}

	// no online contacts? if we're trying to message, use 'send offline' contact
	if (most_online_status == ID_STATUS_OFFLINE && capability == PF1_IM) {
		MCONTACT hOffline = Meta_GetContactHandle(cc, db_get_dw(cc->contactID, META_PROTO, "OfflineSend", INVALID_CONTACT_ID));
		if (hOffline)
			most_online_contact = hOffline;
	}

	return most_online_contact;
}

DBCachedContact* CheckMeta(MCONTACT hMeta)
{
	DBCachedContact *cc = currDb->m_cache->GetCachedContact(hMeta);
	return (cc == NULL || cc->nSubs == -1) ? NULL : cc;
}

int Meta_GetContactNumber(DBCachedContact *cc, MCONTACT hContact)
{
	for (int i = 0; i < cc->nSubs; i++)
		if (cc->pSubs[i] == hContact)
			return i;

	return -1;
}

MCONTACT Meta_GetContactHandle(DBCachedContact *cc, int contact_number)
{
	if (contact_number >= cc->nSubs || contact_number < 0)
		return 0;

	return cc->pSubs[contact_number];
}

/** Hide all contacts linked to any meta contact, and set handle links
*
* Additionally, set all sub contacts and metacontacts to offline so that status notifications are always sent
*
* and ensure metafilter in place
*/

#define szMsg LPGEN("The 'MetaContacts Hidden Group' has been added to your contact list.\n\
This is most likely due to server-side contact information. To fix this, so that\n\
MetaContacts continues to function correctly, you should:\n\
   - disable MetaContacts using the 'Toggle MetaContacts Off' main menu item\n\
   - move all contacts out of this group\n\
   - synchronize your contacts with the server\n\
   - re-enable MetaContacts")

int Meta_HideLinkedContacts(void)
{
	DBVARIANT dbv, dbv2;
	char buffer[512], buffer2[512];

	for (MCONTACT hContact = db_find_first(); hContact; hContact = db_find_next(hContact)) {
		DBCachedContact *cc = currDb->m_cache->GetCachedContact(hContact);
		if (cc == NULL || cc->parentID == 0)
			continue;

		DBCachedContact *ccMeta = CheckMeta(cc->parentID);
		if (ccMeta == NULL)
			continue;

		// get contact number
		int contact_number = Meta_GetContactNumber(cc, hContact);

		// prepare to update metacontact record of subcontat status
		char *szProto = GetContactProto(hContact);

		// find metacontact
		if (contact_number < 0 || contact_number >= ccMeta->nSubs)
			continue;

		// update metacontact's record of status for this contact
		strcpy(buffer, "Status");
		strcat(buffer, _itoa(contact_number, buffer2, 10));

		WORD status = (!szProto) ? ID_STATUS_OFFLINE : db_get_w(hContact, szProto, "Status", ID_STATUS_OFFLINE);
		db_set_w(ccMeta->contactID, META_PROTO, buffer, status);

		// update metacontact's record of nick for this contact
		if (szProto && !db_get(hContact, szProto, "Nick", &dbv)) {
			strcpy(buffer, "Nick");
			strcat(buffer, _itoa(contact_number, buffer2, 10));
			db_set(ccMeta->contactID, META_PROTO, buffer, &dbv);

			strcpy(buffer, "CListName");
			strcat(buffer, _itoa(contact_number, buffer2, 10));
			if (db_get(hContact, "CList", "MyHandle", &dbv2))
				db_set(ccMeta->contactID, META_PROTO, buffer, &dbv);
			else {
				db_set(ccMeta->contactID, META_PROTO, buffer, &dbv2);
				db_free(&dbv2);
			}

			db_free(&dbv);
		}
		else {
			if (!db_get(hContact, "CList", "MyHandle", &dbv)) {
				strcpy(buffer, "CListName");
				strcat(buffer, _itoa(contact_number, buffer2, 10));
				db_set(ccMeta->contactID, META_PROTO, buffer, &dbv);
				db_free(&dbv);
			}
		}

		if (options.bSuppressStatus)
			CallService(MS_IGNORE_IGNORE, hContact, IGNOREEVENT_USERONLINE);
	}

	// do metacontacts after handles set properly above
	for (MCONTACT hContact = db_find_first(); hContact; hContact = db_find_next(hContact)) {
		DBCachedContact *cc = CheckMeta(hContact);
		if (cc == NULL)
			continue;

		MCONTACT hMostOnline = Meta_GetMostOnline(cc); // set nick
		Meta_CopyContactNick(cc, hMostOnline);
		Meta_FixStatus(cc);
	}

	return 0;
}

int Meta_HideMetaContacts(int hide)
{
	// set status suppression
	if (hide)
		Meta_SuppressStatus(FALSE);
	else
		Meta_SuppressStatus(options.bSuppressStatus);

	for (MCONTACT hContact = db_find_first(); hContact; hContact = db_find_next(hContact)) {
		if (CheckMeta(hContact) == NULL)
			continue;

		if (hide)
			db_set_b(hContact, "CList", "Hidden", 1);
		else
			db_unset(hContact, "CList", "Hidden");
	}

	return 0;
}

int Meta_SuppressStatus(BOOL suppress)
{
	for (MCONTACT hContact = db_find_first(); hContact; hContact = db_find_next(hContact))
		if (db_mc_isSub(hContact))
			CallService((suppress) ? MS_IGNORE_IGNORE : MS_IGNORE_UNIGNORE, hContact, IGNOREEVENT_USERONLINE);

	return 0;
}

int Meta_CopyContactNick(DBCachedContact *ccMeta, MCONTACT hContact)
{
	if (options.bLockHandle)
		hContact = Meta_GetContactHandle(ccMeta, 0);

	if (!hContact)
		return 1;

	char *szProto = GetContactProto(hContact);
	if (szProto == NULL)
		return 1;

	if (options.clist_contact_name == CNNT_NICK) {
		ptrT tszNick(db_get_tsa(hContact, szProto, "Nick"));
		if (tszNick) {
			db_set_ts(ccMeta->contactID, META_PROTO, "Nick", tszNick);
			return 0;
		}
	}
	else if (options.clist_contact_name == CNNT_DISPLAYNAME) {
		TCHAR *name = cli.pfnGetContactDisplayName(hContact, 0);
		if (name && _tcscmp(name, TranslateT("(Unknown Contact)")) != 0) {
			db_set_ts(ccMeta->contactID, META_PROTO, "Nick", name);
			return 0;
		}
	}
	return 1;
}

int Meta_SetAllNicks()
{
	for (MCONTACT hContact = db_find_first(); hContact; hContact = db_find_next(hContact)) {
		DBCachedContact *cc = CheckMeta(hContact);
		if (cc == NULL)
			continue;
		MCONTACT most_online = Meta_GetMostOnline(cc);
		Meta_CopyContactNick(cc, most_online);
		Meta_FixStatus(cc);
	}
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////

static void SwapValues(MCONTACT hContact, LPCSTR szSetting, int n1, int n2)
{
	char buf1[100], buf2[100];
	mir_snprintf(buf1, sizeof(buf1), "%s%d", szSetting, n1);
	mir_snprintf(buf2, sizeof(buf2), "%s%d", szSetting, n2);

	DBVARIANT dbv1, dbv2;
	int ok1 = !db_get_s(hContact, META_PROTO, buf1, &dbv1, 0);
	int ok2 = !db_get_s(hContact, META_PROTO, buf2, &dbv2, 0);
	if (ok1) {
		db_set(hContact, META_PROTO, buf2, &dbv1);
		db_free(&dbv1);
	}
	if (ok2) {
		db_set(hContact, META_PROTO, buf1, &dbv2);
		db_free(&dbv2);
	}
}

int Meta_SwapContacts(DBCachedContact *cc, int n1, int n2)
{
	MCONTACT hContact1 = Meta_GetContactHandle(cc, n1), hContact2 = Meta_GetContactHandle(cc, n2);

	SwapValues(cc->contactID, "Protocol", n1, n2);
	SwapValues(cc->contactID, "Status", n1, n2);
	SwapValues(cc->contactID, "StatusString", n1, n2);
	SwapValues(cc->contactID, "Login", n1, n2);
	SwapValues(cc->contactID, "Nick", n1, n2);
	SwapValues(cc->contactID, "CListName", n1, n2);
	SwapValues(cc->contactID, "Handle", n1, n2);

	MCONTACT tmp = cc->pSubs[n1];
	cc->pSubs[n1] = cc->pSubs[n2];
	cc->pSubs[n2] = tmp;
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK DlgProcNull(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_INITDIALOG:
		HWND prog = GetDlgItem(hwndDlg, IDC_PROG);

		TranslateDialogDefault(hwndDlg);

		SendMessage(prog, PBM_SETPOS, 0, 0);
		return TRUE;
	}
	return FALSE;
}

void Meta_FixStatus(DBCachedContact *ccMeta)
{
	MCONTACT most_online = Meta_GetMostOnlineSupporting(ccMeta, PFLAGNUM_1, 0);
	if (most_online) {
		char *szProto = GetContactProto(most_online);
		if (szProto) {
			WORD status = db_get_w(most_online, szProto, "Status", ID_STATUS_OFFLINE);
			db_set_w(ccMeta->contactID, META_PROTO, "Status", status);
		}
		else db_set_w(ccMeta->contactID, META_PROTO, "Status", ID_STATUS_OFFLINE);
	}
	else db_set_w(ccMeta->contactID, META_PROTO, "Status", ID_STATUS_OFFLINE);
}
