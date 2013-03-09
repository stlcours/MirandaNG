/* 
Copyright (C) 2006-2010 Ricardo Pescuma Domenecci

This is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with this file; see the file license.txt.  If
not, write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  
*/

#include "commons.h"

// Prototypes ///////////////////////////////////////////////////////////////////////////

PLUGININFOEX pluginInfo={
	sizeof(PLUGININFOEX),
	__PLUGIN_NAME,
	PLUGIN_MAKE_VERSION(__MAJOR_VERSION, __MINOR_VERSION, __RELEASE_NUM, __BUILD_NUM),
	__DESCRIPTION,
	__AUTHOR,
	__AUTHOREMAIL,
	__COPYRIGHT,
	__AUTHORWEB,
	UNICODE_AWARE,
	// {36753AE3-840B-4797-94A5-FD9F5852B942}
	{0x36753ae3, 0x840b, 0x4797, {0x94, 0xa5, 0xfd, 0x9f, 0x58, 0x52, 0xb9, 0x42}} 
};

HINSTANCE hInst;

int hLangpack = 0;

HANDLE hDictionariesFolder = NULL;
TCHAR *dictionariesFolder;

HANDLE hCustomDictionariesFolder = NULL;
TCHAR *customDictionariesFolder;

HANDLE hFlagsDllFolder = NULL;
TCHAR *flagsDllFolder;

HBITMAP hCheckedBmp;
BITMAP bmpChecked;

BOOL variables_enabled = FALSE;
BOOL loaded = FALSE;

LIST<Dictionary> languages(1);

// Functions ////////////////////////////////////////////////////////////////////////////

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) 
{
	hInst = hinstDLL;
	return TRUE;
}

extern "C" __declspec(dllexport) PLUGININFOEX* MirandaPluginInfoEx(DWORD mirandaVersion)
{
	return &pluginInfo;
}

static int IconsChanged(WPARAM wParam, LPARAM lParam) 
{
	if ( ServiceExists(MS_MSG_MODIFYICON)) {
		StatusIconData sid = { sizeof(sid) };
		sid.szModule = MODULE_NAME;
		sid.hIconDisabled = Skin_GetIcon("spellchecker_disabled");
		sid.flags = MBF_HIDDEN;

		for (int i = 0; i < languages.getCount(); i++) {
			sid.dwId = i;

			char tmp[128];
			mir_snprintf(tmp, SIZEOF(tmp), "%s - %S", 
				Translate("Spell Checker"), languages[i]->full_name);
			sid.szTooltip = tmp;

			HICON hIcon = (opts.use_flags) ? Skin_GetIconByHandle(languages[i]->hIcolib) : Skin_GetIcon("spellchecker_enabled");
			sid.hIcon = CopyIcon(hIcon);
			Skin_ReleaseIcon(hIcon);

			CallService(MS_MSG_MODIFYICON, 0, (LPARAM) &sid);
		}
	}

	return 0;
}

static int PreShutdown(WPARAM wParam, LPARAM lParam)
{
	if (ServiceExists(MS_MSG_REMOVEICON)) {
		StatusIconData sid = { sizeof(sid) };
		sid.szModule = MODULE_NAME;
		CallService(MS_MSG_REMOVEICON, 0, (LPARAM) &sid);
	}

	mir_free(dictionariesFolder);
	mir_free(customDictionariesFolder);
	mir_free(flagsDllFolder);
	return 0;
}

// Called when all the modules are loaded
static int ModulesLoaded(WPARAM wParam, LPARAM lParam) 
{
	variables_enabled = ServiceExists(MS_VARS_FORMATSTRING);

	// add our modules to the KnownModules list
	CallService("DBEditorpp/RegisterSingleModule", (WPARAM) MODULE_NAME, 0);

	// Folders plugin support
	if (hDictionariesFolder = FoldersRegisterCustomPathT(LPGEN("Spell Checker"), LPGEN("Dictionaries"), DICTIONARIES_FOLDER)) {
		dictionariesFolder = (TCHAR *) mir_alloc(sizeof(TCHAR) * MAX_PATH);
		FoldersGetCustomPathT(hDictionariesFolder, dictionariesFolder, MAX_PATH, _T("."));
	}
	else dictionariesFolder = Utils_ReplaceVarsT(DICTIONARIES_FOLDER);

	if (hCustomDictionariesFolder = FoldersRegisterCustomPathT(LPGEN("Spell Checker"), LPGEN("Custom Dictionaries"), CUSTOM_DICTIONARIES_FOLDER)) {
		customDictionariesFolder = (TCHAR *) mir_alloc(sizeof(TCHAR) * MAX_PATH);
		FoldersGetCustomPathT(hCustomDictionariesFolder, customDictionariesFolder, MAX_PATH, _T("."));
	}
	else customDictionariesFolder = Utils_ReplaceVarsT(CUSTOM_DICTIONARIES_FOLDER);
		
	if (hFlagsDllFolder = FoldersRegisterCustomPathT(LPGEN("Spell Checker"), LPGEN("Flags DLL"), FLAGS_DLL_FOLDER)) {
		flagsDllFolder = (TCHAR *) mir_alloc(sizeof(TCHAR) * MAX_PATH);
		FoldersGetCustomPathT(hFlagsDllFolder, flagsDllFolder, MAX_PATH, _T("."));
	}
	else flagsDllFolder = Utils_ReplaceVarsT(FLAGS_DLL_FOLDER);

	InitOptions();
	
	GetAvaibleDictionaries(languages, dictionariesFolder, customDictionariesFolder);

	LoadOptions();

	if (opts.use_flags) {
		// Load flags dll
		TCHAR flag_file[MAX_PATH];
		mir_sntprintf(flag_file, SIZEOF(flag_file), _T("%s\\flags_icons.dll"), flagsDllFolder);
		HMODULE hFlagsDll = LoadLibraryEx(flag_file, NULL, LOAD_LIBRARY_AS_DATAFILE);

		TCHAR path[MAX_PATH];
		GetModuleFileName(hInst, path, MAX_PATH);

		SKINICONDESC sid = { sizeof(sid) };
		sid.flags = SIDF_ALL_TCHAR | SIDF_SORTED;
		sid.ptszSection = _T("Spell Checker/Flags");

		// Get language flags
		for(int i = 0; i < languages.getCount(); i++) {
			Dictionary *p = languages[i];
			sid.ptszDescription = p->full_name;

			char lang[32];
			mir_snprintf(lang, SIZEOF(lang), "spell_lang_%d", i);
			sid.pszName = lang;

			HICON hFlag;
			if (hFlagsDll != NULL)
				hFlag = (HICON)LoadImage(hFlagsDll, p->language, IMAGE_ICON, 16, 16, 0);
			else
				hFlag = NULL;

			if (hFlag != NULL) {
				sid.hDefaultIcon = hFlag;
				sid.ptszDefaultFile = NULL;
				sid.iDefaultIndex = 0;
			}
			else {
				sid.hDefaultIcon = NULL;
				sid.ptszDefaultFile = path;
				sid.iDefaultIndex = -IDI_UNKNOWN_FLAG;
			}

			// Oki, lets add to IcoLib, then
			p->hIcolib = Skin_AddIcon(&sid);
			
			if (hFlag != NULL)
				DestroyIcon(hFlag);
		}
		FreeLibrary(hFlagsDll);
	}

	for (int j = 0; j < languages.getCount(); j++) {
		Dictionary *dict = languages[j];

		TCHAR filename[MAX_PATH];
		mir_sntprintf(filename, MAX_PATH, _T("%s\\%s.ar"), customDictionariesFolder, dict->language);
		dict->autoReplace = new AutoReplaceMap(filename, dict);

		if (lstrcmp(dict->language, opts.default_language) == 0)
			dict->load();
	}

	HookEvent(ME_SKIN2_ICONSCHANGED, &IconsChanged);
	HookEvent(ME_MSG_WINDOWEVENT, &MsgWindowEvent);
	HookEvent(ME_MSG_WINDOWPOPUP, &MsgWindowPopup);
	HookEvent(ME_MSG_ICONPRESSED, &IconPressed);

	CreateServiceFunction(MS_SPELLCHECKER_ADD_RICHEDIT, AddContactTextBoxService);
	CreateServiceFunction(MS_SPELLCHECKER_REMOVE_RICHEDIT, RemoveContactTextBoxService);
	CreateServiceFunction(MS_SPELLCHECKER_SHOW_POPUP_MENU, ShowPopupMenuService);

	if (ServiceExists(MS_MSG_ADDICON)) {
		StatusIconData sid = { sizeof(sid) };
		sid.szModule = MODULE_NAME;
		sid.hIconDisabled = Skin_GetIcon("spellchecker_disabled");
		sid.flags = MBF_HIDDEN;

		for (int i = 0; i < languages.getCount(); i++) {
			sid.dwId = i;

			char tmp[128];
			mir_snprintf(tmp, SIZEOF(tmp), "%s - %S", 
				Translate("Spell Checker"), languages[i]->full_name);
			sid.szTooltip = tmp;

			HICON hIcon = (opts.use_flags) ? Skin_GetIconByHandle(languages[i]->hIcolib) : Skin_GetIcon("spellchecker_enabled");
			sid.hIcon = CopyIcon(hIcon);
			Skin_ReleaseIcon(hIcon);

			CallService(MS_MSG_ADDICON, 0, (LPARAM) &sid);
		}
	}

	HOTKEYDESC hkd = {0};
	hkd.cbSize = sizeof(hkd);
	hkd.pszName = "Spell Checker/Toggle";
	hkd.pszSection = LPGEN("Spell Checker");
	hkd.pszDescription = LPGEN("Enable/disable spell checker");
	hkd.DefHotKey = HOTKEYCODE(HOTKEYF_SHIFT|HOTKEYF_ALT, 'S');
	hkd.lParam = HOTKEY_ACTION_TOGGLE;
	Hotkey_Register( &hkd);

	loaded = TRUE;

	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////

static IconItem iconList[] =
{
	{ LPGEN("Enabled"),  "spellchecker_enabled",  IDI_CHECK    },
	{ LPGEN("Disabled"), "spellchecker_disabled", IDI_NO_CHECK }
};

extern "C" int __declspec(dllexport) Load(void) 
{
	mir_getLP(&pluginInfo);

	// icons
	Icon_Register(hInst, LPGEN("Spell Checker"), iconList, SIZEOF(iconList));

	// hooks
	HookEvent(ME_SYSTEM_MODULESLOADED, ModulesLoaded);
	HookEvent(ME_SYSTEM_PRESHUTDOWN, PreShutdown);

	hCheckedBmp = LoadBitmap(NULL, MAKEINTRESOURCE(OBM_CHECK));
	if (GetObject(hCheckedBmp, sizeof(bmpChecked), &bmpChecked) == 0)
		bmpChecked.bmHeight = bmpChecked.bmWidth = 10;

	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////

extern "C" int __declspec(dllexport) Unload(void) 
{
	DeleteObject(hCheckedBmp);
	FreeDictionaries(languages);

	return 0;
}
