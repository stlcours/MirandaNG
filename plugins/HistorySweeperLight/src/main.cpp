/*
Miranda IM History Sweeper Light plugin
Copyright (C) 2002-2003  Sergey V. Gershovich
Copyright (C) 2006-2009  Boris Krasnovskiy
Copyright (C) 2010, 2011 tico-tico

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

#include "historysweeperlight.h"

HINSTANCE hInst;

static HANDLE hHooks[5];
int hLangpack;

static PLUGININFOEX pluginInfoEx =
{ 
	sizeof(PLUGININFOEX),
	__PLUGIN_NAME,
	PLUGIN_MAKE_VERSION(__MAJOR_VERSION, __MINOR_VERSION, __RELEASE_NUM, __BUILD_NUM),
	__DESCRIPTION,
	__AUTHOR,
	__AUTHOREMAIL,
	__COPYRIGHT,
	__AUTHORWEB,
	UNICODE_AWARE,
	// {1D9BF74A-44A8-4B3F-A6E5-73069D3A8979}
	{0x1d9bf74a, 0x44a8, 0x4b3f, {0xa6, 0xe5, 0x73, 0x6, 0x9d, 0x3a, 0x89, 0x79}}
};

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	hInst = hinstDLL;
	return TRUE;
}

int OnIconPressed(WPARAM wParam, LPARAM lParam) 
{
	StatusIconClickData *sicd = (StatusIconClickData *)lParam;

	if ( !(sicd->flags & MBCF_RIGHTBUTTON) && !lstrcmpA(sicd->szModule, ModuleName) 
		&& DBGetContactSettingByte(NULL, ModuleName, "ChangeInMW", 0))
	{
		int nh = sicd->dwId; HANDLE hContact = (HANDLE)wParam; StatusIconData sid = {0};

		sid.cbSize = sizeof(sid);
		sid.szModule = ModuleName;
		sid.dwId = nh;
		sid.flags = MBF_HIDDEN;
		CallService(MS_MSG_MODIFYICON, (WPARAM)hContact, (LPARAM)&sid);	
		
		nh = (nh + 1) % 4;
		DBWriteContactSettingByte((HANDLE)wParam, ModuleName, "SweepHistory", (BYTE)nh);

		sid.dwId = nh;
		sid.flags = 0;
		CallService(MS_MSG_MODIFYICON, (WPARAM)hContact, (LPARAM)&sid);
	}
	return 0;
}


int OnModulesLoaded(WPARAM wParam, LPARAM lParam) 
{
	StatusIconData sid = {0};
	int i, sweep = DBGetContactSettingByte(NULL, ModuleName, "SweepHistory", 0);
	HANDLE hContact = db_find_first();

	sid.cbSize = sizeof(sid);
	sid.szModule = ModuleName;

	sid.hIcon = LoadIconEx("actG");
	if (sweep == 0)
		sid.szTooltip = Translate("Keep all events");
	else if (sweep == 1)
	{
		sid.szTooltip = Translate(time_stamp_strings[DBGetContactSettingByte(NULL, ModuleName, "StartupShutdownOlder", 0)]);
	}
	else if (sweep == 2)
	{
		sid.szTooltip = Translate(keep_strings[DBGetContactSettingByte(NULL, ModuleName, "StartupShutdownKeep", 0)]);
	}
	else if (sweep == 3)
	{
		sid.szTooltip = Translate("Delete all events");
	}
	sid.flags = MBF_HIDDEN;
	CallService(MS_MSG_ADDICON, 0, (LPARAM)&sid);

	sid.dwId = 1;
	sid.hIcon = LoadIconEx("act1");
	sid.szTooltip = Translate(time_stamp_strings[DBGetContactSettingByte(NULL, ModuleName, "StartupShutdownOlder", 0)]);
	sid.flags = MBF_HIDDEN;
	CallService(MS_MSG_ADDICON, 0, (LPARAM)&sid);

	sid.dwId = 2;
	sid.hIcon = LoadIconEx("act2");
	sid.szTooltip = Translate(keep_strings[DBGetContactSettingByte(NULL, ModuleName, "StartupShutdownKeep", 0)]);
	sid.flags = MBF_HIDDEN;
	CallService(MS_MSG_ADDICON, 0, (LPARAM)&sid);

	sid.dwId = 3;
	sid.hIcon = LoadIconEx("actDel");
	sid.szTooltip = Translate("Delete all events");
	sid.flags = MBF_HIDDEN;
	CallService(MS_MSG_ADDICON, 0, (LPARAM)&sid);
	
	// for new contacts
	while ( hContact != NULL )
	{
		ZeroMemory(&sid,sizeof(sid));

		sweep = DBGetContactSettingByte(hContact, ModuleName, "SweepHistory", 0);

		sid.cbSize = sizeof(sid);
		sid.szModule = ModuleName;
		
		for(i = 0; i < 4; i++)
		{
			sid.dwId = i;
			sid.flags = (sweep == i) ? 0 : MBF_HIDDEN;
			CallService(MS_MSG_MODIFYICON, (WPARAM)hContact, (LPARAM)&sid);
		}

		hContact = db_find_next(hContact);
	}

	hHooks[2] = HookEvent(ME_MSG_WINDOWEVENT, OnWindowEvent);
	hHooks[3] = HookEvent(ME_MSG_ICONPRESSED, OnIconPressed);

	return 0;
}

extern "C" __declspec(dllexport) PLUGININFOEX* MirandaPluginInfoEx(DWORD mirandaVersion)
{
	return &pluginInfoEx;
}

extern "C" __declspec(dllexport) int Load(void)
{

	mir_getLP(&pluginInfoEx);

	hHooks[0] = HookEvent(ME_SYSTEM_MODULESLOADED, OnModulesLoaded);
	hHooks[1] = HookEvent(ME_OPT_INITIALISE, HSOptInitialise);
	
	InitIcons();

	return 0;
}

extern "C" __declspec(dllexport) int Unload(void)
{ 
	int i;

	ShutdownAction();

	for (i = 0; i < SIZEOF(hHooks); i++)
	{
		if (hHooks[i])
			UnhookEvent(hHooks[i]);
	}

	return 0;
}