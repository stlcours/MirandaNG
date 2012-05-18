/*
Miranda IM Country Flags Plugin
Copyright (C) 2006-2007 H. Herkenrath

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program (Flags-License.txt); if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define NONAMELESSUNION
#include <commctrl.h>  /* for ImageList functions */
#define NOWIN2K
#include <win2k.h>
#define MIRANDA_VER  0x0A00
#include <stdio.h>
#include <newpluginapi.h>
#include <m_system.h>
#include <m_utils.h>
#include <m_langpack.h>
#include <m_icolib.h>
#include <m_clui.h>
#include <m_cluiframes.h>
#include <m_message.h>
#include <m_database.h>
#include <m_options.h>
#include <m_contacts.h>
#include <m_protocols.h>
#define FLAGS_NOHELPERFUNCTIONS
#include "m_flags.h"
#include "resource.h"

#if defined(_MSC_VER) && !defined(FASTCALL)
	#define FASTCALL  __fastcall
#else
	#define FASTCALL
#endif
#if defined(_DEBUG)
	#undef FASTCALL
	#define FASTCALL 
#endif

/* countrylistext.c */
void InitCountryListExt(void);
void UninitCountryListExt(void);

/* huffman.c */
#ifdef HUFFMAN_ENCODE
	int Huffman_Compress(unsigned char *in,unsigned char *out,unsigned int insize );
#endif
void Huffman_Uncompress(unsigned char *in,unsigned char *out,unsigned int insize,unsigned int outsize);

/* icons.c */
HICON FASTCALL LoadFlagIcon(int countryNumber);
int FASTCALL CountryNumberToIndex(int countryNumber);
void InitIcons(void);
void UninitIcons(void);

/* ip2country.c */
int ServiceIpToCountry(WPARAM wParam,LPARAM lParam);
void InitIpToCountry(void);
void UninitIpToCountry(void);

/* extraimg.c */
void InitExtraImg(void);
void UninitExtraImg(void);

/* utils.c */
typedef void (CALLBACK *BUFFEREDPROC)(LPARAM lParam);
#ifdef _DEBUG
	void _CallFunctionBuffered(BUFFEREDPROC pfnBuffProc,const char *pszProcName,LPARAM lParam,BOOL fAccumulateSameParam,UINT uElapse);
	#define CallFunctionBuffered(proc,param,acc,elapse) _CallFunctionBuffered(proc,#proc,param,acc,elapse)
#else
	void _CallFunctionBuffered(BUFFEREDPROC pfnBuffProc,LPARAM lParam,BOOL fAccumulateSameParam,UINT uElapse);
	#define CallFunctionBuffered(proc,param,acc,elapse) _CallFunctionBuffered(proc,param,acc,elapse)
#endif
void PrepareBufferedFunctions(void);
void KillBufferedFunctions(void);
