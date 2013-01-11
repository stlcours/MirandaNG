/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2003 Miranda ICQ/IM project,
all portions of this codebase are copyrighted to the people
listed in contributors.txt.

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

#define _CRT_SECURE_NO_WARNINGS

#define MIRANDA_VER 0x0A00

#define _WIN32_WINNT 0x0501
#include "m_stdhdr.h"

//windows headers

#include <windows.h>
#include <shlobj.h>
#include <commctrl.h>

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stddef.h>
#include <process.h>
#include <io.h>
#include <string.h>
#include <direct.h>
#include <crtdbg.h>
#include <memory>

//miranda headers
#include <newpluginapi.h>
#include <win2k.h>
#include <m_system.h>
#include <m_system_cpp.h>
#include <m_database.h>
#include <m_db_int.h>
#include <m_langpack.h>
#include <m_utils.h>
#include <m_options.h>

//non-official miranda-plugins sdk
#include "m_folders.h"

//own headers
#include "dbintf_sa.h"
#include "..\Db3x_mmap\src\database.h"
#include "resource.h"
#include "version.h"

extern HINSTANCE g_hInst;
extern HANDLE hSetPwdMenu;

#ifdef __GNUC__
#define mir_i64(x) (x##LL)
#else
#define mir_i64(x) (x##i64)
#endif

//global procedures
//int InitSkin();
void EncodeCopyMemory(void * dst, void * src, size_t size );
void DecodeCopyMemory(void * dst, void * src, size_t size );
void EncodeDBWrite(DWORD ofs, void * src, size_t size);
void DecodeDBWrite(DWORD ofs, void * src, size_t size);

struct DlgStdInProcParam
{
	CDbxMmapSA *p_Db;
	const TCHAR *pStr;
};
INT_PTR CALLBACK DlgStdInProc(HWND hDlg, UINT uMsg,WPARAM wParam,LPARAM lParam);

struct DlgChangePassParam
{
	CDbxMmapSA *p_Db;
	char *pszNewPass;
};
INT_PTR CALLBACK DlgChangePass(HWND hDlg, UINT uMsg,WPARAM wParam,LPARAM lParam);

INT_PTR CALLBACK DlgStdNewPass(HWND hDlg, UINT uMsg,WPARAM wParam,LPARAM lParam);
void xModifyMenu(HANDLE hMenu,long flags,const TCHAR* name, HICON hIcon);

extern DBSignature dbSignature, dbSignatureSecured, dbSignatureNonSecured;

extern LIST<CDbxMmapSA> g_Dbs;

int InitPreset();
void UninitPreset();

typedef struct{
	void* (__stdcall *GenerateKey)(char* pwd);
	void (__stdcall *FreeKey)(void* key);
	void (__stdcall *EncryptMem)(BYTE* data, int size, void* key);
	void (__stdcall *DecryptMem)(BYTE* data, int size, void* key);

    char* Name;
    char* Info;
    char* Author;
    char* Site;
    char* Email;

	DWORD Version;

	WORD uid;
} Cryptor;

typedef struct{
	TCHAR dllname[MAX_PATH];
	HMODULE hLib;
	Cryptor* cryptor;
} CryptoModule;

extern Cryptor* CryptoEngine;
extern void* key;
