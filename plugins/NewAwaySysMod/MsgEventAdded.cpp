/*
	New Away System - plugin for Miranda IM
	Copyright (C) 2005-2007 Chervov Dmitry
	Copyright (C) 2004-2005 Iksaif Entertainment
	Copyright (C) 2002-2003 Goblineye Entertainment

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "Common.h"
#include "Properties.h"

// stupid compiler.. kept returning me an "INTERNAL COMPILER ERROR" on almost every line of MsgEventAdded function with Release mode and unicode enabled >:( even disabling optimization through #pragma optimize didn't help.. The only acceptable solution I found is to move this function to a separate file and to disable optimization for the whole file in the project properties.

static struct
{
	int Status, DisableReplyCtlID, DontShowDialogCtlID;
} StatusModeList[] = {
	ID_STATUS_ONLINE, IDC_REPLYDLG_DISABLE_ONL, IDC_MOREOPTDLG_DONTPOPDLG_ONL,
	ID_STATUS_AWAY, IDC_REPLYDLG_DISABLE_AWAY, IDC_MOREOPTDLG_DONTPOPDLG_AWAY,
	ID_STATUS_NA, IDC_REPLYDLG_DISABLE_NA, IDC_MOREOPTDLG_DONTPOPDLG_NA,
	ID_STATUS_OCCUPIED, IDC_REPLYDLG_DISABLE_OCC, IDC_MOREOPTDLG_DONTPOPDLG_OCC,
	ID_STATUS_DND, IDC_REPLYDLG_DISABLE_DND, IDC_MOREOPTDLG_DONTPOPDLG_DND,
	ID_STATUS_FREECHAT, IDC_REPLYDLG_DISABLE_FFC, IDC_MOREOPTDLG_DONTPOPDLG_FFC,
	ID_STATUS_INVISIBLE, IDC_REPLYDLG_DISABLE_INV, IDC_MOREOPTDLG_DONTPOPDLG_INV,
	ID_STATUS_ONTHEPHONE, IDC_REPLYDLG_DISABLE_OTP, IDC_MOREOPTDLG_DONTPOPDLG_OTP,
	ID_STATUS_OUTTOLUNCH, IDC_REPLYDLG_DISABLE_OTL, IDC_MOREOPTDLG_DONTPOPDLG_OTL
};


class CAutoreplyData
{
public:
	CAutoreplyData(HANDLE hContact, TCString Reply): hContact(hContact), Reply(Reply) {}

	HANDLE hContact;
	TCString Reply;
};


void __cdecl AutoreplyDelayThread(void *_ad)
{ // _ad must be allocated using "new CAutoreplyData()"
	CAutoreplyData *ad = (CAutoreplyData*)_ad;
	_ASSERT(ad && ad->hContact && ad->Reply.GetLen());
	char *szProto = (char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)ad->hContact, 0);
	if (!szProto)
	{
		_ASSERT(0);
		return;
	}
#ifdef _UNICODE
	int ReplyLen = (ad->Reply.GetLen() + 1) * (sizeof(char) + sizeof(WCHAR));
	PBYTE pBuf = (PBYTE)malloc(ReplyLen);
	memcpy(pBuf, TCHAR2ANSI(ad->Reply), ad->Reply.GetLen() + 1);
	memcpy(pBuf + ad->Reply.GetLen() + 1, ad->Reply, (ad->Reply.GetLen() + 1) * sizeof(WCHAR));
	CallContactService(ad->hContact, ServiceExists(CString(szProto) + PSS_MESSAGE "W") ? (PSS_MESSAGE "W") : PSS_MESSAGE, PREF_UNICODE, (LPARAM)pBuf);
#else
	CallContactService(ad->hContact, PSS_MESSAGE, 0, (LPARAM)(char*)ad->Reply);
#endif
	if (g_AutoreplyOptPage.GetDBValueCopy(IDC_REPLYDLG_LOGREPLY))
	{ // store in the history
		DBEVENTINFO dbeo = {0};
		dbeo.cbSize = sizeof(dbeo);
		dbeo.eventType = EVENTTYPE_MESSAGE;
		dbeo.flags = DBEF_SENT;
		dbeo.szModule = szProto;
		dbeo.timestamp = time(NULL);
#ifdef _UNICODE
		dbeo.cbBlob = ReplyLen;
		dbeo.pBlob = pBuf;
#else
		dbeo.cbBlob = ad->Reply.GetLen() + 1;
		dbeo.pBlob = (PBYTE)(char*)ad->Reply;
#endif
		SleepEx(1000, true); // delay before sending the reply, as we need it to be later than the message we're replying to (without this delay, srmm puts the messages in a wrong order)
		CallService(MS_DB_EVENT_ADD, (WPARAM)ad->hContact, (LPARAM)&dbeo);
	}
#ifdef _UNICODE
	free(pBuf);
#endif
/*
	char *utf8Reply = mir_utf8encodeT(ad->Reply); // todo: use this instead of the code above, when 0.7 will be released
	if (g_AutoreplyOptPage.GetDBValueCopy(IDC_REPLYDLG_LOGREPLY))
	{ // store in the history
		DBEVENTINFO dbeo = {0};
		dbeo.cbSize = sizeof(dbeo);
		dbeo.eventType = EVENTTYPE_MESSAGE;
		dbeo.flags = DBEF_SENT | DBEF_UTF;
		dbeo.szModule = szProto;
		dbeo.timestamp = time(NULL);
		dbeo.cbBlob = strlen(utf8Reply);
		dbeo.pBlob = utf8Reply;
		CallService(MS_DB_EVENT_ADD, (WPARAM)ad->hContact, (LPARAM)&dbeo);
	}
	CallContactService(ad->hContact, ServiceExists(CString(szProto) + PSS_MESSAGE "W") ? (PSS_MESSAGE "W") : PSS_MESSAGE, PREF_UTF, (LPARAM)utf8Reply);
	mir_free(utf8Reply);
*/
	delete ad;
}


int IsSRMsgWindowOpen(HANDLE hContact, int DefaultRetVal)
{
	if (ServiceExists(MS_MSG_GETWINDOWDATA))
	{
		MessageWindowData mwd = {0};
		mwd.cbSize = sizeof(mwd);
		MessageWindowInputData mwid = {0};
		mwid.cbSize = sizeof(mwid);
		mwid.uFlags = MSG_WINDOW_UFLAG_MSG_BOTH;
		mwid.hContact = hContact;
		return !CallService(MS_MSG_GETWINDOWDATA, (WPARAM)&mwid, (LPARAM)&mwd) && mwd.hwndWindow;
	} else
	{
		return DefaultRetVal;
	}
}


#define MAX_REPLY_TIMEDIFF 5 // maximum "age" of an event to remain unfiltered; in seconds
#define MSGWNDOPEN_UNDEFINED (-1)

class CMetacontactEvent
{
public:
	CMetacontactEvent(HANDLE hMetaContact, DWORD timestamp, int bMsgWindowIsOpen): hMetaContact(hMetaContact), timestamp(timestamp), bMsgWindowIsOpen(bMsgWindowIsOpen) {};

	HANDLE hMetaContact;
	DWORD timestamp;
	int bMsgWindowIsOpen;
};

TMyArray<CMetacontactEvent> MetacontactEvents;


int MsgEventAdded(WPARAM wParam, LPARAM lParam)
{
	HANDLE hContact = (HANDLE)wParam;
	DBEVENTINFO *dbei = (DBEVENTINFO*)lParam;
	if (!hContact)
	{
		return 0;
	}
	if (dbei->flags & DBEF_SENT || (dbei->eventType != EVENTTYPE_MESSAGE && dbei->eventType != EVENTTYPE_URL && dbei->eventType != EVENTTYPE_FILE))
	{
		return 0;
	}
	if (time(NULL) - dbei->timestamp > MAX_REPLY_TIMEDIFF)
	{ // don't reply to offline messages
		return 0;
	}
	char *szProto = (char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
	if (!szProto)
	{
		return 0;
	}
	DWORD Flags1 = CallProtoService(szProto, PS_GETCAPS, PFLAGNUM_1, 0);
	if (!(Flags1 & PF1_IMSEND))
	{ // don't reply to protocols that don't support outgoing messages
		return 0;
	}
	int bMsgWindowIsOpen = MSGWNDOPEN_UNDEFINED;
	if (dbei->flags & DBEF_READ)
	{
		HANDLE hMetaContact;
		if (ServiceExists(MS_MC_GETMETACONTACT) && (hMetaContact = (HANDLE)CallService(MS_MC_GETMETACONTACT, (WPARAM)hContact, 0))) // if it's a subcontact of a metacontact
		{ // ugly workaround for metacontacts, part II
		// remove outdated events first
			DWORD CurTime = time(NULL);
			int I;
			for (I = MetacontactEvents.GetSize() - 1; I >= 0; I--)
			{
				if (CurTime - MetacontactEvents[I].timestamp > MAX_REPLY_TIMEDIFF)
				{
					MetacontactEvents.RemoveElem(I);
				}
			}
		// we compare only event timestamps, and do not look at the message itself. it's unlikely that there'll be two events from a contact at the same second, so it's a trade-off between speed and reliability
			for (I = MetacontactEvents.GetSize() - 1; I >= 0; I--)
			{
				if (MetacontactEvents[I].timestamp == dbei->timestamp && MetacontactEvents[I].hMetaContact == hMetaContact)
				{
					bMsgWindowIsOpen = MetacontactEvents[I].bMsgWindowIsOpen;
					break;
				}
			}
			if (I < 0)
			{
				_ASSERT(0);
				return 0;
			}
		} else
		{
			return 0;
		}
	}
	if (ServiceExists(MS_MC_GETPROTOCOLNAME) && !lstrcmpA(szProto, (char*)CallService(MS_MC_GETPROTOCOLNAME, 0, 0)))
	{ // ugly workaround for metacontacts, part I; store all metacontacts' events to a temporary array, so we'll be able to get the 'source' protocol when subcontact event happens later. we need the protocol to get its status and per-status settings properly
	// remove outdated events first
		DWORD CurTime = time(NULL);
		int I;
		for (I = MetacontactEvents.GetSize() - 1; I >= 0; I--)
		{
			if (CurTime - MetacontactEvents[I].timestamp > MAX_REPLY_TIMEDIFF)
			{
				MetacontactEvents.RemoveElem(I);
			}
		}
	// add the new event and wait for a subcontact's event
		MetacontactEvents.AddElem(CMetacontactEvent(hContact, dbei->timestamp, IsSRMsgWindowOpen(hContact, false)));
		return 0;
	}
	unsigned int iMode = CallProtoService(szProto, PS_GETSTATUS, 0, 0);
	int I;
	for (I = lengthof(StatusModeList) - 1; I >= 0; I--)
	{
		if (iMode == StatusModeList[I].Status)
		{
			break;
		}
	}
	if (I < 0)
	{
		return 0;
	}
	COptPage AutoreplyOptData(g_AutoreplyOptPage);
	AutoreplyOptData.DBToMem();
	if (dbei->eventType == EVENTTYPE_MESSAGE)
	{
		DBWriteContactSettingWord(hContact, MOD_NAME, DB_MESSAGECOUNT, DBGetContactSettingWord(hContact, MOD_NAME, DB_MESSAGECOUNT, 0) + 1); // increment message counter
	}
	if (AutoreplyOptData.GetValue(StatusModeList[I].DisableReplyCtlID))
	{
		return 0;
	}
	HANDLE hContactForSettings = hContact; // used to take into account not-on-list contacts when getting contact settings, but at the same time allows to get correct contact info for contacts that are in the DB
	if (hContactForSettings != INVALID_HANDLE_VALUE && DBGetContactSettingByte(hContactForSettings, "CList", "NotOnList", 0))
	{
		hContactForSettings = INVALID_HANDLE_VALUE; // INVALID_HANDLE_VALUE means the contact is not-on-list
	}
	if (!CContactSettings(iMode, hContactForSettings).Autoreply.IncludingParents(szProto) || CContactSettings(iMode, hContactForSettings).Ignore)
	{
		return 0;
	}
	if (AutoreplyOptData.GetValue(IDC_REPLYDLG_DONTREPLYINVISIBLE))
	{
		WORD ApparentMode = DBGetContactSettingWord(hContact, szProto, "ApparentMode", 0);
		if ((iMode == ID_STATUS_INVISIBLE && (!(Flags1 & PF1_INVISLIST) || ApparentMode != ID_STATUS_ONLINE)) ||
			(Flags1 & PF1_VISLIST && ApparentMode == ID_STATUS_OFFLINE))
		{
			return 0;
		}
	}
	if (AutoreplyOptData.GetValue(IDC_REPLYDLG_ONLYCLOSEDDLGREPLY))
	{
		if (bMsgWindowIsOpen && bMsgWindowIsOpen != MSGWNDOPEN_UNDEFINED)
		{
			return 0;
		}
	// we never get here for a metacontact; we did check for metacontact's window earlier, and here we need to check only for subcontact's window
		if (IsSRMsgWindowOpen(hContact, false))
		{
			return 0;
		}
	}
	if (AutoreplyOptData.GetValue(IDC_REPLYDLG_ONLYIDLEREPLY) && !g_bIsIdle)
	{
		return 0;
	}
	int UIN = 0;
	if (IsAnICQProto(szProto))
	{
		UIN = DBGetContactSettingDword(hContact, szProto, "UIN", 0);
	}
	int SendCount = AutoreplyOptData.GetValue(IDC_REPLYDLG_SENDCOUNT);
	if ((AutoreplyOptData.GetValue(IDC_REPLYDLG_DONTSENDTOICQ) && UIN) || // an icq contact
		(SendCount != -1 && DBGetContactSettingByte(hContact, MOD_NAME, DB_SENDCOUNT, 0) >= SendCount))
	{
		return 0;
	}
	if ((dbei->eventType == EVENTTYPE_MESSAGE && !AutoreplyOptData.GetValue(IDC_REPLYDLG_EVENTMSG)) || (dbei->eventType == EVENTTYPE_URL && !AutoreplyOptData.GetValue(IDC_REPLYDLG_EVENTURL)) || (dbei->eventType == EVENTTYPE_FILE && !AutoreplyOptData.GetValue(IDC_REPLYDLG_EVENTFILE)))
	{
		return 0;
	}
	DBWriteContactSettingByte(hContact, MOD_NAME, DB_SENDCOUNT, DBGetContactSettingByte(hContact, MOD_NAME, DB_SENDCOUNT, 0) + 1);
	GetDynamicStatMsg(hContact); // it updates VarParseData.Message needed for %extratext% in the format
	TCString Reply(*(TCString*)AutoreplyOptData.GetValue(IDC_REPLYDLG_PREFIX));
	if (Reply != NULL && ServiceExists(MS_VARS_FORMATSTRING) && !g_SetAwayMsgPage.GetDBValueCopy(IDS_SAWAYMSG_DISABLEVARIABLES))
	{
		FORMATINFO fi = {0};
		fi.cbSize = sizeof(FORMATINFO);
		fi.tszFormat = Reply;
		fi.hContact = hContact;
		fi.flags = FIF_TCHAR;
		fi.tszExtraText = VarParseData.Message;
		TCHAR *szResult = (TCHAR *)CallService(MS_VARS_FORMATSTRING, (WPARAM)&fi, 0);
		if (szResult != NULL)
		{
			Reply = szResult;
			CallService(MS_VARS_FREEMEMORY, (WPARAM)szResult, 0);
		}
	}
	if (Reply.GetLen())
	{
		CAutoreplyData *ad = new CAutoreplyData(hContact, Reply);
		mir_forkthread(AutoreplyDelayThread, ad);
	}
	return 0;
}

