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

// This file holds functions that are called upon receiving
// certain commands from the server.

#include "stdafx.h"
#include "version.h"

using namespace irc;

VOID CALLBACK IdentTimerProc(HWND, UINT, UINT_PTR idEvent, DWORD)
{
	CIrcProto *ppro = GetTimerOwner(idEvent);
	if (ppro == NULL)
		return;

	ppro->KillChatTimer(ppro->IdentTimer);
	if (ppro->m_iStatus == ID_STATUS_OFFLINE || ppro->m_iStatus == ID_STATUS_CONNECTING)
		return;

	if (ppro->IsConnected() && ppro->m_identTimer)
		ppro->KillIdent();
}

VOID CALLBACK TimerProc(HWND, UINT, UINT_PTR idEvent, DWORD)
{
	CIrcProto *ppro = GetTimerOwner(idEvent);
	if (ppro == NULL)
		return;

	ppro->KillChatTimer(ppro->InitTimer);
	if (ppro->m_iStatus == ID_STATUS_OFFLINE || ppro->m_iStatus == ID_STATUS_CONNECTING)
		return;

	if (ppro->m_forceVisible)
		ppro->PostIrcMessage(L"/MODE %s -i", ppro->m_info.sNick.c_str());

	if (mir_strlen(ppro->m_myHost) == 0 && ppro->IsConnected())
		ppro->DoUserhostWithReason(2, (L"S" + ppro->m_info.sNick).c_str(), true, L"%s", ppro->m_info.sNick.c_str());
}

VOID CALLBACK KeepAliveTimerProc(HWND, UINT, UINT_PTR idEvent, DWORD)
{
	CIrcProto *ppro = GetTimerOwner(idEvent);
	if (ppro == NULL)
		return;

	if (!ppro->m_sendKeepAlive || (ppro->m_iStatus == ID_STATUS_OFFLINE || ppro->m_iStatus == ID_STATUS_CONNECTING)) {
		ppro->KillChatTimer(ppro->KeepAliveTimer);
		return;
	}

	wchar_t temp2[270];
	if (!ppro->m_info.sServerName.IsEmpty())
		mir_snwprintf(temp2, L"PING %s", ppro->m_info.sServerName.c_str());
	else
		mir_snwprintf(temp2, L"PING %u", time(0));

	if (ppro->IsConnected())
		ppro->SendIrcMessage(temp2, false);
}

VOID CALLBACK OnlineNotifTimerProc3(HWND, UINT, UINT_PTR idEvent, DWORD)
{
	CIrcProto *ppro = GetTimerOwner(idEvent);
	if (ppro == NULL)
		return;

	if (!ppro->m_channelAwayNotification ||
		ppro->m_iStatus == ID_STATUS_OFFLINE || ppro->m_iStatus == ID_STATUS_CONNECTING ||
		(!ppro->m_autoOnlineNotification && !ppro->bTempForceCheck) || ppro->bTempDisableCheck) {
		ppro->KillChatTimer(ppro->OnlineNotifTimer3);
		ppro->m_channelsToWho = L"";
		return;
	}

	CMStringW name = GetWord(ppro->m_channelsToWho.c_str(), 0);
	if (name.IsEmpty()) {
		ppro->m_channelsToWho = L"";
		int count = (int)CallServiceSync(MS_GC_GETSESSIONCOUNT, 0, (LPARAM)ppro->m_szModuleName);
		for (int i = 0; i < count; i++) {
			GC_INFO gci = { 0 };
			gci.Flags = GCF_BYINDEX | GCF_NAME | GCF_TYPE | GCF_COUNT;
			gci.iItem = i;
			gci.pszModule = ppro->m_szModuleName;
			if (!CallServiceSync(MS_GC_GETINFO, 0, (LPARAM)&gci) && gci.iType == GCW_CHATROOM)
			if (gci.iCount <= ppro->m_onlineNotificationLimit)
				ppro->m_channelsToWho += CMStringW(gci.pszName) + L" ";
		}
	}

	if (ppro->m_channelsToWho.IsEmpty()) {
		ppro->SetChatTimer(ppro->OnlineNotifTimer3, 60 * 1000, OnlineNotifTimerProc3);
		return;
	}

	name = GetWord(ppro->m_channelsToWho.c_str(), 0);
	ppro->DoUserhostWithReason(2, L"S" + name, true, L"%s", name.c_str());
	CMStringW temp = GetWordAddress(ppro->m_channelsToWho.c_str(), 1);
	ppro->m_channelsToWho = temp;
	if (ppro->m_iTempCheckTime)
		ppro->SetChatTimer(ppro->OnlineNotifTimer3, ppro->m_iTempCheckTime * 1000, OnlineNotifTimerProc3);
	else
		ppro->SetChatTimer(ppro->OnlineNotifTimer3, ppro->m_onlineNotificationTime * 1000, OnlineNotifTimerProc3);
}

VOID CALLBACK OnlineNotifTimerProc(HWND, UINT, UINT_PTR idEvent, DWORD)
{
	CIrcProto *ppro = GetTimerOwner(idEvent);
	if (ppro == NULL)
		return;

	if (ppro->m_iStatus == ID_STATUS_OFFLINE || ppro->m_iStatus == ID_STATUS_CONNECTING ||
		(!ppro->m_autoOnlineNotification && !ppro->bTempForceCheck) || ppro->bTempDisableCheck) {
		ppro->KillChatTimer(ppro->OnlineNotifTimer);
		ppro->m_namesToWho = L"";
		return;
	}

	CMStringW name = GetWord(ppro->m_namesToWho.c_str(), 0);
	CMStringW name2 = GetWord(ppro->m_namesToUserhost.c_str(), 0);

	if (name.IsEmpty() && name2.IsEmpty()) {
		DBVARIANT dbv;
		for (MCONTACT hContact = db_find_first(ppro->m_szModuleName); hContact; hContact = db_find_next(hContact, ppro->m_szModuleName)) {
			if (ppro->isChatRoom(hContact))
				continue;

			BYTE bDCC = ppro->getByte(hContact, "DCC", 0);
			BYTE bHidden = db_get_b(hContact, "CList", "Hidden", 0);
			if (bDCC || bHidden)
				continue;
			if (ppro->getWString(hContact, "Default", &dbv))
				continue;

			BYTE bAdvanced = ppro->getByte(hContact, "AdvancedMode", 0);
			if (!bAdvanced) {
				db_free(&dbv);
				if (!ppro->getWString(hContact, "Nick", &dbv)) {
					ppro->m_namesToUserhost += CMStringW(dbv.ptszVal) + L" ";
					db_free(&dbv);
				}
			}
			else {
				db_free(&dbv);
				DBVARIANT dbv2;

				wchar_t* DBNick = NULL;
				wchar_t* DBWildcard = NULL;
				if (!ppro->getWString(hContact, "Nick", &dbv))
					DBNick = dbv.ptszVal;
				if (!ppro->getWString(hContact, "UWildcard", &dbv2))
					DBWildcard = dbv2.ptszVal;

				if (DBNick && (!DBWildcard || !WCCmp(CharLower(DBWildcard), CharLower(DBNick))))
					ppro->m_namesToWho += CMStringW(DBNick) + L" ";
				else if (DBWildcard)
					ppro->m_namesToWho += CMStringW(DBWildcard) + L" ";

				if (DBNick)     db_free(&dbv);
				if (DBWildcard) db_free(&dbv2);
			}
		}
	}

	if (ppro->m_namesToWho.IsEmpty() && ppro->m_namesToUserhost.IsEmpty()) {
		ppro->SetChatTimer(ppro->OnlineNotifTimer, 60 * 1000, OnlineNotifTimerProc);
		return;
	}

	name = GetWord(ppro->m_namesToWho.c_str(), 0);
	name2 = GetWord(ppro->m_namesToUserhost.c_str(), 0);
	CMStringW temp;
	if (!name.IsEmpty()) {
		ppro->DoUserhostWithReason(2, L"S" + name, true, L"%s", name.c_str());
		temp = GetWordAddress(ppro->m_namesToWho.c_str(), 1);
		ppro->m_namesToWho = temp;
	}

	if (!name2.IsEmpty()) {
		CMStringW params;
		for (int i = 0; i < 3; i++) {
			params = L"";
			for (int j = 0; j < 5; j++)
				params += GetWord(ppro->m_namesToUserhost, i * 5 + j) + L" ";

			if (params[0] != ' ')
				ppro->DoUserhostWithReason(1, CMStringW(L"S") + params, true, params);
		}
		temp = GetWordAddress(ppro->m_namesToUserhost.c_str(), 15);
		ppro->m_namesToUserhost = temp;
	}

	if (ppro->m_iTempCheckTime)
		ppro->SetChatTimer(ppro->OnlineNotifTimer, ppro->m_iTempCheckTime * 1000, OnlineNotifTimerProc);
	else
		ppro->SetChatTimer(ppro->OnlineNotifTimer, ppro->m_onlineNotificationTime * 1000, OnlineNotifTimerProc);
}

int CIrcProto::AddOutgoingMessageToDB(MCONTACT hContact, wchar_t* msg)
{
	if (m_iStatus == ID_STATUS_OFFLINE || m_iStatus == ID_STATUS_CONNECTING)
		return 0;

	CMStringW S = DoColorCodes(msg, TRUE, FALSE);

	DBEVENTINFO dbei = { sizeof(dbei) };
	dbei.szModule = m_szModuleName;
	dbei.eventType = EVENTTYPE_MESSAGE;
	dbei.timestamp = (DWORD)time(NULL);
	dbei.flags = DBEF_SENT + DBEF_UTF;
	dbei.pBlob = (PBYTE)mir_utf8encodeW(S.c_str());
	dbei.cbBlob = (DWORD)mir_strlen((char*)dbei.pBlob) + 1;
	db_event_add(hContact, &dbei);
	mir_free(dbei.pBlob);
	return 1;
}

void __cdecl CIrcProto::ResolveIPThread(LPVOID di)
{
	Thread_SetName("IRC: ResolveIPThread");
	IPRESOLVE* ipr = (IPRESOLVE *)di;
	{
		mir_cslock lock(m_resolve);

		if (ipr != NULL && (ipr->iType == IP_AUTO && mir_strlen(m_myHost) == 0 || ipr->iType == IP_MANUAL)) {
			hostent* myhost = gethostbyname(ipr->sAddr.c_str());
			if (myhost) {
				IN_ADDR in;
				memcpy(&in, myhost->h_addr, 4);
				if (ipr->iType == IP_AUTO)
					strncpy_s(m_myHost, inet_ntoa(in), _TRUNCATE);
				else
					strncpy_s(m_mySpecifiedHostIP, inet_ntoa(in), _TRUNCATE);
			}
		}
	}

	delete ipr;
}

bool CIrcProto::OnIrc_PING(const CIrcMessage* pmsg)
{
	wchar_t szResponse[100];
	mir_snwprintf(szResponse, L"PONG %s", pmsg->parameters[0].c_str());
	SendIrcMessage(szResponse);
	return false;
}

bool CIrcProto::OnIrc_WELCOME(const CIrcMessage* pmsg)
{
	if (pmsg->parameters[0] != m_info.sNick)
		m_info.sNick = pmsg->parameters[0];

	if (pmsg->m_bIncoming && pmsg->parameters.getCount() > 1) {
		static wchar_t host[1024];
		int i = 0;
		CMStringW word = GetWord(pmsg->parameters[1].c_str(), i);
		while (!word.IsEmpty()) {
			if (wcschr(word.c_str(), '!') && wcschr(word.c_str(), '@')) {
				mir_wstrncpy(host, word.c_str(), _countof(host));
				wchar_t* p1 = wcschr(host, '@');
				if (p1)
					ForkThread(&CIrcProto::ResolveIPThread, new IPRESOLVE(_T2A(p1 + 1), IP_AUTO));
			}

			word = GetWord(pmsg->parameters[1].c_str(), ++i);
		}
	}

	ShowMessage(pmsg);
	return true;
}

bool CIrcProto::OnIrc_WHOTOOLONG(const CIrcMessage* pmsg)
{
	CMStringW command = GetNextUserhostReason(2);
	if (command[0] == 'U')
		ShowMessage(pmsg);

	return true;
}

bool CIrcProto::OnIrc_BACKFROMAWAY(const CIrcMessage* pmsg)
{
	if (pmsg->m_bIncoming) {
		int Temp = m_iStatus;
		m_iStatus = m_iDesiredStatus = ID_STATUS_ONLINE;
		ProtoBroadcastAck(NULL, ACKTYPE_STATUS, ACKRESULT_SUCCESS, (HANDLE)Temp, ID_STATUS_ONLINE);

		if (m_perform)
			DoPerform("Event: Available");
	}

	ShowMessage(pmsg);
	return true;
}

bool CIrcProto::OnIrc_SETAWAY(const CIrcMessage* pmsg)
{
	if (pmsg->m_bIncoming) {
		int Temp = m_iDesiredStatus;
		m_iStatus = m_iDesiredStatus = ID_STATUS_AWAY;
		ProtoBroadcastAck(NULL, ACKTYPE_STATUS, ACKRESULT_SUCCESS, (HANDLE)Temp, ID_STATUS_AWAY);

		if (m_perform) {
			switch (Temp) {
			case ID_STATUS_AWAY:
				DoPerform("Event: Away");
				break;
			case ID_STATUS_NA:
				DoPerform("Event: Not available");
				break;
			case ID_STATUS_DND:
				DoPerform("Event: Do not disturb");
				break;
			case ID_STATUS_OCCUPIED:
				DoPerform("Event: Occupied");
				break;
			case ID_STATUS_OUTTOLUNCH:
				DoPerform("Event: Out for lunch");
				break;
			case ID_STATUS_ONTHEPHONE:
				DoPerform("Event: On the phone");
				break;
			default:
				m_iStatus = ID_STATUS_AWAY;
				DoPerform("Event: Away");
				break;
			}
		}
	}

	ShowMessage(pmsg);
	return true;
}

bool CIrcProto::OnIrc_JOIN(const CIrcMessage* pmsg)
{
	if (pmsg->parameters.getCount() > 0 && pmsg->m_bIncoming && pmsg->prefix.sNick != m_info.sNick) {
		CMStringW host = pmsg->prefix.sUser + L"@" + pmsg->prefix.sHost;
		DoEvent(GC_EVENT_JOIN, pmsg->parameters[0].c_str(), pmsg->prefix.sNick.c_str(), NULL, L"Normal", host.c_str(), NULL, true, false);
		DoEvent(GC_EVENT_SETCONTACTSTATUS, pmsg->parameters[0].c_str(), pmsg->prefix.sNick.c_str(), NULL, NULL, NULL, ID_STATUS_ONLINE, FALSE, FALSE);
	}
	else ShowMessage(pmsg);

	return true;
}

bool CIrcProto::OnIrc_QUIT(const CIrcMessage* pmsg)
{
	if (pmsg->m_bIncoming) {
		CMStringW host = pmsg->prefix.sUser + L"@" + pmsg->prefix.sHost;
		DoEvent(GC_EVENT_QUIT, NULL, pmsg->prefix.sNick.c_str(), pmsg->parameters.getCount() > 0 ? pmsg->parameters[0].c_str() : NULL, NULL, host.c_str(), NULL, true, false);
		struct CONTACT user = { (LPTSTR)pmsg->prefix.sNick.c_str(), (LPTSTR)pmsg->prefix.sUser.c_str(), (LPTSTR)pmsg->prefix.sHost.c_str(), false, false, false };
		CList_SetOffline(&user);
		if (pmsg->prefix.sNick == m_info.sNick) {
			GCDEST gcd = { m_szModuleName, NULL, GC_EVENT_CONTROL };
			GCEVENT gce = { sizeof(gce), &gcd };
			CallChatEvent(SESSION_OFFLINE, (LPARAM)&gce);
		}
	}
	else ShowMessage(pmsg);

	return true;
}

bool CIrcProto::OnIrc_PART(const CIrcMessage* pmsg)
{
	if (pmsg->parameters.getCount() > 0 && pmsg->m_bIncoming) {
		CMStringW host = pmsg->prefix.sUser + L"@" + pmsg->prefix.sHost;
		DoEvent(GC_EVENT_PART, pmsg->parameters[0].c_str(), pmsg->prefix.sNick.c_str(), pmsg->parameters.getCount() > 1 ? pmsg->parameters[1].c_str() : NULL, NULL, host.c_str(), NULL, true, false);
		if (pmsg->prefix.sNick == m_info.sNick) {
			CMStringW S = MakeWndID(pmsg->parameters[0].c_str());
			GCDEST gcd = { m_szModuleName, S.c_str(), GC_EVENT_CONTROL };
			GCEVENT gce = { sizeof(gce), &gcd };
			CallChatEvent(SESSION_OFFLINE, (LPARAM)&gce);
		}
	}
	else ShowMessage(pmsg);

	return true;
}

bool CIrcProto::OnIrc_KICK(const CIrcMessage* pmsg)
{
	if (pmsg->m_bIncoming && pmsg->parameters.getCount() > 1)
		DoEvent(GC_EVENT_KICK, pmsg->parameters[0].c_str(), pmsg->parameters[1].c_str(), pmsg->parameters.getCount() > 2 ? pmsg->parameters[2].c_str() : NULL, pmsg->prefix.sNick.c_str(), NULL, NULL, true, false);
	else
		ShowMessage(pmsg);

	if (pmsg->parameters[1] == m_info.sNick) {
		CMStringW S = MakeWndID(pmsg->parameters[0].c_str());
		GCDEST gcd = { m_szModuleName, S.c_str(), GC_EVENT_CONTROL };
		GCEVENT gce = { sizeof(gce), &gcd };
		CallChatEvent(SESSION_OFFLINE, (LPARAM)&gce);

		if (m_rejoinIfKicked) {
			CHANNELINFO *wi = (CHANNELINFO *)DoEvent(GC_EVENT_GETITEMDATA, pmsg->parameters[0].c_str(), NULL, NULL, NULL, NULL, NULL, FALSE, FALSE, 0);
			if (wi && wi->pszPassword)
				PostIrcMessage(L"/JOIN %s %s", pmsg->parameters[0].c_str(), wi->pszPassword);
			else
				PostIrcMessage(L"/JOIN %s", pmsg->parameters[0].c_str());
		}
	}

	return true;
}

bool CIrcProto::OnIrc_MODEQUERY(const CIrcMessage* pmsg)
{
	if (pmsg->parameters.getCount() > 2 && pmsg->m_bIncoming && IsChannel(pmsg->parameters[1])) {
		CMStringW sPassword = L"";
		CMStringW sLimit = L"";
		bool bAdd = false;
		int iParametercount = 3;

		LPCTSTR p1 = pmsg->parameters[2].c_str();
		while (*p1 != '\0') {
			if (*p1 == '+')
				bAdd = true;
			if (*p1 == '-')
				bAdd = false;
			if (*p1 == 'l' && bAdd) {
				if ((int)pmsg->parameters.getCount() > iParametercount)
					sLimit = pmsg->parameters[iParametercount];
				iParametercount++;
			}
			if (*p1 == 'k' && bAdd) {
				if ((int)pmsg->parameters.getCount() > iParametercount)
					sPassword = pmsg->parameters[iParametercount];
				iParametercount++;
			}

			p1++;
		}

		AddWindowItemData(pmsg->parameters[1].c_str(), sLimit.IsEmpty() ? 0 : sLimit.c_str(), pmsg->parameters[2].c_str(), sPassword.IsEmpty() ? 0 : sPassword.c_str(), 0);
	}
	ShowMessage(pmsg);
	return true;
}

bool CIrcProto::OnIrc_MODE(const CIrcMessage* pmsg)
{
	bool flag = false;
	bool bContainsValidModes = false;
	CMStringW sModes = L"";
	CMStringW sParams = L"";

	if (pmsg->parameters.getCount() > 1 && pmsg->m_bIncoming) {
		if (IsChannel(pmsg->parameters[0])) {
			bool bAdd = false;
			int  iParametercount = 2;
			LPCTSTR p1 = pmsg->parameters[1].c_str();

			while (*p1 != '\0') {
				if (*p1 == '+') {
					bAdd = true;
					sModes += L"+";
				}
				if (*p1 == '-') {
					bAdd = false;
					sModes += L"-";
				}
				if (*p1 == 'l' && bAdd && iParametercount < (int)pmsg->parameters.getCount()) {
					bContainsValidModes = true;
					sModes += L"l";
					sParams += L" " + pmsg->parameters[iParametercount];
					iParametercount++;
				}
				if (*p1 == 'b' || *p1 == 'k' && iParametercount < (int)pmsg->parameters.getCount()) {
					bContainsValidModes = true;
					sModes += *p1;
					sParams += L" " + pmsg->parameters[iParametercount];
					iParametercount++;
				}
				if (strchr(sUserModes.c_str(), (char)*p1)) {
					CMStringW sStatus = ModeToStatus(*p1);
					if ((int)pmsg->parameters.getCount() > iParametercount) {
						if (!mir_wstrcmp(pmsg->parameters[2].c_str(), m_info.sNick.c_str())) {
							char cModeBit = -1;
							CHANNELINFO *wi = (CHANNELINFO *)DoEvent(GC_EVENT_GETITEMDATA, pmsg->parameters[0].c_str(), NULL, NULL, NULL, NULL, NULL, false, false, 0);
							switch (*p1) {
							case 'v':      cModeBit = 0;       break;
							case 'h':      cModeBit = 1;       break;
							case 'o':      cModeBit = 2;       break;
							case 'a':      cModeBit = 3;       break;
							case 'q':      cModeBit = 4;       break;
							}

							// set bit for own mode on this channel (voice/hop/op/admin/owner)
							if (bAdd && cModeBit >= 0)
								wi->OwnMode |= (1 << cModeBit);
							else
								wi->OwnMode &= ~(1 << cModeBit);

							DoEvent(GC_EVENT_SETITEMDATA, pmsg->parameters[0].c_str(), NULL, NULL, NULL, NULL, (DWORD_PTR)wi, false, false, 0);
						}
						DoEvent(bAdd ? GC_EVENT_ADDSTATUS : GC_EVENT_REMOVESTATUS, pmsg->parameters[0].c_str(), pmsg->parameters[iParametercount].c_str(), pmsg->prefix.sNick.c_str(), sStatus.c_str(), NULL, NULL, m_oldStyleModes ? false : true, false);
						iParametercount++;
					}
				}
				else if (*p1 != 'b' && *p1 != ' ' && *p1 != '+' && *p1 != '-') {
					bContainsValidModes = true;
					if (*p1 != 'l' && *p1 != 'k')
						sModes += *p1;
					flag = true;
				}

				p1++;
			}

			if (m_oldStyleModes) {
				wchar_t temp[256];
				mir_snwprintf(temp, TranslateT("%s sets mode %s"),
					pmsg->prefix.sNick.c_str(), pmsg->parameters[1].c_str());

				CMStringW sMessage = temp;
				for (int i = 2; i < (int)pmsg->parameters.getCount(); i++)
					sMessage += L" " + pmsg->parameters[i];

				DoEvent(GC_EVENT_INFORMATION, pmsg->parameters[0].c_str(), pmsg->prefix.sNick.c_str(), sMessage.c_str(), NULL, NULL, NULL, true, false);
			}
			else if (bContainsValidModes) {
				for (int i = iParametercount; i < (int)pmsg->parameters.getCount(); i++)
					sParams += L" " + pmsg->parameters[i];

				wchar_t temp[4000];
				mir_snwprintf(temp, TranslateT("%s sets mode %s%s"), pmsg->prefix.sNick.c_str(), sModes.c_str(), sParams.c_str());
				DoEvent(GC_EVENT_INFORMATION, pmsg->parameters[0].c_str(), pmsg->prefix.sNick.c_str(), temp, NULL, NULL, NULL, true, false);
			}

			if (flag)
				PostIrcMessage(L"/MODE %s", pmsg->parameters[0].c_str());
		}
		else {
			wchar_t temp[256];
			mir_snwprintf(temp, TranslateT("%s sets mode %s"), pmsg->prefix.sNick.c_str(), pmsg->parameters[1].c_str());

			CMStringW sMessage = temp;
			for (int i = 2; i < (int)pmsg->parameters.getCount(); i++)
				sMessage += L" " + pmsg->parameters[i];

			DoEvent(GC_EVENT_INFORMATION, SERVERWINDOW, pmsg->prefix.sNick.c_str(), sMessage.c_str(), NULL, NULL, NULL, true, false);
		}
	}
	else ShowMessage(pmsg);

	return true;
}

bool CIrcProto::OnIrc_NICK(const CIrcMessage* pmsg)
{
	if (pmsg->m_bIncoming && pmsg->parameters.getCount() > 0) {
		bool bIsMe = pmsg->prefix.sNick.c_str() == m_info.sNick ? true : false;

		if (m_info.sNick == pmsg->prefix.sNick && pmsg->parameters.getCount() > 0) {
			m_info.sNick = pmsg->parameters[0];
			setWString("Nick", m_info.sNick.c_str());
		}

		CMStringW host = pmsg->prefix.sUser + L"@" + pmsg->prefix.sHost;
		DoEvent(GC_EVENT_NICK, NULL, pmsg->prefix.sNick.c_str(), pmsg->parameters[0].c_str(), NULL, host.c_str(), NULL, true, bIsMe);
		DoEvent(GC_EVENT_CHUID, NULL, pmsg->prefix.sNick.c_str(), pmsg->parameters[0].c_str(), NULL, NULL, NULL, true, false);

		struct CONTACT user = { (wchar_t*)pmsg->prefix.sNick.c_str(), (wchar_t*)pmsg->prefix.sUser.c_str(), (wchar_t*)pmsg->prefix.sHost.c_str(), false, false, false };
		MCONTACT hContact = CList_FindContact(&user);
		if (hContact) {
			if (getWord(hContact, "Status", ID_STATUS_OFFLINE) == ID_STATUS_OFFLINE)
				setWord(hContact, "Status", ID_STATUS_ONLINE);
			setWString(hContact, "Nick", pmsg->parameters[0].c_str());
			setWString(hContact, "User", pmsg->prefix.sUser.c_str());
			setWString(hContact, "Host", pmsg->prefix.sHost.c_str());
		}
	}
	else ShowMessage(pmsg);

	return true;
}

bool CIrcProto::OnIrc_NOTICE(const CIrcMessage* pmsg)
{
	if (pmsg->m_bIncoming && pmsg->parameters.getCount() > 1) {
		if (IsCTCP(pmsg))
			return true;

		if (!m_ignore || !IsIgnored(pmsg->prefix.sNick, pmsg->prefix.sUser, pmsg->prefix.sHost, 'n')) {
			CMStringW S;
			CMStringW S2;
			CMStringW S3;
			if (pmsg->prefix.sNick.GetLength() > 0)
				S = pmsg->prefix.sNick;
			else
				S = m_info.sNetwork;
			S3 = m_info.sNetwork;
			if (IsChannel(pmsg->parameters[0]))
				S2 = pmsg->parameters[0].c_str();
			else {
				GC_INFO gci = { 0 };
				gci.Flags = GCF_BYID | GCF_TYPE;
				gci.pszModule = m_szModuleName;

				CMStringW str = GetWord(pmsg->parameters[1].c_str(), 0);
				if (str[0] == '[' && str[1] == '#' && str[str.GetLength() - 1] == ']') {
					str.Delete(str.GetLength() - 1, 1);
					str.Delete(0, 1);
					CMStringW Wnd = MakeWndID(str.c_str());
					gci.pszID = Wnd.c_str();
					if (!CallServiceSync(MS_GC_GETINFO, 0, (LPARAM)&gci) && gci.iType == GCW_CHATROOM)
						S2 = GetWord(gci.pszID, 0);
					else
						S2 = L"";
				}
				else S2 = L"";
			}
			DoEvent(GC_EVENT_NOTICE, S2.IsEmpty() ? 0 : S2.c_str(), S.c_str(), pmsg->parameters[1].c_str(), NULL, S3.c_str(), NULL, true, false);
		}
	}
	else ShowMessage(pmsg);

	return true;
}

bool CIrcProto::OnIrc_YOURHOST(const CIrcMessage* pmsg)
{
	if (pmsg->m_bIncoming) {
		static const wchar_t* lpszFmt = L"Your host is %99[^ \x5b,], running version %99s";
		wchar_t szHostName[100], szVersion[100];
		if (swscanf(pmsg->parameters[1].c_str(), lpszFmt, &szHostName, &szVersion) > 0)
			m_info.sServerName = szHostName;
		if (pmsg->parameters[0] != m_info.sNick)
			m_info.sNick = pmsg->parameters[0];
	}

	ShowMessage(pmsg);
	return true;
}

bool CIrcProto::OnIrc_INVITE(const CIrcMessage* pmsg)
{
	if (pmsg->m_bIncoming && (m_ignore && IsIgnored(pmsg->prefix.sNick, pmsg->prefix.sUser, pmsg->prefix.sHost, 'i')))
		return true;

	if (pmsg->m_bIncoming && m_joinOnInvite && pmsg->parameters.getCount() > 1 && mir_wstrcmpi(pmsg->parameters[0].c_str(), m_info.sNick.c_str()) == 0)
		PostIrcMessage(L"/JOIN %s", pmsg->parameters[1].c_str());

	ShowMessage(pmsg);
	return true;
}

bool CIrcProto::OnIrc_PINGPONG(const CIrcMessage* pmsg)
{
	if (pmsg->m_bIncoming && pmsg->sCommand == L"PING") {
		wchar_t szResponse[100];
		mir_snwprintf(szResponse, L"PONG %s", pmsg->parameters[0].c_str());
		SendIrcMessage(szResponse);
	}

	return true;
}

bool CIrcProto::OnIrc_PRIVMSG(const CIrcMessage* pmsg)
{
	if (pmsg->parameters.getCount() > 1) {
		if (IsCTCP(pmsg))
			return true;

		CMStringW mess = pmsg->parameters[1];
		bool bIsChannel = IsChannel(pmsg->parameters[0]);

		if (pmsg->m_bIncoming && !bIsChannel) {
			mess = DoColorCodes(mess.c_str(), TRUE, FALSE);

			struct CONTACT user = { (wchar_t*)pmsg->prefix.sNick.c_str(), (wchar_t*)pmsg->prefix.sUser.c_str(), (wchar_t*)pmsg->prefix.sHost.c_str(), false, false, false };

			if (CallService(MS_IGNORE_ISIGNORED, NULL, IGNOREEVENT_MESSAGE))
			if (!CList_FindContact(&user))
				return true;

			if ((m_ignore && IsIgnored(pmsg->prefix.sNick, pmsg->prefix.sUser, pmsg->prefix.sHost, 'q'))) {
				MCONTACT hContact = CList_FindContact(&user);
				if (!hContact || (hContact && db_get_b(hContact, "CList", "Hidden", 0) == 1))
					return true;
			}

			MCONTACT hContact = CList_AddContact(&user, false, true);

			PROTORECVEVENT pre = { 0 };
			pre.timestamp = (DWORD)time(NULL);
			pre.szMessage = mir_utf8encodeW(mess.c_str());
			setWString(hContact, "User", pmsg->prefix.sUser.c_str());
			setWString(hContact, "Host", pmsg->prefix.sHost.c_str());
			ProtoChainRecvMsg(hContact, &pre);
			mir_free(pre.szMessage);
			return true;
		}

		if (bIsChannel) {
			if (!(pmsg->m_bIncoming && m_ignore && IsIgnored(pmsg->prefix.sNick, pmsg->prefix.sUser, pmsg->prefix.sHost, 'm'))) {
				if (!pmsg->m_bIncoming)
					mess.Replace(L"%%", L"%");
				DoEvent(GC_EVENT_MESSAGE, pmsg->parameters[0].c_str(), pmsg->m_bIncoming ? pmsg->prefix.sNick.c_str() : m_info.sNick.c_str(), mess.c_str(), NULL, NULL, NULL, true, pmsg->m_bIncoming ? false : true);
			}
			return true;
		}
	}

	ShowMessage(pmsg);
	return true;
}

bool CIrcProto::IsCTCP(const CIrcMessage* pmsg)
{
	// is it a ctcp command, i e is the first and last characer of a PRIVMSG or NOTICE text ASCII 1
	CMStringW mess = pmsg->parameters[1];
	if (!(mess.GetLength() > 3 && mess[0] == 1 && mess[mess.GetLength() - 1] == 1))
		return false;

	// set mess to contain the ctcp command, excluding the leading and trailing  ASCII 1
	mess.Delete(0, 1);
	mess.Delete(mess.GetLength() - 1, 1);

	// exploit???
	if (mess.Find(1) != -1 || mess.Find(L"%newl") != -1) {
		wchar_t temp[4096];
		mir_snwprintf(temp, TranslateT("CTCP ERROR: Malformed CTCP command received from %s!%s@%s. Possible attempt to take control of your IRC client registered"), pmsg->prefix.sNick.c_str(), pmsg->prefix.sUser.c_str(), pmsg->prefix.sHost.c_str());
		DoEvent(GC_EVENT_INFORMATION, 0, m_info.sNick.c_str(), temp, NULL, NULL, NULL, true, false);
		return true;
	}

	// extract the type of ctcp command
	CMStringW ocommand = GetWord(mess.c_str(), 0);
	CMStringW command = GetWord(mess.c_str(), 0);
	command.MakeLower();

	// should it be ignored?
	if (m_ignore) {
		if (IsChannel(pmsg->parameters[0])) {
			if (command == L"action" && IsIgnored(pmsg->prefix.sNick, pmsg->prefix.sUser, pmsg->prefix.sHost, 'm'))
				return true;
		}
		else {
			if (command == L"action") {
				if (IsIgnored(pmsg->prefix.sNick, pmsg->prefix.sUser, pmsg->prefix.sHost, 'q'))
					return true;
			}
			else if (command == L"dcc") {
				if (IsIgnored(pmsg->prefix.sNick, pmsg->prefix.sUser, pmsg->prefix.sHost, 'd'))
					return true;
			}
			else if (IsIgnored(pmsg->prefix.sNick, pmsg->prefix.sUser, pmsg->prefix.sHost, 'c'))
				return true;
		}
	}

	if (pmsg->sCommand == L"PRIVMSG") {
		// incoming ACTION
		if (command == L"action") {
			mess.Delete(0, 6);

			if (IsChannel(pmsg->parameters[0])) {
				if (mess.GetLength() > 1) {
					mess.Delete(0, 1);
					if (!pmsg->m_bIncoming)
						mess.Replace(L"%%", L"%");

					DoEvent(GC_EVENT_ACTION, pmsg->parameters[0].c_str(), pmsg->m_bIncoming ? pmsg->prefix.sNick.c_str() : m_info.sNick.c_str(), mess.c_str(), NULL, NULL, NULL, true, pmsg->m_bIncoming ? false : true);
				}
			}
			else if (pmsg->m_bIncoming) {
				mess.Insert(0, pmsg->prefix.sNick.c_str());
				mess.Insert(0, L"* ");
				mess.Insert(mess.GetLength(), L" *");
				CIrcMessage msg = *pmsg;
				msg.parameters[1] = mess;
				OnIrc_PRIVMSG(&msg);
			}
		}
		// incoming FINGER
		else if (pmsg->m_bIncoming && command == L"finger") {
			PostIrcMessage(L"/NOTICE %s \001FINGER %s (%s)\001", pmsg->prefix.sNick.c_str(), m_name, m_userID);

			wchar_t temp[300];
			mir_snwprintf(temp, TranslateT("CTCP FINGER requested by %s"), pmsg->prefix.sNick.c_str());
			DoEvent(GC_EVENT_INFORMATION, SERVERWINDOW, NULL, temp, NULL, NULL, NULL, true, false);
		}

		// incoming VERSION
		else if (pmsg->m_bIncoming && command == L"version") {
			PostIrcMessage(L"/NOTICE %s \001VERSION Miranda NG %%mirver (IRC v.%%version)" L", " _A2W(__COPYRIGHT) L"\001", pmsg->prefix.sNick.c_str());

			wchar_t temp[300];
			mir_snwprintf(temp, TranslateT("CTCP VERSION requested by %s"), pmsg->prefix.sNick.c_str());
			DoEvent(GC_EVENT_INFORMATION, SERVERWINDOW, NULL, temp, NULL, NULL, NULL, true, false);
		}

		// incoming SOURCE
		else if (pmsg->m_bIncoming && command == L"source") {
			PostIrcMessage(L"/NOTICE %s \001SOURCE Get Miranda IRC here: http://miranda-ng.org/ \001", pmsg->prefix.sNick.c_str());

			wchar_t temp[300];
			mir_snwprintf(temp, TranslateT("CTCP SOURCE requested by %s"), pmsg->prefix.sNick.c_str());
			DoEvent(GC_EVENT_INFORMATION, SERVERWINDOW, NULL, temp, NULL, NULL, NULL, true, false);
		}

		// incoming USERINFO
		else if (pmsg->m_bIncoming && command == L"userinfo") {
			PostIrcMessage(L"/NOTICE %s \001USERINFO %s\001", pmsg->prefix.sNick.c_str(), m_userInfo);

			wchar_t temp[300];
			mir_snwprintf(temp, TranslateT("CTCP USERINFO requested by %s"), pmsg->prefix.sNick.c_str());
			DoEvent(GC_EVENT_INFORMATION, SERVERWINDOW, NULL, temp, NULL, NULL, NULL, true, false);
		}

		// incoming PING
		else if (pmsg->m_bIncoming && command == L"ping") {
			PostIrcMessage(L"/NOTICE %s \001%s\001", pmsg->prefix.sNick.c_str(), mess.c_str());

			wchar_t temp[300];
			mir_snwprintf(temp, TranslateT("CTCP PING requested by %s"), pmsg->prefix.sNick.c_str());
			DoEvent(GC_EVENT_INFORMATION, SERVERWINDOW, NULL, temp, NULL, NULL, NULL, true, false);
		}

		// incoming TIME
		else if (pmsg->m_bIncoming && command == L"time") {
			wchar_t temp[300];
			time_t tim = time(NULL);
			mir_wstrncpy(temp, _wctime(&tim), 25);
			PostIrcMessage(L"/NOTICE %s \001TIME %s\001", pmsg->prefix.sNick.c_str(), temp);

			mir_snwprintf(temp, TranslateT("CTCP TIME requested by %s"), pmsg->prefix.sNick.c_str());
			DoEvent(GC_EVENT_INFORMATION, SERVERWINDOW, NULL, temp, NULL, NULL, NULL, true, false);
		}

		// incoming DCC request... lots of stuff happening here...
		else if (pmsg->m_bIncoming && command == L"dcc") {
			CMStringW type = GetWord(mess.c_str(), 1);
			type.MakeLower();

			// components of a dcc message
			CMStringW sFile = L"";
			DWORD dwAdr = 0;
			int iPort = 0;
			unsigned __int64 dwSize = 0;
			CMStringW sToken = L"";
			bool bIsChat = (type == L"chat");

			// 1. separate the dcc command into the correct pieces
			if (bIsChat || type == L"send") {
				// if the filename is surrounded by quotes, do this
				if (GetWord(mess.c_str(), 2)[0] == '\"') {
					int end = 0;
					int begin = mess.Find('\"', 0);
					if (begin >= 0) {
						end = mess.Find('\"', begin + 1);
						if (end >= 0) {
							sFile = mess.Mid(begin + 1, end - begin - 1);

							begin = mess.Find(' ', end);
							if (begin >= 0) {
								CMStringW rest = mess.Mid(begin);
								dwAdr = wcstoul(GetWord(rest.c_str(), 0).c_str(), NULL, 10);
								iPort = _wtoi(GetWord(rest.c_str(), 1).c_str());
								dwSize = _wtoi64(GetWord(rest.c_str(), 2).c_str());
								sToken = GetWord(rest.c_str(), 3);
							}
						}
					}
				}
				// ... or try another method of separating the dcc command
				else if (!GetWord(mess.c_str(), (bIsChat) ? 4 : 5).IsEmpty()) {
					int index = (bIsChat) ? 4 : 5;
					bool bFlag = false;

					// look for the part of the ctcp command that contains adress, port and size
					while (!bFlag && !GetWord(mess.c_str(), index).IsEmpty()) {
						CMStringW sTemp;

						if (type == L"chat")
							sTemp = GetWord(mess.c_str(), index - 1) + GetWord(mess.c_str(), index);
						else
							sTemp = GetWord(mess.c_str(), index - 2) + GetWord(mess.c_str(), index - 1) + GetWord(mess.c_str(), index);

						// if all characters are number it indicates we have found the adress, port and size parameters
						int ind = 0;
						while (sTemp[ind] != '\0') {
							if (!iswdigit(sTemp[ind]))
								break;
							ind++;
						}

						if (sTemp[ind] == '\0' && GetWord(mess.c_str(), index + ((bIsChat) ? 1 : 2)).IsEmpty())
							bFlag = true;
						index++;
					}

					if (bFlag) {
						wchar_t* p1 = wcsdup(GetWordAddress(mess.c_str(), 2));
						wchar_t* p2 = (wchar_t*)GetWordAddress(p1, index - 5);

						if (type == L"send") {
							if (p2 > p1) {
								p2--;
								while (p2 != p1 && *p2 == ' ') {
									*p2 = '\0';
									p2--;
								}
								sFile = p1;
							}
						}
						else sFile = L"chat";

						free(p1);

						dwAdr = wcstoul(GetWord(mess.c_str(), index - (bIsChat ? 2 : 3)).c_str(), NULL, 10);
						iPort = _wtoi(GetWord(mess.c_str(), index - (bIsChat ? 1 : 2)).c_str());
						dwSize = _wtoi64(GetWord(mess.c_str(), index - 1).c_str());
						sToken = GetWord(mess.c_str(), index);
					}
				}
			}
			else if (type == L"accept" || type == L"resume") {
				// if the filename is surrounded by quotes, do this
				if (GetWord(mess.c_str(), 2)[0] == '\"') {
					int end = 0;
					int begin = mess.Find('\"', 0);
					if (begin >= 0) {
						end = mess.Find('\"', begin + 1);
						if (end >= 0) {
							sFile = mess.Mid(begin + 1, end);

							begin = mess.Find(' ', end);
							if (begin >= 0) {
								CMStringW rest = mess.Mid(begin);
								iPort = _wtoi(GetWord(rest.c_str(), 0).c_str());
								dwSize = _wtoi(GetWord(rest.c_str(), 1).c_str());
								sToken = GetWord(rest.c_str(), 2);
							}
						}
					}
				}
				// ... or try another method of separating the dcc command
				else if (!GetWord(mess.c_str(), 4).IsEmpty()) {
					int index = 4;
					bool bFlag = false;

					// look for the part of the ctcp command that contains adress, port and size
					while (!bFlag && !GetWord(mess.c_str(), index).IsEmpty()) {
						CMStringW sTemp = GetWord(mess.c_str(), index - 1) + GetWord(mess.c_str(), index);

						// if all characters are number it indicates we have found the adress, port and size parameters
						int ind = 0;

						while (sTemp[ind] != '\0') {
							if (!iswdigit(sTemp[ind]))
								break;
							ind++;
						}

						if (sTemp[ind] == '\0' && GetWord(mess.c_str(), index + 2).IsEmpty())
							bFlag = true;
						index++;
					}
					if (bFlag) {
						wchar_t* p1 = wcsdup(GetWordAddress(mess.c_str(), 2));
						wchar_t* p2 = (wchar_t*)GetWordAddress(p1, index - 4);

						if (p2 > p1) {
							p2--;
							while (p2 != p1 && *p2 == ' ') {
								*p2 = '\0';
								p2--;
							}
							sFile = p1;
						}

						free(p1);

						iPort = _wtoi(GetWord(mess.c_str(), index - 2).c_str());
						dwSize = _wtoi64(GetWord(mess.c_str(), index - 1).c_str());
						sToken = GetWord(mess.c_str(), index);
					}
				}
			}
			// end separating dcc commands

			// 2. Check for malformed dcc commands or other errors
			if (bIsChat || type == L"send") {
				wchar_t szTemp[256];
				szTemp[0] = '\0';

				unsigned long ulAdr = 0;
				if (m_manualHost)
					ulAdr = ConvertIPToInteger(m_mySpecifiedHostIP);
				else
					ulAdr = ConvertIPToInteger(m_IPFromServer ? m_myHost : m_myLocalHost);

				if (bIsChat && !m_DCCChatEnabled)
					mir_snwprintf(szTemp, TranslateT("DCC: Chat request from %s denied"), pmsg->prefix.sNick.c_str());

				else if (type == L"send" && !m_DCCFileEnabled)
					mir_snwprintf(szTemp, TranslateT("DCC: File transfer request from %s denied"), pmsg->prefix.sNick.c_str());

				else if (type == L"send" && !iPort && ulAdr == 0)
					mir_snwprintf(szTemp, TranslateT("DCC: Reverse file transfer request from %s denied [No local IP]"), pmsg->prefix.sNick.c_str());

				if (sFile.IsEmpty() || dwAdr == 0 || dwSize == 0 || iPort == 0 && sToken.IsEmpty())
					mir_snwprintf(szTemp, TranslateT("DCC ERROR: Malformed CTCP request from %s [%s]"), pmsg->prefix.sNick.c_str(), mess.c_str());

				if (szTemp[0]) {
					DoEvent(GC_EVENT_INFORMATION, 0, m_info.sNick.c_str(), szTemp, NULL, NULL, NULL, true, false);
					return true;
				}

				// remove path from the filename if the remote client (stupidly) sent it
				CMStringW sFileCorrected = sFile;
				int i = sFile.ReverseFind('\\');
				if (i != -1)
					sFileCorrected = sFile.Mid(i + 1);
				sFile = sFileCorrected;
			}
			else if (type == L"accept" || type == L"resume") {
				wchar_t szTemp[256];
				szTemp[0] = '\0';

				if (type == L"resume" && !m_DCCFileEnabled)
					mir_snwprintf(szTemp, TranslateT("DCC: File transfer resume request from %s denied"), pmsg->prefix.sNick.c_str());

				if (sToken.IsEmpty() && iPort == 0 || sFile.IsEmpty())
					mir_snwprintf(szTemp, TranslateT("DCC ERROR: Malformed CTCP request from %s [%s]"), pmsg->prefix.sNick.c_str(), mess.c_str());

				if (szTemp[0]) {
					DoEvent(GC_EVENT_INFORMATION, 0, m_info.sNick.c_str(), szTemp, NULL, NULL, NULL, true, false);
					return true;
				}

				// remove path from the filename if the remote client (stupidly) sent it
				CMStringW sFileCorrected = sFile;
				int i = sFile.ReverseFind('\\');
				if (i != -1)
					sFileCorrected = sFile.Mid(i + 1);
				sFile = sFileCorrected;
			}

			// 3. Take proper actions considering type of command

			// incoming chat request
			if (bIsChat) {
				CONTACT user = { (wchar_t*)pmsg->prefix.sNick.c_str(), 0, 0, false, false, true };
				MCONTACT hContact = CList_FindContact(&user);

				// check if it should be ignored
				if (m_DCCChatIgnore == 1 ||
					m_DCCChatIgnore == 2 && hContact &&
					db_get_b(hContact, "CList", "NotOnList", 0) == 0 &&
					db_get_b(hContact, "CList", "Hidden", 0) == 0) {
					CMStringW host = pmsg->prefix.sUser + L"@" + pmsg->prefix.sHost;
					CList_AddDCCChat(pmsg->prefix.sNick, host, dwAdr, iPort); // add a CHAT event to the clist
				}
				else {
					wchar_t szTemp[512];
					mir_snwprintf(szTemp, TranslateT("DCC: Chat request from %s denied"), pmsg->prefix.sNick.c_str());
					DoEvent(GC_EVENT_INFORMATION, 0, m_info.sNick.c_str(), szTemp, NULL, NULL, NULL, true, false);
				}
			}

			// remote requested that the file should be resumed
			if (type == L"resume") {
				CDccSession* dcc;
				if (sToken.IsEmpty())
					dcc = FindDCCSendByPort(iPort);
				else
					dcc = FindPassiveDCCSend(_wtoi(sToken.c_str())); // reverse ft

				if (dcc) {
					InterlockedExchange(&dcc->dwWhatNeedsDoing, (long)FILERESUME_RESUME);
					dcc->dwResumePos = dwSize; // dwSize is the resume position
					PostIrcMessage(L"/PRIVMSG %s \001DCC ACCEPT %s\001", pmsg->prefix.sNick.c_str(), GetWordAddress(mess.c_str(), 2));
				}
			}

			// remote accepted your request for a file resume
			if (type == L"accept") {
				CDccSession* dcc;
				if (sToken.IsEmpty())
					dcc = FindDCCRecvByPortAndName(iPort, pmsg->prefix.sNick.c_str());
				else
					dcc = FindPassiveDCCRecv(pmsg->prefix.sNick, sToken); // reverse ft

				if (dcc) {
					InterlockedExchange(&dcc->dwWhatNeedsDoing, (long)FILERESUME_RESUME);
					dcc->dwResumePos = dwSize;	// dwSize is the resume position					
					SetEvent(dcc->hEvent);
				}
			}

			if (type == L"send") {
				CMStringW sTokenBackup = sToken;
				bool bTurbo = false; // TDCC indicator

				if (!sToken.IsEmpty() && sToken[sToken.GetLength() - 1] == 'T') {
					bTurbo = true;
					sToken.Delete(sToken.GetLength() - 1, 1);
				}

				// if a token exists and the port is non-zero it is the remote
				// computer telling us that is has accepted to act as server for
				// a reverse filetransfer. The plugin should connect to that computer
				// and start sedning the file (if the token is valid). Compare to DCC RECV
				if (!sToken.IsEmpty() && iPort) {
					CDccSession* dcc = FindPassiveDCCSend(_wtoi(sToken.c_str()));
					if (dcc) {
						dcc->SetupPassive(dwAdr, iPort);
						dcc->Connect();
					}
				}
				else {
					struct CONTACT user = { (wchar_t*)pmsg->prefix.sNick.c_str(), (wchar_t*)pmsg->prefix.sUser.c_str(), (wchar_t*)pmsg->prefix.sHost.c_str(), false, false, false };
					if (CallService(MS_IGNORE_ISIGNORED, NULL, IGNOREEVENT_FILE))
					if (!CList_FindContact(&user))
						return true;

					MCONTACT hContact = CList_AddContact(&user, false, true);
					if (hContact) {
						DCCINFO* di = new DCCINFO;
						di->hContact = hContact;
						di->sFile = sFile;
						di->dwSize = dwSize;
						di->sContactName = pmsg->prefix.sNick;
						di->dwAdr = dwAdr;
						di->iPort = iPort;
						di->iType = DCC_SEND;
						di->bSender = false;
						di->bTurbo = bTurbo;
						di->bSSL = false;
						di->bReverse = (iPort == 0 && !sToken.IsEmpty()) ? true : false;
						if (di->bReverse)
							di->sToken = sTokenBackup;

						setWString(hContact, "User", pmsg->prefix.sUser.c_str());
						setWString(hContact, "Host", pmsg->prefix.sHost.c_str());

						wchar_t* tszTemp = (wchar_t*)sFile.c_str();

						PROTORECVFILET pre = { 0 };
						pre.dwFlags = PRFF_UNICODE;
						pre.timestamp = (DWORD)time(NULL);
						pre.fileCount = 1;
						pre.files.w = &tszTemp;
						pre.lParam = (LPARAM)di;
						ProtoChainRecvFile(hContact, &pre);
					}
				}
			}
			// end type == "send"
		}
		else if (pmsg->m_bIncoming) {
			wchar_t temp[300];
			mir_snwprintf(temp, TranslateT("CTCP %s requested by %s"), ocommand.c_str(), pmsg->prefix.sNick.c_str());
			DoEvent(GC_EVENT_INFORMATION, SERVERWINDOW, NULL, temp, NULL, NULL, NULL, true, false);
		}
	}

	// handle incoming ctcp in notices. This technique is used for replying to CTCP queries
	else if (pmsg->sCommand == L"NOTICE") {
		wchar_t szTemp[300];
		szTemp[0] = '\0';

		//if we got incoming CTCP Version for contact in CList - then write its as MirVer for that contact!
		if (pmsg->m_bIncoming && command == L"version") {
			struct CONTACT user = { (wchar_t*)pmsg->prefix.sNick.c_str(), (wchar_t*)pmsg->prefix.sUser.c_str(), (wchar_t*)pmsg->prefix.sHost.c_str(), false, false, false };
			MCONTACT hContact = CList_FindContact(&user);
			if (hContact)
				setWString(hContact, "MirVer", DoColorCodes(GetWordAddress(mess.c_str(), 1), TRUE, FALSE));
		}

		// if the whois window is visible and the ctcp reply belongs to the user in it, then show the reply in the whois window
		if (m_whoisDlg && IsWindowVisible(m_whoisDlg->GetHwnd())) {
			m_whoisDlg->m_InfoNick.GetText(szTemp, _countof(szTemp));
			if (mir_wstrcmpi(szTemp, pmsg->prefix.sNick.c_str()) == 0) {
				if (pmsg->m_bIncoming && (command == L"version" || command == L"userinfo" || command == L"time")) {
					SetActiveWindow(m_whoisDlg->GetHwnd());
					m_whoisDlg->m_Reply.SetText(DoColorCodes(GetWordAddress(mess.c_str(), 1), TRUE, FALSE));
					return true;
				}
				if (pmsg->m_bIncoming && command == L"ping") {
					SetActiveWindow(m_whoisDlg->GetHwnd());
					int s = (int)time(0) - (int)_wtol(GetWordAddress(mess.c_str(), 1));
					wchar_t szTmp[30];
					if (s == 1)
						mir_snwprintf(szTmp, TranslateT("%u second"), s);
					else
						mir_snwprintf(szTmp, TranslateT("%u seconds"), s);

					m_whoisDlg->m_Reply.SetText(DoColorCodes(szTmp, TRUE, FALSE));
					return true;
				}
			}
		}

		//... else show the reply in the current window
		if (pmsg->m_bIncoming && command == L"ping") {
			int s = (int)time(0) - (int)_wtol(GetWordAddress(mess.c_str(), 1));
			mir_snwprintf(szTemp, TranslateT("CTCP PING reply from %s: %u sec(s)"), pmsg->prefix.sNick.c_str(), s);
			DoEvent(GC_EVENT_INFORMATION, SERVERWINDOW, NULL, szTemp, NULL, NULL, NULL, true, false);
		}
		else {
			mir_snwprintf(szTemp, TranslateT("CTCP %s reply from %s: %s"), ocommand.c_str(), pmsg->prefix.sNick.c_str(), GetWordAddress(mess.c_str(), 1));
			DoEvent(GC_EVENT_INFORMATION, SERVERWINDOW, NULL, szTemp, NULL, NULL, NULL, true, false);
		}
	}

	return true;
}

bool CIrcProto::OnIrc_NAMES(const CIrcMessage* pmsg)
{
	if (pmsg->m_bIncoming && pmsg->parameters.getCount() > 3)
		sNamesList += pmsg->parameters[3] + L" ";
	ShowMessage(pmsg);
	return true;
}

bool CIrcProto::OnIrc_ENDNAMES(const CIrcMessage* pmsg)
{
	if (pmsg->m_bIncoming && pmsg->parameters.getCount() > 1) {
		CMStringW name = L"a";
		int i = 0;
		BOOL bFlag = false;

		// Is the user on the names list?
		while (!name.IsEmpty()) {
			name = GetWord(sNamesList.c_str(), i);
			i++;
			if (!name.IsEmpty()) {
				int index = 0;
				while (wcschr(sUserModePrefixes.c_str(), name[index]))
					index++;

				if (!mir_wstrcmpi(name.Mid(index).c_str(), m_info.sNick.c_str())) {
					bFlag = true;
					break;
				}
			}
		}

		if (bFlag) {
			const wchar_t* sChanName = pmsg->parameters[1].c_str();
			if (sChanName[0] == '@' || sChanName[0] == '*' || sChanName[0] == '=')
				sChanName++;

			// Add a new chat window
			CMStringW sID = MakeWndID(sChanName);
			BYTE btOwnMode = 0;

			GCSESSION gcw = { sizeof(gcw) };
			gcw.iType = GCW_CHATROOM;
			gcw.ptszID = sID.c_str();
			gcw.pszModule = m_szModuleName;
			gcw.ptszName = sChanName;
			if (!CallServiceSync(MS_GC_NEWSESSION, 0, (LPARAM)&gcw)) {
				DBVARIANT dbv;
				GCDEST gcd = { m_szModuleName, sID.c_str(), GC_EVENT_ADDGROUP };
				GCEVENT gce = { sizeof(gce), &gcd };

				PostIrcMessage(L"/MODE %s", sChanName);

				// register the statuses
				gce.ptszStatus = L"Owner";
				CallChatEvent(0, (LPARAM)&gce);
				gce.ptszStatus = L"Admin";
				CallChatEvent(0, (LPARAM)&gce);
				gce.ptszStatus = L"Op";
				CallChatEvent(0, (LPARAM)&gce);
				gce.ptszStatus = L"Halfop";
				CallChatEvent(0, (LPARAM)&gce);
				gce.ptszStatus = L"Voice";
				CallChatEvent(0, (LPARAM)&gce);
				gce.ptszStatus = L"Normal";
				CallChatEvent(0, (LPARAM)&gce);
				{
					int k = 0;
					CMStringW sTemp = GetWord(sNamesList.c_str(), k);

					// Fill the nicklist
					while (!sTemp.IsEmpty()) {
						CMStringW sStat;
						CMStringW sTemp2 = sTemp;
						sStat = PrefixToStatus(sTemp[0]);

						// fix for networks like freshirc where they allow more than one prefix
						while (PrefixToStatus(sTemp[0]) != L"Normal")
							sTemp.Delete(0, 1);

						gcd.iType = GC_EVENT_JOIN;
						gce.ptszUID = sTemp.c_str();
						gce.ptszNick = sTemp.c_str();
						gce.ptszStatus = sStat.c_str();
						BOOL bIsMe = (!mir_wstrcmpi(gce.ptszNick, m_info.sNick.c_str())) ? TRUE : FALSE;
						if (bIsMe) {
							char BitNr = -1;
							switch (sTemp2[0]) {
							case '+':   BitNr = 0;   break;
							case '%':   BitNr = 1;   break;
							case '@':   BitNr = 2;   break;
							case '!':   BitNr = 3;   break;
							case '*':   BitNr = 4;   break;
							}
							if (BitNr >= 0)
								btOwnMode = (1 << BitNr);
							else
								btOwnMode = 0;
						}
						gce.bIsMe = bIsMe;
						gce.time = bIsMe ? time(0) : 0;
						CallChatEvent(0, (LPARAM)&gce);
						DoEvent(GC_EVENT_SETCONTACTSTATUS, sChanName, sTemp.c_str(), NULL, NULL, NULL, ID_STATUS_ONLINE, FALSE, FALSE);
						// fix for networks like freshirc where they allow more than one prefix
						if (PrefixToStatus(sTemp2[0]) != L"Normal") {
							sTemp2.Delete(0, 1);
							sStat = PrefixToStatus(sTemp2[0]);
							while (sStat != L"Normal") {
								DoEvent(GC_EVENT_ADDSTATUS, sID.c_str(), sTemp.c_str(), L"system", sStat.c_str(), NULL, NULL, false, false, 0);
								sTemp2.Delete(0, 1);
								sStat = PrefixToStatus(sTemp2[0]);
							}
						}

						k++;
						sTemp = GetWord(sNamesList.c_str(), k);
					}
				}

				//Set the item data for the window
				{
					CHANNELINFO *wi = (CHANNELINFO *)DoEvent(GC_EVENT_GETITEMDATA, sChanName, NULL, NULL, NULL, NULL, NULL, FALSE, FALSE, 0);
					if (!wi)
						wi = new CHANNELINFO;
					wi->OwnMode = btOwnMode;
					wi->pszLimit = 0;
					wi->pszMode = 0;
					wi->pszPassword = 0;
					wi->pszTopic = 0;
					wi->codepage = getCodepage();
					DoEvent(GC_EVENT_SETITEMDATA, sChanName, NULL, NULL, NULL, NULL, (DWORD_PTR)wi, false, false, 0);

					if (!sTopic.IsEmpty() && !mir_wstrcmpi(GetWord(sTopic.c_str(), 0).c_str(), sChanName)) {
						DoEvent(GC_EVENT_TOPIC, sChanName, sTopicName.IsEmpty() ? NULL : sTopicName.c_str(), GetWordAddress(sTopic.c_str(), 1), NULL, sTopicTime.IsEmpty() ? NULL : sTopicTime.c_str(), NULL, true, false);
						AddWindowItemData(sChanName, 0, 0, 0, GetWordAddress(sTopic.c_str(), 1));
						sTopic = L"";
						sTopicName = L"";
						sTopicTime = L"";
					}	}

				gcd.ptszID = (wchar_t*)sID.c_str();
				gcd.iType = GC_EVENT_CONTROL;
				gce.cbSize = sizeof(GCEVENT);
				gce.dwFlags = 0;
				gce.bIsMe = false;
				gce.dwItemData = false;
				gce.ptszNick = NULL;
				gce.ptszStatus = NULL;
				gce.ptszText = NULL;
				gce.ptszUID = NULL;
				gce.ptszUserInfo = NULL;
				gce.time = time(0);
				gce.pDest = &gcd;

				if (!getWString("JTemp", &dbv)) {
					CMStringW command = L"a";
					CMStringW save = L"";
					int k = 0;

					while (!command.IsEmpty()) {
						command = GetWord(dbv.ptszVal, k);
						k++;
						if (!command.IsEmpty()) {
							CMStringW S = command.Mid(1);
							if (!mir_wstrcmpi(sChanName, S))
								break;

							save += command + L" ";
						}
					}

					if (!command.IsEmpty()) {
						save += GetWordAddress(dbv.ptszVal, k);
						switch (command[0]) {
						case 'M':
							CallChatEvent(WINDOW_HIDDEN, (LPARAM)&gce);
							break;
						case 'X':
							CallChatEvent(WINDOW_MAXIMIZE, (LPARAM)&gce);
							break;
						default:
							CallChatEvent(SESSION_INITDONE, (LPARAM)&gce);
							break;
						}
					}
					else CallChatEvent(SESSION_INITDONE, (LPARAM)&gce);

					if (save.IsEmpty())
						db_unset(NULL, m_szModuleName, "JTemp");
					else
						setWString("JTemp", save.c_str());
					db_free(&dbv);
				}
				else CallChatEvent(SESSION_INITDONE, (LPARAM)&gce);

				gcd.iType = GC_EVENT_CONTROL;
				gce.pDest = &gcd;
				CallChatEvent(SESSION_ONLINE, (LPARAM)&gce);
			}
		}
	}

	sNamesList = L"";
	ShowMessage(pmsg);
	return true;
}

bool CIrcProto::OnIrc_INITIALTOPIC(const CIrcMessage* pmsg)
{
	if (pmsg->m_bIncoming && pmsg->parameters.getCount() > 2) {
		AddWindowItemData(pmsg->parameters[1].c_str(), 0, 0, 0, pmsg->parameters[2].c_str());
		sTopic = pmsg->parameters[1] + L" " + pmsg->parameters[2];
		sTopicName = L"";
		sTopicTime = L"";
	}
	ShowMessage(pmsg);
	return true;
}

bool CIrcProto::OnIrc_INITIALTOPICNAME(const CIrcMessage* pmsg)
{
	if (pmsg->m_bIncoming && pmsg->parameters.getCount() > 3) {
		wchar_t tTimeBuf[128], *tStopStr;
		time_t ttTopicTime;
		sTopicName = pmsg->parameters[2];
		ttTopicTime = wcstol(pmsg->parameters[3].c_str(), &tStopStr, 10);
		wcsftime(tTimeBuf, 128, L"%#c", localtime(&ttTopicTime));
		sTopicTime = tTimeBuf;
	}
	ShowMessage(pmsg);
	return true;
}

bool CIrcProto::OnIrc_TOPIC(const CIrcMessage* pmsg)
{
	if (pmsg->parameters.getCount() > 1 && pmsg->m_bIncoming) {
		DoEvent(GC_EVENT_TOPIC, pmsg->parameters[0].c_str(), pmsg->prefix.sNick.c_str(), pmsg->parameters[1].c_str(), NULL, sTopicTime.IsEmpty() ? NULL : sTopicTime.c_str(), NULL, true, false);
		AddWindowItemData(pmsg->parameters[0].c_str(), 0, 0, 0, pmsg->parameters[1].c_str());
	}
	ShowMessage(pmsg);
	return true;
}

static void __stdcall sttShowDlgList(void* param)
{
	CIrcProto *ppro = (CIrcProto*)param;
	if (ppro->m_listDlg == NULL) {
		ppro->m_listDlg = new CListDlg(ppro);
		ppro->m_listDlg->Show();
	}
	SetEvent(ppro->m_evWndCreate);
}

bool CIrcProto::OnIrc_LISTSTART(const CIrcMessage* pmsg)
{
	if (pmsg->m_bIncoming) {
		CallFunctionAsync(sttShowDlgList, this);
		WaitForSingleObject(m_evWndCreate, INFINITE);
		m_channelNumber = 0;
	}

	ShowMessage(pmsg);
	return true;
}

bool CIrcProto::OnIrc_LIST(const CIrcMessage* pmsg)
{
	if (pmsg->m_bIncoming == 1 && m_listDlg && pmsg->parameters.getCount() > 2) {
		m_channelNumber++;
		LVITEM lvItem;
		HWND hListView = GetDlgItem(m_listDlg->GetHwnd(), IDC_INFO_LISTVIEW);
		lvItem.iItem = ListView_GetItemCount(hListView);
		lvItem.mask = LVIF_TEXT | LVIF_PARAM;
		lvItem.iSubItem = 0;
		lvItem.pszText = (wchar_t*)pmsg->parameters[1].c_str();
		lvItem.lParam = lvItem.iItem;
		lvItem.iItem = ListView_InsertItem(hListView, &lvItem);
		lvItem.mask = LVIF_TEXT;
		lvItem.iSubItem = 1;
		lvItem.pszText = (wchar_t*)pmsg->parameters[pmsg->parameters.getCount() - 2].c_str();
		ListView_SetItem(hListView, &lvItem);

		wchar_t* temp = mir_wstrdup(pmsg->parameters[pmsg->parameters.getCount() - 1]);
		wchar_t* find = wcsstr(temp, L"[+");
		wchar_t* find2 = wcsstr(temp, L"]");
		wchar_t* save = temp;
		if (find == temp && find2 != NULL && find + 8 >= find2) {
			temp = wcsstr(temp, L"]");
			if (mir_wstrlen(temp) > 1) {
				temp++;
				temp[0] = '\0';
				lvItem.iSubItem = 2;
				lvItem.pszText = save;
				ListView_SetItem(hListView, &lvItem);
				temp[0] = ' ';
				temp++;
			}
			else temp = save;
		}

		lvItem.iSubItem = 3;
		CMStringW S = DoColorCodes(temp, TRUE, FALSE);
		lvItem.pszText = (wchar_t*)S.c_str();
		ListView_SetItem(hListView, &lvItem);
		temp = save;
		mir_free(temp);

		int percent = 100;
		if (m_noOfChannels > 0)
			percent = (int)(m_channelNumber * 100) / m_noOfChannels;

		wchar_t text[100];
		if (percent < 100)
			mir_snwprintf(text, TranslateT("Downloading list (%u%%) - %u channels"), percent, m_channelNumber);
		else
			mir_snwprintf(text, TranslateT("Downloading list - %u channels"), m_channelNumber);
		m_listDlg->m_status.SetText(text);
	}

	return true;
}

bool CIrcProto::OnIrc_LISTEND(const CIrcMessage* pmsg)
{
	if (pmsg->m_bIncoming && m_listDlg) {
		EnableWindow(GetDlgItem(m_listDlg->GetHwnd(), IDC_JOIN), true);
		ListView_SetSelectionMark(GetDlgItem(m_listDlg->GetHwnd(), IDC_INFO_LISTVIEW), 0);
		ListView_SetColumnWidth(GetDlgItem(m_listDlg->GetHwnd(), IDC_INFO_LISTVIEW), 1, LVSCW_AUTOSIZE);
		ListView_SetColumnWidth(GetDlgItem(m_listDlg->GetHwnd(), IDC_INFO_LISTVIEW), 2, LVSCW_AUTOSIZE);
		ListView_SetColumnWidth(GetDlgItem(m_listDlg->GetHwnd(), IDC_INFO_LISTVIEW), 3, LVSCW_AUTOSIZE);
		m_listDlg->UpdateList();

		wchar_t text[100];
		mir_snwprintf(text, TranslateT("Done: %u channels"), m_channelNumber);
		int percent = 100;
		if (m_noOfChannels > 0)
			percent = (int)(m_channelNumber * 100) / m_noOfChannels;
		if (percent < 70) {
			mir_wstrcat(text, L" ");
			mir_wstrcat(text, TranslateT("(probably truncated by server)"));
		}
		SetDlgItemText(m_listDlg->GetHwnd(), IDC_TEXT, text);
	}
	ShowMessage(pmsg);
	return true;
}

bool CIrcProto::OnIrc_BANLIST(const CIrcMessage* pmsg)
{
	if (pmsg->m_bIncoming && pmsg->parameters.getCount() > 2) {
		if (m_managerDlg->GetHwnd() && (
			m_managerDlg->m_radio1.GetState() && pmsg->sCommand == L"367" ||
			m_managerDlg->m_radio2.GetState() && pmsg->sCommand == L"346" ||
			m_managerDlg->m_radio3.GetState() && pmsg->sCommand == L"348") &&
			!m_managerDlg->m_radio1.Enabled() && !m_managerDlg->m_radio2.Enabled() && !m_managerDlg->m_radio3.Enabled()) {
			CMStringW S = pmsg->parameters[2];
			if (pmsg->parameters.getCount() > 3) {
				S += L"   - ";
				S += pmsg->parameters[3];
				if (pmsg->parameters.getCount() > 4) {
					S += L" -  ( ";
					time_t time = _wtoi(pmsg->parameters[4].c_str());
					S += _wctime(&time);
					S.Replace(L"\n", L" ");
					S += L")";
				}
			}

			SendDlgItemMessage(m_managerDlg->GetHwnd(), IDC_LIST, LB_ADDSTRING, 0, (LPARAM)S.c_str());
		}
	}

	ShowMessage(pmsg);
	return true;
}

bool CIrcProto::OnIrc_BANLISTEND(const CIrcMessage* pmsg)
{
	if (pmsg->m_bIncoming && pmsg->parameters.getCount() > 1) {
		if (m_managerDlg->GetHwnd() &&
			(m_managerDlg->m_radio1.GetState() && pmsg->sCommand == L"368"
			|| m_managerDlg->m_radio2.GetState() && pmsg->sCommand == L"347"
			|| m_managerDlg->m_radio3.GetState() && pmsg->sCommand == L"349") &&
			!m_managerDlg->m_radio1.Enabled() && !m_managerDlg->m_radio2.Enabled() && !m_managerDlg->m_radio3.Enabled()) {
			if (strchr(sChannelModes.c_str(), 'b'))
				m_managerDlg->m_radio1.Enable();
			if (strchr(sChannelModes.c_str(), 'I'))
				m_managerDlg->m_radio2.Enable();
			if (strchr(sChannelModes.c_str(), 'e'))
				m_managerDlg->m_radio3.Enable();
			if (BST_UNCHECKED == IsDlgButtonChecked(m_managerDlg->GetHwnd(), IDC_NOTOP))
				m_managerDlg->m_add.Enable();
		}
	}

	ShowMessage(pmsg);
	return true;
}

static void __stdcall sttShowWhoisWnd(void* param)
{
	CIrcMessage* pmsg = (CIrcMessage*)param;
	CIrcProto *ppro = (CIrcProto*)pmsg->m_proto;
	if (ppro->m_whoisDlg == NULL) {
		ppro->m_whoisDlg = new CWhoisDlg(ppro);
		ppro->m_whoisDlg->Show();
	}
	SetEvent(ppro->m_evWndCreate);

	ppro->m_whoisDlg->ShowMessage(pmsg);
	delete pmsg;
}

bool CIrcProto::OnIrc_WHOIS_NAME(const CIrcMessage* pmsg)
{
	if (pmsg->m_bIncoming && pmsg->parameters.getCount() > 5 && m_manualWhoisCount > 0) {
		CallFunctionAsync(sttShowWhoisWnd, new CIrcMessage(*pmsg));
		WaitForSingleObject(m_evWndCreate, INFINITE);
	}
	ShowMessage(pmsg);
	return true;
}

bool CIrcProto::OnIrc_WHOIS_CHANNELS(const CIrcMessage* pmsg)
{
	if (pmsg->m_bIncoming && m_whoisDlg && pmsg->parameters.getCount() > 2 && m_manualWhoisCount > 0)
		m_whoisDlg->m_InfoChannels.SetText(pmsg->parameters[2].c_str());
	ShowMessage(pmsg);
	return true;
}

bool CIrcProto::OnIrc_WHOIS_AWAY(const CIrcMessage* pmsg)
{
	if (pmsg->m_bIncoming && m_whoisDlg && pmsg->parameters.getCount() > 2 && m_manualWhoisCount > 0)
		m_whoisDlg->m_InfoAway2.SetText(pmsg->parameters[2].c_str());
	if (m_manualWhoisCount < 1 && pmsg->m_bIncoming && pmsg->parameters.getCount() > 2)
		WhoisAwayReply = pmsg->parameters[2];
	ShowMessage(pmsg);
	return true;
}

bool CIrcProto::OnIrc_WHOIS_OTHER(const CIrcMessage* pmsg)
{
	if (pmsg->m_bIncoming && m_whoisDlg && pmsg->parameters.getCount() > 2 && m_manualWhoisCount > 0) {
		wchar_t temp[1024], temp2[1024];
		m_whoisDlg->m_InfoOther.GetText(temp, 1000);
		mir_wstrcat(temp, L"%s\r\n");
		mir_snwprintf(temp2, temp, pmsg->parameters[2].c_str());
		m_whoisDlg->m_InfoOther.SetText(temp2);
	}
	ShowMessage(pmsg);
	return true;
}

bool CIrcProto::OnIrc_WHOIS_END(const CIrcMessage* pmsg)
{
	if (pmsg->m_bIncoming && pmsg->parameters.getCount() > 1 && m_manualWhoisCount < 1) {
		CONTACT user = { (wchar_t*)pmsg->parameters[1].c_str(), NULL, NULL, false, false, true };
		MCONTACT hContact = CList_FindContact(&user);
		if (hContact)
			ProtoBroadcastAck(hContact, ACKTYPE_AWAYMSG, ACKRESULT_SUCCESS, (HANDLE)1, (LPARAM)WhoisAwayReply.c_str());
	}

	m_manualWhoisCount--;
	if (m_manualWhoisCount < 0)
		m_manualWhoisCount = 0;
	ShowMessage(pmsg);
	return true;
}

bool CIrcProto::OnIrc_WHOIS_IDLE(const CIrcMessage* pmsg)
{
	if (pmsg->m_bIncoming && m_whoisDlg && pmsg->parameters.getCount() > 2 && m_manualWhoisCount > 0) {
		int S = _wtoi(pmsg->parameters[2].c_str());
		int D = S / (60 * 60 * 24);
		S -= (D * 60 * 60 * 24);
		int H = S / (60 * 60);
		S -= (H * 60 * 60);
		int M = S / 60;
		S -= (M * 60);

		wchar_t temp[100];
		if (D)
			mir_snwprintf(temp, TranslateT("%ud, %uh, %um, %us"), D, H, M, S);
		else if (H)
			mir_snwprintf(temp, TranslateT("%uh, %um, %us"), H, M, S);
		else if (M)
			mir_snwprintf(temp, TranslateT("%um, %us"), M, S);
		else if (S)
			mir_snwprintf(temp, TranslateT("%us"), S);
		else
			temp[0] = 0;

		wchar_t temp3[256];
		wchar_t tTimeBuf[128], *tStopStr;
		time_t ttTime = wcstol(pmsg->parameters[3].c_str(), &tStopStr, 10);
		wcsftime(tTimeBuf, 128, L"%c", localtime(&ttTime));
		mir_snwprintf(temp3, TranslateT("online since %s, idle %s"), tTimeBuf, temp);
		m_whoisDlg->m_AwayTime.SetText(temp3);
	}
	ShowMessage(pmsg);
	return true;
}

bool CIrcProto::OnIrc_WHOIS_SERVER(const CIrcMessage* pmsg)
{
	if (pmsg->m_bIncoming && m_whoisDlg && pmsg->parameters.getCount() > 2 && m_manualWhoisCount > 0)
		m_whoisDlg->m_InfoServer.SetText(pmsg->parameters[2].c_str());
	ShowMessage(pmsg);
	return true;
}

bool CIrcProto::OnIrc_WHOIS_AUTH(const CIrcMessage* pmsg)
{
	if (pmsg->m_bIncoming && m_whoisDlg && pmsg->parameters.getCount() > 2 && m_manualWhoisCount > 0) {
		if (pmsg->sCommand == L"330")
			m_whoisDlg->m_InfoAuth.SetText(pmsg->parameters[2].c_str());
		else if (pmsg->parameters[2] == L"is an identified user" || pmsg->parameters[2] == L"is a registered nick")
			m_whoisDlg->m_InfoAuth.SetText(pmsg->parameters[2].c_str());
		else
			OnIrc_WHOIS_OTHER(pmsg);
	}
	ShowMessage(pmsg);
	return true;
}

bool CIrcProto::OnIrc_WHOIS_NO_USER(const CIrcMessage* pmsg)
{
	if (pmsg->m_bIncoming && pmsg->parameters.getCount() > 2 && !IsChannel(pmsg->parameters[1])) {
		if (m_whoisDlg)
			m_whoisDlg->ShowMessageNoUser(pmsg);

		CONTACT user = { (wchar_t*)pmsg->parameters[1].c_str(), NULL, NULL, false, false, false };
		MCONTACT hContact = CList_FindContact(&user);
		if (hContact) {
			AddOutgoingMessageToDB(hContact, (wchar_t*)((CMStringW)L"> " + pmsg->parameters[2] + (CMStringW)L": " + pmsg->parameters[1]).c_str());

			DBVARIANT dbv;
			if (!getWString(hContact, "Default", &dbv)) {
				setWString(hContact, "Nick", dbv.ptszVal);

				DBVARIANT dbv2;
				if (getByte(hContact, "AdvancedMode", 0) == 0)
					DoUserhostWithReason(1, ((CMStringW)L"S" + dbv.ptszVal).c_str(), true, dbv.ptszVal);
				else {
					if (!getWString(hContact, "UWildcard", &dbv2)) {
						DoUserhostWithReason(2, ((CMStringW)L"S" + dbv2.ptszVal).c_str(), true, dbv2.ptszVal);
						db_free(&dbv2);
					}
					else DoUserhostWithReason(2, ((CMStringW)L"S" + dbv.ptszVal).c_str(), true, dbv.ptszVal);
				}
				setString(hContact, "User", "");
				setString(hContact, "Host", "");
				db_free(&dbv);
			}
		}
	}

	ShowMessage(pmsg);
	return true;
}

static void __stdcall sttShowNickWnd(void* param)
{
	CIrcMessage* pmsg = (CIrcMessage*)param;
	CIrcProto *ppro = pmsg->m_proto;
	if (ppro->m_nickDlg == NULL) {
		ppro->m_nickDlg = new CNickDlg(ppro);
		ppro->m_nickDlg->Show();
	}
	SetEvent(ppro->m_evWndCreate);
	SetDlgItemText(ppro->m_nickDlg->GetHwnd(), IDC_CAPTION, TranslateT("Change nickname"));
	SetDlgItemText(ppro->m_nickDlg->GetHwnd(), IDC_TEXT, pmsg->parameters.getCount() > 2 ? pmsg->parameters[2].c_str() : L"");
	ppro->m_nickDlg->m_Enick.SetText(pmsg->parameters[1].c_str());
	ppro->m_nickDlg->m_Enick.SendMsg(CB_SETEDITSEL, 0, MAKELPARAM(0, -1));
	delete pmsg;
}

bool CIrcProto::OnIrc_NICK_ERR(const CIrcMessage* pmsg)
{
	if (pmsg->m_bIncoming) {
		if (nickflag && ((m_alternativeNick[0] != 0)) && (pmsg->parameters.getCount() > 2 && mir_wstrcmp(pmsg->parameters[1].c_str(), m_alternativeNick))) {
			wchar_t m[200];
			mir_snwprintf(m, L"NICK %s", m_alternativeNick);
			if (IsConnected())
				SendIrcMessage(m);
		}
		else {
			CallFunctionAsync(sttShowNickWnd, new CIrcMessage(*pmsg));
			WaitForSingleObject(m_evWndCreate, INFINITE);
		}
	}

	ShowMessage(pmsg);
	return true;
}

bool CIrcProto::OnIrc_JOINERROR(const CIrcMessage* pmsg)
{
	if (pmsg->m_bIncoming) {
		DBVARIANT dbv;
		if (!getWString("JTemp", &dbv)) {
			CMStringW command = L"a";
			CMStringW save = L"";
			int i = 0;

			while (!command.IsEmpty()) {
				command = GetWord(dbv.ptszVal, i);
				i++;

				if (!command.IsEmpty() && pmsg->parameters[0] == command.Mid(1))
					save += command + L" ";
			}

			db_free(&dbv);

			if (save.IsEmpty())
				db_unset(NULL, m_szModuleName, "JTemp");
			else
				setWString("JTemp", save.c_str());
		}
	}

	ShowMessage(pmsg);
	return true;
}

bool CIrcProto::OnIrc_UNKNOWN(const CIrcMessage* pmsg)
{
	if (pmsg->m_bIncoming && pmsg->parameters.getCount() > 0) {
		if (pmsg->parameters[0] == L"WHO" && GetNextUserhostReason(2) != L"U")
			return true;
		if (pmsg->parameters[0] == L"USERHOST" && GetNextUserhostReason(1) != L"U")
			return true;
	}
	ShowMessage(pmsg);
	return true;
}

bool CIrcProto::OnIrc_ENDMOTD(const CIrcMessage* pmsg)
{
	if (pmsg->m_bIncoming && !bPerformDone)
		DoOnConnect(pmsg);
	ShowMessage(pmsg);
	return true;
}

bool CIrcProto::OnIrc_NOOFCHANNELS(const CIrcMessage* pmsg)
{
	if (pmsg->m_bIncoming && pmsg->parameters.getCount() > 1)
		m_noOfChannels = _wtoi(pmsg->parameters[1].c_str());

	if (pmsg->m_bIncoming && !bPerformDone)
		DoOnConnect(pmsg);

	ShowMessage(pmsg);
	return true;
}

bool CIrcProto::OnIrc_ERROR(const CIrcMessage* pmsg)
{
	if (pmsg->m_bIncoming && !m_disableErrorPopups && m_iDesiredStatus != ID_STATUS_OFFLINE) {
		MIRANDASYSTRAYNOTIFY msn;
		msn.cbSize = sizeof(MIRANDASYSTRAYNOTIFY);
		msn.szProto = m_szModuleName;
		msn.tszInfoTitle = TranslateT("IRC error");

		CMStringW S;
		if (pmsg->parameters.getCount() > 0)
			S = DoColorCodes(pmsg->parameters[0].c_str(), TRUE, FALSE);
		else
			S = TranslateT("Unknown");

		msn.tszInfo = (wchar_t*)S.c_str();
		msn.dwInfoFlags = NIIF_ERROR | NIIF_INTERN_UNICODE;
		msn.uTimeout = 15000;
		CallService(MS_CLIST_SYSTRAY_NOTIFY, 0, (LPARAM)&msn);
	}
	ShowMessage(pmsg);
	return true;
}

bool CIrcProto::OnIrc_WHO_END(const CIrcMessage* pmsg)
{
	CMStringW command = GetNextUserhostReason(2);
	if (command[0] == 'S') {
		if (pmsg->m_bIncoming && pmsg->parameters.getCount() > 1) {
			// is it a channel?
			if (IsChannel(pmsg->parameters[1])) {
				CMStringW S;
				CMStringW User = GetWord(m_whoReply.c_str(), 0);
				while (!User.IsEmpty()) {
					if (GetWord(m_whoReply.c_str(), 3)[0] == 'G') {
						S += User;
						S += L"\t";
						DoEvent(GC_EVENT_SETCONTACTSTATUS, pmsg->parameters[1].c_str(), User.c_str(), NULL, NULL, NULL, ID_STATUS_AWAY, FALSE, FALSE);
					}
					else DoEvent(GC_EVENT_SETCONTACTSTATUS, pmsg->parameters[1].c_str(), User.c_str(), NULL, NULL, NULL, ID_STATUS_ONLINE, FALSE, FALSE);

					CMStringW SS = GetWordAddress(m_whoReply.c_str(), 4);
					if (SS.IsEmpty())
						break;
					m_whoReply = SS;
					User = GetWord(m_whoReply.c_str(), 0);
				}

				DoEvent(GC_EVENT_SETSTATUSEX, pmsg->parameters[1].c_str(), NULL, S.IsEmpty() ? NULL : S.c_str(), NULL, NULL, GC_SSE_TABDELIMITED, FALSE, FALSE);
				return true;
			}

			/// if it is not a channel
			ptrW UserList(mir_wstrdup(m_whoReply.c_str()));
			const wchar_t* p1 = UserList;
			m_whoReply = L"";
			CONTACT ccUser = { (wchar_t*)pmsg->parameters[1].c_str(), NULL, NULL, false, true, false };
			MCONTACT hContact = CList_FindContact(&ccUser);

			if (hContact && getByte(hContact, "AdvancedMode", 0) == 1) {
				ptrW DBHost(getWStringA(hContact, "UHost"));
				ptrW DBNick(getWStringA(hContact, "Nick"));
				ptrW DBUser(getWStringA(hContact, "UUser"));
				ptrW DBDefault(getWStringA(hContact, "Default"));
				ptrW DBManUser(getWStringA(hContact, "User"));
				ptrW DBManHost(getWStringA(hContact, "Host"));
				ptrW DBWildcard(getWStringA(hContact, "UWildcard"));
				if (DBWildcard)
					CharLower(DBWildcard);

				CMStringW nick;
				CMStringW user;
				CMStringW host;
				CMStringW away = GetWord(p1, 3);

				while (!away.IsEmpty()) {
					nick = GetWord(p1, 0);
					user = GetWord(p1, 1);
					host = GetWord(p1, 2);
					if ((DBWildcard && WCCmp(DBWildcard, nick.c_str()) || DBNick && !mir_wstrcmpi(DBNick, nick.c_str()) || DBDefault && !mir_wstrcmpi(DBDefault, nick.c_str()))
						&& (WCCmp(DBUser, user.c_str()) && WCCmp(DBHost, host.c_str()))) {
						if (away[0] == 'G' && getWord(hContact, "Status", ID_STATUS_OFFLINE) != ID_STATUS_AWAY)
							setWord(hContact, "Status", ID_STATUS_AWAY);
						else if (away[0] == 'H' && getWord(hContact, "Status", ID_STATUS_OFFLINE) != ID_STATUS_ONLINE)
							setWord(hContact, "Status", ID_STATUS_ONLINE);

						if ((DBNick && mir_wstrcmpi(nick.c_str(), DBNick)) || !DBNick)
							setWString(hContact, "Nick", nick.c_str());
						if ((DBManUser && mir_wstrcmpi(user.c_str(), DBManUser)) || !DBManUser)
							setWString(hContact, "User", user.c_str());
						if ((DBManHost && mir_wstrcmpi(host.c_str(), DBManHost)) || !DBManHost)
							setWString(hContact, "Host", host.c_str());
						return true;
					}
					p1 = GetWordAddress(p1, 4);
					away = GetWord(p1, 3);
				}

				if (DBWildcard && DBNick && !WCCmp(CharLower(DBWildcard), CharLower(DBNick))) {
					setWString(hContact, "Nick", DBDefault);

					DoUserhostWithReason(2, ((CMStringW)L"S" + DBWildcard).c_str(), true, (wchar_t*)DBWildcard);

					setString(hContact, "User", "");
					setString(hContact, "Host", "");
					return true;
				}

				if (getWord(hContact, "Status", ID_STATUS_OFFLINE) != ID_STATUS_OFFLINE) {
					setWord(hContact, "Status", ID_STATUS_OFFLINE);
					setWString(hContact, "Nick", DBDefault);
					setString(hContact, "User", "");
					setString(hContact, "Host", "");
				}
			}
		}
	}
	else ShowMessage(pmsg);
	return true;
}

bool CIrcProto::OnIrc_WHO_REPLY(const CIrcMessage* pmsg)
{
	CMStringW command = PeekAtReasons(2);
	if (pmsg->m_bIncoming && pmsg->parameters.getCount() > 6 && command[0] == 'S') {
		m_whoReply.AppendFormat(L"%s %s %s %s ", pmsg->parameters[5].c_str(), pmsg->parameters[2].c_str(), pmsg->parameters[3].c_str(), pmsg->parameters[6].c_str());
		if (mir_wstrcmpi(pmsg->parameters[5].c_str(), m_info.sNick.c_str()) == 0) {
			wchar_t host[1024];
			mir_wstrncpy(host, pmsg->parameters[3].c_str(), 1024);
			ForkThread(&CIrcProto::ResolveIPThread, new IPRESOLVE(_T2A(host), IP_AUTO));
		}
	}

	if (command[0] == 'U')
		ShowMessage(pmsg);
	return true;
}

bool CIrcProto::OnIrc_TRYAGAIN(const CIrcMessage* pmsg)
{
	CMStringW command = L"";
	if (pmsg->m_bIncoming && pmsg->parameters.getCount() > 1) {
		if (pmsg->parameters[1] == L"WHO")
			command = GetNextUserhostReason(2);

		if (pmsg->parameters[1] == L"USERHOST")
			command = GetNextUserhostReason(1);
	}
	if (command[0] == 'U')
		ShowMessage(pmsg);
	return true;
}

bool CIrcProto::OnIrc_USERHOST_REPLY(const CIrcMessage* pmsg)
{
	CMStringW command;
	if (pmsg->m_bIncoming) {
		command = GetNextUserhostReason(1);
		if (!command.IsEmpty() && command != L"U" && pmsg->parameters.getCount() > 1) {
			CONTACT finduser = { NULL, NULL, NULL, false, false, false };
			int awaystatus = 0;
			CMStringW sTemp;
			CMStringW host;
			CMStringW user;
			CMStringW nick;
			CMStringW mask;
			CMStringW mess;
			CMStringW channel;

			// Status-check pre-processing: Setup check-list
			OBJLIST<CMStringW> checklist(10);
			if (command[0] == 'S') {
				sTemp = GetWord(command.c_str(), 0);
				sTemp.Delete(0, 1);
				for (int j = 1; !sTemp.IsEmpty(); j++) {
					checklist.insert(new CMStringW(sTemp));
					sTemp = GetWord(command.c_str(), j);
				}
			}

			// Cycle through results
			for (int j = 0;; j++) {
				sTemp = GetWord(pmsg->parameters[1].c_str(), j);
				if (sTemp.IsEmpty())
					break;

				wchar_t *p1 = mir_wstrdup(sTemp.c_str());

				// Pull out host, user and nick
				wchar_t *p2 = wcschr(p1, '@');
				if (p2) {
					*p2 = '\0';
					p2++;
					host = p2;
				}
				p2 = wcschr(p1, '=');
				if (p2) {
					if (*(p2 - 1) == '*')
						*(p2 - 1) = '\0';  //  remove special char for IRCOps
					*p2 = '\0';
					p2++;
					awaystatus = *p2;
					p2++;
					user = p2;
					nick = p1;
				}
				mess = L"";
				mask = nick + L"!" + user + L"@" + host;
				if (host.IsEmpty() || user.IsEmpty() || nick.IsEmpty()) {
					mir_free(p1);
					continue;
				}

				// Do command
				switch (command[0]) {
				case 'S': // Status check
					finduser.name = (wchar_t*)nick.c_str();
					finduser.host = (wchar_t*)host.c_str();
					finduser.user = (wchar_t*)user.c_str();
					{
						MCONTACT hContact = CList_FindContact(&finduser);
						if (hContact && getByte(hContact, "AdvancedMode", 0) == 0) {
							setWord(hContact, "Status", awaystatus == '-' ? ID_STATUS_AWAY : ID_STATUS_ONLINE);
							setWString(hContact, "User", user.c_str());
							setWString(hContact, "Host", host.c_str());
							setWString(hContact, "Nick", nick.c_str());
	
							// If user found, remove from checklist
							for (int i = 0; i < checklist.getCount(); i++)
								if (!mir_wstrcmpi(checklist[i].c_str(), nick.c_str()))
									checklist.remove(i);
						}
					}
					break;

				case 'I': // m_ignore
					mess = L"/IGNORE %question=\"";
					mess += TranslateT("Please enter the hostmask (nick!user@host)\nNOTE! Contacts on your contact list are never ignored");
					mess += (CMStringW)L"\",\"" + TranslateT("Ignore") + L"\",\"*!*@" + host + L"\"";
					if (m_ignoreChannelDefault)
						mess += L" +qnidcm";
					else
						mess += L" +qnidc";
					break;

				case 'J': // Unignore
					mess = L"/UNIGNORE *!*@" + host;
					break;

				case 'B': // Ban
					channel = (command.c_str() + 1);
					mess = L"/MODE " + channel + L" +b *!*@" + host;
					break;

				case 'K': // Ban & Kick
					channel = (command.c_str() + 1);
					mess.Format(L"/MODE %s +b *!*@%s%%newl/KICK %s %s", channel.c_str(), host.c_str(), channel.c_str(), nick.c_str());
					break;

				case 'L': // Ban & Kick with reason
					channel = (command.c_str() + 1);
					mess.Format(L"/MODE %s +b *!*@%s%%newl/KICK %s %s %%question=\"%s\",\"%s\",\"%s\"",
						channel.c_str(), host.c_str(), channel.c_str(), nick.c_str(),
						TranslateT("Please enter the reason"), TranslateT("Ban'n Kick"), TranslateT("Jerk"));
					break;
				}

				mir_free(p1);

				// Post message
				if (!mess.IsEmpty())
					PostIrcMessageWnd(NULL, NULL, mess);
			}

			// Status-check post-processing: make buddies in ckeck-list offline
			if (command[0] == 'S') {
				for (int i = 0; i < checklist.getCount(); i++) {
					finduser.name = (wchar_t*)checklist[i].c_str();
					finduser.ExactNick = true;
					CList_SetOffline(&finduser);
				}
			}

			return true;
		}
	}

	if (!pmsg->m_bIncoming || command == L"U")
		ShowMessage(pmsg);
	return true;
}

bool CIrcProto::OnIrc_SUPPORT(const CIrcMessage* pmsg)
{
	static const wchar_t *lpszFmt = L"Try server %99[^ ,], port %19s";
	wchar_t szAltServer[100];
	wchar_t szAltPort[20];
	if (pmsg->parameters.getCount() > 1 && swscanf(pmsg->parameters[1].c_str(), lpszFmt, &szAltServer, &szAltPort) == 2) {
		ShowMessage(pmsg);
		mir_strncpy(m_serverName, _T2A(szAltServer), 99);
		mir_strncpy(m_portStart, _T2A(szAltPort), 9);

		m_noOfChannels = 0;
		ConnectToServer();
		return true;
	}

	if (pmsg->m_bIncoming && !bPerformDone)
		DoOnConnect(pmsg);

	if (pmsg->m_bIncoming && pmsg->parameters.getCount() > 0) {
		CMStringW S;
		for (int i = 0; i < pmsg->parameters.getCount(); i++) {
			wchar_t* temp = mir_wstrdup(pmsg->parameters[i].c_str());
			if (wcsstr(temp, L"CHANTYPES=")) {
				wchar_t* p1 = wcschr(temp, '=');
				p1++;
				if (mir_wstrlen(p1) > 0)
					sChannelPrefixes = p1;
			}
			if (wcsstr(temp, L"CHANMODES=")) {
				wchar_t* p1 = wcschr(temp, '=');
				p1++;
				if (mir_wstrlen(p1) > 0)
					sChannelModes = (char*)_T2A(p1);
			}
			if (wcsstr(temp, L"PREFIX=")) {
				wchar_t* p1 = wcschr(temp, '(');
				wchar_t* p2 = wcschr(temp, ')');
				if (p1 && p2) {
					p1++;
					if (p1 != p2)
						sUserModes = (char*)_T2A(p1);
					sUserModes = sUserModes.Mid(0, p2 - p1);
					p2++;
					if (*p2 != '\0')
						sUserModePrefixes = p2;
				}
				else {
					p1 = wcschr(temp, '=');
					p1++;
					sUserModePrefixes = p1;
					for (int n = 0; n < sUserModePrefixes.GetLength() + 1; n++) {
						if (sUserModePrefixes[n] == '@')
							sUserModes.SetAt(n, 'o');
						else if (sUserModePrefixes[n] == '+')
							sUserModes.SetAt(n, 'v');
						else if (sUserModePrefixes[n] == '-')
							sUserModes.SetAt(n, 'u');
						else if (sUserModePrefixes[n] == '%')
							sUserModes.SetAt(n, 'h');
						else if (sUserModePrefixes[n] == '!')
							sUserModes.SetAt(n, 'a');
						else if (sUserModePrefixes[n] == '*')
							sUserModes.SetAt(n, 'q');
						else if (sUserModePrefixes[n] == '\0')
							sUserModes.SetAt(n, '\0');
						else
							sUserModes.SetAt(n, '_');
					}
				}
			}

			mir_free(temp);
		}
	}

	ShowMessage(pmsg);
	return true;
}

void CIrcProto::OnIrcDefault(const CIrcMessage* pmsg)
{
	ShowMessage(pmsg);
}

void CIrcProto::OnIrcDisconnected()
{
	m_statusMessage = L"";
	db_unset(NULL, m_szModuleName, "JTemp");
	bTempDisableCheck = false;
	bTempForceCheck = false;
	m_iTempCheckTime = 0;

	m_myHost[0] = '\0';

	int Temp = m_iStatus;
	KillIdent();
	KillChatTimer(OnlineNotifTimer);
	KillChatTimer(OnlineNotifTimer3);
	KillChatTimer(KeepAliveTimer);
	KillChatTimer(InitTimer);
	KillChatTimer(IdentTimer);
	m_iStatus = m_iDesiredStatus = ID_STATUS_OFFLINE;
	ProtoBroadcastAck(NULL, ACKTYPE_STATUS, ACKRESULT_SUCCESS, (HANDLE)Temp, ID_STATUS_OFFLINE);

	CMStringW sDisconn = L"\035\002";
	sDisconn += TranslateT("*Disconnected*");
	DoEvent(GC_EVENT_INFORMATION, SERVERWINDOW, NULL, sDisconn.c_str(), NULL, NULL, NULL, true, false);

	GCDEST gcd = { m_szModuleName, 0, GC_EVENT_CONTROL };
	GCEVENT gce = { sizeof(gce), &gcd };
	CallChatEvent(SESSION_OFFLINE, (LPARAM)&gce);

	if (!Miranda_Terminated())
		CList_SetAllOffline(m_disconnectDCCChats);

	// restore the original nick, cause it might be changed
	memcpy(m_nick, m_pNick, sizeof(m_nick));
	setWString("Nick", m_pNick);

	Menu_EnableItem(hMenuJoin, false);
	Menu_EnableItem(hMenuList, false);
	Menu_EnableItem(hMenuNick, false);
}

/////////////////////////////////////////////////////////////////////////////////////////
// OnConnect

static void __stdcall sttMainThrdOnConnect(void* param)
{
	CIrcProto *ppro = (CIrcProto*)param;

	ppro->SetChatTimer(ppro->InitTimer, 1 * 1000, TimerProc);
	if (ppro->m_identTimer)
		ppro->SetChatTimer(ppro->IdentTimer, 60 * 1000, IdentTimerProc);
	if (ppro->m_sendKeepAlive)
		ppro->SetChatTimer(ppro->KeepAliveTimer, 60 * 1000, KeepAliveTimerProc);
	if (ppro->m_autoOnlineNotification && !ppro->bTempDisableCheck || ppro->bTempForceCheck) {
		ppro->SetChatTimer(ppro->OnlineNotifTimer, 1000, OnlineNotifTimerProc);
		if (ppro->m_channelAwayNotification)
			ppro->SetChatTimer(ppro->OnlineNotifTimer3, 3000, OnlineNotifTimerProc3);
	}
}

bool CIrcProto::DoOnConnect(const CIrcMessage*)
{
	bPerformDone = true;
	nickflag = true;

	Menu_ModifyItem(hMenuJoin, NULL, INVALID_HANDLE_VALUE, 0);
	Menu_ModifyItem(hMenuList, NULL, INVALID_HANDLE_VALUE, 0);
	Menu_ModifyItem(hMenuNick, NULL, INVALID_HANDLE_VALUE, 0);

	int Temp = m_iStatus;
	m_iStatus = ID_STATUS_ONLINE;
	ProtoBroadcastAck(NULL, ACKTYPE_STATUS, ACKRESULT_SUCCESS, (HANDLE)Temp, m_iStatus);

	if (m_iDesiredStatus == ID_STATUS_AWAY)
		PostIrcMessage(L"/AWAY %s", m_statusMessage.Mid(0, 450));

	if (m_perform) {
		DoPerform("ALL NETWORKS");
		if (IsConnected()) {
			DoPerform(_T2A(m_info.sNetwork.c_str()));
			switch (Temp) {
				case ID_STATUS_FREECHAT:   DoPerform("Event: Free for chat");   break;
				case ID_STATUS_ONLINE:     DoPerform("Event: Available");       break;
			}
		}
	}

	if (m_rejoinChannels) {
		int count = CallServiceSync(MS_GC_GETSESSIONCOUNT, 0, (LPARAM)m_szModuleName);
		for (int i = 0; i < count; i++) {
			GC_INFO gci = { 0 };
			gci.Flags = GCF_BYINDEX | GCF_DATA | GCF_NAME | GCF_TYPE;
			gci.iItem = i;
			gci.pszModule = m_szModuleName;
			if (!CallServiceSync(MS_GC_GETINFO, 0, (LPARAM)&gci) && gci.iType == GCW_CHATROOM) {
				CHANNELINFO *wi = (CHANNELINFO*)gci.dwItemData;
				if (wi && wi->pszPassword)
					PostIrcMessage(L"/JOIN %s %s", gci.pszName, wi->pszPassword);
				else
					PostIrcMessage(L"/JOIN %s", gci.pszName);
			}
		}
	}

	DoEvent(GC_EVENT_ADDGROUP, SERVERWINDOW, NULL, NULL, L"Normal", NULL, NULL, FALSE, TRUE);
	{
		GCDEST gcd = { m_szModuleName, SERVERWINDOW, GC_EVENT_CONTROL };
		GCEVENT gce = { sizeof(gce), &gcd };
		CallChatEvent(SESSION_ONLINE, (LPARAM)&gce);
	}

	CallFunctionAsync(sttMainThrdOnConnect, this);
	nickflag = false;
	return 0;
}

static void __cdecl AwayWarningThread(LPVOID)
{
	Thread_SetName("IRC: AwayWarningThread");
	MessageBox(NULL, TranslateT("The usage of /AWAY in your perform buffer is restricted\n as IRC sends this command automatically."), TranslateT("IRC Error"), MB_OK);
}

int CIrcProto::DoPerform(const char* event)
{
	CMStringA sSetting = CMStringA("PERFORM:") + event;
	sSetting.MakeUpper();

	DBVARIANT dbv;
	if (!getWString(sSetting.c_str(), &dbv)) {
		if (!my_strstri(dbv.ptszVal, L"/away"))
			PostIrcMessageWnd(NULL, NULL, dbv.ptszVal);
		else
			mir_forkthread(AwayWarningThread, NULL);
		db_free(&dbv);
		return 1;
	}
	return 0;
}

int CIrcProto::IsIgnored(const CMStringW& nick, const CMStringW& address, const CMStringW& host, char type)
{
	return IsIgnored(nick + L"!" + address + L"@" + host, type);
}

int CIrcProto::IsIgnored(CMStringW user, char type)
{
	for (int i = 0; i < m_ignoreItems.getCount(); i++) {
		const CIrcIgnoreItem& C = m_ignoreItems[i];

		if (type == 0 && !mir_wstrcmpi(user.c_str(), C.mask.c_str()))
			return i + 1;

		bool bUserContainsWild = (wcschr(user.c_str(), '*') != NULL || wcschr(user.c_str(), '?') != NULL);
		if (!bUserContainsWild && WCCmp(C.mask.c_str(), user.c_str()) ||
			bUserContainsWild && !mir_wstrcmpi(user.c_str(), C.mask.c_str())) {
			if (C.flags.IsEmpty() || C.flags[0] != '+')
				continue;

			if (!wcschr(C.flags.c_str(), type))
				continue;

			if (C.network.IsEmpty())
				return i + 1;

			if (IsConnected() && !mir_wstrcmpi(C.network.c_str(), m_info.sNetwork.c_str()))
				return i + 1;
		}
	}

	return 0;
}

bool CIrcProto::AddIgnore(const wchar_t* mask, const wchar_t* flags, const wchar_t* network)
{
	RemoveIgnore(mask);
	m_ignoreItems.insert(new CIrcIgnoreItem(mask, (L"+" + CMStringW(flags)).c_str(), network));
	RewriteIgnoreSettings();

	if (m_ignoreDlg)
		m_ignoreDlg->RebuildList();
	return true;
}

bool CIrcProto::RemoveIgnore(const wchar_t* mask)
{
	int idx;
	while ((idx = IsIgnored(mask, '\0')) != 0)
		m_ignoreItems.remove(idx - 1);

	RewriteIgnoreSettings();

	if (m_ignoreDlg)
		m_ignoreDlg->RebuildList();
	return true;
}
