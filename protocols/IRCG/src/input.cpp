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
#include "version.h"

#define NICKSUBSTITUTE L"!_nick_!"

void CIrcProto::FormatMsg(CMString& text)
{
	TCHAR temp[30];
	mir_tstrncpy(temp, GetWord(text.c_str(), 0).c_str(), 29);
	CharLower(temp);
	CMString command = temp;
	CMString S = L"";
	if (command == L"/quit" || command == L"/away")
		S = GetWord(text.c_str(), 0) + L" :" + GetWordAddress(text.c_str(), 1);
	else if (command == L"/privmsg" || command == L"/part" || command == L"/topic" || command == L"/notice") {
		S = GetWord(text.c_str(), 0) + L" " + GetWord(text.c_str(), 1) + L" :";
		if (!GetWord(text.c_str(), 2).IsEmpty())
			S += CMString(GetWordAddress(text.c_str(), 2));
	}
	else if (command == L"/kick") {
		S = GetWord(text.c_str(), 0) + L" " + GetWord(text.c_str(), 1) + L" " + GetWord(text.c_str(), 2) + L" :" + GetWordAddress(text.c_str(), 3);
	}
	else if (command == L"/nick") {
		if (!_tcsstr(GetWord(text.c_str(), 1).c_str(), NICKSUBSTITUTE)) {
			sNick4Perform = GetWord(text.c_str(), 1);
			S = GetWordAddress(text.c_str(), 0);
		}
		else {
			CMString sNewNick = GetWord(text.c_str(), 1);
			if (sNick4Perform == L"") {
				DBVARIANT dbv;
				if (!getTString("PNick", &dbv)) {
					sNick4Perform = dbv.ptszVal;
					db_free(&dbv);
				}
			}

			sNewNick.Replace(NICKSUBSTITUTE, sNick4Perform.c_str());
			S = GetWord(text.c_str(), 0) + L" " + sNewNick;
		}
	}
	else S = GetWordAddress(text.c_str(), 0);

	S.Delete(0, 1);
	text = S;
}

static void AddCR(CMString& text)
{
	text.Replace(L"\n", L"\r\n");
	text.Replace(L"\r\r", L"\r");
}

CMString CIrcProto::DoAlias(const TCHAR *text, TCHAR *window)
{
	CMString Messageout = L"";
	const TCHAR* p1 = text;
	const TCHAR* p2 = text;
	bool LinebreakFlag = false, hasAlias = false;
	p2 = _tcsstr(p1, L"\r\n");
	if (!p2)
		p2 = _tcschr(p1, '\0');
	if (p1 == p2)
		return (CMString)text;

	do {
		if (LinebreakFlag)
			Messageout += L"\r\n";

		TCHAR* line = new TCHAR[p2 - p1 + 1];
		mir_tstrncpy(line, p1, p2 - p1 + 1);
		TCHAR* test = line;
		while (*test == ' ')
			test++;
		if (*test == '/') {
			mir_tstrncpy(line, GetWordAddress(line, 0), p2 - p1 + 1);
			CMString S = line;
			delete[] line;
			line = new TCHAR[S.GetLength() + 2];
			mir_tstrncpy(line, S.c_str(), S.GetLength() + 1);
			CMString alias(m_alias);
			const TCHAR* p3 = _tcsstr(alias.c_str(), (GetWord(line, 0) + L" ").c_str());
			if (p3 != alias.c_str()) {
				CMString str = L"\r\n";
				str += GetWord(line, 0) + L" ";
				p3 = _tcsstr(alias.c_str(), str.c_str());
				if (p3)
					p3 += 2;
			}
			if (p3 != NULL) {
				hasAlias = true;
				const TCHAR* p4 = _tcsstr(p3, L"\r\n");
				if (!p4)
					p4 = _tcschr(p3, '\0');

				*(TCHAR*)p4 = 0;
				CMString str = p3;
				str.Replace(L"##", window);
				str.Replace(L"$?", L"%question");

				TCHAR buf[5];
				for (int index = 1; index < 8; index++) {
					mir_sntprintf(buf, L"#$%u", index);
					if (!GetWord(line, index).IsEmpty() && IsChannel(GetWord(line, index)))
						str.Replace(buf, GetWord(line, index).c_str());
					else {
						CMString S1 = L"#";
						S1 += GetWord(line, index);
						str.Replace(buf, S1.c_str());
					}
				}
				for (int index2 = 1; index2 < 8; index2++) {
					mir_sntprintf(buf, L"$%u-", index2);
					str.Replace(buf, GetWordAddress(line, index2));
				}
				for (int index3 = 1; index3 < 8; index3++) {
					mir_sntprintf(buf, L"$%u", index3);
					str.Replace(buf, GetWord(line, index3).c_str());
				}
				Messageout += GetWordAddress(str.c_str(), 1);
			}
			else Messageout += line;
		}
		else Messageout += line;

		p1 = p2;
		if (*p1 == '\r')
			p1 += 2;
		p2 = _tcsstr(p1, L"\r\n");
		if (!p2)
			p2 = _tcschr(p1, '\0');
		delete[] line;
		LinebreakFlag = true;
	}
	while (*p1 != '\0');

	return hasAlias ? DoIdentifiers(Messageout, window) : Messageout;
}

CMString CIrcProto::DoIdentifiers(CMString text, const TCHAR*)
{
	SYSTEMTIME time;
	TCHAR str[2];

	GetLocalTime(&time);
	text.Replace(L"%mnick", m_nick);
	text.Replace(L"%anick", m_alternativeNick);
	text.Replace(L"%awaymsg", m_statusMessage.c_str());
	text.Replace(L"%module", _A2T(m_szModuleName));
	text.Replace(L"%name", m_name);
	text.Replace(L"%newl", L"\r\n");
	text.Replace(L"%network", m_info.sNetwork.c_str());
	text.Replace(L"%me", m_info.sNick.c_str());

	char mirver[100];
	CallService(MS_SYSTEM_GETVERSIONTEXT, _countof(mirver), LPARAM(mirver));
	text.Replace(L"%mirver", _A2T(mirver));

	text.Replace(L"%version", _T(__VERSION_STRING_DOTS));

	str[0] = 3; str[1] = '\0';
	text.Replace(L"%color", str);

	str[0] = 2;
	text.Replace(L"%bold", str);

	str[0] = 31;
	text.Replace(L"%underline", str);

	str[0] = 22;
	text.Replace(L"%italics", str);
	return text;
}

static void __stdcall sttSetTimerOn(void* _pro)
{
	CIrcProto *ppro = (CIrcProto*)_pro;
	ppro->DoEvent(GC_EVENT_INFORMATION, NULL, ppro->m_info.sNick.c_str(), TranslateT("The buddy check function is enabled"), NULL, NULL, NULL, true, false);
	ppro->SetChatTimer(ppro->OnlineNotifTimer, 500, OnlineNotifTimerProc);
	if (ppro->m_channelAwayNotification)
		ppro->SetChatTimer(ppro->OnlineNotifTimer3, 1500, OnlineNotifTimerProc3);
}

static void __stdcall sttSetTimerOff(void* _pro)
{
	CIrcProto *ppro = (CIrcProto*)_pro;
	ppro->DoEvent(GC_EVENT_INFORMATION, NULL, ppro->m_info.sNick.c_str(), TranslateT("The buddy check function is disabled"), NULL, NULL, NULL, true, false);
	ppro->KillChatTimer(ppro->OnlineNotifTimer);
	ppro->KillChatTimer(ppro->OnlineNotifTimer3);
}

BOOL CIrcProto::DoHardcodedCommand(CMString text, TCHAR *window, MCONTACT hContact)
{
	CMString command(GetWord(text, 0)); command.MakeLower();
	CMString one = GetWord(text, 1);
	CMString two = GetWord(text, 2);
	CMString three = GetWord(text, 3);
	CMString therest = GetWordAddress(text, 4);

	if (command == L"/servershow" || command == L"/serverhide") {
		if (m_useServer) {
			GCDEST gcd = { m_szModuleName, SERVERWINDOW, GC_EVENT_CONTROL };
			GCEVENT gce = { sizeof(gce), &gcd };
			CallChatEvent(command == L"/servershow" ? WINDOW_VISIBLE : WINDOW_HIDDEN, (LPARAM)&gce);
		}
		return true;
	}

	else if (command == L"/sleep" || command == L"/wait") {
		if (!one.IsEmpty()) {
			int ms;
			if (_stscanf(one.c_str(), L"%d", &ms) == 1 && ms > 0 && ms <= 4000)
				Sleep(ms);
			else
				DoEvent(GC_EVENT_INFORMATION, NULL, m_info.sNick.c_str(), TranslateT("Incorrect parameters. Usage: /sleep [ms], ms should be greater than 0 and less than 4000."), NULL, NULL, NULL, true, false);
		}
		return true;
	}

	if (command == L"/clear") {
		CMString S;
		if (!one.IsEmpty()) {
			if (one == L"server")
				S = SERVERWINDOW;
			else
				S = MakeWndID(one.c_str());
		}
		else if (mir_tstrcmpi(window, SERVERWINDOW) == 0)
			S = window;
		else
			S = MakeWndID(window);

		GCDEST gcd = { m_szModuleName, S.c_str(), GC_EVENT_CONTROL };
		GCEVENT gce = { sizeof(gce), &gcd };
		CallChatEvent(WINDOW_CLEARLOG, (LPARAM)&gce);
		return true;
	}

	if (command == L"/ignore") {
		if (IsConnected()) {
			CMString IgnoreFlags;
			TCHAR temp[500];
			if (one.IsEmpty()) {
				if (m_ignore)
					DoEvent(GC_EVENT_INFORMATION, NULL, m_info.sNick.c_str(), TranslateT("Ignore system is enabled"), NULL, NULL, NULL, true, false);
				else
					DoEvent(GC_EVENT_INFORMATION, NULL, m_info.sNick.c_str(), TranslateT("Ignore system is disabled"), NULL, NULL, NULL, true, false);
				return true;
			}
			if (!mir_tstrcmpi(one.c_str(), L"on")) {
				m_ignore = 1;
				DoEvent(GC_EVENT_INFORMATION, NULL, m_info.sNick.c_str(), TranslateT("Ignore system is enabled"), NULL, NULL, NULL, true, false);
				return true;
			}
			if (!mir_tstrcmpi(one.c_str(), L"off")) {
				m_ignore = 0;
				DoEvent(GC_EVENT_INFORMATION, NULL, m_info.sNick.c_str(), TranslateT("Ignore system is disabled"), NULL, NULL, NULL, true, false);
				return true;
			}
			if (!_tcschr(one.c_str(), '!') && !_tcschr(one.c_str(), '@'))
				one += L"!*@*";

			if (!two.IsEmpty() && two[0] == '+') {
				if (_tcschr(two.c_str(), 'q'))
					IgnoreFlags += 'q';
				if (_tcschr(two.c_str(), 'n'))
					IgnoreFlags += 'n';
				if (_tcschr(two.c_str(), 'i'))
					IgnoreFlags += 'i';
				if (_tcschr(two.c_str(), 'd'))
					IgnoreFlags += 'd';
				if (_tcschr(two.c_str(), 'c'))
					IgnoreFlags += 'c';
				if (_tcschr(two.c_str(), 'm'))
					IgnoreFlags += 'm';
			}
			else IgnoreFlags = L"qnidc";

			CMString szNetwork;
			if (three.IsEmpty())
				szNetwork = m_info.sNetwork;
			else
				szNetwork = three;

			AddIgnore(one.c_str(), IgnoreFlags.c_str(), szNetwork.c_str());

			mir_sntprintf(temp, TranslateT("%s on %s is now ignored (+%s)"), one.c_str(), szNetwork.c_str(), IgnoreFlags.c_str());
			DoEvent(GC_EVENT_INFORMATION, NULL, m_info.sNick.c_str(), temp, NULL, NULL, NULL, true, false);
		}
		return true;
	}

	if (command == L"/unignore") {
		if (!_tcschr(one.c_str(), '!') && !_tcschr(one.c_str(), '@'))
			one += L"!*@*";

		TCHAR temp[500];
		if (RemoveIgnore(one.c_str()))
			mir_sntprintf(temp, TranslateT("%s is not ignored now"), one.c_str());
		else
			mir_sntprintf(temp, TranslateT("%s was not ignored"), one.c_str());
		DoEvent(GC_EVENT_INFORMATION, NULL, m_info.sNick.c_str(), temp, NULL, NULL, NULL, true, false);
		return true;
	}

	if (command == L"/userhost") {
		if (one.IsEmpty())
			return true;

		DoUserhostWithReason(1, L"U", false, command);
		return false;
	}

	if (command == L"/joinx") {
		if (!one.IsEmpty()) {
			for (int i = 1;; i++) {
				CMString tmp = GetWord(text, i);
				if (tmp.IsEmpty())
					break;

				AddToJTemp('X', tmp);
			}

			PostIrcMessage(L"/JOIN %s", GetWordAddress(text, 1));
		}
		return true;
	}

	if (command == L"/joinm") {
		if (!one.IsEmpty()) {
			for (int i = 1;; i++) {
				CMString tmp = GetWord(text, i);
				if (tmp.IsEmpty())
					break;

				AddToJTemp('M', tmp);
			}

			PostIrcMessage(L"/JOIN %s", GetWordAddress(text, 1));
		}
		return true;
	}

	if (command == L"/nusers") {
		TCHAR szTemp[40];
		CMString S = MakeWndID(window);
		GC_INFO gci = { 0 };
		gci.Flags = GCF_BYID | GCF_NAME | GCF_COUNT;
		gci.pszModule = m_szModuleName;
		gci.pszID = S.c_str();
		if (!CallServiceSync(MS_GC_GETINFO, 0, (LPARAM)&gci))
			mir_sntprintf(szTemp, L"users: %u", gci.iCount);

		DoEvent(GC_EVENT_INFORMATION, NULL, m_info.sNick.c_str(), szTemp, NULL, NULL, NULL, true, false);
		return true;
	}

	if (command == L"/echo") {
		if (one.IsEmpty())
			return true;

		if (!mir_tstrcmpi(one.c_str(), L"on")) {
			bEcho = TRUE;
			DoEvent(GC_EVENT_INFORMATION, NULL, m_info.sNick.c_str(), TranslateT("Outgoing commands are shown"), NULL, NULL, NULL, true, false);
		}

		if (!mir_tstrcmpi(one.c_str(), L"off")) {
			DoEvent(GC_EVENT_INFORMATION, NULL, m_info.sNick.c_str(), TranslateT("Outgoing commands are not shown"), NULL, NULL, NULL, true, false);
			bEcho = FALSE;
		}

		return true;
	}

	if (command == L"/buddycheck") {
		if (one.IsEmpty()) {
			if ((m_autoOnlineNotification && !bTempDisableCheck) || bTempForceCheck)
				DoEvent(GC_EVENT_INFORMATION, NULL, m_info.sNick.c_str(), TranslateT("The buddy check function is enabled"), NULL, NULL, NULL, true, false);
			else
				DoEvent(GC_EVENT_INFORMATION, NULL, m_info.sNick.c_str(), TranslateT("The buddy check function is disabled"), NULL, NULL, NULL, true, false);
			return true;
		}
		if (!mir_tstrcmpi(one.c_str(), L"on")) {
			bTempForceCheck = true;
			bTempDisableCheck = false;
			CallFunctionAsync(sttSetTimerOn, this);
		}
		if (!mir_tstrcmpi(one.c_str(), L"off")) {
			bTempForceCheck = false;
			bTempDisableCheck = true;
			CallFunctionAsync(sttSetTimerOff, this);
		}
		if (!mir_tstrcmpi(one.c_str(), L"time") && !two.IsEmpty()) {
			m_iTempCheckTime = _ttoi(two.c_str());
			if (m_iTempCheckTime < 10 && m_iTempCheckTime != 0)
				m_iTempCheckTime = 10;

			if (m_iTempCheckTime == 0)
				DoEvent(GC_EVENT_INFORMATION, NULL, m_info.sNick.c_str(), TranslateT("The time interval for the buddy check function is now at default setting"), NULL, NULL, NULL, true, false);
			else {
				TCHAR temp[200];
				mir_sntprintf(temp, TranslateT("The time interval for the buddy check function is now %u seconds"), m_iTempCheckTime);
				DoEvent(GC_EVENT_INFORMATION, NULL, m_info.sNick.c_str(), temp, NULL, NULL, NULL, true, false);
			}
		}
		return true;
	}

	if (command == L"/whois") {
		if (one.IsEmpty())
			return false;
		m_manualWhoisCount++;
		return false;
	}

	if (command == L"/channelmanager") {
		if (window && !hContact && IsChannel(window)) {
			if (IsConnected()) {
				if (m_managerDlg != NULL) {
					SetActiveWindow(m_managerDlg->GetHwnd());
					m_managerDlg->Close();
				}
				else {
					m_managerDlg = new CManagerDlg(this);
					m_managerDlg->Show();
					m_managerDlg->InitManager(1, window);
				}
			}
		}

		return true;
	}

	if (command == L"/who") {
		if (one.IsEmpty())
			return true;

		DoUserhostWithReason(2, L"U", false, L"%s", one.c_str());
		return false;
	}

	if (command == L"/hop") {
		if (!IsChannel(window))
			return true;

		PostIrcMessage(L"/PART %s", window);

		if ((one.IsEmpty() || !IsChannel(one))) {
			CHANNELINFO *wi = (CHANNELINFO *)DoEvent(GC_EVENT_GETITEMDATA, window, NULL, NULL, NULL, NULL, NULL, FALSE, FALSE, 0);
			if (wi && wi->pszPassword)
				PostIrcMessage(L"/JOIN %s %s", window, wi->pszPassword);
			else
				PostIrcMessage(L"/JOIN %s", window);
			return true;
		}

		CMString S = MakeWndID(window);
		GCDEST gcd = { m_szModuleName, S.c_str(), GC_EVENT_CONTROL };
		GCEVENT gce = { sizeof(gce), &gcd };
		CallChatEvent(SESSION_TERMINATE, (LPARAM)&gce);

		PostIrcMessage(L"/JOIN %s", GetWordAddress(text, 1));
		return true;
	}

	if (command == L"/list") {
		if (m_listDlg == NULL) {
			m_listDlg = new CListDlg(this);
			m_listDlg->Show();
		}
		SetActiveWindow(m_listDlg->GetHwnd());
		int minutes = (int)m_noOfChannels / 4000;
		int minutes2 = (int)m_noOfChannels / 9000;

		CMString szMsg(FORMAT, TranslateT("This command is not recommended on a network of this size!\r\nIt will probably cause high CPU usage and/or high bandwidth\r\nusage for around %u to %u minute(s).\r\n\r\nDo you want to continue?"), minutes2, minutes);
		if (m_noOfChannels < 4000 || (m_noOfChannels >= 4000 && MessageBox(NULL, szMsg, TranslateT("IRC warning"), MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) == IDYES)) {
			ListView_DeleteAllItems(GetDlgItem(m_listDlg->GetHwnd(), IDC_INFO_LISTVIEW));
			PostIrcMessage(L"/lusers");
			return false;
		}
		m_listDlg->m_status.SetText(TranslateT("Aborted"));
		return true;
	}

	if (command == L"/me") {
		if (one.IsEmpty())
			return true;

		TCHAR szTemp[4000];
		mir_sntprintf(szTemp, L"\001ACTION %s\001", GetWordAddress(text.c_str(), 1));
		PostIrcMessageWnd(window, hContact, szTemp);
		return true;
	}

	if (command == L"/ame") {
		if (one.IsEmpty())
			return true;

		CMString S = L"/ME " + DoIdentifiers(GetWordAddress(text.c_str(), 1), window);
		S.Replace(L"%", L"%%");
		DoEvent(GC_EVENT_SENDMESSAGE, NULL, NULL, S.c_str(), NULL, NULL, NULL, FALSE, FALSE);
		return true;
	}

	if (command == L"/amsg") {
		if (one.IsEmpty())
			return true;

		CMString S = DoIdentifiers(GetWordAddress(text.c_str(), 1), window);
		S.Replace(L"%", L"%%");
		DoEvent(GC_EVENT_SENDMESSAGE, NULL, NULL, S.c_str(), NULL, NULL, NULL, FALSE, FALSE);
		return true;
	}

	if (command == L"/msg") {
		if (one.IsEmpty() || two.IsEmpty())
			return true;

		TCHAR szTemp[4000];
		mir_sntprintf(szTemp, L"/PRIVMSG %s", GetWordAddress(text.c_str(), 1));

		PostIrcMessageWnd(window, hContact, szTemp);
		return true;
	}

	if (command == L"/query") {
		if (one.IsEmpty() || IsChannel(one.c_str()))
			return true;

		CONTACT user = { (TCHAR*)one.c_str(), NULL, NULL, false, false, false };
		MCONTACT hContact2 = CList_AddContact(&user, false, false);
		if (hContact2) {
			if (getByte(hContact, "AdvancedMode", 0) == 0)
				DoUserhostWithReason(1, (L"S" + one).c_str(), true, one.c_str());
			else {
				DBVARIANT dbv1;
				if (!getTString(hContact, "UWildcard", &dbv1)) {
					CMString S = L"S";
					S += dbv1.ptszVal;
					DoUserhostWithReason(2, S.c_str(), true, dbv1.ptszVal);
					db_free(&dbv1);
				}
				else {
					CMString S = L"S";
					S += one;
					DoUserhostWithReason(2, S.c_str(), true, one.c_str());
				}
			}

			CallService(MS_MSG_SENDMESSAGE, (WPARAM)hContact2, 0);
		}

		if (!two.IsEmpty()) {
			TCHAR szTemp[4000];
			mir_sntprintf(szTemp, L"/PRIVMSG %s", GetWordAddress(text.c_str(), 1));
			PostIrcMessageWnd(window, hContact, szTemp);
		}
		return true;
	}

	if (command == L"/ctcp") {
		if (one.IsEmpty() || two.IsEmpty())
			return true;

		TCHAR szTemp[1000];
		unsigned long ulAdr = 0;
		if (m_manualHost)
			ulAdr = ConvertIPToInteger(m_mySpecifiedHostIP);
		else
			ulAdr = ConvertIPToInteger(m_IPFromServer ? m_myHost : m_myLocalHost);

		// if it is not dcc or if it is dcc and a local ip exist
		if (mir_tstrcmpi(two.c_str(), L"dcc") != 0 || ulAdr) {
			if (mir_tstrcmpi(two.c_str(), L"ping") == 0)
				mir_sntprintf(szTemp, L"/PRIVMSG %s \001%s %u\001", one.c_str(), two.c_str(), time(0));
			else
				mir_sntprintf(szTemp, L"/PRIVMSG %s \001%s\001", one.c_str(), GetWordAddress(text.c_str(), 2));
			PostIrcMessageWnd(window, hContact, szTemp);
		}

		if (mir_tstrcmpi(two.c_str(), L"dcc") != 0) {
			mir_sntprintf(szTemp, TranslateT("CTCP %s request sent to %s"), two.c_str(), one.c_str());
			DoEvent(GC_EVENT_INFORMATION, SERVERWINDOW, m_info.sNick.c_str(), szTemp, NULL, NULL, NULL, true, false);
		}

		return true;
	}

	if (command == L"/dcc") {
		if (one.IsEmpty() || two.IsEmpty())
			return true;

		if (mir_tstrcmpi(one.c_str(), L"send") == 0) {
			TCHAR szTemp[1000];
			unsigned long ulAdr = 0;

			if (m_manualHost)
				ulAdr = ConvertIPToInteger(m_mySpecifiedHostIP);
			else
				ulAdr = ConvertIPToInteger(m_IPFromServer ? m_myHost : m_myLocalHost);

			if (ulAdr) {
				CONTACT user = { (TCHAR*)two.c_str(), NULL, NULL, false, false, true };
				MCONTACT ccNew = CList_AddContact(&user, false, false);
				if (ccNew) {
					CMString s;

					if (getByte(ccNew, "AdvancedMode", 0) == 0)
						DoUserhostWithReason(1, (L"S" + two).c_str(), true, two.c_str());
					else {
						DBVARIANT dbv1;
						CMString S = L"S";
						if (!getTString(ccNew, "UWildcard", &dbv1)) {
							S += dbv1.ptszVal;
							DoUserhostWithReason(2, S.c_str(), true, dbv1.ptszVal);
							db_free(&dbv1);
						}
						else {
							S += two;
							DoUserhostWithReason(2, S.c_str(), true, two.c_str());
						}
					}

					if (three.IsEmpty())
						CallService(MS_FILE_SENDFILE, ccNew, 0);
					else {
						CMString temp = GetWordAddress(text.c_str(), 3);
						TCHAR* pp[2];
						TCHAR* p = (TCHAR*)temp.c_str();
						pp[0] = p;
						pp[1] = NULL;
						CallService(MS_FILE_SENDSPECIFICFILEST, ccNew, (LPARAM)pp);
					}
				}
			}
			else {
				mir_sntprintf(szTemp, TranslateT("DCC ERROR: Unable to automatically resolve external IP"));
				DoEvent(GC_EVENT_INFORMATION, 0, m_info.sNick.c_str(), szTemp, NULL, NULL, NULL, true, false);
			}
			return true;
		}

		if (mir_tstrcmpi(one.c_str(), L"chat") == 0) {
			TCHAR szTemp[1000];

			unsigned long ulAdr = 0;
			if (m_manualHost)
				ulAdr = ConvertIPToInteger(m_mySpecifiedHostIP);
			else
				ulAdr = ConvertIPToInteger(m_IPFromServer ? m_myHost : m_myLocalHost);

			if (ulAdr) {
				CMString contact = two;  contact += DCCSTRING;
				CONTACT user = { (TCHAR*)contact.c_str(), NULL, NULL, false, false, true };
				MCONTACT ccNew = CList_AddContact(&user, false, false);
				setByte(ccNew, "DCC", 1);

				int iPort = 0;
				if (ccNew) {
					DCCINFO* dci = new DCCINFO;
					dci->hContact = ccNew;
					dci->sContactName = two;
					dci->iType = DCC_CHAT;
					dci->bSender = true;

					CDccSession* dcc = new CDccSession(this, dci);
					CDccSession* olddcc = FindDCCSession(ccNew);
					if (olddcc)
						olddcc->Disconnect();
					AddDCCSession(ccNew, dcc);
					iPort = dcc->Connect();
				}

				if (iPort != 0) {
					PostIrcMessage(L"/CTCP %s DCC CHAT chat %u %u", two.c_str(), ulAdr, iPort);
					mir_sntprintf(szTemp, TranslateT("DCC CHAT request sent to %s"), two.c_str(), one.c_str());
					DoEvent(GC_EVENT_INFORMATION, 0, m_info.sNick.c_str(), szTemp, NULL, NULL, NULL, true, false);
				}
				else {
					mir_sntprintf(szTemp, TranslateT("DCC ERROR: Unable to bind port"));
					DoEvent(GC_EVENT_INFORMATION, 0, m_info.sNick.c_str(), szTemp, NULL, NULL, NULL, true, false);
				}
			}
			else {
				mir_sntprintf(szTemp, TranslateT("DCC ERROR: Unable to automatically resolve external IP"));
				DoEvent(GC_EVENT_INFORMATION, 0, m_info.sNick.c_str(), szTemp, NULL, NULL, NULL, true, false);
			}
		}
		return true;
	}
	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////

struct DoInputRequestParam
{
	DoInputRequestParam(CIrcProto* _pro, const TCHAR* _str) :
	ppro(_pro),
	str(mir_tstrdup(_str))
	{}

	CIrcProto *ppro;
	TCHAR* str;
};

static void __stdcall DoInputRequestAliasApcStub(void* _par)
{
	DoInputRequestParam* param = (DoInputRequestParam*)_par;
	CIrcProto *ppro = param->ppro;
	TCHAR* str = param->str;

	TCHAR* infotext = NULL;
	TCHAR* title = NULL;
	TCHAR* defaulttext = NULL;
	CMString command = (TCHAR*)str;
	TCHAR* p = _tcsstr((TCHAR*)str, L"%question");
	if (p[9] == '=' && p[10] == '\"') {
		infotext = &p[11];
		p = _tcschr(infotext, '\"');
		if (p) {
			*p = '\0';
			p++;
			if (*p == ',' && p[1] == '\"') {
				p++; p++;
				title = p;
				p = _tcschr(title, '\"');
				if (p) {
					*p = '\0';
					p++;
					if (*p == ',' && p[1] == '\"') {
						p++; p++;
						defaulttext = p;
						p = _tcschr(defaulttext, '\"');
						if (p)
							*p = '\0';
					}
				}
			}
		}
	}

	CQuestionDlg* dlg = new CQuestionDlg(ppro);
	dlg->Show();
	HWND question_hWnd = dlg->GetHwnd();

	if (title)
		SetDlgItemText(question_hWnd, IDC_CAPTION, title);
	else
		SetDlgItemText(question_hWnd, IDC_CAPTION, TranslateT("Input command"));

	if (infotext)
		SetDlgItemText(question_hWnd, IDC_TEXT, infotext);
	else
		SetDlgItemText(question_hWnd, IDC_TEXT, TranslateT("Please enter the reply"));

	if (defaulttext)
		SetDlgItemText(question_hWnd, IDC_EDIT, defaulttext);

	SetDlgItemText(question_hWnd, IDC_HIDDENEDIT, command.c_str());
	dlg->Activate();

	mir_free(str);
	delete param;
}

bool CIrcProto::PostIrcMessage(const TCHAR* fmt, ...)
{
	if (!fmt || mir_tstrlen(fmt) < 1 || mir_tstrlen(fmt) > 4000)
		return 0;

	va_list marker;
	va_start(marker, fmt);
	static TCHAR szBuf[4 * 1024];
	mir_vsntprintf(szBuf, _countof(szBuf), fmt, marker);
	va_end(marker);

	return PostIrcMessageWnd(NULL, NULL, szBuf);
}

bool CIrcProto::PostIrcMessageWnd(TCHAR *window, MCONTACT hContact, const TCHAR *szBuf)
{
	DBVARIANT dbv;
	TCHAR windowname[256];
	BYTE bDCC = 0;

	if (hContact)
		bDCC = getByte(hContact, "DCC", 0);

	if (!IsConnected() && !bDCC || !szBuf || mir_tstrlen(szBuf) < 1)
		return 0;

	if (hContact && !getTString(hContact, "Nick", &dbv)) {
		mir_tstrncpy(windowname, dbv.ptszVal, 255);
		db_free(&dbv);
	}
	else if (window)
		mir_tstrncpy(windowname, window, 255);
	else
		mir_tstrncpy(windowname, SERVERWINDOW, 255);

	if (mir_tstrcmpi(window, SERVERWINDOW) != 0) {
		TCHAR* p1 = _tcschr(windowname, ' ');
		if (p1)
			*p1 = '\0';
	}

	// remove unecessary linebreaks, and do the aliases
	CMString Message = szBuf;
	AddCR(Message);
	RemoveLinebreaks(Message);
	if (!hContact && IsConnected()) {
		Message = DoAlias(Message.c_str(), windowname);

		if (Message.Find(L"%question") != -1) {
			CallFunctionAsync(DoInputRequestAliasApcStub, new DoInputRequestParam(this, Message));
			return 1;
		}

		Message.Replace(L"%newl", L"\r\n");
		RemoveLinebreaks(Message);
	}

	if (Message.IsEmpty())
		return 0;

	CHANNELINFO *wi = (CHANNELINFO *)DoEvent(GC_EVENT_GETITEMDATA, windowname, NULL, NULL, NULL, NULL, NULL, FALSE, FALSE, 0);
	int cp = (wi) ? wi->codepage : getCodepage();

	// process the message
	while (!Message.IsEmpty()) {
		// split the text into lines, and do an automatic textsplit on long lies as well
		bool flag = false;
		CMString DoThis = L"";
		int index = Message.Find(L"\r\n", 0);
		if (index == -1)
			index = Message.GetLength();

		if (index > 464)
			index = 432;
		DoThis = Message.Mid(0, index);
		Message.Delete(0, index);
		if (Message.Find(L"\r\n", 0) == 0)
			Message.Delete(0, 2);

		//do this if it's a /raw
		if (IsConnected() && (GetWord(DoThis.c_str(), 0) == L"/raw" || GetWord(DoThis.c_str(), 0) == L"/quote")) {
			if (GetWord(DoThis.c_str(), 1).IsEmpty())
				continue;

			CMString S = GetWordAddress(DoThis.c_str(), 1);
			SendIrcMessage(S.c_str(), true, cp);
			continue;
		}

		// Do this if the message is not a command
		if ((GetWord(DoThis.c_str(), 0)[0] != '/') ||													// not a command
			((GetWord(DoThis.c_str(), 0)[0] == '/') && (GetWord(DoThis.c_str(), 0)[1] == '/')) ||		// or double backslash at the beginning
			hContact) {
			CMString S = L"/PRIVMSG ";
			if (mir_tstrcmpi(window, SERVERWINDOW) == 0 && !m_info.sServerName.IsEmpty())
				S += m_info.sServerName + L" " + DoThis;
			else
				S += CMString(windowname) + L" " + DoThis;

			DoThis = S;
			flag = true;
		}

		// and here we send it unless the command was a hardcoded one that should not be sent
		if (DoHardcodedCommand(DoThis, windowname, hContact))
			continue;

		if (!IsConnected() && !bDCC)
			continue;

		if (!flag && IsConnected())
			DoThis = DoIdentifiers(DoThis, windowname);

		if (hContact) {
			if (flag && bDCC) {
				CDccSession* dcc = FindDCCSession(hContact);
				if (dcc) {
					FormatMsg(DoThis);
					CMString mess = GetWordAddress(DoThis.c_str(), 2);
					if (mess[0] == ':')
						mess.Delete(0, 1);
					mess += '\n';
					dcc->SendStuff(mess.c_str());
				}
			}
			else if (IsConnected()) {
				FormatMsg(DoThis);
				SendIrcMessage(DoThis.c_str(), false, cp);
			}
		}
		else {
			FormatMsg(DoThis);
			SendIrcMessage(DoThis.c_str(), true, cp);
		}
	}

	return 1;
}
