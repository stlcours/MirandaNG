#include "stdafx.h"

HANDLE hExtraIcon = NULL;

int ExtraIconsApply(WPARAM hContact, LPARAM)
{
	if (hContact == NULL) return 0;

	if (HasUnread(hContact))
		ExtraIcon_SetIcon(hExtraIcon, hContact, Icons[ICON_EXTRA].hIcolib);
	else
		ExtraIcon_Clear(hExtraIcon, hContact);

	return 0;
}

void InitClistExtraIcon()
{
	hExtraIcon = ExtraIcon_RegisterIcolib("messagestate_unread", LPGEN("MessageState unread extra icon"), "clist_unread_icon");
	HookEvent(ME_CLIST_EXTRA_IMAGE_APPLY, ExtraIconsApply);
}