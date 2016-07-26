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

/////////////////////////////////////////////////////////////////////////////////////////

void CIrcProto::AddToJTemp(wchar_t op, CMString& sCommand)
{
	CMString res;

	int pos = 0;
	for (;;) {
		CMString tmp = sCommand.Tokenize(L",", pos);
		if (pos == -1)
			break;

		tmp = op + tmp;
		if (res.IsEmpty())
			res = tmp;
		else
			res += L" " + tmp;
	}

	DBVARIANT dbv;
	if (!getTString("JTemp", &dbv)) {
		res = CMString(dbv.ptszVal) + L" " + res;
		db_free(&dbv);
	}

	setTString("JTemp", res.c_str());
}

CMString __stdcall GetWord(const wchar_t* text, int index)
{
	if (text && *text) {
		wchar_t* p1 = (wchar_t*)text;
		wchar_t* p2 = NULL;

		while (*p1 == ' ')
			p1++;

		if (*p1 != '\0') {
			for (int i = 0; i < index; i++) {
				p2 = wcschr(p1, ' ');
				if (!p2)
					p2 = wcschr(p1, '\0');
				else
				while (*p2 == ' ')
					p2++;

				p1 = p2;
			}

			p2 = wcschr(p1, ' ');
			if (!p2)
				p2 = wcschr(p1, '\0');

			if (p1 != p2)
				return CMString(p1, p2 - p1);
		}
	}

	return CMString();
}

const wchar_t* __stdcall GetWordAddress(const wchar_t* text, int index)
{
	if (!text || !mir_tstrlen(text))
		return text;

	const wchar_t* temp = text;

	while (*temp == ' ')
		temp++;

	if (index == 0)
		return temp;

	for (int i = 0; i < index; i++) {
		temp = wcschr(temp, ' ');
		if (!temp)
			temp = (wchar_t*)wcschr(text, '\0');
		else
		while (*temp == ' ')
			temp++;
		text = temp;
	}

	return temp;
}

void __stdcall RemoveLinebreaks(CMString &Message)
{
	while (Message.Find(L"\r\n\r\n", 0) != -1)
		Message.Replace(L"\r\n\r\n", L"\r\n");

	if (Message.Find(L"\r\n", 0) == 0)
		Message.Delete(0, 2);

	if ((Message.GetLength() > 1) && (Message.Find(L"\r\n", Message.GetLength() - 2) == 0))
		Message.Delete(Message.GetLength() - 2, 2);
}

char* __stdcall IrcLoadFile(wchar_t* szPath)
{
	char * szContainer = NULL;
	DWORD dwSiz = 0;
	FILE *hFile = _wfopen(szPath, L"rb");
	if (hFile != NULL) {
		fseek(hFile, 0, SEEK_END); // seek to end
		dwSiz = ftell(hFile); // size
		fseek(hFile, 0, SEEK_SET); // seek back to original pos
		szContainer = new char[dwSiz + 1];
		fread(szContainer, 1, dwSiz, hFile);
		szContainer[dwSiz] = '\0';
		fclose(hFile);
		return szContainer;
	}

	return 0;
}

int __stdcall WCCmp(const wchar_t* wild, const wchar_t* string)
{
	if (wild == NULL || !mir_tstrlen(wild) || string == NULL || !mir_tstrlen(string))
		return 1;

	const wchar_t *cp = NULL, *mp = NULL;
	while ((*string) && (*wild != '*')) {
		if ((*wild != *string) && (*wild != '?'))
			return 0;

		wild++;
		string++;
	}

	while (*string) {
		if (*wild == '*') {
			if (!*++wild)
				return 1;

			mp = wild;
			cp = string + 1;
		}
		else if ((*wild == *string) || (*wild == '?')) {
			wild++;
			string++;
		}
		else {
			wild = mp;
			string = cp++;
		}
	}

	while (*wild == '*')
		wild++;

	return !*wild;
}

bool CIrcProto::IsChannel(const wchar_t* sName)
{
	return (sChannelPrefixes.Find(sName[0]) != -1);
}

CMStringA __stdcall GetWord(const char* text, int index)
{
	if (text && text[0]) {
		char* p1 = (char*)text;
		char* p2 = NULL;

		while (*p1 == ' ')
			p1++;

		if (*p1 != '\0') {
			for (int i = 0; i < index; i++) {
				p2 = strchr(p1, ' ');
				if (!p2)
					p2 = strchr(p1, '\0');
				else
				while (*p2 == ' ')
					p2++;

				p1 = p2;
			}

			p2 = strchr(p1, ' ');
			if (!p2)
				p2 = strchr(p1, '\0');

			if (p1 != p2)
				return CMStringA(p1, p2 - p1 + 1);
		}
	}

	return CMStringA();
}

bool CIrcProto::IsChannel(const char* sName)
{
	return (sChannelPrefixes.Find(sName[0]) != -1);
}

wchar_t* __stdcall my_strstri(const wchar_t* s1, const wchar_t* s2)
{
	int i, j, k;
	for (i = 0; s1[i]; i++)
	for (j = i, k = 0; towlower(s1[j]) == towlower(s2[k]); j++, k++)
	if (!s2[k + 1])
		return (wchar_t*)(s1 + i);

	return NULL;
}

wchar_t* __stdcall DoColorCodes(const wchar_t* text, bool bStrip, bool bReplacePercent)
{
	static wchar_t szTemp[4000]; szTemp[0] = '\0';
	wchar_t* p = szTemp;
	bool bBold = false;
	bool bUnderline = false;
	bool bItalics = false;

	if (!text)
		return szTemp;

	while (*text != '\0') {
		int iFG = -1;
		int iBG = -1;

		switch (*text) {
		case '%': //escape
			*p++ = '%';
			if (bReplacePercent)
				*p++ = '%';
			text++;
			break;

		case 2: //bold
			if (!bStrip) {
				*p++ = '%';
				*p++ = bBold ? 'B' : 'b';
			}
			bBold = !bBold;
			text++;
			break;

		case 15: //reset
			if (!bStrip) {
				*p++ = '%';
				*p++ = 'r';
			}
			bUnderline = false;
			bBold = false;
			text++;
			break;

		case 22: //italics
			if (!bStrip) {
				*p++ = '%';
				*p++ = bItalics ? 'I' : 'i';
			}
			bItalics = !bItalics;
			text++;
			break;

		case 31: //underlined
			if (!bStrip) {
				*p++ = '%';
				*p++ = bUnderline ? 'U' : 'u';
			}
			bUnderline = !bUnderline;
			text++;
			break;

		case 3: //colors
			text++;

			// do this if the colors should be reset to default
			if (*text <= 47 || *text >= 58 || *text == '\0') {
				if (!bStrip) {
					*p++ = '%';
					*p++ = 'C';
					*p++ = '%';
					*p++ = 'F';
				}
				break;
			}
			else { // some colors should be set... need to find out who
				wchar_t buf[3];

				// fix foreground index
				if (text[1] > 47 && text[1] < 58 && text[1] != '\0')
					mir_tstrncpy(buf, text, 3);
				else
					mir_tstrncpy(buf, text, 2);
				text += mir_tstrlen(buf);
				iFG = _wtoi(buf);

				// fix background color
				if (*text == ',' && text[1] > 47 && text[1] < 58 && text[1] != '\0') {
					text++;

					if (text[1] > 47 && text[1] < 58 && text[1] != '\0')
						mir_tstrncpy(buf, text, 3);
					else
						mir_tstrncpy(buf, text, 2);
					text += mir_tstrlen(buf);
					iBG = _wtoi(buf);
				}
			}

			if (iFG >= 0 && iFG != 99)
			while (iFG > 15)
				iFG -= 16;
			if (iBG >= 0 && iBG != 99)
			while (iBG > 15)
				iBG -= 16;

			// create tag for chat.dll
			if (!bStrip) {
				wchar_t buf[10];
				if (iFG >= 0 && iFG != 99) {
					*p++ = '%';
					*p++ = 'c';

					mir_sntprintf(buf, L"%02u", iFG);
					for (int i = 0; i < 2; i++)
						*p++ = buf[i];
				}
				else if (iFG == 99) {
					*p++ = '%';
					*p++ = 'C';
				}

				if (iBG >= 0 && iBG != 99) {
					*p++ = '%';
					*p++ = 'f';

					mir_sntprintf(buf, L"%02u", iBG);
					for (int i = 0; i < 2; i++)
						*p++ = buf[i];
				}
				else if (iBG == 99) {
					*p++ = '%';
					*p++ = 'F';
				}
			}
			break;

		default:
			*p++ = *text++;
			break;
		}
	}

	*p = '\0';
	return szTemp;
}

INT_PTR CIrcProto::CallChatEvent(WPARAM wParam, LPARAM lParam)
{
	return CallServiceSync(MS_GC_EVENT, wParam, (LPARAM)lParam);
}

INT_PTR CIrcProto::DoEvent(int iEvent, const wchar_t* pszWindow, const wchar_t* pszNick,
	const wchar_t* pszText, const wchar_t* pszStatus, const wchar_t* pszUserInfo,
	DWORD_PTR dwItemData, bool bAddToLog, bool bIsMe, time_t timestamp)
{
	GCDEST gcd = { m_szModuleName, NULL, iEvent };
	CMString sID;
	CMString sText = L"";

	if (iEvent == GC_EVENT_INFORMATION && bIsMe && !bEcho)
		return false;

	if (pszText) {
		if (iEvent != GC_EVENT_SENDMESSAGE)
			sText = DoColorCodes(pszText, FALSE, TRUE);
		else
			sText = pszText;
	}

	if (pszWindow) {
		if (mir_tstrcmpi(pszWindow, SERVERWINDOW))
			sID = pszWindow + (CMString)L" - " + m_info.sNetwork;
		else
			sID = pszWindow;
		gcd.ptszID = (wchar_t*)sID.c_str();
	}
	else gcd.ptszID = NULL;

	GCEVENT gce = { sizeof(gce), &gcd };
	gce.ptszStatus = pszStatus;
	gce.dwFlags = (bAddToLog) ? GCEF_ADDTOLOG : 0;
	gce.ptszNick = pszNick;
	gce.ptszUID = pszNick;
	if (iEvent == GC_EVENT_TOPIC)
		gce.ptszUserInfo = pszUserInfo;
	else
		gce.ptszUserInfo = m_showAddresses ? pszUserInfo : NULL;

	if (!sText.IsEmpty())
		gce.ptszText = sText.c_str();

	gce.dwItemData = dwItemData;
	if (timestamp == 1)
		gce.time = time(NULL);
	else
		gce.time = timestamp;
	gce.bIsMe = bIsMe;
	return CallChatEvent(0, (LPARAM)&gce);
}

CMString CIrcProto::ModeToStatus(int sMode)
{
	if (sUserModes.Find(sMode) != -1) {
		switch (sMode) {
		case 'q':
			return (CMString)L"Owner";
		case 'o':
			return (CMString)L"Op";
		case 'v':
			return (CMString)L"Voice";
		case 'h':
			return (CMString)L"Halfop";
		case 'a':
			return (CMString)L"Admin";
		default:
			return (CMString)L"Unknown";
		}
	}

	return (CMString)L"Normal";
}

CMString CIrcProto::PrefixToStatus(int cPrefix)
{
	const wchar_t* p = wcschr(sUserModePrefixes.c_str(), cPrefix);
	if (p) {
		int index = int(p - sUserModePrefixes.c_str());
		return ModeToStatus(sUserModes[index]);
	}

	return (CMString)L"Normal";
}

/////////////////////////////////////////////////////////////////////////////////////////
// Timer functions 

struct TimerPair
{
	TimerPair(CIrcProto* _pro, UINT_PTR _id) :
		ppro(_pro),
		idEvent(_id)
		{}

	UINT_PTR idEvent;
	CIrcProto *ppro;
};

static int CompareTimers(const TimerPair* p1, const TimerPair* p2)
{
	if (p1->idEvent < p2->idEvent)
		return -1;
	return (p1->idEvent == p2->idEvent) ? 0 : 1;
}

static OBJLIST<TimerPair> timers(10, CompareTimers);
static mir_cs timers_cs;

void UninitTimers(void)
{
	mir_cslock lck(timers_cs);
	timers.destroy();
}

CIrcProto* GetTimerOwner(UINT_PTR nIDEvent)
{
	mir_cslock lck(timers_cs);

	TimerPair temp(NULL, nIDEvent);
	int idx = timers.getIndex(&temp);
	return (idx == -1) ? NULL : timers[idx].ppro;
}

void CIrcProto::SetChatTimer(UINT_PTR &nIDEvent, UINT uElapse, TIMERPROC lpTimerFunc)
{
	if (nIDEvent)
		KillChatTimer(nIDEvent);

	nIDEvent = SetTimer(NULL, NULL, uElapse, lpTimerFunc);

	mir_cslock lck(timers_cs);
	timers.insert(new TimerPair(this, nIDEvent));
}

void CIrcProto::KillChatTimer(UINT_PTR &nIDEvent)
{
	if (nIDEvent == 0)
		return;
	{
		mir_cslock lck(timers_cs);
		TimerPair temp(this, nIDEvent);
		int idx = timers.getIndex(&temp);
		if (idx != -1)
			timers.remove(idx);
	}

	KillTimer(NULL, nIDEvent);
	nIDEvent = NULL;
}

/////////////////////////////////////////////////////////////////////////////////////////

int CIrcProto::SetChannelSBText(CMString sWindow, CHANNELINFO * wi)
{
	CMString sTemp = L"";
	if (wi->pszMode) {
		sTemp += L"[";
		sTemp += wi->pszMode;
		sTemp += L"] ";
	}
	if (wi->pszTopic)
		sTemp += wi->pszTopic;
	sTemp = DoColorCodes(sTemp.c_str(), TRUE, FALSE);
	return DoEvent(GC_EVENT_SETSBTEXT, sWindow.c_str(), NULL, sTemp.c_str(), NULL, NULL, NULL, FALSE, FALSE, 0);
}

CMString CIrcProto::MakeWndID(const wchar_t* sWindow)
{
	wchar_t buf[200];
	mir_sntprintf(buf, L"%s - %s", sWindow, (IsConnected()) ? m_info.sNetwork.c_str() : TranslateT("Offline"));
	return CMString(buf);
}

bool CIrcProto::FreeWindowItemData(CMString window, CHANNELINFO* wis)
{
	CHANNELINFO *wi;
	if (!wis)
		wi = (CHANNELINFO *)DoEvent(GC_EVENT_GETITEMDATA, window.c_str(), NULL, NULL, NULL, NULL, NULL, FALSE, FALSE, 0);
	else
		wi = wis;
	if (wi) {
		delete[] wi->pszLimit;
		delete[] wi->pszMode;
		delete[] wi->pszPassword;
		delete[] wi->pszTopic;
		delete wi;
		return true;
	}
	return false;
}

bool CIrcProto::AddWindowItemData(CMString window, const wchar_t* pszLimit, const wchar_t* pszMode, const wchar_t* pszPassword, const wchar_t* pszTopic)
{
	CHANNELINFO *wi = (CHANNELINFO *)DoEvent(GC_EVENT_GETITEMDATA, window.c_str(), NULL, NULL, NULL, NULL, NULL, FALSE, FALSE, 0);
	if (wi) {
		if (pszLimit) {
			wi->pszLimit = (wchar_t*)realloc(wi->pszLimit, sizeof(wchar_t)*(mir_tstrlen(pszLimit) + 1));
			mir_tstrcpy(wi->pszLimit, pszLimit);
		}
		if (pszMode) {
			wi->pszMode = (wchar_t*)realloc(wi->pszMode, sizeof(wchar_t)*(mir_tstrlen(pszMode) + 1));
			mir_tstrcpy(wi->pszMode, pszMode);
		}
		if (pszPassword) {
			wi->pszPassword = (wchar_t*)realloc(wi->pszPassword, sizeof(wchar_t)*(mir_tstrlen(pszPassword) + 1));
			mir_tstrcpy(wi->pszPassword, pszPassword);
		}
		if (pszTopic) {
			wi->pszTopic = (wchar_t*)realloc(wi->pszTopic, sizeof(wchar_t)*(mir_tstrlen(pszTopic) + 1));
			mir_tstrcpy(wi->pszTopic, pszTopic);
		}

		SetChannelSBText(window, wi);
		return true;
	}
	return false;
}

void CIrcProto::FindLocalIP(HANDLE hConn) // inspiration from jabber
{
	// Determine local IP
	int socket = CallService(MS_NETLIB_GETSOCKET, (WPARAM)hConn, 0);
	if (socket != INVALID_SOCKET) {
		struct sockaddr_in saddr;
		int len = sizeof(saddr);
		getsockname(socket, (struct sockaddr *) &saddr, &len);
		mir_strncpy(m_myLocalHost, inet_ntoa(saddr.sin_addr), 49);
		m_myLocalPort = ntohs(saddr.sin_port);
	}
}

void CIrcProto::DoUserhostWithReason(int type, CMString reason, bool bSendCommand, CMString userhostparams, ...)
{
	wchar_t temp[4096];
	CMString S = L"";
	switch (type) {
	case 1:
		S = L"USERHOST";
		break;
	case 2:
		S = L"WHO";
		break;
	default:
		S = L"USERHOST";
		break;
	}

	va_list ap;
	va_start(ap, userhostparams);
	mir_vsntprintf(temp, _countof(temp), (S + L" " + userhostparams).c_str(), ap);
	va_end(ap);

	// Add reason
	if (type == 1)
		vUserhostReasons.insert(new CMString(reason));
	else if (type == 2)
		vWhoInProgress.insert(new CMString(reason));

	// Do command
	if (IsConnected() && bSendCommand)
		SendIrcMessage(temp, false);
}

CMString CIrcProto::GetNextUserhostReason(int type)
{
	CMString reason = L"";
	switch (type) {
	case 1:
		if (!vUserhostReasons.getCount())
			return CMString();

		// Get reason
		reason = vUserhostReasons[0];
		vUserhostReasons.remove(0);
		break;
	case 2:
		if (!vWhoInProgress.getCount())
			return CMString();

		// Get reason
		reason = vWhoInProgress[0];
		vWhoInProgress.remove(0);
		break;
	}

	return reason;
}

CMString CIrcProto::PeekAtReasons(int type)
{
	switch (type) {
	case 1:
		if (!vUserhostReasons.getCount())
			return CMString();
		return vUserhostReasons[0];

	case 2:
		if (!vWhoInProgress.getCount())
			return CMString();
		return vWhoInProgress[0];

	}
	return CMString();
}

void CIrcProto::ClearUserhostReasons(int type)
{
	switch (type) {
	case 1:
		vUserhostReasons.destroy();
		break;
	case 2:
		vWhoInProgress.destroy();
		break;
	}
}

////////////////////////////////////////////////////////////////////

SERVER_INFO::~SERVER_INFO()
{
	mir_free(m_name);
	mir_free(m_address);
	mir_free(m_group);
}
