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

//---------------------------------------------------------------------------
#include "global.h"


//---------------------------------------------------------------------------
CSendFTPFile::CSendFTPFile(HWND Owner, HANDLE hContact, bool bFreeOnExit)
: CSend(Owner, hContact, bFreeOnExit){
	m_EnableItem		= NULL ; //SS_DLG_DESCRIPTION| SS_DLG_AUTOSEND | SS_DLG_DELETEAFTERSSEND;
	m_pszSendTyp		= LPGENT("FTPFile transfer");
	m_pszFileName		= NULL;
	m_URL				= NULL;
}

CSendFTPFile::~CSendFTPFile(){
	mir_free(m_pszFileName);
	mir_free(m_URL);
}

//---------------------------------------------------------------------------
void	CSendFTPFile::Send() {

	/*********************************************************************************************
	 * Send file (files) to the FTP server and copy file URL 
	 * to message log or clipboard (according to plugin setting)
	 *   wParam = (HANDLE)hContact
	 *   lParam = (char *)filename
	 * Filename format is same as GetOpenFileName (OPENFILENAME.lpstrFile) returns, 
	 * see http://msdn2.microsoft.com/en-us/library/ms646839.aspx
	 * Returns 0 on success or nonzero on failure
	 * if (!wParam || !lParam) return 1
	 ********************************************************************************************/
	mir_freeAndNil(m_pszFileName);
	m_pszFileName = (LPSTR)GetFileName(m_pszFile, DBVT_ASCIIZ);
	size_t size = sizeof(char)*(strlen(m_pszFileName)+2);
	m_pszFileName = (LPSTR) mir_realloc(m_pszFileName, size);
	m_pszFileName[size-1] = NULL;

	//start Send thread
	m_bFreeOnExit = TRUE;
	mir_forkthread(&CSendFTPFile::SendThreadWrapper, this);
}

void CSendFTPFile::SendThread() {
	mir_freeAndNil(m_URL);

	INT_PTR ret = CallService(MS_FTPFILE_SHAREFILE, (WPARAM)m_hContact, (LPARAM)m_pszFileName);
	if (ret != 0) {
		Error(TranslateT("%s (%i):\nCould not add a share to the FTP File plugin."),TranslateTS(m_pszSendTyp),ret);
		Exit(ret);
	}

	//Can't delete the file since FTP File plugin will use it
	m_bDeleteAfterSend = false;

	if (m_URL && m_URL[0]!= NULL) {
		m_ChatRoom ? svcSendChat(m_URL) : svcSendMsg(m_URL);
		return;
	}
}

void	CSendFTPFile::SendThreadWrapper(void * Obj) {
	reinterpret_cast<CSendFTPFile*>(Obj)->SendThread();
}

//---------------------------------------------------------------------------

