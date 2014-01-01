/*

Copyright 2000-12 Miranda IM, 2012-14 Miranda NG project,
all portions of this codebase are copyrighted to the people
listed in contributors.txt.

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

#include "commonheaders.h"
#include "statusicon.h"

HANDLE hHookIconPressedEvt;

static int OnSrmmIconChanged(WPARAM wParam, LPARAM)
{
	HANDLE hContact = (HANDLE)wParam;
	if (hContact == NULL)
		WindowList_Broadcast(g_dat.hMessageWindowList, DM_STATUSICONCHANGE, 0, 0);
	else {
		HWND hwnd = WindowList_Find(g_dat.hMessageWindowList, hContact);
		if (hwnd != NULL)
			PostMessage(hwnd, DM_STATUSICONCHANGE, 0, 0);
	}
	return 0;
}

void DrawStatusIcons(HANDLE hContact, HDC hDC, RECT r, int gap)
{
	HICON hIcon;
	int x = r.left;

	int nIcon = 0;
	StatusIconData *sid;
	while ((sid = Srmm_GetNthIcon(hContact, nIcon++)) != 0 && x < r.right) {
		if ((sid->flags & MBF_DISABLED) && sid->hIconDisabled) hIcon = sid->hIconDisabled;
		else hIcon = sid->hIcon;

		SetBkMode(hDC, TRANSPARENT);
		DrawIconEx(hDC, x, (r.top + r.bottom - GetSystemMetrics(SM_CYSMICON)) >> 1, hIcon, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0, NULL, DI_NORMAL);

		x += GetSystemMetrics(SM_CYSMICON) + gap;
	}
}

void CheckIconClick(HANDLE hContact, HWND hwndFrom, POINT pt, RECT r, int gap, int click_flags)
{
	int iconNum = (pt.x - r.left) / (GetSystemMetrics(SM_CXSMICON) + gap);
	StatusIconData *sid = Srmm_GetNthIcon(hContact, iconNum);
	if (sid == NULL)
		return;

	StatusIconClickData sicd = { sizeof(sicd) };
	ClientToScreen(hwndFrom, &pt);
	sicd.clickLocation = pt;
	sicd.dwId = sid->dwId;
	sicd.szModule = sid->szModule;
	sicd.flags = click_flags;

	NotifyEventHooks(hHookIconPressedEvt, (WPARAM)hContact, (LPARAM)&sicd);
}

HANDLE hServiceIcon[3];

int InitStatusIcons()
{
	HookEvent(ME_MSG_ICONSCHANGED, OnSrmmIconChanged);

	hHookIconPressedEvt = CreateHookableEvent(ME_MSG_ICONPRESSED);
	return 0;
}

int DeinitStatusIcons()
{
	DestroyHookableEvent(hHookIconPressedEvt);
	return 0;
}

int GetStatusIconsCount(HANDLE hContact)
{
	int nIcon = 0;
	while ( Srmm_GetNthIcon(hContact, nIcon) != NULL)
		nIcon++;
	return nIcon;
}
