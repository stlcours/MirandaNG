/*
WhenWasIt (birthday reminder) plugin for Miranda IM

Copyright � 2006 Cristian Libotean

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

#include "commonheaders.h"

char ModuleName[] = "WhenWasIt";
HINSTANCE hInstance;
HWND hBirthdaysDlg = NULL;
HWND hUpcomingDlg = NULL;
extern HANDLE hAddBirthdayWndsList = NULL;
int hLangpack;

CommonData commonData = {0};

PLUGININFOEX pluginInfo = {
	sizeof(PLUGININFOEX),
	__PLUGIN_DISPLAY_NAME,
	VERSION,
	__DESC,
	__AUTHOR,
	__AUTHOREMAIL,
	__COPYRIGHT,
	__AUTHORWEB,
	UNICODE_AWARE,
	//{2ff96c84-b0b5-470e-bbf9-907b9f3f5d2f}
	{0x2ff96c84, 0xb0b5, 0x470e, {0xbb, 0xf9, 0x90, 0x7b, 0x9f, 0x3f, 0x5d, 0x2f}}
};

extern "C" __declspec(dllexport) PLUGININFOEX *MirandaPluginInfoEx(DWORD mirandaVersion) 
{
	return &pluginInfo;
}

extern "C" __declspec(dllexport) const MUUID MirandaInterfaces[] = {MIID_BIRTHDAYNOTIFY, MIID_LAST};

#include <commctrl.h>

extern "C" int __declspec(dllexport) Load(void)
{
	Log("%s", "Entering function " __FUNCTION__);

	mir_getLP(&pluginInfo);

	INITCOMMONCONTROLSEX icex;
	icex.dwSize = sizeof(icex);
	icex.dwICC = ICC_DATE_CLASSES;
	InitCommonControlsEx(&icex);
	
	LoadIcons();

	Log("%s", "Creating service functions ...");
	InitServices();

	Log("%s", "Hooking events ...");	
	HookEvents();
	
	hAddBirthdayWndsList = (HANDLE) CallService(MS_UTILS_ALLOCWINDOWLIST, 0, 0);
	
	Log("%s", "Leaving function " __FUNCTION__);
	
	return 0;
}

extern "C" int __declspec(dllexport) Unload()
{
	Log("%s", "Entering function " __FUNCTION__);
	
	if (hBirthdaysDlg)
		SendMessage(hBirthdaysDlg, WM_CLOSE, 0, 0);
	
	if (hUpcomingDlg)
		SendMessage(hUpcomingDlg, WM_CLOSE, 0, 0);
	
	WindowList_Broadcast(hAddBirthdayWndsList, WM_CLOSE, 0, 0);

	Log("%s", "Killing timers ...");
	KillTimers();
	
	Log("%s", "Unhooking events ...");
	UnhookEvents();
	
	Log("%s", "Destroying service functions ...");
	DestroyServices();

	Log("%s", "Leaving function " __FUNCTION__);
	return 0;
}

bool WINAPI DllMain(HINSTANCE hinstDLL,DWORD fdwReason,LPVOID lpvReserved)
{
	hInstance = hinstDLL;
	if (fdwReason == DLL_PROCESS_ATTACH)
		DisableThreadLibraryCalls(hinstDLL);

	return TRUE;
}
