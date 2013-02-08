
//This file is part of Msg_Export a Miranda IM plugin
//Copyright (C)2002 Kennet Nielsen ( http://sourceforge.net/projects/msg-export/ )
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include "Glob.h"

HINSTANCE hInstance = NULL;
int hLangpack = 0;

// static so they can not be used from other modules ( sourcefiles )
static HANDLE hEventOptionsInitialize = 0;
static HANDLE hDBEventAdded = 0;
static HANDLE hDBContactDeleted = 0;
static HANDLE hEventSystemInit = 0;
static HANDLE hEventSystemShutdown = 0;

static HANDLE hServiceFunc = 0;

static HANDLE hOpenHistoryMenuItem = 0;

HANDLE hInternalWindowList = NULL;

/////////////////////////////////////////////////////
// Remember to update the Version in the resource !!!
/////////////////////////////////////////////////////

PLUGININFOEX pluginInfo = {
	sizeof(PLUGININFOEX),
	__PLUGIN_NAME,
	PLUGIN_MAKE_VERSION(__MAJOR_VERSION, __MINOR_VERSION, __RELEASE_NUM, __BUILD_NUM),
	__DESCRIPTION,
	__AUTHOR,
	__AUTHOREMAIL,
	__COPYRIGHT,
	__AUTHORWEB,
	UNICODE_AWARE,
	// {46102B07-C215-4162-9C83-D377881DA7CC}
	{0x46102b07, 0xc215, 0x4162, {0x9c, 0x83, 0xd3, 0x77, 0x88, 0x1d, 0xa7, 0xcc}}
};


/////////////////////////////////////////////////////////////////////
// Member Function : ShowExportHistory
// Type            : Global
// Parameters      : wParam - (WPARAM)(HANDLE)hContact
//                   lParam - ?
// Returns         : static int
// Description     : Called when user selects my menu item "Open Exported History"
//                   
// References      : -
// Remarks         : -
// Created         : 020422 , 22 April 2002
// Developer       : KN   
/////////////////////////////////////////////////////////////////////

static INT_PTR ShowExportHistory(WPARAM wParam, LPARAM /*lParam*/)
{
	if (bUseInternalViewer())
	{
		bShowFileViewer((HANDLE)wParam);
		return 0;
	}
	bOpenExternaly((HANDLE)wParam);
	return 0;
}

/////////////////////////////////////////////////////////////////////
// Member Function : nSystemShutdown
// Type            : Global
// Parameters      : wparam - 0
//                   lparam - 0
// Returns         : int
// Description     : 
//                   
// References      : -
// Remarks         : -
// Created         : 020428 , 28 April 2002
// Developer       : KN   
/////////////////////////////////////////////////////////////////////

int nSystemShutdown(WPARAM /*wparam*/, LPARAM /*lparam*/)
{
	WindowList_Broadcast(hInternalWindowList, WM_CLOSE, 0, 0);

	if (hEventOptionsInitialize)
	{
		UnhookEvent(hEventOptionsInitialize);
		hEventOptionsInitialize = 0;
	}

	if (hDBEventAdded)
	{
		UnhookEvent(hDBEventAdded);
		hDBEventAdded = 0;
	}

	if (hDBContactDeleted)
	{
		UnhookEvent(hDBContactDeleted);
		hDBContactDeleted = 0;
	}

	if (hServiceFunc)
	{
		DestroyServiceFunction(hServiceFunc);
		hServiceFunc = 0;
	}

	if (hEventSystemInit)
	{
		UnhookEvent(hEventSystemInit);
		hEventSystemInit = 0;
	}

	if (hEventSystemShutdown)
	{
		UnhookEvent(hEventSystemShutdown); // here we unhook the fun we are in, might not bee good
		hEventSystemShutdown = 0;
	}
	return 0;
}

/////////////////////////////////////////////////////////////////////
// Member Function : MainInit
// Type            : Global
// Parameters      : wparam - ?
//                   lparam - ?
// Returns         : int
// Description     : Called when system modules has been loaded
//                   
// References      : -
// Remarks         : -
// Created         : 020422 , 22 April 2002
// Developer       : KN   
/////////////////////////////////////////////////////////////////////

int MainInit(WPARAM /*wparam*/, LPARAM /*lparam*/)
{

	Initilize();

	bReadMirandaDirAndPath();
	UpdateFileToColWidth();

	hDBEventAdded = HookEvent(ME_DB_EVENT_ADDED, nExportEvent);
	if ( !hDBEventAdded)
		MessageBox(NULL, TranslateT("Failed to HookEvent ME_DB_EVENT_ADDED"), MSG_BOX_TITEL, MB_OK);

	hDBContactDeleted = HookEvent(ME_DB_CONTACT_DELETED, nContactDeleted);
	if ( !hDBContactDeleted)
		MessageBox(NULL, TranslateT("Failed to HookEvent ME_DB_CONTACT_DELETED"), MSG_BOX_TITEL, MB_OK);

	hEventOptionsInitialize = HookEvent(ME_OPT_INITIALISE, OptionsInitialize);
	if ( !hEventOptionsInitialize)
		MessageBox(NULL, TranslateT("Failed to HookEvent ME_OPT_INITIALISE"), MSG_BOX_TITEL, MB_OK);

	if ( !bReplaceHistory)
	{
		CLISTMENUITEM mi = { sizeof(mi) };
		mi.flags = 0;
		mi.pszContactOwner = NULL;    //all contacts
		mi.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_EXPORT_MESSAGE));
		mi.position = 1000090100;
		mi.pszName = LPGEN("Open E&xported History");
		mi.pszService = MS_SHOW_EXPORT_HISTORY;
		hOpenHistoryMenuItem = Menu_AddContactMenuItem(&mi);

		if ( !hOpenHistoryMenuItem)
			MessageBox(NULL, TranslateT("Failed to add menu item Open Exported History\nCallService(MS_CLIST_ADDCONTACTMENUITEM,...)"), MSG_BOX_TITEL, MB_OK);
	}

	hEventSystemShutdown = HookEvent(ME_SYSTEM_SHUTDOWN, nSystemShutdown);

	if ( !hEventSystemShutdown)
		MessageBox(NULL, TranslateT("Failed to HookEvent ME_SYSTEM_SHUTDOWN"), MSG_BOX_TITEL, MB_OK);

	return 0;
}



/////////////////////////////////////////////////////////////////////
// Member Function : DllMain
// Type            : Global
// Parameters      : hinst       - ?
//                   fdwReason   - ?
//                   lpvReserved - ?
// Returns         : BOOL WINAPI
// Description     : 
//                   
// References      : -
// Remarks         : -
// Created         : 020422 , 22 April 2002
// Developer       : KN   
/////////////////////////////////////////////////////////////////////

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD /*fdwReason*/, LPVOID /*lpvReserved*/)
{
	hInstance = hinst;
	return 1;
}


/////////////////////////////////////////////////////////////////////
// Member Function : MirandaPluginInfo
// Type            : Global
// Parameters      : mirandaVersion - ?
// Returns         : 
// Description     : 
//                   
// References      : -
// Remarks         : -
// Created         : 020422 , 22 April 2002
// Developer       : KN   
/////////////////////////////////////////////////////////////////////

extern "C" __declspec(dllexport) PLUGININFOEX* MirandaPluginInfoEx(DWORD mirandaVersion)
{
	return &pluginInfo;
}

/////////////////////////////////////////////////////////////////////
// Member Function : Load
// Type            : Global
// Parameters      : link - ?
// Returns         : int
// Description     : 
//                   
// References      : -
// Remarks         : -
// Created         : 020422 , 22 April 2002
// Developer       : KN   
/////////////////////////////////////////////////////////////////////

extern "C" __declspec(dllexport) int Load()
{
	mir_getLP(&pluginInfo);
	hEventSystemInit = HookEvent(ME_SYSTEM_MODULESLOADED, MainInit);

	if ( !hEventSystemInit)
	{
		MessageBox(NULL, TranslateT("Failed to HookEvent ME_SYSTEM_MODULESLOADED"), MSG_BOX_TITEL, MB_OK);
		return 0;
	}

	nMaxLineWidth = db_get_w(NULL, MODULE, "MaxLineWidth", nMaxLineWidth);
	if(nMaxLineWidth < 5)
		nMaxLineWidth = 5;

	sExportDir  = _DBGetString(NULL, MODULE, "ExportDir", _T("%dbpath%\\MsgExport\\"));
	sDefaultFile = _DBGetString(NULL, MODULE, "DefaultFile", _T("%nick%.txt"));

	sTimeFormat = _DBGetString(NULL, MODULE, "TimeFormat", _T("d s"));

	sFileViewerPrg = _DBGetString(NULL, MODULE, "FileViewerPrg", _T(""));
	bUseInternalViewer(db_get_b(NULL, MODULE, "UseInternalViewer", bUseInternalViewer()) != 0);

	bReplaceHistory = db_get_b(NULL, MODULE, "ReplaceHistory", bReplaceHistory) != 0;
	bAppendNewLine = db_get_b(NULL, MODULE, "AppendNewLine", bAppendNewLine) != 0;
	bUseUtf8InNewFiles = db_get_b(NULL, MODULE, "UseUtf8InNewFiles", bUseUtf8InNewFiles) != 0;
	bUseLessAndGreaterInExport = db_get_b(NULL, MODULE, "UseLessAndGreaterInExport", bUseLessAndGreaterInExport) != 0;

	enRenameAction = (ENDialogAction)db_get_b(NULL, MODULE, "RenameAction", enRenameAction);
	enDeleteAction = (ENDialogAction)db_get_b(NULL, MODULE, "DeleteAction", enDeleteAction);

	// Plugin sweeper support
	db_set_ts(NULL, "Uninstall", "Message Export", _T(MODULE));

	if (bReplaceHistory)
	{
		hServiceFunc = CreateServiceFunction(MS_HISTORY_SHOWCONTACTHISTORY, ShowExportHistory); //this need new code

		if ( !hServiceFunc) 
			MessageBox(NULL, TranslateT("Failed to replace Miranda History.\r\nThis is most likely due to changes in Miranda."), MSG_BOX_TITEL, MB_OK);
	}

	if ( !hServiceFunc)
	{
		hServiceFunc = CreateServiceFunction(MS_SHOW_EXPORT_HISTORY, ShowExportHistory);
	}

	if ( !hServiceFunc)
	{
		MessageBox(NULL, TranslateT("Failed to CreateServiceFunction MS_SHOW_EXPORT_HISTORY"), MSG_BOX_TITEL, MB_OK);
	}
	
	hInternalWindowList = (HANDLE)CallService(MS_UTILS_ALLOCWINDOWLIST, 0, 0);

	return 0;
}

/////////////////////////////////////////////////////////////////////
// Member Function : Unload
// Type            : Global
// Parameters      : none
// Returns         : 
// Description     : 
//                   
// References      : -
// Remarks         : -
// Created         : 020422 , 22 April 2002
// Developer       : KN   
/////////////////////////////////////////////////////////////////////

extern "C" __declspec(dllexport) int Unload(void)
{
	Uninitilize();
	bUseInternalViewer(false);
	return 0;
}