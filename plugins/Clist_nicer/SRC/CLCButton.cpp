/*
Miranda IM
Copyright (C) 2002 Robert Rainwater

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

#include <m_button_int.h>
#include <m_toptoolbar.h>

struct
{
	int   ctrlid;
	char *pszButtonID, *pszButtonDn, *pszButtonName;
	int    isPush, isVis, isAction;
	HANDLE hButton;
}
static BTNS[] = 
{
	{ IDC_TBTOPMENU,     "CLN_topmenu",      NULL,     LPGEN("Show menu"),                     1, 1, 1 },
	{ IDC_TBHIDEOFFLINE, "CLN_online",       NULL,     LPGEN("Show / hide offline contacts"),  0, 1, 0 },
	{ IDC_TBHIDEGROUPS,  "CLN_groups",       NULL,     LPGEN("Toggle group mode"),             0, 1, 0 },
	{ IDC_TBFINDANDADD,  "CLN_findadd",      NULL,     LPGEN("Find and add contacts"),         1, 1, 0 },
	{ IDC_TBACCOUNTS,    "CLN_accounts",     NULL,     LPGEN("Accounts"),                      1, 1, 0 },
	{ IDC_TBOPTIONS,     "CLN_options",      NULL,     LPGEN("Open preferences"),              1, 1, 0 },
	{ IDC_TBSOUND,       "CLN_sound", "CLN_soundsoff", LPGEN("Toggle sounds"),                 0, 1, 0 },
	{ IDC_TBMINIMIZE,    "CLN_minimize",     NULL,     LPGEN("Minimize contact list"),         1, 0, 0 },
	{ IDC_TBTOPSTATUS,   "CLN_topstatus",    NULL,     LPGEN("Status menu"),                   1, 0, 1 },
	{ IDC_TABSRMMSLIST,  "CLN_slist",        NULL,     LPGEN("tabSRMM session list"),          1, 0, 1 },
	{ IDC_TABSRMMMENU,   "CLN_menu",         NULL,     LPGEN("tabSRMM Menu"),                  1, 0, 1 },

	{ IDC_TBSELECTVIEWMODE,   "CLN_CLVM_select",  NULL, LPGEN("Select view mode"),             1, 0, 1 },
	{ IDC_TBCONFIGUREVIEWMODE,"CLN_CLVM_options", NULL, LPGEN("Setup view modes"),             1, 0, 0 },
	{ IDC_TBCLEARVIEWMODE,    "CLN_CLVM_reset",   NULL, LPGEN("Clear view mode"),              1, 0, 0 }
};

static int g_index = -1;

static int InitDefaultButtons(WPARAM, LPARAM)
{
	TTBButton tbb = { 0 };
	tbb.cbSize = sizeof(tbb);

	for (int i=0; i < SIZEOF(BTNS); i++ ) {
		g_index = i;
		if (BTNS[i].pszButtonID) {
			tbb.pszTooltipUp = tbb.name = LPGEN(BTNS[i].pszButtonName);
			tbb.pszService = BTNS[i].pszButtonID;
			tbb.hIconHandleUp = Skin_GetIconHandle(BTNS[i].pszButtonID);
			if (BTNS[i].pszButtonDn)
				tbb.hIconHandleUp = Skin_GetIconHandle(BTNS[i].pszButtonDn);
		}
		else tbb.dwFlags |= TTBBF_ISSEPARATOR;

		tbb.dwFlags |= (BTNS[i].isVis ? TTBBF_VISIBLE : 0 );
		BTNS[i].hButton = TopToolbar_AddButton(&tbb);
	}
	g_index = -1;
	return 1;
}

/////////////////////////////////////////////////////////////////////////////////////////

struct MButtonExtension : public MButtonCtrl
{
	HICON hIconPrivate;
	TCHAR szText[128];
	SIZE sLabel;
	HIMAGELIST hIml;
	int iIcon, iCtrlID;
	BOOL bSendOnDown;
	ButtonItem *buttonItem;
	LONG lastGlyphMetrics[4];
};

// Used for our own cheap TrackMouseEvent
#define BUTTON_POLLID       100
#define BUTTON_POLLDELAY    50

static int TBStateConvert2Flat(int state)
{
	switch (state) {
	case PBS_NORMAL:
		return TS_NORMAL;
	case PBS_HOT:
		return TS_HOT;
	case PBS_PRESSED:
		return TS_PRESSED;
	case PBS_DISABLED:
		return TS_DISABLED;
	case PBS_DEFAULTED:
		return TS_NORMAL;
	}
	return TS_NORMAL;
}

static void PaintWorker(MButtonExtension *ctl, HDC hdcPaint)
{
	if (hdcPaint) {
		HDC hdcMem;
		HBITMAP hbmMem;
		HBITMAP hbmOld = 0;
		RECT rcClient;
		HFONT hOldFont = 0;
		int xOffset = 0;

		GetClientRect(ctl->hwnd, &rcClient);
		hdcMem = CreateCompatibleDC(hdcPaint);
		hbmMem = CreateCompatibleBitmap(hdcPaint, rcClient.right - rcClient.left, rcClient.bottom - rcClient.top);
		hbmOld = reinterpret_cast<HBITMAP>(SelectObject(hdcMem, hbmMem));

		hOldFont = reinterpret_cast<HFONT>(SelectObject(hdcMem, ctl->hFont));
		// If its a push button, check to see if it should stay pressed
		if (ctl->bIsPushBtn && ctl->bIsPushed)
			ctl->stateId = PBS_PRESSED;

		// Draw the flat button
		if (ctl->bIsFlat) {
			if (ctl->hThemeToolbar && ctl->bIsThemed) {
				RECT rc = rcClient;
				int state = IsWindowEnabled(ctl->hwnd) ? (ctl->stateId == PBS_NORMAL && ctl->bIsDefault ? PBS_DEFAULTED : ctl->stateId) : PBS_DISABLED;
				SkinDrawBg(ctl->hwnd, hdcMem);
				if (API::pfnIsThemeBackgroundPartiallyTransparent(ctl->hThemeToolbar, TP_BUTTON, TBStateConvert2Flat(state))) {
					API::pfnDrawThemeParentBackground(ctl->hwnd, hdcMem, &rc);
				}
				API::pfnDrawThemeBackground(ctl->hThemeToolbar, hdcMem, TP_BUTTON, TBStateConvert2Flat(state), &rc, &rc);
			} else {
				HBRUSH hbr;
				RECT rc = rcClient;

				if(ctl->buttonItem) {
					RECT rcParent;
					POINT pt;
					HWND hwndParent = pcli->hwndContactList;
					ImageItem *imgItem = ctl->stateId == PBS_HOT ? ctl->buttonItem->imgHover : (ctl->stateId == PBS_PRESSED ? ctl->buttonItem->imgPressed : ctl->buttonItem->imgNormal);
					LONG *glyphMetrics = ctl->stateId == PBS_HOT ? ctl->buttonItem->hoverGlyphMetrics : (ctl->stateId == PBS_PRESSED ? ctl->buttonItem->pressedGlyphMetrics : ctl->buttonItem->normalGlyphMetrics);

					GetWindowRect(ctl->hwnd, &rcParent);
					pt.x = rcParent.left;
					pt.y = rcParent.top;

					ScreenToClient(pcli->hwndContactList, &pt);

					BitBlt(hdcMem, 0, 0, rc.right, rc.bottom, cfg::dat.hdcBg, pt.x, pt.y, SRCCOPY);
					if(imgItem)
						DrawAlpha(hdcMem, &rc, 0, 0, 0, 0, 0, 0, 0, imgItem);
					if(g_glyphItem) {
						API::pfnAlphaBlend(hdcMem, (rc.right - glyphMetrics[2]) / 2, (rc.bottom - glyphMetrics[3]) / 2,
							glyphMetrics[2], glyphMetrics[3], g_glyphItem->hdc,
							glyphMetrics[0], glyphMetrics[1], glyphMetrics[2],
							glyphMetrics[3], g_glyphItem->bf);
					}
				}
				else if(ctl->bIsSkinned) {      // skinned
					RECT rcParent;
					POINT pt;
					HWND hwndParent = pcli->hwndContactList;
					StatusItems_t *item;
					int item_id;

					GetWindowRect(ctl->hwnd, &rcParent);
					pt.x = rcParent.left;
					pt.y = rcParent.top;

					ScreenToClient(pcli->hwndContactList, &pt);

					if(HIWORD(ctl->bIsSkinned))
						item_id = ctl->stateId == PBS_HOT ? ID_EXTBKTBBUTTONMOUSEOVER : (ctl->stateId == PBS_PRESSED ? ID_EXTBKTBBUTTONSPRESSED : ID_EXTBKTBBUTTONSNPRESSED);
					else
						item_id = ctl->stateId == PBS_HOT ? ID_EXTBKBUTTONSMOUSEOVER : (ctl->stateId == PBS_PRESSED ? ID_EXTBKBUTTONSPRESSED : ID_EXTBKBUTTONSNPRESSED);
					item = &StatusItems[item_id - ID_STATUS_OFFLINE];

					SetTextColor(hdcMem, item->TEXTCOLOR);
					if(item->IGNORED) {
						if(pt.y < 10 || cfg::dat.bWallpaperMode)
							BitBlt(hdcMem, 0, 0, rc.right, rc.bottom, cfg::dat.hdcBg, pt.x, pt.y, SRCCOPY);
						else
							FillRect(hdcMem, &rc, GetSysColorBrush(COLOR_3DFACE));
					}
					else {
						if(pt.y < 10 || cfg::dat.bWallpaperMode)
							BitBlt(hdcMem, 0, 0, rc.right, rc.bottom, cfg::dat.hdcBg, pt.x, pt.y, SRCCOPY);
						else
							FillRect(hdcMem, &rc, GetSysColorBrush(COLOR_3DFACE));
						rc.top += item->MARGIN_TOP; rc.bottom -= item->MARGIN_BOTTOM;
						rc.left += item->MARGIN_LEFT; rc.right -= item->MARGIN_RIGHT;
						DrawAlpha(hdcMem, &rc, item->COLOR, item->ALPHA, item->COLOR2, item->COLOR2_TRANSPARENT, item->GRADIENT,
							item->CORNER, item->BORDERSTYLE, item->imageItem);
					}
				}
				else {
					if (ctl->stateId == PBS_PRESSED || ctl->stateId == PBS_HOT)
						hbr = GetSysColorBrush(COLOR_3DFACE);
					else {
						HDC dc;
						HWND hwndParent;

						hwndParent = GetParent(ctl->hwnd);
						dc = GetDC(hwndParent);
						hbr = (HBRUSH) SendMessage(hwndParent, WM_CTLCOLORDLG, (WPARAM) dc, (LPARAM) hwndParent);
						ReleaseDC(hwndParent, dc);
					}
					if (hbr) {
						FillRect(hdcMem, &rc, hbr);
						DeleteObject(hbr);
					}
				}
				if(!ctl->bIsSkinned && ctl->buttonItem == 0) {
					if (ctl->stateId == PBS_HOT || ctl->focus) {
						if (ctl->bIsPushed)
							DrawEdge(hdcMem, &rc, EDGE_ETCHED, BF_RECT | BF_SOFT);
						else
							DrawEdge(hdcMem, &rc, BDR_RAISEDOUTER, BF_RECT | BF_SOFT);
					} else if (ctl->stateId == PBS_PRESSED)
						DrawEdge(hdcMem, &rc, BDR_SUNKENOUTER, BF_RECT | BF_SOFT);
				}
			}
		} else {
			// Draw background/border
			if (ctl->hThemeButton && ctl->bIsThemed) {
				int state = IsWindowEnabled(ctl->hwnd) ? (ctl->stateId == PBS_NORMAL && ctl->bIsDefault ? PBS_DEFAULTED : ctl->stateId) : PBS_DISABLED;
				POINT pt;
				RECT rcParent;

				GetWindowRect(ctl->hwnd, &rcParent);
				pt.x = rcParent.left;
				pt.y = rcParent.top;
				ScreenToClient(pcli->hwndContactList, &pt);
				BitBlt(hdcMem, 0, 0, rcClient.right, rcClient.bottom, cfg::dat.hdcBg, pt.x, pt.y, SRCCOPY);

				if (API::pfnIsThemeBackgroundPartiallyTransparent(ctl->hThemeButton, BP_PUSHBUTTON, state)) {
					API::pfnDrawThemeParentBackground(ctl->hwnd, hdcMem, &rcClient);
				}
				API::pfnDrawThemeBackground(ctl->hThemeButton, hdcMem, BP_PUSHBUTTON, state, &rcClient, &rcClient);
			} else {
				UINT uState = DFCS_BUTTONPUSH | ((ctl->stateId == PBS_HOT) ? DFCS_HOT : 0) | ((ctl->stateId == PBS_PRESSED) ? DFCS_PUSHED : 0);
				if (ctl->bIsDefault && ctl->stateId == PBS_NORMAL)
					uState |= DLGC_DEFPUSHBUTTON;
				DrawFrameControl(hdcMem, &rcClient, DFC_BUTTON, uState);
			}

			// Draw focus rectangle if button has focus
			if (ctl->focus) {
				RECT focusRect = rcClient;
				InflateRect(&focusRect, -3, -3);
				DrawFocusRect(hdcMem, &focusRect);
			}
		}

		// If we have an icon or a bitmap, ignore text and only draw the image on the button
		if (ctl->hIcon || ctl->hIconPrivate || ctl->iIcon) {
			int ix = (rcClient.right - rcClient.left) / 2 - (g_cxsmIcon / 2);
			int iy = (rcClient.bottom - rcClient.top) / 2 - (g_cxsmIcon / 2);
			HICON hIconNew = ctl->hIconPrivate != 0 ? ctl->hIconPrivate : ctl->hIcon;
			if (lstrlen(ctl->szText) == 0) {
				if (ctl->iIcon)
					ImageList_DrawEx(ctl->hIml, ctl->iIcon, hdcMem, ix, iy, g_cxsmIcon, g_cysmIcon, CLR_NONE, CLR_NONE, ILD_NORMAL);
				else
					DrawState(hdcMem, NULL, NULL, (LPARAM) hIconNew, 0, ix, iy, g_cxsmIcon, g_cysmIcon, IsWindowEnabled(ctl->hwnd) ? DST_ICON | DSS_NORMAL : DST_ICON | DSS_DISABLED);
				ctl->sLabel.cx = ctl->sLabel.cy = 0;
			} else {
				GetTextExtentPoint32(hdcMem, ctl->szText, lstrlen(ctl->szText), &ctl->sLabel);

				if(g_cxsmIcon + ctl->sLabel.cx + 8 > rcClient.right - rcClient.left)
					ctl->sLabel.cx = (rcClient.right - rcClient.left) - g_cxsmIcon - 8;
				else
					ctl->sLabel.cx += 4;

				ix = (rcClient.right - rcClient.left) / 2 - ((g_cxsmIcon + ctl->sLabel.cx) / 2);
				if (ctl->iIcon)
					ImageList_DrawEx(ctl->hIml, ctl->iIcon, hdcMem, ix, iy, g_cxsmIcon, g_cysmIcon, CLR_NONE, CLR_NONE, ILD_NORMAL);
				else
					DrawState(hdcMem, NULL, NULL, (LPARAM) hIconNew, 0, ix, iy, g_cxsmIcon, g_cysmIcon, IsWindowEnabled(ctl->hwnd) ? DST_ICON | DSS_NORMAL : DST_ICON | DSS_DISABLED);
				xOffset = ix + g_cxsmIcon + 4;
			}
		} else if (ctl->hBitmap) {
			BITMAP bminfo;
			int ix, iy;

			GetObject(ctl->hBitmap, sizeof(bminfo), &bminfo);
			ix = (rcClient.right - rcClient.left) / 2 - (bminfo.bmWidth / 2);
			iy = (rcClient.bottom - rcClient.top) / 2 - (bminfo.bmHeight / 2);
			if (ctl->stateId == PBS_PRESSED) {
				ix++;
				iy++;
			}
			DrawState(hdcMem, NULL, NULL, (LPARAM) ctl->hBitmap, 0, ix, iy, bminfo.bmWidth, bminfo.bmHeight, IsWindowEnabled(ctl->hwnd) ? DST_BITMAP : DST_BITMAP | DSS_DISABLED);
		}
		if (GetWindowTextLength(ctl->hwnd)) {
			// Draw the text and optinally the arrow
			RECT rcText;

			CopyRect(&rcText, &rcClient);
			SetBkMode(hdcMem, TRANSPARENT);
			// XP w/themes doesn't used the glossy disabled text.  Is it always using COLOR_GRAYTEXT?  Seems so.
			if(!ctl->bIsSkinned)
				SetTextColor(hdcMem, IsWindowEnabled(ctl->hwnd) || !ctl->hThemeButton ? GetSysColor(COLOR_BTNTEXT) : GetSysColor(COLOR_GRAYTEXT));
			if (ctl->arrow)
				DrawState(hdcMem, NULL, NULL, (LPARAM) ctl->arrow, 0, rcClient.right - rcClient.left - 5 - g_cxsmIcon + (!ctl->hThemeButton && ctl->stateId == PBS_PRESSED ? 1 : 0), (rcClient.bottom - rcClient.top) / 2 - g_cysmIcon / 2 + (!ctl->hThemeButton && ctl->stateId == PBS_PRESSED ? 1 : 0), g_cxsmIcon, g_cysmIcon, IsWindowEnabled(ctl->hwnd) ? DST_ICON : DST_ICON | DSS_DISABLED);
			DrawState(hdcMem, NULL, NULL, (LPARAM) ctl->szText, 0, xOffset + (!ctl->hThemeButton && ctl->stateId == PBS_PRESSED ? 1 : 0), ctl->hThemeButton ? (rcText.bottom - rcText.top - ctl->sLabel.cy) / 2 + 1 : (rcText.bottom - rcText.top - ctl->sLabel.cy) / 2 + (ctl->stateId == PBS_PRESSED ? 1 : 0), ctl->sLabel.cx, ctl->sLabel.cy, IsWindowEnabled(ctl->hwnd) || ctl->hThemeButton ? DST_PREFIXTEXT | DSS_NORMAL : DST_PREFIXTEXT | DSS_DISABLED);
		}
		if (hOldFont)
			SelectObject(hdcMem, hOldFont);
		BitBlt(hdcPaint, 0, 0, rcClient.right - rcClient.left, rcClient.bottom - rcClient.top, hdcMem, 0, 0, SRCCOPY);
		SelectObject(hdcMem, hbmOld);
		DeleteObject(hbmMem);
		DeleteDC(hdcMem);
		DeleteObject(hbmOld);
	}
}

static LRESULT CALLBACK TSButtonWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	MButtonExtension *bct = (MButtonExtension*) GetWindowLongPtr(hwnd, 0);

	switch (msg) {
	case WM_DESTROY:
		if (bct && bct->hIconPrivate)
			DestroyIcon(bct->hIconPrivate);
		break;

	case WM_SETTEXT:
		lstrcpyn(bct->szText, (TCHAR *)lParam, 127);
		bct->szText[127] = 0;
		break;

	case WM_SYSKEYUP:
		if (bct->stateId != PBS_DISABLED && bct->cHot && bct->cHot == tolower((int) wParam)) {
			if (!bct->bSendOnDown)
				SendMessage(pcli->hwndContactList, WM_COMMAND, MAKELONG(bct->iCtrlID, BN_CLICKED), (LPARAM) hwnd);
			bct->lResult = 0;
			return 1;
		}
		break;

	case BM_GETIMAGE:
		if (wParam == IMAGE_ICON) {
			bct->lResult = (LRESULT)(bct->hIconPrivate ? bct->hIconPrivate : bct->hIcon);
			return 1;
		}
		break;

	case BM_SETIMAGE:
		if(!lParam)
			break;

		bct->hIml = 0;
		bct->iIcon = 0;
		if (wParam == IMAGE_ICON) {
			ICONINFO ii = {0};
			BITMAP bm = {0};

			if (bct->hIconPrivate) {
				DestroyIcon(bct->hIconPrivate);
				bct->hIconPrivate = 0;
			}

			GetIconInfo((HICON) lParam, &ii);
			GetObject(ii.hbmColor, sizeof(bm), &bm);
			if (bm.bmWidth > g_cxsmIcon || bm.bmHeight > g_cysmIcon) {
				HIMAGELIST hImageList;
				hImageList = ImageList_Create(g_cxsmIcon, g_cysmIcon, IsWinVerXPPlus() ? ILC_COLOR32 | ILC_MASK : ILC_COLOR16 | ILC_MASK, 1, 0);
				ImageList_AddIcon(hImageList, (HICON) lParam);
				bct->hIconPrivate = ImageList_GetIcon(hImageList, 0, ILD_NORMAL);
				ImageList_RemoveAll(hImageList);
				ImageList_Destroy(hImageList);
				bct->hIcon = 0;
			} else {
				bct->hIcon = (HICON) lParam;
				bct->hIconPrivate = 0;
			}

			DeleteObject(ii.hbmMask);
			DeleteObject(ii.hbmColor);
			bct->hBitmap = NULL;
			InvalidateRect(bct->hwnd, NULL, TRUE);
		}
		else if (wParam == IMAGE_BITMAP) {
			bct->hBitmap = (HBITMAP) lParam;
			if (bct->hIconPrivate)
				DestroyIcon(bct->hIconPrivate);
			bct->hIcon = bct->hIconPrivate = NULL;
			InvalidateRect(bct->hwnd, NULL, TRUE);
		}
		return 1;

	case BUTTONSETIMLICON:
		if (bct->hIconPrivate)
			DestroyIcon(bct->hIconPrivate);
		bct->hIml = (HIMAGELIST) wParam;
		bct->iIcon = (int) lParam;
		bct->hIcon = bct->hIconPrivate = 0;
		InvalidateRect(bct->hwnd, NULL, TRUE);
		break;

	case BUTTONSETSKINNED:
		bct->bIsSkinned = lParam != 0;
		bct->bIsThemed = bct->bIsSkinned ? FALSE : bct->bIsThemed;
		InvalidateRect(bct->hwnd, NULL, TRUE);
		break;

	case BUTTONSETBTNITEM:
		bct->buttonItem = (ButtonItem *)lParam;
		break;

	case BUTTONSETASMENUACTION:
		bct->bSendOnDown = wParam ? TRUE : FALSE;
		break;

	case WM_LBUTTONDOWN:
		if (bct->stateId != PBS_DISABLED && bct->stateId != PBS_PRESSED) {
			bct->stateId = PBS_PRESSED;
			InvalidateRect(bct->hwnd, NULL, TRUE);
			if (bct->bSendOnDown) {
				SendMessage( GetParent(hwnd), WM_COMMAND, MAKELONG(bct->iCtrlID, BN_CLICKED), (LPARAM) hwnd);
				bct->stateId = PBS_NORMAL;
				InvalidateRect(bct->hwnd, NULL, TRUE);
			}
		}
		return 1;

	case WM_LBUTTONUP:
		if (bct->bIsPushBtn)
			bct->bIsPushed = !bct->bIsPushed;

		if (bct->stateId != PBS_DISABLED) {
			// don't change states if disabled
			bct->stateId = PBS_HOT;
			InvalidateRect(bct->hwnd, NULL, TRUE);
		}
		if ( !bct->bSendOnDown)
			SendMessage( GetParent(hwnd), WM_COMMAND, MAKELONG(bct->iCtrlID, BN_CLICKED), (LPARAM) hwnd);
		return 1;

	case WM_NCHITTEST:
		switch( SendMessage(pcli->hwndContactList, WM_NCHITTEST, wParam, lParam)) {
		case HTLEFT:	case HTRIGHT:	case HTBOTTOM:	  case HTTOP:
		case HTTOPLEFT: case HTTOPRIGHT: case HTBOTTOMLEFT:  case HTBOTTOMRIGHT:
			bct->lResult = HTTRANSPARENT;
			return 1;
		}
	}
	return 0;
}

static void SetButtonAsCustom(HWND hWnd)
{
	MButtonCustomize Custom;
	Custom.cbLen = sizeof(MButtonExtension);
	Custom.fnPainter = (pfnPainterFunc)PaintWorker;
	Custom.fnWindowProc = TSButtonWndProc;
	SendMessage(hWnd, BUTTONSETCUSTOM, 0, (LPARAM)&Custom);
}

static LRESULT CALLBACK ToolbarWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg == WM_COMMAND && HIWORD(wParam) == BN_CLICKED) {
		SendMessage(pcli->hwndContactList, msg, wParam, lParam);
		return 1;
	}
	return 0;
}

static void CustomizeToolbar(HANDLE hButton, HWND hWnd, LPARAM)
{
	// we don't customize the toolbar window, only buttons
	if (hButton == TTB_WINDOW_HANDLE) {
		TTBCtrlCustomize custData = { sizeof(TTBCtrl), ToolbarWndProc };
		SendMessage(hWnd, TTB_SETCUSTOM, 0, (LPARAM)&custData);
		return;
	}
	
	SetButtonAsCustom(hWnd);

	MButtonExtension *bct = (MButtonExtension*) GetWindowLongPtr(hWnd, 0);
	if (g_index != -1) { // adding built-in button
		bct->iCtrlID = BTNS[g_index].ctrlid;
		if (BTNS[g_index].isAction) 
			bct->bSendOnDown = TRUE;
		if (!BTNS[g_index].isPush)
			bct->bIsPushBtn = TRUE;
	}
}

void CustomizeButton(HWND hWnd, bool bIsSkinned, bool bIsThemed, bool bIsFlat)
{
	SetButtonAsCustom(hWnd);

	MButtonExtension *bct = (MButtonExtension*) GetWindowLongPtr(hWnd, 0);
	if (bct)
		bct->iCtrlID = GetDlgCtrlID(hWnd);

	SendMessage(hWnd, BUTTONSETSKINNED, bIsSkinned, 0);
	SendMessage(hWnd, BUTTONSETASTHEMEDBTN, bIsThemed, 0);
	SendMessage(hWnd, BUTTONSETASFLATBTN, bIsFlat, 0);
}

static int Nicer_CustomizeToolbar(WPARAM, LPARAM)
{	
	HookEvent(ME_TTB_INITBUTTONS, InitDefaultButtons);
	TopToolbar_SetCustomProc(CustomizeToolbar, 0);
	return 0;
}

void LoadButtonModule()
{
	HookEvent(ME_SYSTEM_MODULESLOADED, Nicer_CustomizeToolbar);
}
