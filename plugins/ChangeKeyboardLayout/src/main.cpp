#include "commonheaders.h"

int hLangpack;
LPTSTR ptszLayStrings[20];
HANDLE hChangeLayout, hGetLayoutOfText, hChangeTextLayout;
HICON hPopupIcon, hCopyIcon;
HKL hklLayouts[20];
BYTE bLayNum;
HINSTANCE hInst;
HHOOK kbHook_All;
MainOptions moOptions;
PopupOptions poOptions, poOptionsTemp;
HANDLE hIcoLibIconsChanged;

PLUGININFOEX pluginInfoEx = {
	sizeof(PLUGININFOEX),
	__PLUGIN_NAME,
	PLUGIN_MAKE_VERSION(__MAJOR_VERSION, __MINOR_VERSION, __RELEASE_NUM, __BUILD_NUM),
	__DESCRIPTION,
	__AUTHOR,
	__AUTHOREMAIL,
	__COPYRIGHT,
	__AUTHORWEB,
	UNICODE_AWARE,
	// {c5ef53a8-80d4-4ce9-b341-ec90d3ec9156}
	{0xc5ef53a8, 0x80d4, 0x4ce9, {0xb3, 0x41, 0xec, 0x90, 0xd3, 0xec, 0x91, 0x56}}
};

LPCTSTR ptszKeybEng = _T("`1234567890- = \\qwertyuiop[]asdfghjkl;'zxcvbnm,./~!@#$%^&*()_+|QWERTYUIOP{}ASDFGHJKL:\"ZXCVBNM<>?");
HKL hklEng = (HKL)0x04090409;

LPCTSTR ptszSeparators = _T(" \t\n\r");

HANDLE hOptionsInitialize;
HANDLE hModulesLoaded;

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	hInst = hinstDLL;
	return TRUE;
}

extern "C" __declspec(dllexport) PLUGININFOEX* MirandaPluginInfoEx(DWORD mirandaVersion)
{
	return &pluginInfoEx;
}

extern "C" __declspec(dllexport) int Load(void)
{	
	mir_getLP(&pluginInfoEx);
	ZeroMemory(hklLayouts, 20 * sizeof(HKL));
	bLayNum = GetKeyboardLayoutList(20,hklLayouts);
	if (bLayNum<2) 
		return 1;
	hOptionsInitialize = HookEvent(ME_OPT_INITIALISE, OnOptionsInitialise);
	hModulesLoaded = HookEvent(ME_SYSTEM_MODULESLOADED,ModulesLoaded);
	return 0;
}

extern "C" __declspec(dllexport) int Unload(void)
{
	DWORD i;

	for (i = 0;i<bLayNum;i++)	
		mir_free(ptszLayStrings[i]);

	UnhookEvent(hOptionsInitialize);
	UnhookEvent(hIcoLibIconsChanged);
	UnhookEvent(hModulesLoaded);
	DestroyServiceFunction(hChangeLayout);
	DestroyServiceFunction(hGetLayoutOfText);
	DestroyServiceFunction(hChangeTextLayout);
	UnhookWindowsHookEx(kbHook_All);
	return 0;
}