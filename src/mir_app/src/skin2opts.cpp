/*

Miranda NG: the free IM client for Microsoft* Windows*

Copyright (�) 2012-16 Miranda NG project (http://miranda-ng.org),
Copyright (c) 2000-12 Miranda IM project,
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

#include "stdafx.h"

#include "IcoLib.h"

struct TreeItem
{
	char *paramName;
	DWORD value;
};

struct IcoLibOptsData
{
	HWND hwndIndex;
};

/////////////////////////////////////////////////////////////////////////////////////////

static HICON ExtractIconFromPath(const wchar_t *path, int cxIcon, int cyIcon)
{
	wchar_t *comma;
	wchar_t file[MAX_PATH], fileFull[MAX_PATH];
	int n;
	HICON hIcon;

	if (!path)
		return (HICON)NULL;

	mir_wstrncpy(file, path, _countof(file));
	comma = wcsrchr(file, ',');
	if (!comma)
		n = 0;
	else {
		n = _wtoi(comma + 1);
		*comma = 0;
	}
	PathToAbsoluteW(file, fileFull);
	hIcon = NULL;

	//SHOULD BE REPLACED WITH GOOD ENOUGH FUNCTION
	_ExtractIconEx(fileFull, n, cxIcon, cyIcon, &hIcon, LR_COLOR);
	return hIcon;
}

/////////////////////////////////////////////////////////////////////////////////////////
// IconItem_GetIcon_Preview

HICON IconItem_GetIcon_Preview(IcolibItem* item)
{
	HICON hIcon = NULL;

	if (!item->temp_reset) {
		HICON hRefIcon = IcoLib_GetIconByHandle((HANDLE)item, false);
		hIcon = CopyIcon(hRefIcon);
		if (item->source_small && item->source_small->icon == hRefIcon)
			item->source_small->releaseIcon();
	}
	else {
		if (item->default_icon) {
			HICON hRefIcon = item->default_icon->getIcon();
			if (hRefIcon) {
				hIcon = CopyIcon(hRefIcon);
				if (item->default_icon->icon == hRefIcon)
					item->default_icon->releaseIcon();
			}
		}

		if (!hIcon && item->default_file) {
			item->default_icon->release();
			item->default_icon = GetIconSourceItem(item->default_file->file, item->default_indx, item->cx, item->cy);
			if (item->default_icon) {
				HICON hRefIcon = item->default_icon->getIcon();
				if (hRefIcon) {
					hIcon = CopyIcon(hRefIcon);
					if (item->default_icon->icon == hRefIcon)
						item->default_icon->releaseIcon();
				}
			}
		}

		if (!hIcon)
			return CopyIcon(hIconBlank);
	}
	return hIcon;
}

/////////////////////////////////////////////////////////////////////////////////////////
// IcoLib GUI service routines

static void __fastcall MySetCursor(wchar_t* nCursor)
{
	SetCursor(LoadCursor(NULL, nCursor));
}

static void LoadSectionIcons(wchar_t *filename, SectionItem* sectionActive)
{
	wchar_t path[MAX_PATH];
	mir_snwprintf(path, L"%s,", filename);
	size_t suffIndx = mir_wstrlen(path);

	mir_cslock lck(csIconList);

	for (int indx = 0; indx < iconList.getCount(); indx++) {
		IcolibItem *item = iconList[indx];

		if (item->default_file && item->section == sectionActive) {
			_itow(item->default_indx, path + suffIndx, 10);
			HICON hIcon = ExtractIconFromPath(path, item->cx, item->cy);
			if (!hIcon)
				continue;

			replaceStrW(item->temp_file, NULL);
			SafeDestroyIcon(item->temp_icon);

			item->temp_file = mir_wstrdup(path);
			item->temp_icon = hIcon;
			item->temp_reset = FALSE;
		}
	}
}

void LoadSubIcons(HWND htv, wchar_t *filename, HTREEITEM hItem)
{
	TVITEM tvi;
	tvi.mask = TVIF_HANDLE | TVIF_PARAM;
	tvi.hItem = hItem;
	TreeView_GetItem(htv, &tvi);

	TreeItem *treeItem = (TreeItem *)tvi.lParam;
	SectionItem* sectionActive = sectionList[SECTIONPARAM_INDEX(treeItem->value)];

	tvi.hItem = TreeView_GetChild(htv, tvi.hItem);
	while (tvi.hItem) {
		LoadSubIcons(htv, filename, tvi.hItem);
		tvi.hItem = TreeView_GetNextSibling(htv, tvi.hItem);
	}

	if (SECTIONPARAM_FLAGS(treeItem->value) & SECTIONPARAM_HAVEPAGE)
		LoadSectionIcons(filename, sectionActive);
}

static void UndoChanges(int iconIndx, int cmd)
{
	IcolibItem *item = iconList[iconIndx];

	if (!item->temp_file && !item->temp_icon && item->temp_reset && cmd == ID_CANCELCHANGE)
		item->temp_reset = FALSE;
	else {
		replaceStrW(item->temp_file, NULL);
		SafeDestroyIcon(item->temp_icon);
	}

	if (cmd == ID_RESET)
		item->temp_reset = TRUE;
}

void UndoSubItemChanges(HWND htv, HTREEITEM hItem, int cmd)
{
	TVITEM tvi = { 0 };
	tvi.mask = TVIF_HANDLE | TVIF_PARAM;
	tvi.hItem = hItem;
	TreeView_GetItem(htv, &tvi);

	TreeItem *treeItem = (TreeItem *)tvi.lParam;
	if (SECTIONPARAM_FLAGS(treeItem->value) & SECTIONPARAM_HAVEPAGE) {
		mir_cslock lck(csIconList);

		for (int indx = 0; indx < iconList.getCount(); indx++)
			if (iconList[indx]->section == sectionList[SECTIONPARAM_INDEX(treeItem->value)])
				UndoChanges(indx, cmd);
	}

	tvi.hItem = TreeView_GetChild(htv, tvi.hItem);
	while (tvi.hItem) {
		UndoSubItemChanges(htv, tvi.hItem, cmd);
		tvi.hItem = TreeView_GetNextSibling(htv, tvi.hItem);
	}
}

static void OpenIconsPage()
{
	Utils_OpenUrl("http://miranda-ng.org/");
}

static int OpenPopupMenu(HWND hwndDlg)
{
	HMENU hMenu, hPopup;
	POINT pt;
	int cmd;

	GetCursorPos(&pt);
	hMenu = LoadMenu(g_hInst, MAKEINTRESOURCE(IDR_ICOLIB_CONTEXT));
	hPopup = GetSubMenu(hMenu, 0);
	TranslateMenu(hPopup);
	cmd = TrackPopupMenu(hPopup, TPM_RIGHTBUTTON | TPM_RETURNCMD, pt.x, pt.y, 0, hwndDlg, NULL);
	DestroyMenu(hMenu);
	return cmd;
}

static wchar_t* OpenFileDlg(HWND hParent, const wchar_t* szFile, BOOL bAll)
{
	OPENFILENAME ofn = { 0 };
	wchar_t filter[512], *pfilter, file[MAX_PATH * 2];

	ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;
	ofn.hwndOwner = hParent;

	mir_wstrcpy(filter, TranslateT("Icon sets"));
	if (bAll)
		mir_wstrcat(filter, L" (*.dll;*.mir;*.icl;*.exe;*.ico)");
	else
		mir_wstrcat(filter, L" (*.dll;*.mir)");

	pfilter = filter + mir_wstrlen(filter) + 1;
	if (bAll)
		mir_wstrcpy(pfilter, L"*.DLL;*.MIR;*.ICL;*.EXE;*.ICO");
	else
		mir_wstrcpy(pfilter, L"*.DLL;*.MIR");

	pfilter += mir_wstrlen(pfilter) + 1;
	mir_wstrcpy(pfilter, TranslateT("All files"));
	mir_wstrcat(pfilter, L" (*)");
	pfilter += mir_wstrlen(pfilter) + 1;
	mir_wstrcpy(pfilter, L"*");
	pfilter += mir_wstrlen(pfilter) + 1;
	*pfilter = '\0';

	ofn.lpstrFilter = filter;
	ofn.lpstrDefExt = L"dll";
	mir_wstrncpy(file, szFile, _countof(file));
	ofn.lpstrFile = file;
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_DONTADDTORECENT;
	ofn.nMaxFile = MAX_PATH * 2;

	if (!GetOpenFileName(&ofn))
		return NULL;

	return mir_wstrdup(file);
}

//
//  User interface
//

#define DM_REBUILDICONSPREVIEW  (WM_USER+10)
#define DM_CHANGEICON           (WM_USER+11)
#define DM_CHANGESPECIFICICON   (WM_USER+12)
#define DM_UPDATEICONSPREVIEW   (WM_USER+13)
#define DM_REBUILD_CTREE        (WM_USER+14)

INT_PTR CALLBACK DlgProcIconImport(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);

void DoOptionsChanged(HWND hwndDlg)
{
	SendMessage(hwndDlg, DM_UPDATEICONSPREVIEW, 0, 0);
	SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
}

void DoIconsChanged(HWND hwndDlg)
{
	SendMessage(hwndDlg, DM_UPDATEICONSPREVIEW, 0, 0);

	iconEventActive = 1; // Disable icon destroying - performance boost
	NotifyEventHooks(hIconsChangedEvent, 0, 0);
	NotifyEventHooks(hIcons2ChangedEvent, 0, 0);
	iconEventActive = 0;

	mir_cslock lck(csIconList); // Destroy unused icons
	for (int indx = 0; indx < iconList.getCount(); indx++) {
		IcolibItem *item = iconList[indx];
		if (item->source_small && !item->source_small->icon_ref_count) {
			item->source_small->icon_ref_count++;
			item->source_small->releaseIcon();
		}
		if (item->source_big && !item->source_big->icon_ref_count) {
			item->source_big->icon_ref_count++;
			item->source_big->releaseIcon();
		}
	}
}

static HTREEITEM FindNamedTreeItemAt(HWND hwndTree, HTREEITEM hItem, const wchar_t *name)
{
	TVITEM tvi = { 0 };
	wchar_t str[MAX_PATH];

	if (hItem)
		tvi.hItem = TreeView_GetChild(hwndTree, hItem);
	else
		tvi.hItem = TreeView_GetRoot(hwndTree);

	if (!name)
		return tvi.hItem;

	tvi.mask = TVIF_TEXT;
	tvi.pszText = str;
	tvi.cchTextMax = _countof(str);

	while (tvi.hItem) {
		TreeView_GetItem(hwndTree, &tvi);

		if (!mir_wstrcmp(tvi.pszText, name))
			return tvi.hItem;

		tvi.hItem = TreeView_GetNextSibling(hwndTree, tvi.hItem);
	}
	return NULL;
}

/////////////////////////////////////////////////////////////////////////////////////////
// icon import dialog's window procedure

static int IconDlg_Resize(HWND, LPARAM, UTILRESIZECONTROL *urc)
{
	switch (urc->wId) {
	case IDC_ICONSET:
		return RD_ANCHORX_WIDTH | RD_ANCHORY_TOP;

	case IDC_BROWSE:
		return RD_ANCHORX_RIGHT | RD_ANCHORY_TOP;

	case IDC_PREVIEW:
		return RD_ANCHORX_WIDTH | RD_ANCHORY_HEIGHT;

	case IDC_GETMORE:
		return RD_ANCHORX_CENTRE | RD_ANCHORY_BOTTOM;
	}
	return RD_ANCHORX_LEFT | RD_ANCHORY_TOP; // default
}

INT_PTR CALLBACK DlgProcIconImport(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static HWND hwndParent, hwndDragOver;
	static int dragging;
	static int dragItem, dropHiLite;
	static HWND hPreview = NULL;

	switch (msg) {
	case WM_INITDIALOG:
		TranslateDialogDefault(hwndDlg);
		hwndParent = (HWND)lParam;
		hPreview = GetDlgItem(hwndDlg, IDC_PREVIEW);
		dragging = dragItem = 0;
		ListView_SetImageList(hPreview, ImageList_Create(g_iIconSX, g_iIconSY, ILC_COLOR32 | ILC_MASK, 0, 100), LVSIL_NORMAL);
		ListView_SetIconSpacing(hPreview, 56, 67);
		{
			RECT rcThis, rcParent;
			int cxScreen = GetSystemMetrics(SM_CXSCREEN);

			GetWindowRect(hwndDlg, &rcThis);
			GetWindowRect(hwndParent, &rcParent);
			OffsetRect(&rcThis, rcParent.right - rcThis.left, 0);
			OffsetRect(&rcThis, 0, rcParent.top - rcThis.top);
			GetWindowRect(GetParent(hwndParent), &rcParent);
			if (rcThis.right > cxScreen) {
				OffsetRect(&rcParent, cxScreen - rcThis.right, 0);
				OffsetRect(&rcThis, cxScreen - rcThis.right, 0);
				MoveWindow(GetParent(hwndParent), rcParent.left, rcParent.top, rcParent.right - rcParent.left, rcParent.bottom - rcParent.top, TRUE);
			}
			MoveWindow(hwndDlg, rcThis.left, rcThis.top, rcThis.right - rcThis.left, rcThis.bottom - rcThis.top, FALSE);
			GetClientRect(hwndDlg, &rcThis);
			SendMessage(hwndDlg, WM_SIZE, 0, MAKELPARAM(rcThis.right - rcThis.left, rcThis.bottom - rcThis.top));
		}

		SHAutoComplete(GetDlgItem(hwndDlg, IDC_ICONSET), 1);

		SetDlgItemText(hwndDlg, IDC_ICONSET, L"icons.dll");
		return TRUE;

	case DM_REBUILDICONSPREVIEW:
		{
			MySetCursor(IDC_WAIT);
			ListView_DeleteAllItems(hPreview);
			HIMAGELIST hIml = ListView_GetImageList(hPreview, LVSIL_NORMAL);
			ImageList_RemoveAll(hIml);

			wchar_t filename[MAX_PATH], caption[64];
			GetDlgItemText(hwndDlg, IDC_ICONSET, filename, _countof(filename));
			{
				RECT rcPreview, rcGroup;

				GetWindowRect(hPreview, &rcPreview);
				GetWindowRect(GetDlgItem(hwndDlg, IDC_IMPORTMULTI), &rcGroup);
				//SetWindowPos(hPreview, 0, 0, 0, rcPreview.right-rcPreview.left, rcGroup.bottom-rcPreview.top, SWP_NOZORDER|SWP_NOMOVE);
			}

			if (_waccess(filename, 0) != 0) {
				MySetCursor(IDC_ARROW);
				break;
			}

			LVITEM lvi;
			lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
			lvi.iSubItem = 0;
			lvi.iItem = 0;
			int count = (int)_ExtractIconEx(filename, -1, 16, 16, NULL, LR_DEFAULTCOLOR);
			for (int i = 0; i < count; lvi.iItem++, i++) {
				mir_snwprintf(caption, L"%d", i + 1);
				lvi.pszText = caption;

				HICON hIcon = NULL;
				if (_ExtractIconEx(filename, i, 16, 16, &hIcon, LR_DEFAULTCOLOR) == 1) {
					lvi.iImage = ImageList_AddIcon(hIml, hIcon);
					DestroyIcon(hIcon);
					lvi.lParam = i;
					ListView_InsertItem(hPreview, &lvi);
				}
			}
			MySetCursor(IDC_ARROW);
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_BROWSE:
			{
				wchar_t str[MAX_PATH], *file;
				GetDlgItemText(hwndDlg, IDC_ICONSET, str, _countof(str));
				if (!(file = OpenFileDlg(GetParent(hwndDlg), str, TRUE)))
					break;
				SetDlgItemText(hwndDlg, IDC_ICONSET, file);
				mir_free(file);
			}
			break;

		case IDC_GETMORE:
			OpenIconsPage();
			break;

		case IDC_ICONSET:
			if (HIWORD(wParam) == EN_CHANGE)
				SendMessage(hwndDlg, DM_REBUILDICONSPREVIEW, 0, 0);
			break;
		}
		break;

	case WM_MOUSEMOVE:
		if (dragging) {
			LVHITTESTINFO lvhti;
			int onItem = 0;
			HWND hwndOver;
			RECT rc;
			POINT ptDrag;
			HWND hPPreview = GetDlgItem(hwndParent, IDC_PREVIEW);

			lvhti.pt.x = (short)LOWORD(lParam); lvhti.pt.y = (short)HIWORD(lParam);
			ClientToScreen(hwndDlg, &lvhti.pt);
			hwndOver = WindowFromPoint(lvhti.pt);
			GetWindowRect(hwndOver, &rc);
			ptDrag.x = lvhti.pt.x - rc.left; ptDrag.y = lvhti.pt.y - rc.top;
			if (hwndOver != hwndDragOver) {
				ImageList_DragLeave(hwndDragOver);
				hwndDragOver = hwndOver;
				ImageList_DragEnter(hwndDragOver, ptDrag.x, ptDrag.y);
			}

			ImageList_DragMove(ptDrag.x, ptDrag.y);
			if (hwndOver == hPPreview) {
				ScreenToClient(hPPreview, &lvhti.pt);

				if (ListView_HitTest(hPPreview, &lvhti) != -1) {
					if (lvhti.iItem != dropHiLite) {
						ImageList_DragLeave(hwndDragOver);
						if (dropHiLite != -1)
							ListView_SetItemState(hPPreview, dropHiLite, 0, LVIS_DROPHILITED);
						dropHiLite = lvhti.iItem;
						ListView_SetItemState(hPPreview, dropHiLite, LVIS_DROPHILITED, LVIS_DROPHILITED);
						UpdateWindow(hPPreview);
						ImageList_DragEnter(hwndDragOver, ptDrag.x, ptDrag.y);
					}
					onItem = 1;
				}
			}

			if (!onItem && dropHiLite != -1) {
				ImageList_DragLeave(hwndDragOver);
				ListView_SetItemState(hPPreview, dropHiLite, 0, LVIS_DROPHILITED);
				UpdateWindow(hPPreview);
				ImageList_DragEnter(hwndDragOver, ptDrag.x, ptDrag.y);
				dropHiLite = -1;
			}
			MySetCursor(onItem ? IDC_ARROW : IDC_NO);
		}
		break;

	case WM_LBUTTONUP:
		if (dragging) {
			ReleaseCapture();
			ImageList_EndDrag();
			dragging = 0;
			if (dropHiLite != -1) {
				wchar_t path[MAX_PATH], fullPath[MAX_PATH], filename[MAX_PATH];
				LVITEM lvi;

				GetDlgItemText(hwndDlg, IDC_ICONSET, fullPath, _countof(fullPath));
				PathToRelativeT(fullPath, filename);
				lvi.mask = LVIF_PARAM;
				lvi.iItem = dragItem; lvi.iSubItem = 0;
				ListView_GetItem(hPreview, &lvi);
				mir_snwprintf(path, L"%s,%d", filename, (int)lvi.lParam);
				SendMessage(hwndParent, DM_CHANGEICON, dropHiLite, (LPARAM)path);
				ListView_SetItemState(GetDlgItem(hwndParent, IDC_PREVIEW), dropHiLite, 0, LVIS_DROPHILITED);
			}
		}
		break;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->idFrom) {
		case IDC_PREVIEW:
			switch (((LPNMHDR)lParam)->code) {
			case LVN_BEGINDRAG:
				SetCapture(hwndDlg);
				dragging = 1;
				dragItem = ((LPNMLISTVIEW)lParam)->iItem;
				dropHiLite = -1;
				ImageList_BeginDrag(ListView_GetImageList(hPreview, LVSIL_NORMAL), dragItem, g_iIconX / 2, g_iIconY / 2);
				{
					POINT pt;
					RECT rc;

					GetCursorPos(&pt);
					GetWindowRect(hwndDlg, &rc);
					ImageList_DragEnter(hwndDlg, pt.x - rc.left, pt.y - rc.top);
					hwndDragOver = hwndDlg;
				}
				break;
			}
			break;
		}
		break;

	case WM_SIZE: // make the dlg resizeable
		if (!IsIconic(hwndDlg))
			Utils_ResizeDialog(hwndDlg, g_hInst, MAKEINTRESOURCEA(IDD_ICOLIB_IMPORT), IconDlg_Resize);
		break;

	case WM_CLOSE:
		DestroyWindow(hwndDlg);
		EnableWindow(GetDlgItem(hwndParent, IDC_IMPORT), TRUE);
		break;

	}

	return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////////////
// IcoLib options window procedure

static int CALLBACK DoSortIconsFunc(LPARAM lParam1, LPARAM lParam2, LPARAM)
{
	return mir_wstrcmpi(iconList[lParam1]->getDescr(), iconList[lParam2]->getDescr());
}

static int CALLBACK DoSortIconsFuncByOrder(LPARAM lParam1, LPARAM lParam2, LPARAM)
{
	return iconList[lParam1]->orderID - iconList[lParam2]->orderID;
}

static void SaveCollapseState(HWND hwndTree)
{
	HTREEITEM hti = TreeView_GetRoot(hwndTree);
	while (hti != NULL) {
		TVITEM tvi;
		tvi.mask = TVIF_STATE | TVIF_HANDLE | TVIF_CHILDREN | TVIF_PARAM;
		tvi.hItem = hti;
		tvi.stateMask = (DWORD)-1;
		TreeView_GetItem(hwndTree, &tvi);

		if (tvi.cChildren > 0) {
			TreeItem *treeItem = (TreeItem *)tvi.lParam;
			if (tvi.state & TVIS_EXPANDED)
				db_set_b(NULL, "SkinIconsUI", treeItem->paramName, TVIS_EXPANDED);
			else
				db_set_b(NULL, "SkinIconsUI", treeItem->paramName, 0);
		}

		HTREEITEM ht = TreeView_GetChild(hwndTree, hti);
		if (ht == NULL) {
			ht = TreeView_GetNextSibling(hwndTree, hti);
			while (ht == NULL) {
				hti = TreeView_GetParent(hwndTree, hti);
				if (hti == NULL) break;
				ht = TreeView_GetNextSibling(hwndTree, hti);
			}
		}

		hti = ht;
	}
}

INT_PTR CALLBACK DlgProcIcoLibOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	struct IcoLibOptsData *dat;
	static HTREEITEM prevItem = 0;
	static HWND hPreview = NULL;

	dat = (struct IcoLibOptsData*)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
	switch (msg) {
	case WM_INITDIALOG:
		TranslateDialogDefault(hwndDlg);
		hPreview = GetDlgItem(hwndDlg, IDC_PREVIEW);
		dat = (struct IcoLibOptsData*)mir_alloc(sizeof(struct IcoLibOptsData));
		dat->hwndIndex = NULL;
		SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR)dat);
		//
		//  Reset temporary data & upload sections list
		//
		{
			mir_cslock lck(csIconList);

			for (int indx = 0; indx < iconList.getCount(); indx++) {
				iconList[indx]->temp_file = NULL;
				iconList[indx]->temp_icon = NULL;
				iconList[indx]->temp_reset = FALSE;
			}
			bNeedRebuild = FALSE;
		}

		//
		//  Setup preview listview
		//
		ListView_SetUnicodeFormat(hPreview, TRUE);
		ListView_SetExtendedListViewStyleEx(hPreview, LVS_EX_INFOTIP, LVS_EX_INFOTIP);
		ListView_SetImageList(hPreview, ImageList_Create(g_iIconSX, g_iIconSY, ILC_COLOR32 | ILC_MASK, 0, 30), LVSIL_NORMAL);
		ListView_SetIconSpacing(hPreview, 56, 67);

		SendMessage(hwndDlg, DM_REBUILD_CTREE, 0, 0);
		return TRUE;

	case DM_REBUILD_CTREE:
		{
			HWND hwndTree = GetDlgItem(hwndDlg, IDC_CATEGORYLIST);
			int indx;
			wchar_t itemName[1024];
			HTREEITEM hSection;

			if (!hwndTree) break;

			TreeView_SelectItem(hwndTree, NULL);
			TreeView_DeleteAllItems(hwndTree);

			for (indx = 0; indx < sectionList.getCount(); indx++) {
				wchar_t* sectionName;
				int sectionLevel = 0;

				hSection = NULL;
				mir_wstrcpy(itemName, sectionList[indx]->name);
				sectionName = itemName;

				while (sectionName) {
					// allow multi-level tree
					wchar_t* pItemName = sectionName;
					HTREEITEM hItem;

					if (sectionName = wcschr(sectionName, '/')) {
						// one level deeper
						*sectionName = 0;
					}

					pItemName = TranslateW(pItemName);
					hItem = FindNamedTreeItemAt(hwndTree, hSection, pItemName);
					if (!sectionName || !hItem) {
						if (!hItem) {
							TVINSERTSTRUCT tvis = { 0 };
							TreeItem *treeItem = (TreeItem *)mir_alloc(sizeof(TreeItem));
							treeItem->value = SECTIONPARAM_MAKE(indx, sectionLevel, sectionName ? 0 : SECTIONPARAM_HAVEPAGE);
							treeItem->paramName = mir_u2a(itemName);

							tvis.hParent = hSection;
							tvis.hInsertAfter = TVI_SORT;
							tvis.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_STATE;
							tvis.item.pszText = pItemName;
							tvis.item.lParam = (LPARAM)treeItem;

							tvis.item.state = tvis.item.stateMask = db_get_b(NULL, "SkinIconsUI", treeItem->paramName, TVIS_EXPANDED);
							hItem = TreeView_InsertItem(hwndTree, &tvis);
						}
						else {
							TVITEM tvi = { 0 };
							tvi.hItem = hItem;
							tvi.mask = TVIF_HANDLE | TVIF_PARAM;
							TreeView_GetItem(hwndTree, &tvi);
							TreeItem *treeItem = (TreeItem *)tvi.lParam;
							treeItem->value = SECTIONPARAM_MAKE(indx, sectionLevel, SECTIONPARAM_HAVEPAGE);
						}
					}

					if (sectionName) {
						*sectionName = '/';
						sectionName++;
					}
					sectionLevel++;

					hSection = hItem;
				}
			}

			ShowWindow(hwndTree, SW_SHOW);

			TreeView_SelectItem(hwndTree, FindNamedTreeItemAt(hwndTree, 0, NULL));
		}
		break;

		//  Rebuild preview to new section
	case DM_REBUILDICONSPREVIEW:
		{
			SectionItem* sectionActive = (SectionItem*)lParam;
			EnableWindow(hPreview, sectionActive != NULL);

			ListView_DeleteAllItems(hPreview);
			HIMAGELIST hIml = ListView_GetImageList(hPreview, LVSIL_NORMAL);
			ImageList_RemoveAll(hIml);

			if (sectionActive == NULL)
				break;

			LVITEM lvi = { 0 };
			lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
			{
				mir_cslock lck(csIconList);

				for (int indx = 0; indx < iconList.getCount(); indx++) {
					IcolibItem *item = iconList[indx];
					if (item->section == sectionActive) {
						lvi.pszText = item->getDescr();
						HICON hIcon = item->temp_icon;
						if (!hIcon)
							hIcon = IconItem_GetIcon_Preview(item);
						lvi.iImage = ImageList_AddIcon(hIml, hIcon);
						lvi.lParam = indx;
						ListView_InsertItem(hPreview, &lvi);
						if (hIcon != item->temp_icon)
							SafeDestroyIcon(hIcon);
					}
				}
			}

			if (sectionActive->flags & SIDF_SORTED)
				ListView_SortItems(hPreview, DoSortIconsFunc, 0);
			else
				ListView_SortItems(hPreview, DoSortIconsFuncByOrder, 0);
		}
		break;

	// Refresh preview to new section
	case DM_UPDATEICONSPREVIEW:
		{
			LVITEM lvi = { 0 };
			HICON hIcon;
			int indx, count;
			HIMAGELIST hIml = ListView_GetImageList(hPreview, LVSIL_NORMAL);

			lvi.mask = LVIF_IMAGE | LVIF_PARAM;
			count = ListView_GetItemCount(hPreview);

			for (indx = 0; indx < count; indx++) {
				lvi.iItem = indx;
				ListView_GetItem(hPreview, &lvi);
				{
					mir_cslock lck(csIconList);
					hIcon = iconList[lvi.lParam]->temp_icon;
					if (!hIcon)
						hIcon = IconItem_GetIcon_Preview(iconList[lvi.lParam]);
				}

				if (hIcon)
					ImageList_ReplaceIcon(hIml, lvi.iImage, hIcon);
				if (hIcon != iconList[lvi.lParam]->temp_icon)
					SafeDestroyIcon(hIcon);
			}
			ListView_RedrawItems(hPreview, 0, count);
		}
		break;

	// Temporary change icon - only inside options dialog
	case DM_CHANGEICON:
		{
			LVITEM lvi = { 0 };
			lvi.mask = LVIF_PARAM;
			lvi.iItem = wParam;
			ListView_GetItem(hPreview, &lvi);
			{
				mir_cslock lck(csIconList);

				IcolibItem *item = iconList[lvi.lParam];
				SafeDestroyIcon(item->temp_icon);

				wchar_t *path = (wchar_t*)lParam;
				replaceStrW(item->temp_file, path);
				item->temp_icon = (HICON)ExtractIconFromPath(path, item->cx, item->cy);
				item->temp_reset = FALSE;
			}
			DoOptionsChanged(hwndDlg);
		}
		break;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDC_IMPORT) {
			dat->hwndIndex = CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_ICOLIB_IMPORT), GetParent(hwndDlg), DlgProcIconImport, (LPARAM)hwndDlg);
			EnableWindow((HWND)lParam, FALSE);
		}
		else if (LOWORD(wParam) == IDC_GETMORE) {
			OpenIconsPage();
			break;
		}
		else if (LOWORD(wParam) == IDC_LOADICONS) {
			wchar_t filetmp[1] = { 0 };
			wchar_t *file;

			if (file = OpenFileDlg(hwndDlg, filetmp, FALSE)) {
				HWND htv = GetDlgItem(hwndDlg, IDC_CATEGORYLIST);
				wchar_t filename[MAX_PATH];

				PathToRelativeT(file, filename);
				mir_free(file);

				MySetCursor(IDC_WAIT);
				LoadSubIcons(htv, filename, TreeView_GetSelection(htv));
				MySetCursor(IDC_ARROW);

				DoOptionsChanged(hwndDlg);
			}
		}
		break;

	case WM_CONTEXTMENU:
		if ((HWND)wParam == hPreview) {
			UINT count = ListView_GetSelectedCount(hPreview);

			if (count > 0) {
				int cmd = OpenPopupMenu(hwndDlg);
				switch (cmd) {
				case ID_CANCELCHANGE:
				case ID_RESET:
					{
						LVITEM lvi = { 0 };
						int itemIndx = -1;

						while ((itemIndx = ListView_GetNextItem(hPreview, itemIndx, LVNI_SELECTED)) != -1) {
							lvi.mask = LVIF_PARAM;
							lvi.iItem = itemIndx; //lvhti.iItem;
							ListView_GetItem(hPreview, &lvi);

							UndoChanges(lvi.lParam, cmd);
						}

						DoOptionsChanged(hwndDlg);
						break;
					}
				}
			}
		}
		else {
			HWND htv = GetDlgItem(hwndDlg, IDC_CATEGORYLIST);
			if ((HWND)wParam == htv) {
				int cmd = OpenPopupMenu(hwndDlg);

				switch (cmd) {
				case ID_CANCELCHANGE:
				case ID_RESET:
					UndoSubItemChanges(htv, TreeView_GetSelection(htv), cmd);
					DoOptionsChanged(hwndDlg);
					break;
				}
			}
		}
		break;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->idFrom) {
		case 0:
			switch (((LPNMHDR)lParam)->code) {
			case PSN_APPLY:
				{
					mir_cslock lck(csIconList);

					for (int indx = 0; indx < iconList.getCount(); indx++) {
						IcolibItem *item = iconList[indx];
						if (item->temp_reset) {
							db_unset(NULL, "SkinIcons", item->name);
							if (item->source_small != item->default_icon) {
								item->source_small->release();
								item->source_small = NULL;
							}
						}
						else if (item->temp_file) {
							db_set_ws(NULL, "SkinIcons", item->name, item->temp_file);
							item->source_small->release();
							item->source_small = NULL;
							SafeDestroyIcon(item->temp_icon);
						}
					}
				}

				DoIconsChanged(hwndDlg);
				return TRUE;
			}
			break;

		case IDC_PREVIEW:
			if (((LPNMHDR)lParam)->code == LVN_GETINFOTIP) {
				NMLVGETINFOTIP *pInfoTip = (NMLVGETINFOTIP *)lParam;
				LVITEM lvi;
				lvi.mask = LVIF_PARAM;
				lvi.iItem = pInfoTip->iItem;
				ListView_GetItem(pInfoTip->hdr.hwndFrom, &lvi);

				if (lvi.lParam < iconList.getCount()) {
					IcolibItem *item = iconList[lvi.lParam];
					if (item->temp_file)
						wcsncpy_s(pInfoTip->pszText, pInfoTip->cchTextMax, item->temp_file, _TRUNCATE);
					else if (item->default_file)
						mir_snwprintf(pInfoTip->pszText, pInfoTip->cchTextMax, L"%s, %d", item->default_file->file, item->default_indx);
				}
			}
			if (bNeedRebuild) {
				bNeedRebuild = FALSE;
				SendMessage(hwndDlg, DM_REBUILD_CTREE, 0, 0);
			}
			break;

		case IDC_CATEGORYLIST:
			switch (((NMHDR*)lParam)->code) {
			case TVN_SELCHANGED:
				{
					NMTREEVIEW *pnmtv = (NMTREEVIEW*)lParam;
					TVITEM tvi = pnmtv->itemNew;
					TreeItem *treeItem = (TreeItem *)tvi.lParam;
					if (treeItem)
						SendMessage(hwndDlg, DM_REBUILDICONSPREVIEW, 0, (LPARAM)(
						(SECTIONPARAM_FLAGS(treeItem->value)&SECTIONPARAM_HAVEPAGE) ?
						sectionList[SECTIONPARAM_INDEX(treeItem->value)] : NULL));
				}
				break;

			case TVN_DELETEITEM:
				TreeItem *treeItem = (TreeItem *)(((LPNMTREEVIEW)lParam)->itemOld.lParam);
				if (treeItem) {
					mir_free(treeItem->paramName);
					mir_free(treeItem);
				}
				break;
			}

			if (bNeedRebuild) {
				bNeedRebuild = FALSE;
				SendMessage(hwndDlg, DM_REBUILD_CTREE, 0, 0);
			}
		}
		break;

	case WM_DESTROY:
		SaveCollapseState(GetDlgItem(hwndDlg, IDC_CATEGORYLIST));
		DestroyWindow(dat->hwndIndex);
		{
			mir_cslock lck(csIconList);
			for (int indx = 0; indx < iconList.getCount(); indx++) {
				IcolibItem *item = iconList[indx];

				replaceStrW(item->temp_file, NULL);
				SafeDestroyIcon(item->temp_icon);
			}
		}

		mir_free(dat);
		break;
	}

	return FALSE;
}

int SkinOptionsInit(WPARAM wParam, LPARAM)
{
	OPTIONSDIALOGPAGE odp = { 0 };
	odp.hInstance = g_hInst;
	odp.flags = ODPF_BOLDGROUPS;
	odp.position = -180000000;
	odp.pszTemplate = MAKEINTRESOURCEA(IDD_OPT_ICOLIB);
	odp.pszTitle = LPGEN("Icons");
	odp.pfnDlgProc = DlgProcIcoLibOpts;
	Options_AddPage(wParam, &odp);
	return 0;
}
