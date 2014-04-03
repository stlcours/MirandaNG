#include "common.h"

int CSteamProto::OnModulesLoaded(WPARAM, LPARAM)
{
	TCHAR name[128];
	mir_sntprintf(name, SIZEOF(name), TranslateT("%s connection"), m_tszUserName);

	NETLIBUSER nlu = { sizeof(nlu) };
	nlu.flags = NUF_INCOMING | NUF_OUTGOING | NUF_HTTPCONNS | NUF_TCHAR;
	nlu.ptszDescriptiveName = name;
	nlu.szSettingsModule = m_szModuleName;
	m_hNetlibUser = (HANDLE)CallService(MS_NETLIB_REGISTERUSER, 0, (LPARAM)&nlu);

	return 0;
}

int CSteamProto::OnPreShutdown(WPARAM, LPARAM)
{
	//SetStatus(ID_STATUS_OFFLINE);

	Netlib_CloseHandle(this->m_hNetlibUser);
	this->m_hNetlibUser = NULL;

	return 0;
}

INT_PTR __cdecl CSteamProto::OnAccountManagerInit(WPARAM wParam, LPARAM lParam)
{
	return (int)CreateDialogParam(
		g_hInstance,
		MAKEINTRESOURCE(IDD_ACCMGR),
		(HWND)lParam,
		CSteamProto::MainOptionsProc,
		(LPARAM)this);
}

int __cdecl CSteamProto::OnOptionsInit(WPARAM wParam, LPARAM lParam)
{
	char *title = mir_t2a(this->m_tszUserName);

	OPTIONSDIALOGPAGE odp = { sizeof(odp) };
	odp.hInstance = g_hInstance;
	odp.pszTitle = title;
	odp.dwInitParam = LPARAM(this);
	odp.flags = ODPF_BOLDGROUPS;
	odp.pszGroup = LPGEN("Network");

	odp.pszTab = LPGEN("Account");
	odp.pszTemplate = MAKEINTRESOURCEA(IDD_OPT_MAIN);
	odp.pfnDlgProc = MainOptionsProc;
	Options_AddPage(wParam, &odp);

	mir_free(title);

	return 0;
}