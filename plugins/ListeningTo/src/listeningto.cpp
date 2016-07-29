/*
Copyright (C) 2006-2009 Ricardo Pescuma Domenecci

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

#include "stdafx.h"

CLIST_INTERFACE *pcli;
int hLangpack;

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
	// {F981F3F5-035A-444F-9892-CA722C195ADA}
	{ 0xf981f3f5, 0x35a, 0x444f, { 0x98, 0x92, 0xca, 0x72, 0x2c, 0x19, 0x5a, 0xda } }
};

HINSTANCE hInst;

static HANDLE hEnableStateChangedEvent;
HANDLE hExtraIcon;
static HGENMENU hMainMenuGroup = NULL;
static HANDLE hListeningInfoChangedEvent = NULL;

static HANDLE hTTB = NULL;
BOOL loaded = FALSE;
static UINT hTimer = 0;
static DWORD lastInfoSetTime = 0;

std::vector<ProtocolInfo> proto_items;

int ModulesLoaded(WPARAM wParam, LPARAM lParam);
int PreShutdown(WPARAM wParam, LPARAM lParam);
int PreBuildContactMenu(WPARAM wParam, LPARAM lParam);
int TopToolBarLoaded(WPARAM wParam, LPARAM lParam);
int SettingChanged(WPARAM wParam, LPARAM lParam);

INT_PTR MainMenuClicked(WPARAM wParam, LPARAM lParam);
bool    ListeningToEnabled(char *proto, bool ignoreGlobal = false);
INT_PTR ListeningToEnabled(WPARAM wParam, LPARAM lParam);
INT_PTR EnableListeningTo(WPARAM wParam, LPARAM lParam);
INT_PTR EnableListeningTo(char *proto = NULL, bool enabled = false);
INT_PTR GetTextFormat(WPARAM wParam, LPARAM lParam);
wchar_t*  GetParsedFormat(LISTENINGTOINFO *lti);
INT_PTR GetParsedFormat(WPARAM wParam, LPARAM lParam);
INT_PTR GetOverrideContactOption(WPARAM wParam, LPARAM lParam);
INT_PTR GetUnknownText(WPARAM wParam, LPARAM lParam);
INT_PTR SetNewSong(WPARAM wParam, LPARAM lParam);
void    SetExtraIcon(MCONTACT hContact, BOOL set);
void    SetListeningInfos(LISTENINGTOINFO *lti = NULL);
INT_PTR HotkeysEnable(WPARAM wParam, LPARAM lParam);
INT_PTR HotkeysDisable(WPARAM wParam, LPARAM lParam);
INT_PTR HotkeysToggle(WPARAM wParam, LPARAM lParam);

wchar_t* VariablesParseInfo(ARGUMENTSINFO *ai);
wchar_t* VariablesParseType(ARGUMENTSINFO *ai);
wchar_t* VariablesParseArtist(ARGUMENTSINFO *ai);
wchar_t* VariablesParseAlbum(ARGUMENTSINFO *ai);
wchar_t* VariablesParseTitle(ARGUMENTSINFO *ai);
wchar_t* VariablesParseTrack(ARGUMENTSINFO *ai);
wchar_t* VariablesParseYear(ARGUMENTSINFO *ai);
wchar_t* VariablesParseGenre(ARGUMENTSINFO *ai);
wchar_t* VariablesParseLength(ARGUMENTSINFO *ai);
wchar_t* VariablesParsePlayer(ARGUMENTSINFO *ai);


#define XSTATUS_MUSIC 11

#define UNKNOWN(_X_) ( _X_ == NULL || _X_[0] == '\0' ? opts.unknown : _X_ )

// Functions ////////////////////////////////////////////////////////////////////////////

extern "C" BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD, LPVOID)
{
	hInst = hinstDLL;
	return TRUE;
}

extern "C" __declspec(dllexport) PLUGININFOEX* MirandaPluginInfoEx(DWORD)
{
	return &pluginInfo;
}

static IconItem iconList[] =
{
	{ LPGEN("Listening to (enabled)"), "listening_to_icon", IDI_LISTENINGTO },
	{ LPGEN("Listening to (disabled)"), "listening_off_icon", IDI_LISTENINGOFF },
};

extern "C" int __declspec(dllexport) Load(void)
{
	mir_getLP(&pluginInfo);
	mir_getCLI();

	CoInitialize(NULL);

	// Services
	CreateServiceFunction(MS_LISTENINGTO_ENABLED, ListeningToEnabled);
	CreateServiceFunction(MS_LISTENINGTO_ENABLE, EnableListeningTo);
	CreateServiceFunction(MS_LISTENINGTO_GETTEXTFORMAT, GetTextFormat);
	CreateServiceFunction(MS_LISTENINGTO_GETPARSEDTEXT, GetParsedFormat);
	CreateServiceFunction(MS_LISTENINGTO_OVERRIDECONTACTOPTION, GetOverrideContactOption);
	CreateServiceFunction(MS_LISTENINGTO_GETUNKNOWNTEXT, GetUnknownText);
	CreateServiceFunction(MS_LISTENINGTO_MAINMENU, MainMenuClicked);
	CreateServiceFunction(MS_LISTENINGTO_SET_NEW_SONG, SetNewSong);
	CreateServiceFunction(MS_LISTENINGTO_HOTKEYS_ENABLE, HotkeysEnable);
	CreateServiceFunction(MS_LISTENINGTO_HOTKEYS_DISABLE, HotkeysDisable);
	CreateServiceFunction(MS_LISTENINGTO_HOTKEYS_TOGGLE, HotkeysToggle);

	// Hooks
	HookEvent(ME_SYSTEM_MODULESLOADED, ModulesLoaded);
	HookEvent(ME_SYSTEM_PRESHUTDOWN, PreShutdown);
	HookEvent(ME_DB_CONTACT_SETTINGCHANGED, SettingChanged);

	hEnableStateChangedEvent = CreateHookableEvent(ME_LISTENINGTO_ENABLE_STATE_CHANGED);
	hListeningInfoChangedEvent = CreateHookableEvent(ME_LISTENINGTO_LISTENING_INFO_CHANGED);

	InitMusic();
	InitOptions();

	// icons
	Icon_Register(hInst, LPGEN("ListeningTo"), iconList, _countof(iconList));

	// Extra icon support
	hExtraIcon = ExtraIcon_RegisterIcolib(MODULE_NAME "_icon", LPGEN("Listening to music"), "listening_to_icon");
	return 0;
}

extern "C" int __declspec(dllexport) Unload(void)
{
	CoUninitialize();

	return 0;
}

void UpdateGlobalStatusMenus()
{
	bool enabled = ListeningToEnabled(NULL, true);

	Menu_SetChecked(proto_items[0].hMenu, enabled);
	Menu_EnableItem(proto_items[0].hMenu, opts.enable_sending);

	if (hTTB != NULL)
		CallService(MS_TTB_SETBUTTONSTATE, (WPARAM)hTTB, (LPARAM)(enabled ? TTBST_PUSHED : 0));
}


struct compareFunc : std::binary_function<const ProtocolInfo, const ProtocolInfo, bool>
{
	bool operator()(const ProtocolInfo &one, const ProtocolInfo &two) const
	{
		return mir_wstrcmp(one.account, two.account) < 0;
	}
};


void RebuildMenu()
{
	std::sort(proto_items.begin(), proto_items.end(), compareFunc());

	for (unsigned int i = 1; i < proto_items.size(); i++) {
		ProtocolInfo *info = &proto_items[i];

		if (info->hMenu != NULL)
			Menu_RemoveItem(info->hMenu);

		wchar_t text[512];
		mir_snwprintf(text, TranslateT("Send to %s"), info->account);

		CMenuItem mi;
		mi.position = 100000 + i;
		mi.root = hMainMenuGroup;
		mi.position = 500080000 + i;
		mi.pszService = MS_LISTENINGTO_MAINMENU;
		mi.name.w = text;
		mi.flags = CMIF_UNICODE | CMIF_UNMOVABLE
			| (ListeningToEnabled(info->proto, TRUE) ? CMIF_CHECKED : 0)
			| (opts.enable_sending ? 0 : CMIF_GRAYED);

		info->hMenu = Menu_AddMainMenuItem(&mi);
		Menu_ConfigureItem(info->hMenu, MCI_OPT_EXECPARAM, i);
	}

	UpdateGlobalStatusMenus();
}

void RegisterProtocol(char *proto, wchar_t *account)
{
	if (!ProtoServiceExists(proto, PS_SET_LISTENINGTO) && !ProtoServiceExists(proto, PS_SETCUSTOMSTATUSEX) && !ProtoServiceExists(proto, PS_SETAWAYMSG))
		return;

	size_t id = proto_items.size();
	proto_items.resize(id + 1);

	strncpy(proto_items[id].proto, proto, _countof(proto_items[id].proto));
	proto_items[id].proto[_countof(proto_items[id].proto) - 1] = 0;

	mir_wstrncpy(proto_items[id].account, account, _countof(proto_items[id].account));

	proto_items[id].hMenu = NULL;
	proto_items[id].old_xstatus = 0;
	proto_items[id].old_xstatus_name[0] = '\0';
	proto_items[id].old_xstatus_message[0] = '\0';
}


int AccListChanged(WPARAM wParam, LPARAM lParam)
{
	PROTOACCOUNT *proto = (PROTOACCOUNT *)lParam;
	if (proto == NULL)
		return 0;

	ProtocolInfo *info = GetProtoInfo(proto->szModuleName);
	if (info != NULL) {
		if (wParam == PRAC_UPGRADED || wParam == PRAC_CHANGED) {
			mir_wstrncpy(info->account, proto->tszAccountName, _countof(info->account));

			wchar_t text[512];
			mir_snwprintf(text, TranslateT("Send to %s"), info->account);
			Menu_ModifyItem(info->hMenu, text);
		}
		else if (wParam == PRAC_REMOVED || (wParam == PRAC_CHECKED && !proto->bIsEnabled)) {
			Menu_RemoveItem(info->hMenu);

			for (std::vector<ProtocolInfo>::iterator it = proto_items.begin(); it != proto_items.end(); ++it) {
				if (&(*it) == info) {
					proto_items.erase(it);
					break;
				}
			}

			RebuildMenu();
		}
	}
	else {
		if (wParam == PRAC_ADDED || (wParam == PRAC_CHECKED && proto->bIsEnabled)) {
			RegisterProtocol(proto->szModuleName, proto->tszAccountName);
			RebuildMenu();
		}
	}

	return 0;
}

// Called when all the modules are loaded

int ModulesLoaded(WPARAM, LPARAM)
{
	EnableDisablePlayers();

	for (MCONTACT hContact = db_find_first(); hContact; hContact = db_find_next(hContact)) {
		char *proto = GetContactProto(hContact);
		if (proto != NULL) {
			DBVARIANT dbv;
			if (!db_get_ws(hContact, proto, "ListeningTo", &dbv)) {
				if (dbv.ptszVal != NULL && dbv.ptszVal[0] != 0)
					SetExtraIcon(hContact, TRUE);

				db_free(&dbv);
			}
		}
	}

	// Add main menu item
	CMenuItem mi;
	SET_UID(mi, 0xe8e4e594, 0x255e, 0x434d, 0x83, 0x74, 0x79, 0x44, 0x1b, 0x4e, 0xe7, 0x16);
	mi.position = 500080000;
	mi.name.w = LPGENW("Listening to");
	mi.flags = CMIF_UNICODE;
	mi.hIcolibItem = iconList[0].hIcolib;
	hMainMenuGroup = Menu_AddMainMenuItem(&mi);

	mi.root = hMainMenuGroup;
	mi.position = 0;
	mi.pszService = MS_LISTENINGTO_MAINMENU;
	mi.hIcolibItem = NULL;

	// Add all protos
	SET_UID(mi, 0xc396a9dd, 0x9a00, 0x46af, 0x96, 0x2e, 0x5, 0x5a, 0xbc, 0x52, 0xfc, 0x9b);
	mi.name.w = LPGENW("Send to all protocols");
	mi.flags = CMIF_UNICODE
		| (ListeningToEnabled(NULL, true) ? CMIF_CHECKED : 0)
		| (opts.enable_sending ? 0 : CMIF_GRAYED);
	proto_items.resize(1);
	proto_items[0].hMenu = Menu_AddMainMenuItem(&mi);
	proto_items[0].proto[0] = 0;
	proto_items[0].account[0] = 0;
	proto_items[0].old_xstatus = 0;
	proto_items[0].old_xstatus_name[0] = '\0';
	proto_items[0].old_xstatus_message[0] = '\0';

	// Add each proto
	PROTOACCOUNT **protos;
	int count;
	Proto_EnumAccounts(&count, &protos);

	for (int i = 0; i < count; i++)
		if (protos[i]->bIsEnabled)
			RegisterProtocol(protos[i]->szModuleName, protos[i]->tszAccountName);

	HookEvent(ME_PROTO_ACCLISTCHANGED, AccListChanged);

	RebuildMenu();

	HookEvent(ME_TTB_MODULELOADED, TopToolBarLoaded);

	// Variables support
	if (ServiceExists(MS_VARS_REGISTERTOKEN)) {
		TOKENREGISTER tr = { 0 };
		tr.cbSize = sizeof(TOKENREGISTER);
		tr.memType = TR_MEM_MIRANDA;
		tr.flags = TRF_FREEMEM | TRF_PARSEFUNC | TRF_FIELD | TRF_TCHAR;

		tr.tszTokenString = L"listening_info";
		tr.parseFunctionT = VariablesParseInfo;
		tr.szHelpText = LPGEN("Listening info") "\t" LPGEN("Listening info as set in the options");
		CallService(MS_VARS_REGISTERTOKEN, 0, (LPARAM)&tr);

		tr.tszTokenString = L"listening_type";
		tr.parseFunctionT = VariablesParseType;
		tr.szHelpText = LPGEN("Listening info") "\t" LPGEN("Media type: Music, Video, etc.");
		CallService(MS_VARS_REGISTERTOKEN, 0, (LPARAM)&tr);

		tr.tszTokenString = L"listening_artist";
		tr.parseFunctionT = VariablesParseArtist;
		tr.szHelpText = LPGEN("Listening info") "\t" LPGEN("Artist name");
		CallService(MS_VARS_REGISTERTOKEN, 0, (LPARAM)&tr);

		tr.tszTokenString = L"listening_album";
		tr.parseFunctionT = VariablesParseAlbum;
		tr.szHelpText = LPGEN("Listening info") "\t" LPGEN("Album name");
		CallService(MS_VARS_REGISTERTOKEN, 0, (LPARAM)&tr);

		tr.tszTokenString = L"listening_title";
		tr.parseFunctionT = VariablesParseTitle;
		tr.szHelpText = LPGEN("Listening info") "\t" LPGEN("Song name");
		CallService(MS_VARS_REGISTERTOKEN, 0, (LPARAM)&tr);

		tr.tszTokenString = L"listening_track";
		tr.parseFunctionT = VariablesParseTrack;
		tr.szHelpText = LPGEN("Listening info") "\t" LPGEN("Track number");
		CallService(MS_VARS_REGISTERTOKEN, 0, (LPARAM)&tr);

		tr.tszTokenString = L"listening_year";
		tr.parseFunctionT = VariablesParseYear;
		tr.szHelpText = LPGEN("Listening info") "\t" LPGEN("Song year");
		CallService(MS_VARS_REGISTERTOKEN, 0, (LPARAM)&tr);

		tr.tszTokenString = L"listening_genre";
		tr.parseFunctionT = VariablesParseGenre;
		tr.szHelpText = LPGEN("Listening info") "\t" LPGEN("Song genre");
		CallService(MS_VARS_REGISTERTOKEN, 0, (LPARAM)&tr);

		tr.tszTokenString = L"listening_length";
		tr.parseFunctionT = VariablesParseLength;
		tr.szHelpText = LPGEN("Listening info") "\t" LPGEN("Song length");
		CallService(MS_VARS_REGISTERTOKEN, 0, (LPARAM)&tr);

		tr.tszTokenString = L"listening_player";
		tr.parseFunctionT = VariablesParsePlayer;
		tr.szHelpText = LPGEN("Listening info") "\t" LPGEN("Player name");
		CallService(MS_VARS_REGISTERTOKEN, 0, (LPARAM)&tr);
	}

	// Hotkeys support
	HOTKEYDESC hkd = { 0 };
	hkd.cbSize = sizeof(hkd);
	hkd.pszSection = LPGEN("Listening to");

	hkd.pszService = MS_LISTENINGTO_HOTKEYS_ENABLE;
	hkd.pszName = "ListeningTo/EnableAll";
	hkd.pszDescription = LPGEN("Send to all protocols");
	Hotkey_Register(&hkd);

	hkd.pszService = MS_LISTENINGTO_HOTKEYS_DISABLE;
	hkd.pszName = "ListeningTo/DisableAll";
	hkd.pszDescription = LPGEN("Don't send to any protocols");
	Hotkey_Register(&hkd);

	hkd.pszService = MS_LISTENINGTO_HOTKEYS_TOGGLE;
	hkd.pszName = "ListeningTo/ToggleAll";
	hkd.pszDescription = LPGEN("Toggle send to all protocols");
	Hotkey_Register(&hkd);

	//
	SetListeningInfos();
	StartTimer();

	loaded = TRUE;
	return 0;
}


int PreShutdown(WPARAM, LPARAM)
{
	loaded = FALSE;

	if (hTimer != NULL) {
		KillTimer(NULL, hTimer);
		hTimer = NULL;
	}

	DeInitOptions();

	DestroyHookableEvent(hEnableStateChangedEvent);
	DestroyHookableEvent(hListeningInfoChangedEvent);

	FreeMusic();
	return 0;
}

static INT_PTR TopToolBarClick(WPARAM, LPARAM)
{
	EnableListeningTo(NULL, !ListeningToEnabled(NULL, true));
	return 0;
}

// Toptoolbar hook to put an icon in the toolbar
int TopToolBarLoaded(WPARAM, LPARAM)
{
	BOOL enabled = ListeningToEnabled(NULL, true);

	CreateServiceFunction(MS_LISTENINGTO_TTB, TopToolBarClick);

	TTBButton ttb = { 0 };
	ttb.hIconHandleDn = iconList[0].hIcolib;
	ttb.hIconHandleUp = iconList[1].hIcolib;
	ttb.pszService = MS_LISTENINGTO_TTB;
	ttb.dwFlags = TTBBF_VISIBLE | TTBBF_SHOWTOOLTIP | TTBBF_ASPUSHBUTTON | (enabled ? TTBBF_PUSHED : 0);
	ttb.name = LPGEN("Enable/Disable sending Listening To info (to all protocols)");
	ttb.pszTooltipDn = LPGEN("Disable ListeningTo (to all protocols)");
	ttb.pszTooltipUp = LPGEN("Enable ListeningTo (to all protocols)");
	hTTB = TopToolbar_AddButton(&ttb);
	return 0;
}

INT_PTR MainMenuClicked(WPARAM wParam, LPARAM)
{
	if (!loaded)
		return -1;

	if (wParam < proto_items.size()) {
		ProtocolInfo &pi = proto_items[wParam];
		EnableListeningTo(pi.proto, !ListeningToEnabled(pi.proto, TRUE));
	}
	return 0;
}

bool ListeningToEnabled(char *proto, bool ignoreGlobal)
{
	if (!ignoreGlobal && !opts.enable_sending)
		return FALSE;

	if (proto == NULL || proto[0] == 0) {
		// Check all protocols
		for (unsigned int i = 1; i < proto_items.size(); ++i)
			if (!ListeningToEnabled(proto_items[i].proto, TRUE))
				return FALSE;

		return TRUE;
	}

	char setting[256];
	mir_snprintf(setting, "%sEnabled", proto);
	return db_get_b(NULL, MODULE_NAME, setting, false) != 0;
}

INT_PTR ListeningToEnabled(WPARAM wParam, LPARAM)
{
	if (!loaded)
		return -1;

	return ListeningToEnabled((char*)wParam);
}

ProtocolInfo* GetProtoInfo(char *proto)
{
	for (unsigned int i = 1; i < proto_items.size(); i++)
		if (mir_strcmp(proto, proto_items[i].proto) == 0)
			return &proto_items[i];

	return NULL;
}

void SetListeningInfo(char *proto, LISTENINGTOINFO *lti = NULL)
{
	if (proto == NULL || !ListeningToEnabled(proto))
		return;

	if (ProtoServiceExists(proto, PS_SET_LISTENINGTO))
		CallProtoService(proto, PS_SET_LISTENINGTO, 0, (LPARAM)lti);

	else if (ProtoServiceExists(proto, PS_SETCUSTOMSTATUSEX)) {
		if (opts.xstatus_set == IGNORE_XSTATUS)
			return;

		int status;
		CUSTOM_STATUS ics = { 0 };
		ics.cbSize = sizeof(CUSTOM_STATUS);
		ics.status = &status;

		// Set or reset?
		if (lti == NULL) {
			// Reset -> only if is still in music xstatus
			ics.flags = CSSF_MASK_STATUS;
			if (CallProtoService(proto, PS_GETCUSTOMSTATUSEX, 0, (LPARAM)&ics) || status != XSTATUS_MUSIC) {
				if (opts.xstatus_set == SET_XSTATUS) {
					ProtocolInfo *pi = GetProtoInfo(proto);
					if (pi != NULL) {
						pi->old_xstatus = 0;
						pi->old_xstatus_name[0] = '\0';
						pi->old_xstatus_message[0] = '\0';
					}
				}
				return;
			}

			if (opts.xstatus_set == CHECK_XSTATUS_MUSIC) {
				// Set text to nothing
				wchar_t *fr[] = {
					L"listening", opts.nothing
				};

				Buffer<wchar_t> name;
				ReplaceTemplate(&name, NULL, opts.xstatus_name, fr, _countof(fr));
				Buffer<wchar_t> msg;
				ReplaceTemplate(&msg, NULL, opts.xstatus_message, fr, _countof(fr));

				ics.flags = CSSF_UNICODE | CSSF_MASK_STATUS | CSSF_MASK_NAME | CSSF_MASK_MESSAGE;
				ics.ptszName = name.str;
				ics.ptszMessage = msg.str;

				CallProtoService(proto, PS_SETCUSTOMSTATUSEX, 0, (LPARAM)&ics);
			}
			else if (opts.xstatus_set == CHECK_XSTATUS) {
				status = 0;
				ics.flags = CSSF_MASK_STATUS;

				CallProtoService(proto, PS_SETCUSTOMSTATUSEX, 0, (LPARAM)&ics);
			}
			else {
				// Set to old text
				ProtocolInfo *pi = GetProtoInfo(proto);
				if (pi != NULL) {
					ics.flags = CSSF_UNICODE | CSSF_MASK_STATUS | CSSF_MASK_NAME | CSSF_MASK_MESSAGE;
					ics.status = &pi->old_xstatus;
					ics.ptszName = pi->old_xstatus_name;
					ics.ptszMessage = pi->old_xstatus_message;
				}
				else {
					status = 0;
					ics.flags = CSSF_MASK_STATUS;
				}

				CallProtoService(proto, PS_SETCUSTOMSTATUSEX, 0, (LPARAM)&ics);

				if (pi != NULL) {
					pi->old_xstatus = 0;
					pi->old_xstatus_name[0] = '\0';
					pi->old_xstatus_message[0] = '\0';
				}
			}
		}
		else {
			// Set it
			if (opts.xstatus_set == CHECK_XSTATUS_MUSIC) {
				ics.flags = CSSF_MASK_STATUS;
				if (CallProtoService(proto, PS_GETCUSTOMSTATUSEX, 0, (LPARAM)&ics) || status != XSTATUS_MUSIC)
					return;
			}
			else if (opts.xstatus_set == CHECK_XSTATUS) {
				ics.flags = CSSF_MASK_STATUS;
				if (!CallProtoService(proto, PS_GETCUSTOMSTATUSEX, 0, (LPARAM)&ics) && status != XSTATUS_MUSIC && status != 0)
					return;
			}
			else {
				// Store old data
				ics.flags = CSSF_MASK_STATUS;
				if (!CallProtoService(proto, PS_GETCUSTOMSTATUSEX, 0, (LPARAM)&ics) && status != XSTATUS_MUSIC) {
					ProtocolInfo *pi = GetProtoInfo(proto);
					if (pi != NULL) {
						ics.flags = CSSF_UNICODE | CSSF_MASK_STATUS | CSSF_MASK_NAME | CSSF_MASK_MESSAGE;
						ics.status = &pi->old_xstatus;
						ics.ptszName = pi->old_xstatus_name;
						ics.ptszMessage = pi->old_xstatus_message;

						CallProtoService(proto, PS_GETCUSTOMSTATUSEX, 0, (LPARAM)&ics);
					}
				}
			}

			wchar_t *fr[] = {
				L"listening", GetParsedFormat(lti),
				L"artist", UNKNOWN(lti->ptszArtist),
				L"album", UNKNOWN(lti->ptszAlbum),
				L"title", UNKNOWN(lti->ptszTitle),
				L"track", UNKNOWN(lti->ptszTrack),
				L"year", UNKNOWN(lti->ptszYear),
				L"genre", UNKNOWN(lti->ptszGenre),
				L"length", UNKNOWN(lti->ptszLength),
				L"player", UNKNOWN(lti->ptszPlayer),
				L"type", UNKNOWN(lti->ptszType)
			};

			Buffer<wchar_t> name;
			ReplaceTemplate(&name, NULL, opts.xstatus_name, fr, _countof(fr));
			Buffer<wchar_t> msg;
			ReplaceTemplate(&msg, NULL, opts.xstatus_message, fr, _countof(fr));

			status = XSTATUS_MUSIC;
			ics.flags = CSSF_UNICODE | CSSF_MASK_STATUS | CSSF_MASK_NAME | CSSF_MASK_MESSAGE;
			ics.status = &status;
			ics.ptszName = name.str;
			ics.ptszMessage = msg.str;

			CallProtoService(proto, PS_SETCUSTOMSTATUSEX, 0, (LPARAM)&ics);

			mir_free(fr[1]);
		}
	}
	else if (db_get_b(0, MODULE_NAME, "UseStatusMessage", 1) && ProtoServiceExists(proto, PS_SETAWAYMSG)) {
		int status = CallProtoService(proto, PS_GETSTATUS, 0, 0);
		if (lti == NULL)
			CallProtoService(proto, PS_SETAWAYMSG, status, 0);
		else {
			ptrW fr(GetParsedFormat(lti));
			CallProtoService(proto, PS_SETAWAYMSG, status, fr);
		}
	}
}

INT_PTR EnableListeningTo(char *proto, bool enabled)
{
	if (!loaded)
		return -1;

	if (proto == NULL || proto[0] == 0) {
		// For all protocols
		for (unsigned int i = 1; i < proto_items.size(); ++i)
			EnableListeningTo(proto_items[i].proto, enabled);
	}
	else {
		if (!ProtoServiceExists(proto, PS_SET_LISTENINGTO) && !ProtoServiceExists(proto, PS_SETCUSTOMSTATUSEX) && !ProtoServiceExists(proto, PS_SETAWAYMSG))
			return 0;

		char setting[256];
		mir_snprintf(setting, "%sEnabled", proto);
		db_set_b(NULL, MODULE_NAME, setting, enabled);

		// Modify menu info
		ProtocolInfo *info = GetProtoInfo(proto);
		if (info != NULL) {
			Menu_EnableItem(info->hMenu, opts.enable_sending);
			Menu_SetChecked(info->hMenu, enabled);

			SetListeningInfo(proto, (opts.enable_sending && enabled) ? GetListeningInfo() : NULL);
		}

		// Set all protos info
		UpdateGlobalStatusMenus();
	}

	StartTimer();

	NotifyEventHooks(hEnableStateChangedEvent, (WPARAM)proto, (LPARAM)enabled);

	return 0;
}

INT_PTR EnableListeningTo(WPARAM wParam, LPARAM lParam)
{
	return EnableListeningTo((char*)wParam, lParam != 0);
}

INT_PTR HotkeysEnable(WPARAM, LPARAM lParam)
{
	return EnableListeningTo(lParam, true);
}

INT_PTR HotkeysDisable(WPARAM, LPARAM lParam)
{
	return EnableListeningTo(lParam, FALSE);
}

INT_PTR HotkeysToggle(WPARAM, LPARAM lParam)
{
	return EnableListeningTo(lParam, !ListeningToEnabled((char *)lParam, TRUE));
}

INT_PTR GetTextFormat(WPARAM, LPARAM)
{
	if (!loaded)
		return NULL;

	return (INT_PTR)mir_wstrdup(opts.templ);
}

wchar_t *GetParsedFormat(LISTENINGTOINFO *lti)
{
	if (lti == NULL)
		return NULL;

	wchar_t *fr[] = {
		L"artist", UNKNOWN(lti->ptszArtist),
		L"album", UNKNOWN(lti->ptszAlbum),
		L"title", UNKNOWN(lti->ptszTitle),
		L"track", UNKNOWN(lti->ptszTrack),
		L"year", UNKNOWN(lti->ptszYear),
		L"genre", UNKNOWN(lti->ptszGenre),
		L"length", UNKNOWN(lti->ptszLength),
		L"player", UNKNOWN(lti->ptszPlayer),
		L"type", UNKNOWN(lti->ptszType)
	};

	Buffer<wchar_t> ret;
	ReplaceTemplate(&ret, NULL, opts.templ, fr, _countof(fr));
	return ret.detach();
}

INT_PTR GetParsedFormat(WPARAM, LPARAM lParam)
{
	return (INT_PTR)GetParsedFormat((LISTENINGTOINFO *)lParam);
}

INT_PTR GetOverrideContactOption(WPARAM, LPARAM)
{
	return (INT_PTR)opts.override_contact_template;
}


INT_PTR GetUnknownText(WPARAM, LPARAM)
{
	return (INT_PTR)opts.unknown;
}

void SetListeningInfos(LISTENINGTOINFO *lti)
{
	for (unsigned int i = 1; i < proto_items.size(); ++i)
		SetListeningInfo(proto_items[i].proto, lti);

	wchar_t *fr = NULL;
	char *info = NULL;

	if (lti) {
		fr = GetParsedFormat(lti);
		if (fr) info = mir_u2a(fr);
	}

	NotifyEventHooks(hListeningInfoChangedEvent, (WPARAM)info, 0);

	if (lti) {
		if (fr) mir_free(fr);
		if (info) mir_free(info);
	}
}

static void CALLBACK GetInfoTimer(HWND, UINT, UINT_PTR, DWORD)
{
	if (hTimer != NULL) {
		KillTimer(NULL, hTimer);
		hTimer = NULL;
	}

	// Check if we can set it now...
	DWORD now = GetTickCount();
	if (now < lastInfoSetTime + MIN_TIME_BEETWEEN_SETS) {
		hTimer = SetTimer(NULL, NULL, lastInfoSetTime + MIN_TIME_BEETWEEN_SETS - now, GetInfoTimer);
		return;
	}
	lastInfoSetTime = GetTickCount(); // TODO Move this to inside the if that really sets

	if (!opts.enable_sending) {
		SetListeningInfos();
		return;
	}

	// Set it
	int changed = ChangedListeningInfo();
	if (changed < 0)
		SetListeningInfos();
	else if (changed > 0)
		SetListeningInfos(GetListeningInfo());

	StartTimer();
}

void StartTimer()
{
	// See if any protocol want Listening info
	BOOL want = FALSE;

	if (opts.enable_sending) {
		if (!players[WATRACK]->enabled) {
			// See if any player needs it
			BOOL needPoll = FALSE;
			for (int i = FIRST_PLAYER; i < NUM_PLAYERS; i++) {
				if (players[i]->needPoll) {
					needPoll = TRUE;
					break;
				}
			}

			if (needPoll) {
				// Now see protocols
				for (unsigned int i = 1; i < proto_items.size(); ++i) {
					if (ListeningToEnabled(proto_items[i].proto)) {
						want = TRUE;
						break;
					}
				}
			}
		}
	}

	if (want) {
		if (hTimer == NULL)
			hTimer = SetTimer(NULL, NULL, opts.time_to_pool * 1000, GetInfoTimer);
	}
	else {
		if (hTimer != NULL) {
			KillTimer(NULL, hTimer);
			hTimer = NULL;

			// To be sure that no one was left behind
			SetListeningInfos();
		}
	}
}

void HasNewListeningInfo()
{
	if (hTimer != NULL) {
		KillTimer(NULL, hTimer);
		hTimer = NULL;
	}

	hTimer = SetTimer(NULL, NULL, 100, GetInfoTimer);
}


void SetExtraIcon(MCONTACT hContact, BOOL set)
{
	ExtraIcon_SetIconByName(hExtraIcon, hContact, set ? "listening_to_icon" : NULL);
}

int SettingChanged(WPARAM hContact, LPARAM lParam)
{
	if (hContact == NULL)
		return 0;

	DBCONTACTWRITESETTING *cws = (DBCONTACTWRITESETTING*)lParam;
	if (strcmp(cws->szSetting, "ListeningTo") != 0)
		return 0;

	char *proto = GetContactProto(hContact);
	if (proto == NULL || strcmp(cws->szModule, proto) != 0)
		return 0;

	if (cws->value.type == DBVT_DELETED || cws->value.ptszVal == NULL || cws->value.ptszVal[0] == 0)
		SetExtraIcon(hContact, FALSE);
	else
		SetExtraIcon(hContact, TRUE);

	return 0;
}

INT_PTR SetNewSong(WPARAM wParam, LPARAM lParam)
{
	if (lParam == NULL)
		return -1;

	if (lParam == LISTENINGTO_ANSI) {
		CharToWchar data((char *)wParam);
		((GenericPlayer *)players[GENERIC])->NewData(data, mir_wstrlen(data));
	}
	else {
		WCHAR *data = (WCHAR *)wParam;
		((GenericPlayer *)players[GENERIC])->NewData(data, mir_wstrlen(data));
	}

	return 0;
}

wchar_t* VariablesParseInfo(ARGUMENTSINFO *ai)
{
	if (ai->cbSize < sizeof(ARGUMENTSINFO))
		return NULL;

	LISTENINGTOINFO *lti = GetListeningInfo();
	if (lti == NULL) {
		ai->flags = AIF_FALSE;
		return mir_wstrdup(L"");
	}

	wchar_t *fr[] = {
		L"artist", UNKNOWN(lti->ptszArtist),
		L"album", UNKNOWN(lti->ptszAlbum),
		L"title", UNKNOWN(lti->ptszTitle),
		L"track", UNKNOWN(lti->ptszTrack),
		L"year", UNKNOWN(lti->ptszYear),
		L"genre", UNKNOWN(lti->ptszGenre),
		L"length", UNKNOWN(lti->ptszLength),
		L"player", UNKNOWN(lti->ptszPlayer),
		L"type", UNKNOWN(lti->ptszType)
	};

	Buffer<wchar_t> ret;
	ReplaceTemplate(&ret, NULL, opts.templ, fr, _countof(fr));
	return ret.detach();
}

#define VARIABLES_PARSE_BODY(__field__) \
	if (ai == NULL || ai->cbSize < sizeof(ARGUMENTSINFO)) \
		return NULL; \
	\
	LISTENINGTOINFO *lti = GetListeningInfo(); \
	if (lti == NULL) \
			{ \
		ai->flags = AIF_FALSE; \
		return mir_wstrdup(L""); \
			} \
				else if (IsEmpty(lti->__field__))  \
	{ \
		ai->flags = AIF_FALSE; \
		return mir_wstrdup(opts.unknown); \
	} \
				else \
	{ \
		ai->flags = AIF_DONTPARSE; \
		wchar_t *ret = mir_wstrdup(lti->__field__); \
		return ret; \
	}


wchar_t* VariablesParseType(ARGUMENTSINFO *ai)
{
	VARIABLES_PARSE_BODY(ptszType);
}

wchar_t* VariablesParseArtist(ARGUMENTSINFO *ai)
{
	VARIABLES_PARSE_BODY(ptszArtist);
}

wchar_t* VariablesParseAlbum(ARGUMENTSINFO *ai)
{
	VARIABLES_PARSE_BODY(ptszAlbum);
}

wchar_t* VariablesParseTitle(ARGUMENTSINFO *ai)
{
	VARIABLES_PARSE_BODY(ptszTitle);
}

wchar_t* VariablesParseTrack(ARGUMENTSINFO *ai)
{
	VARIABLES_PARSE_BODY(ptszTrack);
}

wchar_t* VariablesParseYear(ARGUMENTSINFO *ai)
{
	VARIABLES_PARSE_BODY(ptszYear);
}

wchar_t* VariablesParseGenre(ARGUMENTSINFO *ai)
{
	VARIABLES_PARSE_BODY(ptszGenre);
}

wchar_t* VariablesParseLength(ARGUMENTSINFO *ai)
{
	VARIABLES_PARSE_BODY(ptszLength);
}

wchar_t* VariablesParsePlayer(ARGUMENTSINFO *ai)
{
	VARIABLES_PARSE_BODY(ptszPlayer);
}
