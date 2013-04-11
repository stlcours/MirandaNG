#include "commonheaders.h"

int __cdecl onWindowEvent(WPARAM, LPARAM lParam)
{
	MessageWindowEventData *mwd = (MessageWindowEventData *)lParam;
	if (mwd->uType == MSG_WINDOW_EVT_OPEN || mwd->uType == MSG_WINDOW_EVT_OPENING)
		ShowStatusIcon(mwd->hContact);

	return 0;
}

int __cdecl onIconPressed(WPARAM wParam, LPARAM lParam)
{
	HANDLE hContact = (HANDLE)wParam;
	if (isProtoMetaContacts(hContact))
		hContact = getMostOnline(hContact); // ������� ���, ����� ������� ������ ���������

	StatusIconClickData *sicd = (StatusIconClickData *)lParam;
	if (strcmp(sicd->szModule, MODULENAME) != 0 || !isSecureProtocol(hContact))
		return 0; // not our event

	if (!isContactPGP(hContact) && !isContactGPG(hContact) && !isChatRoom(hContact)) {
		if (isContactSecured(hContact) & SECURED)
			Service_DisableIM(wParam,0);
		else
			Service_CreateIM(wParam,0);
	}

	return 0;
}

void InitSRMMIcons()
{
	// add icon to srmm status icons
	StatusIconData sid = { sizeof(sid) };
	sid.szModule = MODULENAME;
	sid.flags = MBF_DISABLED|MBF_HIDDEN;

	// Native
	sid.dwId = MODE_NATIVE;
	sid.hIcon = mode2icon(MODE_NATIVE|SECURED,3);
	sid.hIconDisabled = mode2icon(MODE_NATIVE,3);
	sid.szTooltip = Translate("SecureIM [Native]");
	Srmm_AddIcon(&sid);

	// PGP
	sid.dwId = MODE_PGP;
	sid.hIcon = mode2icon(MODE_PGP|SECURED,3);
	sid.hIconDisabled = mode2icon(MODE_PGP,3);
	sid.szTooltip = Translate("SecureIM [PGP]");
	Srmm_AddIcon(&sid);
	// GPG
	sid.dwId = MODE_GPG;
	sid.hIcon = mode2icon(MODE_GPG|SECURED,3);
	sid.hIconDisabled = mode2icon(MODE_GPG,3);
	sid.szTooltip = Translate("SecureIM [GPG]");
	Srmm_AddIcon(&sid);
	// RSAAES
	sid.dwId = MODE_RSAAES;
	sid.hIcon = mode2icon(MODE_RSAAES|SECURED,3);
	sid.hIconDisabled = mode2icon(MODE_RSAAES,3);
	sid.szTooltip = Translate("SecureIM [RSA/AES]");
	Srmm_AddIcon(&sid);

	// hook the window events so that we can can change the status of the icon
	HookEvent(ME_MSG_WINDOWEVENT, onWindowEvent);
	HookEvent(ME_MSG_ICONPRESSED, onIconPressed);
}

// EOF
