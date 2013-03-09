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

#ifndef _CSEND_IMAGESHACK_H
#define _CSEND_IMAGESHACK_H

//---------------------------------------------------------------------------
class CSendImageShack : public CSend {
	public:
		// Deklaration Standardkonstruktor/Standarddestructor
		CSendImageShack(HWND Owner, HANDLE hContact, bool bFreeOnExit);
		~CSendImageShack();

		void					Send();
		void					SendSync(bool Sync) {m_SendSync = Sync;};
		LPSTR					GetURL(){return m_Url;};
		LPSTR					GetError(){return mir_t2a(m_ErrorMsg);};

	protected:
		LPSTR					m_pszFileName;
		NETLIBHTTPREQUEST		m_nlhr;
		NETLIBHTTPREQUEST*		m_nlreply;
		NETLIBHTTPHEADER*		m_nlheader;
		char					m_nlheader_ContentType[64];
		LPSTR					m_Url;

		void					AppendToData(const char *pszVal);		//append to netlib DATA
		LPSTR					m_pszContentType;						//hold mimeType (does not need free)
		void					GetContentType();						//get mimeType
		const char *			GetTagContent(char * pszSource, const char * pszTagStart, const char * pszTagEnd);

		char*					m_MFDRboundary;
		void					MFDR_Reset();

		bool					m_SendSync;
		void					SendThread();
		static void				SendThreadWrapper(void * Obj);

};

//---------------------------------------------------------------------------

#endif
