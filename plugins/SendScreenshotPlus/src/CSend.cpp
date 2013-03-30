/*

Miranda IM: the free IM client for Microsoft* Windows*
Copyright 2000-2009 Miranda ICQ/IM project,

This file is part of Send Screenshot Plus, a Miranda IM plugin.
Copyright (c) 2010 Ing.U.Horn

Parts of this file based on original sorce code
(c) 2004-2006 S�rgio Vieira Rolanski (portet from Borland C++)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include "global.h"

//---------------------------------------------------------------------------
CSend::CSend(HWND Owner, HANDLE hContact, bool bFreeOnExit) {
	m_hWndO			= Owner;
	m_bFreeOnExit	= bFreeOnExit;
	m_pszFile		= NULL;
	m_pszFileDesc	= NULL;
	m_pszProto		= NULL;
	m_ChatRoom		= NULL;
	m_PFflag		= NULL;
	m_hContact		= NULL;
	if (hContact) SetContact(hContact);
	m_hOnSend		= NULL;
	m_szEventMsg	= NULL;
	m_szEventMsgT	= NULL;
	m_pszSendTyp	= NULL;

	m_ErrorMsg		= NULL;
	m_ErrorTitle	= NULL;
}

CSend::~CSend(){
	mir_free(m_pszFile);
	mir_free(m_pszFileDesc);
	mir_free(m_szEventMsg);
	mir_free(m_szEventMsgT);
	mir_free(m_ErrorMsg);
	mir_free(m_ErrorTitle);
	if (m_hOnSend) UnhookEvent(m_hOnSend);
}

//---------------------------------------------------------------------------
void CSend::SetContact(HANDLE hContact) {
	m_hContact		= hContact;
	m_pszProto		= GetContactProto(hContact);
	m_ChatRoom		= DBGetContactSettingByte(hContact, m_pszProto, "ChatRoom", 0);
	m_PFflag		= hasCap(PF1_URLSEND);
	m_PFflag		= hasCap(PF1_CHAT);
	m_PFflag		= hasCap(PF1_IMSEND);
}

//---------------------------------------------------------------------------
bool	CSend::hasCap(unsigned int Flag) {
	return (Flag & CallContactService(m_hContact, PS_GETCAPS, PFLAGNUM_1, NULL)) == Flag;
}

//---------------------------------------------------------------------------
void	CSend::svcSendMsg(const char* szMessage) {
	mir_freeAndNil(m_szEventMsg);
	m_cbEventMsg=lstrlenA(szMessage)+1;
	m_szEventMsg=(char*)mir_realloc(m_szEventMsg, sizeof(char)*m_cbEventMsg);
	ZeroMemory(m_szEventMsg, m_cbEventMsg);
	lstrcpyA(m_szEventMsg,szMessage);
	if (m_pszFileDesc && m_pszFileDesc[0] != NULL) {
		char *temp = mir_t2a(m_pszFileDesc);
		mir_stradd(m_szEventMsg, "\r\n");
		mir_stradd(m_szEventMsg, temp);
		m_cbEventMsg = lstrlenA(m_szEventMsg)+1;
		mir_free(temp);
	}
	//create a HookEventObj on ME_PROTO_ACK
	if (!m_hOnSend) {
		int (__cdecl CSend::*hookProc)(WPARAM, LPARAM);
		hookProc = &CSend::OnSend;
		m_hOnSend = HookEventObj(ME_PROTO_ACK, (MIRANDAHOOKOBJ)*(void **)&hookProc, this);
	}
	//start PSS_MESSAGE service
	m_hSend = (HANDLE)CallContactService(m_hContact, PSS_MESSAGE, NULL, (LPARAM)m_szEventMsg);
	// check we actually got an ft handle back from the protocol
	if (!m_hSend) {
		Unhook();
		Error(SS_ERR_INIT, m_pszSendTyp);
		Exit(ACKRESULT_FAILED);
	}
}

void	CSend::svcSendUrl(const char* url) {
//szMessage should be encoded as the URL followed by the description, the
//separator being a single nul (\0). If there is no description, do not forget
//to end the URL with two nuls.
	mir_freeAndNil(m_szEventMsg)
	m_cbEventMsg=lstrlenA(url)+2;
	m_szEventMsg=(char*)mir_realloc(m_szEventMsg, m_cbEventMsg);
	ZeroMemory(m_szEventMsg, m_cbEventMsg);
	lstrcpyA(m_szEventMsg,url);
	if (m_pszFileDesc && m_pszFileDesc[0] != NULL) {
		char *temp = mir_t2a(m_pszFileDesc);
		m_cbEventMsg += lstrlenA(temp);
		m_szEventMsg=(char*)mir_realloc(m_szEventMsg, sizeof(char)*m_cbEventMsg);
		lstrcpyA(m_szEventMsg+lstrlenA(url)+1,temp);
		m_szEventMsg[m_cbEventMsg-1] = 0;
		mir_free(temp);
	}
	//create a HookEventObj on ME_PROTO_ACK
	if (!m_hOnSend) {
		int (__cdecl CSend::*hookProc)(WPARAM, LPARAM);
		hookProc = &CSend::OnSend;
		m_hOnSend = HookEventObj(ME_PROTO_ACK, (MIRANDAHOOKOBJ)*(void **)&hookProc, this);
	}
	//start PSS_URL service
	m_hSend = (HANDLE)CallContactService(m_hContact, PSS_URL, NULL, (LPARAM)m_szEventMsg);
	// check we actually got an ft handle back from the protocol
	if (!m_hSend) {
		//SetFtStatus(hwndDlg, LPGENT("Unable to initiate transfer."), FTS_TEXT);
		//dat->waitingForAcceptance=0;
		Unhook();
	}
}

void	CSend::svcSendChat() {
	GC_INFO gci = {0};
	int res = GC_RESULT_NOSESSION;
	int cnt = (int)CallService(MS_GC_GETSESSIONCOUNT, 0, (LPARAM)m_pszProto);

	//loop on all gc session to get the right (save) ptszID for the chatroom from m_hContact
	gci.pszModule = m_pszProto;
	for (int i = 0; i < cnt ; i++ ) {
		gci.iItem = i;
		gci.Flags = BYINDEX | HCONTACT | ID;
		CallService(MS_GC_GETINFO, 0, (LPARAM) &gci);
		if (gci.hContact == m_hContact) {
			GCDEST	gcd = {0};
			gcd.pszModule = m_pszProto;
			gcd.iType = GC_EVENT_SENDMESSAGE;
			gcd.ptszID = gci.pszID;

			GCEVENT gce = {0};
			gce.cbSize = sizeof(GCEVENT);
			gce.pDest = &gcd;
			gce.bIsMe = TRUE;
			gce.dwFlags = GC_TCHAR|GCEF_ADDTOLOG;
			gce.ptszText = m_szEventMsgT;
			gce.time = time(NULL);

			//* returns 0 on success or error code on failure
			res = 200 + (int)CallService(MS_GC_EVENT, 0, (LPARAM) &gce);
			break;
		}
	}
	Exit(res);
}

void	CSend::svcSendChat(const char* szMessage) {
	if (!m_ChatRoom) {
		svcSendMsg(szMessage);
		return;
	}
	mir_freeAndNil(m_szEventMsgT);
	m_szEventMsgT = mir_a2t(szMessage);
	if (m_pszFileDesc) {
		mir_tcsadd(m_szEventMsgT, _T("\r\n"));
		mir_tcsadd(m_szEventMsgT, m_pszFileDesc);
	}
	svcSendChat();
}

void	CSend::svcSendFile() {
//szMessage should be encoded as the File followed by the description, the
//separator being a single nul (\0). If there is no description, do not forget
//to end the File with two nuls.
	mir_freeAndNil(m_szEventMsg)
	char *szFile = mir_t2a(m_pszFile);
	m_cbEventMsg=lstrlenA(szFile)+2;
	m_szEventMsg=(char*)mir_realloc(m_szEventMsg, sizeof(char)*m_cbEventMsg);
	ZeroMemory(m_szEventMsg, m_cbEventMsg);
	lstrcpyA(m_szEventMsg,szFile);
	if (m_pszFileDesc && m_pszFileDesc[0] != NULL) {
		char* temp = mir_t2a(m_pszFileDesc);
		m_cbEventMsg += lstrlenA(temp);
		m_szEventMsg=(char*)mir_realloc(m_szEventMsg, sizeof(char)*m_cbEventMsg);
		lstrcpyA(m_szEventMsg+lstrlenA(szFile)+1,temp);
		m_szEventMsg[m_cbEventMsg-1] = 0;
		mir_freeAndNil(temp);
	}
	mir_freeAndNil(szFile);

	//create a HookEventObj on ME_PROTO_ACK
	if (!m_hOnSend) {
		int (__cdecl CSend::*hookProc)(WPARAM, LPARAM);
		hookProc = &CSend::OnSend;
		m_hOnSend = HookEventObj(ME_PROTO_ACK, (MIRANDAHOOKOBJ)*(void **)&hookProc, this);
	}

	// Start miranda PSS_FILE based on mir ver (T)
	TCHAR *ppFile[2]={0,0};
	TCHAR *pDesc = mir_tstrdup(m_pszFileDesc);
	ppFile[0] = mir_tstrdup (m_pszFile);
	ppFile[1] = NULL;
	m_hSend = (HANDLE)CallContactService(m_hContact, PSS_FILET, (WPARAM)pDesc, (LPARAM)ppFile);
	mir_free(pDesc);
	mir_free(ppFile[0]);

	// check we actually got an ft handle back from the protocol
	if (!m_hSend) {
		Unhook();
		Error(SS_ERR_INIT, m_pszSendTyp);
		Exit(ACKRESULT_FAILED);
	}
}

//---------------------------------------------------------------------------
int __cdecl	CSend::OnSend(WPARAM wParam, LPARAM lParam){
	ACKDATA *ack=(ACKDATA*)lParam;
	if(ack->hProcess!= m_hSend) return 0;
		/*	if(dat->waitingForAcceptance) {
				SetTimer(hwndDlg,1,1000,NULL);
				dat->waitingForAcceptance=0;
			}
		*/

	switch(ack->result) {
		case ACKRESULT_INITIALISING:	//SetFtStatus(hwndDlg, LPGENT("Initialising..."), FTS_TEXT); break;
		case ACKRESULT_CONNECTING:		//SetFtStatus(hwndDlg, LPGENT("Connecting..."), FTS_TEXT); break;
		case ACKRESULT_CONNECTPROXY:	//SetFtStatus(hwndDlg, LPGENT("Connecting to proxy..."), FTS_TEXT); break;
		case ACKRESULT_LISTENING:		//SetFtStatus(hwndDlg, LPGENT("Waiting for connection..."), FTS_TEXT); break;
		case ACKRESULT_CONNECTED:		//SetFtStatus(hwndDlg, LPGENT("Connected"), FTS_TEXT); break;
		case ACKRESULT_SENTREQUEST:		//SetFtStatus(hwndDlg, LPGENT("Decision sent"), FTS_TEXT); break;
		case ACKRESULT_NEXTFILE:		//SetFtStatus(hwndDlg, LPGENT("Moving to next file..."), FTS_TEXT);
		case ACKRESULT_FILERESUME:		//
		case ACKRESULT_DATA:			//transfer is on progress
			break;
		case ACKRESULT_DENIED:
			Unhook();
			Exit(ack->result);
			break;
		case ACKRESULT_FAILED:
			Unhook();
			Exit(ack->result);
			//type=ACKTYPE_MESSAGE, result=success/failure, (char*)lParam=error message or NULL.
			//type=ACKTYPE_URL, result=success/failure, (char*)lParam=error message or NULL.
			//type=ACKTYPE_FILE, result=ACKRESULT_FAILED then lParam=(LPARAM)(const char*)szReason
			break;
		case ACKRESULT_SUCCESS:
			Unhook();
			switch(ack->type) {
				case ACKTYPE_CHAT:
					break;
				case ACKTYPE_MESSAGE:
					DB_EventAdd((WORD)EVENTTYPE_MESSAGE);
					break;
				case ACKTYPE_URL:
					DB_EventAdd((WORD)EVENTTYPE_URL);
					break;
				case ACKTYPE_FILE:
					m_szEventMsg = (char*) mir_realloc(m_szEventMsg, sizeof(DWORD) + m_cbEventMsg);
					memmove(m_szEventMsg+sizeof(DWORD), m_szEventMsg, m_cbEventMsg);
					m_cbEventMsg += sizeof(DWORD);
					DB_EventAdd((WORD)EVENTTYPE_FILE);
					break;
				default:
					break;
			}
			Exit(ack->result);
			break;
		default:
			return 0;
			break;
	}
	return 0;
}

void	CSend::DB_EventAdd(WORD EventType)
{
	DBEVENTINFO dbei = { sizeof(dbei) };
	dbei.szModule = m_pszProto;
	dbei.eventType = EventType;
	dbei.flags = DBEF_SENT;
	dbei.timestamp = time(NULL);
	dbei.flags |= DBEF_UTF;
	dbei.cbBlob= m_cbEventMsg;
	dbei.pBlob = (PBYTE)m_szEventMsg;
	db_event_add(m_hContact, &dbei);
}

//---------------------------------------------------------------------------
void	CSend::AfterSendDelete() {
	if (m_pszFile && m_bDeleteAfterSend && (m_EnableItem & SS_DLG_DELETEAFTERSSEND) == SS_DLG_DELETEAFTERSSEND) {
		DeleteFile(m_pszFile);
	}
}

//---------------------------------------------------------------------------
void	CSend::Exit(unsigned int Result) {
	bool err = true;
	if (m_hWndO && IsWindow(m_hWndO)){
		;
	}
	switch(Result) {
		case ACKRESULT_SUCCESS:
		case GC_RESULT_SUCCESS:
			SkinPlaySound("FileDone");
			err = false;
			break;
		case ACKRESULT_DENIED:
			SkinPlaySound("FileDenied");
			Error(_T("%s (%i):\nFile transfer denied."),TranslateTS(m_pszSendTyp),Result);
			MsgBoxService(NULL, (LPARAM)&m_box);
			err = false;
			break;
		case GC_RESULT_WRONGVER:	//.You appear to be using the wrong version of GC API.
			Error(_T("%s (%i):\nYou appear to be using the wrong version of GC API"),TranslateT("GCHAT error"),Result);
			break;
		case GC_RESULT_ERROR:		// An internal GC error occurred.
			Error(_T("%s (%i):\nAn internal GC error occurred."),TranslateT("GCHAT error"),Result);
			break;
		case GC_RESULT_NOSESSION:	// contact has no open GC session
			Error(_T("%s (%i):\nContact has no open GC session."),TranslateT("GCHAT error"),Result);
			break;
		case ACKRESULT_FAILED:
		default:
			err = false;
			break;
	}
	if (err){
		SkinPlaySound("FileFailed");
		if(m_ErrorMsg && m_ErrorMsg[0] != 0) MsgBoxService(NULL, (LPARAM)&m_box);
		else MsgErr(NULL, LPGENT("An unknown error has occured."));
	}

	AfterSendDelete();
	if(m_bFreeOnExit) delete this;
}

void	CSend::Error(LPCTSTR pszFormat, ...) {
	if(!pszFormat) return;

	TCHAR	tszTemp[MAX_SECONDLINE];
	va_list	vl;

	mir_sntprintf(tszTemp, SIZEOF(tszTemp),_T("%s - %s") ,_T(MODNAME), TranslateT("Error"));
	mir_freeAndNil(m_ErrorTitle);
	m_ErrorTitle	= mir_tstrdup(tszTemp);

	va_start(vl, pszFormat);
	mir_vsntprintf(tszTemp, SIZEOF(tszTemp), TranslateTS(pszFormat), vl);
	va_end(vl);
	mir_freeAndNil(m_ErrorMsg);
	m_ErrorMsg		= mir_tstrdup(tszTemp);

	ZeroMemory(&m_box, sizeof(m_box));
	m_box.cbSize		= sizeof(MSGBOX);
	m_box.hParent		= NULL;
	m_box.hiLogo		= IcoLib_GetIcon(ICO_PLUG_SSWINDOW1);
	m_box.hiMsg			= NULL;
	m_box.ptszTitle		= m_ErrorTitle;
	m_box.ptszMsg		= m_ErrorMsg;
	m_box.uType			= MB_OK|MB_ICON_ERROR;
}
