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

File name      : $HeadURL: http://merlins-miranda.googlecode.com/svn/trunk/miranda/plugins/SendSSPlus/CSendHTTPServer.cpp $
Revision       : $Revision: 13 $
Last change on : $Date: 2010-04-02 02:54:30 +0400 (Пт, 02 апр 2010) $
Last change by : $Author: ing.u.horn $

*/

//---------------------------------------------------------------------------
#include "CSendHTTPServer.h"
INT_PTR (*g_MirCallService)(const char *, WPARAM, LPARAM)=NULL;
//INT_PTR (*CallService)(const char *,WPARAM,LPARAM);


//---------------------------------------------------------------------------
CSendHTTPServer::CSendHTTPServer(HWND Owner, HANDLE hContact, bool bFreeOnExit)
: CSend(Owner, hContact, bFreeOnExit){
	m_EnableItem		= SS_DLG_DESCRIPTION ; //| SS_DLG_AUTOSEND | SS_DLG_DELETEAFTERSSEND;
	m_pszSendTyp		= _T("HTTPServer transfer");
	m_pszFileName		= NULL;
	m_URL				= NULL;
	m_fsi_pszSrvPath	= NULL;
	m_fsi_pszRealPath	= NULL;
}

CSendHTTPServer::~CSendHTTPServer(){
	mir_free(m_pszFileName);
	mir_free(m_URL);
	mir_free(m_fsi_pszSrvPath);
	mir_free(m_fsi_pszRealPath);
}

//---------------------------------------------------------------------------
void	CSendHTTPServer::Send() {

	if (CallService(MS_HTTP_ACCEPT_CONNECTIONS, (WPARAM)true, (LPARAM)0) != 0) {
		Error(NULL, _T("Could not start the HTTP Server plugin."));
		return;
	}

	if (!m_pszFileName) {
		m_pszFileName = (LPSTR)GetFileName(m_pszFile, DBVT_ASCIIZ);
	}
	mir_freeAndNil(m_fsi_pszSrvPath);
	mir_stradd(m_fsi_pszSrvPath, "/");
	mir_stradd(m_fsi_pszSrvPath, m_pszFileName);

	mir_freeAndNil(m_fsi_pszRealPath);
	m_fsi_pszRealPath = mir_t2a(m_pszFile);

	ZeroMemory(&m_fsi, sizeof(m_fsi));
	m_fsi.lStructSize	= sizeof(STFileShareInfo);
	m_fsi.pszSrvPath	= m_fsi_pszSrvPath;
	m_fsi.nMaxDownloads	= -1;					// -1 = infinite
	m_fsi.pszRealPath	= m_fsi_pszRealPath;
	//m_fsi.dwOptions		= NULL;					//OPT_SEND_LINK only work on single chat;

	//start Send thread
	m_bFreeOnExit = TRUE;
	mir_forkthread(&CSendHTTPServer::SendThreadWrapper, this);
}

void CSendHTTPServer::SendThread() {
	int ret;
	mir_freeAndNil(m_URL);

	if (ServiceExists(MS_HTTP_GET_LINK)) {
		//patched plugin version
		ret = (int)CallService(MS_HTTP_ADD_CHANGE_REMOVE, (WPARAM)m_hContact, (LPARAM)&m_fsi);
		if (!ret) {
			m_URL = (LPSTR)CallService(MS_HTTP_GET_LINK, (WPARAM)m_fsi.pszSrvPath, NULL);
		}
	}
	else {
		//original plugin
		m_fsi.dwOptions  = OPT_SEND_LINK;

		//send DATA and wait for reply
		ret = (int)CallService(MS_HTTP_ADD_CHANGE_REMOVE, (WPARAM)m_hContact, (LPARAM)&m_fsi);
	}

	if (ret != 0) {
		Error(_T("%s (%i):\nCould not add a share to the HTTP Server plugin."),TranslateTS(m_pszSendTyp),ret);
		Exit(ret);
	}

	//Share the file by HTTP Server plugin, SendSS does not own the file anymore = auto-delete won't work
	m_bDeleteAfterSend = false;

	if (m_URL && m_URL[0]!= NULL) {
		m_ChatRoom ? svcSendChat(m_URL) : svcSendMsg(m_URL);
		return;
	}
	Exit(ACKRESULT_FAILED);
}

void	CSendHTTPServer::SendThreadWrapper(void * Obj) {
	reinterpret_cast<CSendHTTPServer*>(Obj)->SendThread();
}

//---------------------------------------------------------------------------
CSendHTTPServer::CContactMapping CSendHTTPServer::_CContactMapping;

INT_PTR CSendHTTPServer::MyCallService(const char *name, WPARAM wParam, LPARAM lParam) {
	CContactMapping::iterator Contact(_CContactMapping.end());
/*
	if ( wParam == m_hContact && (
		(strcmp(name, MS_MSG_SENDMESSAGE)== 0) ||
		(strcmp(name, "SRMsg/LaunchMessageWindow")== 0) ))
	{
		m_URL= mir_strdup((char*)lParam);
		return 0;
	}*/
	return g_MirCallService(name, wParam, lParam);
}

