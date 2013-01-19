/*
Miranda SmileyAdd Plugin
Copyright (C) 2005 - 2011 Boris Krasnovskiy All Rights Reserved
Copyright (C) 2003 - 2004 Rein-Peter de Boer

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation version 2
of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "smileys.h"
#include "customsmiley.h"
#include "services.h"
#include "options.h"
#include "download.h"
#include "imagecache.h"
#include "version.h"
#include "m_metacontacts.h"

//globals
HINSTANCE g_hInst;
HANDLE hEvent1, hContactMenuItem;
extern LIST<void> menuHandleArray;

char* metaProtoName;

int hLangpack;

static const PLUGININFOEX pluginInfoEx =
{
	sizeof(PLUGININFOEX),
	"SmileyAdd",
	__VERSION_DWORD,
	"Smiley support for Miranda NG.",
	"Peacow, nightwish, bid, borkra",
	"borkra@miranda-im.org",
	"Copyright� 2004 - 2012 Boris Krasnovskiy, portions by Rein-Peter de Boer",
	"http://miranda-ng.org/",
	UNICODE_AWARE,
	// {BD542BB4-5AE4-4d0e-A435-BA8DBE39607F}
	{ 0xbd542bb4, 0x5ae4, 0x4d0e, { 0xa4, 0x35, 0xba, 0x8d, 0xbe, 0x39, 0x60, 0x7f } }
};

extern "C" __declspec(dllexport) PLUGININFOEX* MirandaPluginInfoEx(DWORD /* mirandaVersion */)
{
	return (PLUGININFOEX*)&pluginInfoEx;
}

// MirandaInterfaces - returns the protocol interface to the core
extern "C" __declspec(dllexport) const MUUID MirandaInterfaces[] = {MIID_SMILEY, MIID_LAST};

/////////////////////////////////////////////////////////////////////////////////////////

static IconItem icon = { LPGEN("Button Smiley"), "SmileyAdd_ButtonSmiley", IDI_SMILINGICON };

static int ModulesLoaded(WPARAM, LPARAM)
{
	Icon_Register(g_hInst, "SmileyAdd", &icon, 1);

	INT_PTR temp = CallService(MS_MC_GETPROTOCOLNAME, 0, 0);
	metaProtoName = mir_strdup(temp == CALLSERVICE_NOTFOUND ? NULL : (char*)temp);

	CLISTMENUITEM mi = { sizeof(mi) };
	mi.flags = CMIF_ROOTPOPUP | CMIF_ICONFROMICOLIB;
	mi.popupPosition = 2000070050;
	mi.position = 2000070050;
	mi.icolibItem = icon.hIcolib;
	mi.pszPopupName = (char*)-1;
	mi.pszName = "Assign Smiley Category";
	hContactMenuItem = Menu_AddContactMenuItem(&mi);

	DownloadInit();

	//install hooks if enabled
	InstallDialogBoxHook();

	g_SmileyCategories.AddAllProtocolsAsCategory();
	g_SmileyCategories.ClearAndLoadAll();

	return 0;
}

static int MirandaShutdown(WPARAM, LPARAM)
{
	CloseSmileys();
	return 0;
}

extern "C" __declspec(dllexport) int Load(void)
{
	mir_getLP(&pluginInfoEx);

	if (ServiceExists(MS_SMILEYADD_REPLACESMILEYS)) {
		static const TCHAR errmsg[] = L"Only one instance of SmileyAdd could be executed.\n"
			L"Remove duplicate instances from 'Plugins' directory";
		ReportError(TranslateTS(errmsg));

		return 1;
	}

	InitImageCache();

	g_SmileyCategories.SetSmileyPackStore(&g_SmileyPacks);

	opt.Load();

	// create smiley events
	hEvent1 = CreateHookableEvent(ME_SMILEYADD_OPTIONSCHANGED);

	HookEvent(ME_SYSTEM_MODULESLOADED, ModulesLoaded);
	HookEvent(ME_SYSTEM_PRESHUTDOWN, MirandaShutdown);
	HookEvent(ME_OPT_INITIALISE, SmileysOptionsInitialize);
	HookEvent(ME_CLIST_PREBUILDCONTACTMENU, RebuildContactMenu);
	HookEvent(ME_SMILEYADD_OPTIONSCHANGED, UpdateSrmmDlg);
	HookEvent(ME_PROTO_ACCLISTCHANGED, AccountListChanged);
	HookEvent(ME_DB_CONTACT_SETTINGCHANGED, DbSettingChanged);

	//create the smiley services
	CreateServiceFunction(MS_SMILEYADD_REPLACESMILEYS, ReplaceSmileysCommand);
	CreateServiceFunction(MS_SMILEYADD_GETSMILEYICON, GetSmileyIconCommand);
	CreateServiceFunction(MS_SMILEYADD_SHOWSELECTION, ShowSmileySelectionCommand);
	CreateServiceFunction(MS_SMILEYADD_GETINFO, GetInfoCommand);
	CreateServiceFunction(MS_SMILEYADD_GETINFO2, GetInfoCommand2);
	CreateServiceFunction(MS_SMILEYADD_PARSE, ParseText);
	CreateServiceFunction(MS_SMILEYADD_REGISTERCATEGORY, RegisterPack);
	CreateServiceFunction(MS_SMILEYADD_BATCHPARSE, ParseTextBatch);
	CreateServiceFunction(MS_SMILEYADD_BATCHFREE, FreeTextBatch);
	CreateServiceFunction(MS_SMILEYADD_CUSTOMCATMENU, CustomCatMenu);
	CreateServiceFunction(MS_SMILEYADD_RELOAD, ReloadPack);
	CreateServiceFunction(MS_SMILEYADD_LOADCONTACTSMILEYS, LoadContactSmileys);
	CreateServiceFunction(MS_SMILEYADD_PARSEW, ParseTextW);
	return 0;
}

extern "C" __declspec(dllexport) int Unload(void)
{
	RemoveDialogBoxHook();

	DestroyHookableEvent(hEvent1);

	RichEditData_Destroy();
	DestroyAniSmileys();
	DestroySmileyBase();

	g_SmileyCategories.ClearAll();
	g_SmileyPackCStore.ClearAndFreeAll();

	DestroyImageCache();
	DestroyGdiPlus();

	DownloadClose();
	menuHandleArray.destroy();

	mir_free(metaProtoName);
	return 0;
}

extern "C" BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID /*lpvReserved*/)
{
	switch(fdwReason) {
	case DLL_PROCESS_ATTACH:
		g_hInst = hinstDLL;
		DisableThreadLibraryCalls(hinstDLL);
		break;

	case DLL_PROCESS_DETACH:
		break;
	}

	return TRUE;
}
