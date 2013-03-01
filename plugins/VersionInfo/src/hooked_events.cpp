/*
Version information plugin for Miranda IM

Copyright � 2002-2006 Luca Santarelli, Cristian Libotean

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

#include "common.h"
#include "hooked_events.h"

HANDLE hModulesLoaded;
HANDLE hOptionsInitialize;

#define HOST "http://eblis.tla.ro/projects"

#define VERSIONINFO_VERSION_URL HOST "/miranda/VersionInfo/updater/VersionInfo.html"
#define VERSIONINFO_UPDATE_URL HOST "/miranda/VersionInfo/updater/VersionInfo.zip"
#define VERSIONINFO_VERSION_PREFIX "Version Information version "

int HookEvents()
{
	hModulesLoaded = HookEvent(ME_SYSTEM_MODULESLOADED, OnModulesLoaded);
	hOptionsInitialize = HookEvent(ME_OPT_INITIALISE, OnOptionsInitialise);
	//hPreShutdown = HookEvent(ME_SYSTEM_PRESHUTDOWN, OnPreShutdown);
	
	return 0;
}

int UnhookEvents()
{
	UnhookEvent(hModulesLoaded);
	UnhookEvent(hOptionsInitialize);
	
	return 0;
}

int OnModulesLoaded(WPARAM wParam, LPARAM lParam)
{
	hOutputLocation = FoldersRegisterCustomPathT("VersionInfo", "Output folder", _T("%miranda_path%"));
	
	GetStringFromDatabase("UUIDCharMark", _T(DEF_UUID_CHARMARK), PLUGIN_UUID_MARK, cPLUGIN_UUID_MARK);
	return 0;
}

int OnOptionsInitialise(WPARAM wParam, LPARAM lParam)
{
	OPTIONSDIALOGPAGE odp = { 0 };
	odp.cbSize = sizeof(odp);
	odp.position = 100000000;
	odp.hInstance = hInst;
	odp.pszTemplate = MAKEINTRESOURCEA(IDD_OPT_VERSIONINFO);
	odp.pszTitle = LPGEN("Version Information");
	odp.pszGroup = LPGEN("Services");
	odp.groupPosition = 910000000;
	odp.flags = ODPF_BOLDGROUPS;
	odp.pfnDlgProc = DlgProcOpts;
	
	Options_AddPage(wParam, &odp);

	return 0;
}