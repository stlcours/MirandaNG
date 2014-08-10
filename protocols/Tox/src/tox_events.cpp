#include "common.h"

int CToxProto::OnModulesLoaded(WPARAM, LPARAM)
{
	HookEventObj(ME_OPT_INITIALISE, OnOptionsInit, this);

	return 0;
}

INT_PTR CToxProto::OnAccountManagerInit(WPARAM, LPARAM lParam)
{
	return (INT_PTR)CreateDialogParam(
		g_hInstance,
		MAKEINTRESOURCE(IDD_ACCMGR),
		(HWND)lParam,
		&CToxProto::MainOptionsProc,
		(LPARAM)this);
}

int CToxProto::OnOptionsInit(void *obj, WPARAM wParam, LPARAM lParam)
{
	CToxProto *proto = (CToxProto*)obj;

	char *title = mir_t2a(proto->m_tszUserName);

	OPTIONSDIALOGPAGE odp = { sizeof(odp) };
	odp.hInstance = g_hInstance;
	odp.pszTitle = title;
	odp.dwInitParam = LPARAM(obj);
	odp.flags = ODPF_BOLDGROUPS;
	odp.pszGroup = LPGEN("Network");

	odp.pszTab = LPGEN("Account");
	odp.pszTemplate = MAKEINTRESOURCEA(IDD_OPT_MAIN);
	odp.pfnDlgProc = MainOptionsProc;
	Options_AddPage(wParam, &odp);

	mir_free(title);

	return 0;
}

void CToxProto::OnFriendRequest(Tox *tox, const uint8_t *userId, const uint8_t *message, const uint16_t messageSize, void *arg)
{
}

void CToxProto::OnFriendMessage(Tox *tox, const int friendId, const uint8_t *message, const uint16_t messageSize, void *arg)
{
}

void CToxProto::OnFriendNameChange(Tox *tox, const int friendId, const uint8_t *name, const uint16_t nameSize, void *arg)
{
}

void CToxProto::OnStatusMessageChanged(Tox *tox, const int friendId, const uint8_t* message, const uint16_t messageSize, void *arg)
{
}

void CToxProto::OnUserStatusChanged(Tox *tox, int32_t friendnumber, uint8_t TOX_USERSTATUS, void *userdata)
{
}

void CToxProto::OnConnectionStatusChanged(Tox *tox, const int friendId, const uint8_t status, void *arg)
{
}

void CToxProto::OnAction(Tox *tox, const int friendId, const uint8_t *message, const uint16_t messageSize, void *arg)
{
}