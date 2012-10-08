#include <windows.h>
#include <newpluginapi.h>
#include <m_clist.h>
#include <m_skin.h>
#include <m_icolib.h>
#include <m_langpack.h>
#include <m_system.h>
#include <m_genmenu.h>
#include "resource.h"

HINSTANCE hInst;
HANDLE hIconHandle;
int hLangpack;

PLUGININFOEX pluginInfo={
	sizeof(PLUGININFOEX),
	"Restart plugin",
	PLUGIN_MAKE_VERSION(0,0,6,0),
	"Adds Restart menu item.",
	"(..������ �������..)",
	"ep@eugn.me",
	"� 2008 - 2012 -=J-Scar=-",
	"http://miranda-ng.org",
	UNICODE_AWARE,
	0x61bedf3a, 0xcc2, 0x41a3, { 0xb9, 0x80, 0xbb, 0x93, 0x93, 0x36, 0x89, 0x35 } // {61BEDF3A-0CC2-41a3-B980-BB9393368935}
};

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	hInst = hinstDLL;
	return TRUE;
}

extern "C" __declspec(dllexport) PLUGININFOEX* MirandaPluginInfoEx(DWORD mirandaVersion)
{
	return &pluginInfo;
}

extern "C" __declspec(dllexport) int Load(void)
{
	mir_getLP( &pluginInfo );

	TCHAR szFile[MAX_PATH];
	GetModuleFileName(hInst, szFile, MAX_PATH);

	// IcoLib support
	SKINICONDESC sid = { sizeof(sid) };
	sid.ptszDefaultFile = szFile;
	sid.flags = SIDF_ALL_TCHAR;
	sid.ptszSection = _T("Restart Plugin");
	sid.ptszDescription = _T("Restart");
	sid.pszName = "rst_restart_icon";
	sid.iDefaultIndex = -IDI_RESTARTICON;
	hIconHandle = Skin_AddIcon(&sid);

	CLISTMENUITEM mi = { sizeof(mi) };
	mi.position = -0x7FFFFFFF;
	mi.flags = CMIF_ICONFROMICOLIB | CMIF_TCHAR;
	mi.icolibItem = hIconHandle;
	mi.ptszName = _T("Restart");
	mi.pszService = MS_SYSTEM_RESTART;
	Menu_AddMainMenuItem(&mi);
	Menu_AddTrayMenuItem(&mi);
	return 0;
}

extern "C" __declspec(dllexport) int Unload(void)
{
	return 0;
}
