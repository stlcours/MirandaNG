/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2009 Miranda ICQ/IM project, 
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

#include "..\..\core\commonheaders.h"

#define LBN_MY_CHECK	0x1000
#define LBN_MY_RENAME	0x1001

#define WM_MY_REFRESH	(WM_USER+0x1000)
#define WM_MY_RENAME	(WM_USER+0x1001)

INT_PTR  Proto_EnumProtocols(WPARAM, LPARAM);
bool CheckProtocolOrder(void);

#define errMsg \
"WARNING! The account is going to be deleted. It means that all its \
settings, contacts and histories will be also erased.\n\n\
Are you absolutely sure?"

#define upgradeMsg \
"Your account was successfully upgraded. \
To activate it, restart of Miranda is needed.\n\n\
If you want to restart Miranda now, press Yes, if you want to upgrade another account, press No"

#define legacyMsg \
"This account uses legacy protocol plugin. \
Use Miranda NG options dialogs to change it's preferences."

#define welcomeMsg \
"Welcome to Miranda NG's account manager!\n \
Here you can set up your IM accounts.\n\n \
Select an account from the list on the left to see the available options. \
Alternatively, just click on the Plus sign underneath the list to set up a new IM account."

static HWND hAccMgr = NULL;

extern HANDLE hAccListChanged;

int UnloadPlugin(TCHAR* buf, int bufLen);

///////////////////////////////////////////////////////////////////////////////////////////////////
// Account edit form
// Gets PROTOACCOUNT* as a parameter, or NULL to edit a new one

typedef struct
{
	int action;
	PROTOACCOUNT* pa;
}
	AccFormDlgParam;

static INT_PTR CALLBACK AccFormDlgProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message) {
	case WM_INITDIALOG:
		TranslateDialogDefault(hwndDlg);
		{
			PROTOCOLDESCRIPTOR** proto;
			int protoCount, i, cnt = 0;
			Proto_EnumProtocols((WPARAM)&protoCount, (LPARAM)&proto);
			for (i=0; i < protoCount; i++) {
				PROTOCOLDESCRIPTOR* pd = proto[i];
				if (pd->type == PROTOTYPE_PROTOCOL && pd->cbSize == sizeof(*pd)) {
					SendDlgItemMessageA(hwndDlg, IDC_PROTOTYPECOMBO, CB_ADDSTRING, 0, (LPARAM)proto[i]->szName);
					++cnt;
				}
			}
			SendDlgItemMessage(hwndDlg, IDC_PROTOTYPECOMBO, CB_SETCURSEL, 0, 0);
			EnableWindow( GetDlgItem(hwndDlg, IDOK), cnt != 0);

			SetWindowLongPtr(hwndDlg, GWLP_USERDATA, lParam);
			AccFormDlgParam* param = (AccFormDlgParam*)lParam;

			if (param->action == PRAC_ADDED) // new account
				SetWindowText(hwndDlg, TranslateT("Create new account"));
			else {
				TCHAR str[200];
				if (param->action == PRAC_CHANGED) { // update
					EnableWindow( GetDlgItem(hwndDlg, IDC_PROTOTYPECOMBO), FALSE);
					mir_sntprintf(str, SIZEOF(str), _T("%s: %s"), TranslateT("Editing account"), param->pa->tszAccountName);
				}
				else mir_sntprintf(str, SIZEOF(str), _T("%s: %s"), TranslateT("Upgrading account"), param->pa->tszAccountName);

				SetWindowText(hwndDlg, str);
				SetDlgItemText(hwndDlg, IDC_ACCNAME, param->pa->tszAccountName);
				SetDlgItemTextA(hwndDlg, IDC_ACCINTERNALNAME, param->pa->szModuleName);
				SendDlgItemMessageA(hwndDlg, IDC_PROTOTYPECOMBO, CB_SELECTSTRING, -1, (LPARAM)param->pa->szProtoName);

				EnableWindow( GetDlgItem(hwndDlg, IDC_ACCINTERNALNAME), FALSE);
			}
			SendDlgItemMessage(hwndDlg, IDC_ACCINTERNALNAME, EM_LIMITTEXT, 40, 0);
		}
		return TRUE;

	case WM_COMMAND:
		switch(LOWORD(wParam)) {
		case IDOK:
			{
				AccFormDlgParam* param = (AccFormDlgParam*)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
				PROTOACCOUNT* pa = param->pa;

				if (param->action == PRAC_ADDED) {
					char buf[200];
					GetDlgItemTextA(hwndDlg, IDC_ACCINTERNALNAME, buf, SIZEOF(buf));
					rtrim(buf);
					if (buf[0]) {
						for (int i=0; i < accounts.getCount(); ++i)
							if (_stricmp(buf, accounts[i]->szModuleName) == 0)
								return FALSE;
				}	}

				switch(param->action) {
				case PRAC_UPGRADED:
					{
						int idx;
						BOOL oldProto = pa->bOldProto;
						TCHAR szPlugin[MAX_PATH];
						mir_sntprintf(szPlugin, SIZEOF(szPlugin), _T("%s.dll"), StrConvT(pa->szProtoName));
						idx = accounts.getIndex(pa);
						UnloadAccount(pa, false, false);
						accounts.remove(idx);
						if (oldProto && UnloadPlugin(szPlugin, SIZEOF(szPlugin))) 
						{
							TCHAR szNewName[MAX_PATH];
							mir_sntprintf(szNewName, SIZEOF(szNewName), _T("%s~"), szPlugin);
							MoveFile(szPlugin, szNewName);
						}
					}
					// fall through

				case PRAC_ADDED:
					pa = (PROTOACCOUNT*)mir_calloc(sizeof(PROTOACCOUNT));
					pa->cbSize = sizeof(PROTOACCOUNT);
					pa->bIsEnabled = TRUE;
                    pa->bIsVisible = TRUE;
                    
					pa->iOrder = accounts.getCount();
					break;
				}
				{
					TCHAR buf[256];
					GetDlgItemText(hwndDlg, IDC_ACCNAME, buf, SIZEOF(buf));
					mir_free(pa->tszAccountName);
					pa->tszAccountName = mir_tstrdup(buf);
				}
				if (param->action == PRAC_ADDED || param->action == PRAC_UPGRADED) 
                {
					char buf[200];
					GetDlgItemTextA(hwndDlg, IDC_PROTOTYPECOMBO, buf, SIZEOF(buf));
					pa->szProtoName = mir_strdup(buf);
					GetDlgItemTextA(hwndDlg, IDC_ACCINTERNALNAME, buf, SIZEOF(buf));
					rtrim(buf);
					if (buf[0] == 0) {
						int count = 1;
						for (;;) {
							DBVARIANT dbv;
							mir_snprintf(buf, SIZEOF(buf), "%s_%d", pa->szProtoName, count++);
							if (DBGetContactSettingString(NULL, buf, "AM_BaseProto", &dbv))
								break;
							DBFreeVariant(&dbv);
					}	}
					pa->szModuleName = mir_strdup(buf);

					if ( !pa->tszAccountName[0]) {
						mir_free(pa->tszAccountName);
						pa->tszAccountName = mir_a2t(buf);
					}

					DBWriteContactSettingString(NULL, pa->szModuleName, "AM_BaseProto", pa->szProtoName);
					accounts.insert(pa);

					if (ActivateAccount(pa)) {
						pa->ppro->OnEvent(EV_PROTO_ONLOAD, 0, 0);
						if ( !db_get_b(NULL, "CList", "MoveProtoMenus", TRUE))
							pa->ppro->OnEvent(EV_PROTO_ONMENU, 0, 0);
					}
					else pa->bDynDisabled = TRUE;
				}

				WriteDbAccounts();
				NotifyEventHooks(hAccListChanged, param->action, (LPARAM)pa);

				SendMessage(GetParent(hwndDlg), WM_MY_REFRESH, 0, 0);
			}

			EndDialog(hwndDlg, TRUE);
			break;

		case IDCANCEL:
			EndDialog(hwndDlg, FALSE);
			break;
		}
	}

	return FALSE;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Accounts manager

struct TAccMgrData
{
	HFONT hfntTitle, hfntText;
	int titleHeight, textHeight;
	int selectedHeight, normalHeight;
	int iSelected;
};

struct TAccListData
{
	WNDPROC oldWndProc;
	int iItem;
	RECT rcCheck;

	HWND hwndEdit;
	WNDPROC oldEditProc;
};

static void sttClickButton(HWND hwndDlg, int idcButton)
{
	if (IsWindowEnabled( GetDlgItem(hwndDlg, idcButton)))
		PostMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(idcButton, BN_CLICKED), (LPARAM)GetDlgItem(hwndDlg, idcButton));
}

static LRESULT CALLBACK sttEditSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_KEYDOWN:
		switch (wParam) {
		case VK_RETURN:
			DestroyWindow(hwnd);
			return 0;

		case VK_ESCAPE:
			SetWindowLongPtr(hwnd, GWLP_WNDPROC, GetWindowLongPtr(hwnd, GWLP_USERDATA));
			DestroyWindow(hwnd);
			return 0;
		}
		break;

	case WM_GETDLGCODE:
		if (wParam == VK_RETURN || wParam == VK_ESCAPE)
			return DLGC_WANTMESSAGE;
		break;

	case WM_KILLFOCUS:
		{
			int length = GetWindowTextLength(hwnd) + 1;
			TCHAR *str = (TCHAR*)mir_alloc(sizeof(TCHAR) * length);
			GetWindowText(hwnd, str, length);
			SendMessage(GetParent(GetParent(hwnd)), WM_COMMAND, MAKEWPARAM(GetWindowLongPtr(GetParent(hwnd), GWL_ID), LBN_MY_RENAME), (LPARAM)str);
		}
		DestroyWindow(hwnd);
		return 0;
	}
	return CallWindowProc((WNDPROC)GetWindowLongPtr(hwnd, GWLP_USERDATA), hwnd, msg, wParam, lParam);
}

static LRESULT CALLBACK AccListWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	struct TAccListData *dat = (struct TAccListData *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	if ( !dat)
		return DefWindowProc(hwnd, msg, wParam, lParam);

	switch (msg) {
	case WM_LBUTTONDOWN:
		{
			POINT pt = {LOWORD(lParam), HIWORD(lParam)};
			int iItem = LOWORD(SendMessage(hwnd, LB_ITEMFROMPOINT, 0, lParam));
			ListBox_GetItemRect(hwnd, iItem, &dat->rcCheck);

			dat->rcCheck.right = dat->rcCheck.left + GetSystemMetrics(SM_CXSMICON) + 4;
			dat->rcCheck.bottom = dat->rcCheck.top + GetSystemMetrics(SM_CYSMICON) + 4;
			if (PtInRect(&dat->rcCheck, pt))
				dat->iItem = iItem;
			else
				dat->iItem = -1;
		}
		break;

	case WM_LBUTTONUP:
		{
			POINT pt = {LOWORD(lParam), HIWORD(lParam)};
			if ((dat->iItem >= 0) && PtInRect(&dat->rcCheck, pt))
				PostMessage(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(GetWindowLongPtr(hwnd, GWL_ID), LBN_MY_CHECK), (LPARAM)dat->iItem);
			dat->iItem = -1;
		}
		break;

	case WM_CHAR:
		if (wParam == ' ') {
			int iItem = ListBox_GetCurSel(hwnd);
			if (iItem >= 0)
				PostMessage(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(GetWindowLongPtr(hwnd, GWL_ID), LBN_MY_CHECK), (LPARAM)iItem);
			return 0;
		}

		if (wParam == 10 /* enter */)
			return 0;

		break;

	case WM_GETDLGCODE:
		if (wParam == VK_RETURN)
			return DLGC_WANTMESSAGE;
		break;

	case WM_MY_RENAME:
		{
			RECT rc;
			struct TAccMgrData *parentDat = (struct TAccMgrData *)GetWindowLongPtr(GetParent(hwnd), GWLP_USERDATA);
			PROTOACCOUNT *pa = (PROTOACCOUNT *)ListBox_GetItemData(hwnd, ListBox_GetCurSel(hwnd));
			if ( !pa || pa->bOldProto || pa->bDynDisabled)
				return 0;

			ListBox_GetItemRect(hwnd, ListBox_GetCurSel(hwnd), &rc);
			rc.left += 2*GetSystemMetrics(SM_CXSMICON) + 4;
			rc.bottom = rc.top + max(GetSystemMetrics(SM_CXSMICON), parentDat->titleHeight) + 4 - 1;
			++rc.top; --rc.right;

			dat->hwndEdit = CreateWindow(_T("EDIT"), pa->tszAccountName, WS_CHILD|WS_BORDER|ES_AUTOHSCROLL, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top, hwnd, NULL, hInst, NULL);
			SetWindowLongPtr(dat->hwndEdit, GWLP_USERDATA, SetWindowLongPtr(dat->hwndEdit, GWLP_WNDPROC, (LONG_PTR)sttEditSubclassProc));
			SendMessage(dat->hwndEdit, WM_SETFONT, (WPARAM)parentDat->hfntTitle, 0);
			SendMessage(dat->hwndEdit, EM_SETMARGINS, EC_LEFTMARGIN|EC_RIGHTMARGIN|EC_USEFONTINFO, 0);
			SendMessage(dat->hwndEdit, EM_SETSEL, 0, (LPARAM) (-1));
			ShowWindow(dat->hwndEdit, SW_SHOW);
		}
		SetFocus(dat->hwndEdit);
		break;

	case WM_KEYDOWN:
		switch (wParam) {
		case VK_F2:
			PostMessage(hwnd, WM_MY_RENAME, 0, 0);
			return 0;

		case VK_INSERT:
			sttClickButton(GetParent(hwnd), IDC_ADD);
			return 0;

		case VK_DELETE:
			sttClickButton(GetParent(hwnd), IDC_REMOVE);
			return 0;

		case VK_RETURN:
			if (GetAsyncKeyState(VK_CONTROL))
				sttClickButton(GetParent(hwnd), IDC_EDIT);
			else
				sttClickButton(GetParent(hwnd), IDOK);
			return 0;
		}
		break;
	}

	return CallWindowProc(dat->oldWndProc, hwnd, msg, wParam, lParam);
}

static void sttSubclassAccList(HWND hwnd, BOOL subclass)
{
	if (subclass) {
		struct TAccListData *dat = (struct TAccListData *)mir_alloc(sizeof(struct TAccListData));
		dat->iItem = -1;
		dat->oldWndProc = (WNDPROC)GetWindowLongPtr(hwnd, GWLP_WNDPROC);
		SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)dat);
		SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)AccListWndProc);
	}
	else {
		struct TAccListData *dat = (struct TAccListData *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
		SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)dat->oldWndProc);
		SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
		mir_free(dat);
}	}

static void sttSelectItem(struct TAccMgrData *dat, HWND hwndList, int iItem)
{
	if ((dat->iSelected != iItem) && (dat->iSelected >= 0))
		ListBox_SetItemHeight(hwndList, dat->iSelected, dat->normalHeight);

	dat->iSelected = iItem;
	ListBox_SetItemHeight(hwndList, dat->iSelected, dat->selectedHeight);
	RedrawWindow(hwndList, NULL, NULL, RDW_INVALIDATE);
}

static void sttUpdateAccountInfo(HWND hwndDlg, struct TAccMgrData *dat)
{
	HWND hwndList = GetDlgItem(hwndDlg, IDC_ACCLIST);
	int curSel = ListBox_GetCurSel(hwndList);
	if (curSel != LB_ERR) {
		HWND hwnd;
		char svc[MAXMODULELABELLENGTH];

		PROTOACCOUNT *pa = (PROTOACCOUNT *)ListBox_GetItemData(hwndList, curSel);
		if (pa) {
			EnableWindow( GetDlgItem(hwndDlg, IDC_UPGRADE), pa->bOldProto || pa->bDynDisabled);
			EnableWindow( GetDlgItem(hwndDlg, IDC_EDIT), !pa->bOldProto && !pa->bDynDisabled);
			EnableWindow( GetDlgItem(hwndDlg, IDC_REMOVE), TRUE);
			EnableWindow( GetDlgItem(hwndDlg, IDC_OPTIONS), pa->ppro != 0);

			if (dat->iSelected >= 0) {
				PROTOACCOUNT *pa_old = (PROTOACCOUNT *)ListBox_GetItemData(hwndList, dat->iSelected);
				if (pa_old && pa_old != pa && pa_old->hwndAccMgrUI)
					ShowWindow(pa_old->hwndAccMgrUI, SW_HIDE);
			}

			if (pa->hwndAccMgrUI) {
				ShowWindow( GetDlgItem(hwndDlg, IDC_TXT_INFO), SW_HIDE);
				ShowWindow(pa->hwndAccMgrUI, SW_SHOW);
			}
			else if ( !pa->ppro) {
				ShowWindow( GetDlgItem(hwndDlg, IDC_TXT_INFO), SW_SHOW);
				SetWindowText( GetDlgItem(hwndDlg, IDC_TXT_INFO), TranslateT("Account is disabled. Please activate it to access options."));
			}
			else {
				mir_snprintf(svc, SIZEOF(svc), "%s%s", pa->szModuleName, PS_CREATEACCMGRUI);
				hwnd = (HWND)CallService(svc, 0, (LPARAM)hwndDlg);
				if (hwnd && (hwnd != (HWND)CALLSERVICE_NOTFOUND)) {
					RECT rc;

					ShowWindow( GetDlgItem(hwndDlg, IDC_TXT_INFO), SW_HIDE);

					GetWindowRect( GetDlgItem(hwndDlg, IDC_TXT_INFO), &rc);
					MapWindowPoints(NULL, hwndDlg, (LPPOINT)&rc, 2);
					SetWindowPos(hwnd, hwndList, rc.left, rc.top, 0, 0, SWP_NOSIZE|SWP_SHOWWINDOW);

					pa->hwndAccMgrUI = hwnd;
				}
				else {
					ShowWindow( GetDlgItem(hwndDlg, IDC_TXT_INFO), SW_SHOW);
					SetWindowText( GetDlgItem(hwndDlg, IDC_TXT_INFO), TranslateT(legacyMsg));
			}	}
			return;
	}	}
	
	EnableWindow( GetDlgItem(hwndDlg, IDC_UPGRADE), FALSE);
	EnableWindow( GetDlgItem(hwndDlg, IDC_EDIT), FALSE);
	EnableWindow( GetDlgItem(hwndDlg, IDC_REMOVE), FALSE);
	EnableWindow( GetDlgItem(hwndDlg, IDC_OPTIONS), FALSE);

	ShowWindow( GetDlgItem(hwndDlg, IDC_TXT_INFO), SW_SHOW);
	SetWindowText( GetDlgItem(hwndDlg, IDC_TXT_INFO), TranslateT(welcomeMsg));
}

INT_PTR CALLBACK AccMgrDlgProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	struct TAccMgrData *dat = (struct TAccMgrData *)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);

	switch(message) {
	case WM_INITDIALOG:
		{
			TAccMgrData *dat = (TAccMgrData *)mir_alloc(sizeof(TAccMgrData));
			SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR)dat);

			TranslateDialogDefault(hwndDlg);
			Window_SetIcon_IcoLib(hwndDlg, SKINICON_OTHER_ACCMGR);

			Button_SetIcon_IcoLib(hwndDlg, IDC_ADD, SKINICON_OTHER_ADDCONTACT, LPGEN("New account"));
			Button_SetIcon_IcoLib(hwndDlg, IDC_EDIT, SKINICON_OTHER_RENAME, LPGEN("Edit"));
			Button_SetIcon_IcoLib(hwndDlg, IDC_REMOVE, SKINICON_OTHER_DELETE, LPGEN("Remove account"));
			Button_SetIcon_IcoLib(hwndDlg, IDC_OPTIONS, SKINICON_OTHER_OPTIONS, LPGEN("Configure..."));
			Button_SetIcon_IcoLib(hwndDlg, IDC_UPGRADE, SKINICON_OTHER_ACCMGR, LPGEN("Upgrade account"));

			EnableWindow( GetDlgItem(hwndDlg, IDC_EDIT), FALSE);
			EnableWindow( GetDlgItem(hwndDlg, IDC_REMOVE), FALSE);
			EnableWindow( GetDlgItem(hwndDlg, IDC_OPTIONS), FALSE);
			EnableWindow( GetDlgItem(hwndDlg, IDC_UPGRADE), FALSE);

			{
				LOGFONT lf;
				HDC hdc;
				HFONT hfnt;
				TEXTMETRIC tm;

				GetObject((HFONT)SendMessage(hwndDlg, WM_GETFONT, 0, 0), sizeof(lf), &lf);
				dat->hfntText = CreateFontIndirect(&lf);

				GetObject((HFONT)SendMessage(hwndDlg, WM_GETFONT, 0, 0), sizeof(lf), &lf);
				lf.lfWeight = FW_BOLD;
				dat->hfntTitle = CreateFontIndirect(&lf);

				hdc = GetDC(hwndDlg);
				hfnt = (HFONT)SelectObject(hdc, dat->hfntTitle);
				GetTextMetrics(hdc, &tm);
				dat->titleHeight = tm.tmHeight;
				SelectObject(hdc, dat->hfntText);
				GetTextMetrics(hdc, &tm);
				dat->textHeight = tm.tmHeight;
				SelectObject(hdc, hfnt);
				ReleaseDC(hwndDlg, hdc);

				dat->normalHeight = 4 + max(dat->titleHeight, GetSystemMetrics(SM_CYSMICON));
				dat->selectedHeight = dat->normalHeight + 4 + 2 * dat->textHeight;

				SendDlgItemMessage(hwndDlg, IDC_NAME, WM_SETFONT, (WPARAM)dat->hfntTitle, 0);
				SendDlgItemMessage(hwndDlg, IDC_TXT_ACCOUNT, WM_SETFONT, (WPARAM)dat->hfntTitle, 0);
				SendDlgItemMessage(hwndDlg, IDC_TXT_ADDITIONAL, WM_SETFONT, (WPARAM)dat->hfntTitle, 0);
			}

			dat->iSelected = -1;
			sttSubclassAccList( GetDlgItem(hwndDlg, IDC_ACCLIST), TRUE);
			SendMessage(hwndDlg, WM_MY_REFRESH, 0, 0);

			Utils_RestoreWindowPositionNoSize(hwndDlg, NULL, "AccMgr", "");
		}
		return TRUE;

	case WM_CTLCOLORSTATIC:
		switch (GetDlgCtrlID((HWND)lParam)) {
		case IDC_WHITERECT:
		case IDC_NAME:
			SetBkColor((HDC)wParam, GetSysColor(COLOR_WINDOW));
			return (INT_PTR)GetSysColorBrush(COLOR_WINDOW);
		}
		break;

	case WM_MEASUREITEM:
		{
			LPMEASUREITEMSTRUCT lps = (LPMEASUREITEMSTRUCT)lParam;
			PROTOACCOUNT *acc = (PROTOACCOUNT *)lps->itemData;

			if ((lps->CtlID != IDC_ACCLIST) || !acc)
				break;

			lps->itemWidth = 10;
			lps->itemHeight = dat->normalHeight;
		}
		return TRUE;

	case WM_DRAWITEM:
		{
			int tmp, size, length;
			TCHAR *text;
			HICON hIcon;
			HBRUSH hbrBack;
			SIZE sz;

			int cxIcon = GetSystemMetrics(SM_CXSMICON);
			int cyIcon = GetSystemMetrics(SM_CYSMICON);

			LPDRAWITEMSTRUCT lps = (LPDRAWITEMSTRUCT)lParam;
			PROTOACCOUNT *acc = (PROTOACCOUNT *)lps->itemData;

			if ((lps->CtlID != IDC_ACCLIST) || (lps->itemID == -1) || !acc)
				break;

			SetBkMode(lps->hDC, TRANSPARENT);
			if (lps->itemState & ODS_SELECTED) {
				hbrBack = GetSysColorBrush(COLOR_HIGHLIGHT);
				SetTextColor(lps->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
			}
			else {
				hbrBack = GetSysColorBrush(COLOR_WINDOW);
				SetTextColor(lps->hDC, GetSysColor(COLOR_WINDOWTEXT));
			}
			FillRect(lps->hDC, &lps->rcItem, hbrBack);

			lps->rcItem.left += 2;
			lps->rcItem.top += 2;
			lps->rcItem.bottom -= 2;

			if (acc->bOldProto)
				tmp = SKINICON_OTHER_ON;
			else if (acc->bDynDisabled)
				tmp = SKINICON_OTHER_OFF;
			else
				tmp = acc->bIsEnabled ? SKINICON_OTHER_TICK : SKINICON_OTHER_NOTICK;

			hIcon = LoadSkinnedIcon(tmp);
			DrawIconEx(lps->hDC, lps->rcItem.left, lps->rcItem.top, hIcon, cxIcon, cyIcon, 0, hbrBack, DI_NORMAL);
			IcoLib_ReleaseIcon(hIcon, 0);

			lps->rcItem.left += cxIcon + 2;

			if (acc->ppro) {
				hIcon = acc->ppro->GetIcon(PLI_PROTOCOL | PLIF_SMALL);
				DrawIconEx(lps->hDC, lps->rcItem.left, lps->rcItem.top, hIcon, cxIcon, cyIcon, 0, hbrBack, DI_NORMAL);
				DestroyIcon(hIcon);
			}
			lps->rcItem.left += cxIcon + 2;

			length = SendDlgItemMessage(hwndDlg, IDC_ACCLIST, LB_GETTEXTLEN, lps->itemID, 0);
			size = max(length+1, 256);
			text = (TCHAR *)_alloca(sizeof(TCHAR) * size);
			SendDlgItemMessage(hwndDlg, IDC_ACCLIST, LB_GETTEXT, lps->itemID, (LPARAM)text);

			SelectObject(lps->hDC, dat->hfntTitle);
			tmp = lps->rcItem.bottom;
			lps->rcItem.bottom = lps->rcItem.top + max(cyIcon, dat->titleHeight);
			DrawText(lps->hDC, text, -1, &lps->rcItem, DT_LEFT|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS|DT_VCENTER);
			lps->rcItem.bottom = tmp;
			GetTextExtentPoint32(lps->hDC, text, length, &sz);
			lps->rcItem.top += max(cxIcon, sz.cy) + 2;

			if (lps->itemID == (unsigned)dat->iSelected) {
				SelectObject(lps->hDC, dat->hfntText);
				mir_sntprintf(text, size, _T("%s: ") _T(TCHAR_STR_PARAM), TranslateT("Protocol"), acc->szProtoName);
				length = lstrlen(text);
				DrawText(lps->hDC, text, -1, &lps->rcItem, DT_LEFT|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);
				GetTextExtentPoint32(lps->hDC, text, length, &sz);
				lps->rcItem.top += sz.cy + 2;

				if (acc->ppro && Proto_IsProtocolLoaded(acc->szProtoName)) {
					char *szIdName;
					TCHAR *tszIdName;
					CONTACTINFO ci = { 0 };

					szIdName = (char *)acc->ppro->GetCaps(PFLAG_UNIQUEIDTEXT, 0);
 					tszIdName = szIdName ? mir_a2t(szIdName) : mir_tstrdup(TranslateT("Account ID"));
					
					ci.cbSize = sizeof(ci);
					ci.hContact = NULL;
					ci.szProto = acc->szModuleName;
					ci.dwFlag = CNF_UNIQUEID | CNF_TCHAR;
					if ( !CallService(MS_CONTACT_GETCONTACTINFO, 0, (LPARAM) & ci)) {
						switch (ci.type) {
						case CNFT_ASCIIZ:
							mir_sntprintf(text, size, _T("%s: %s"), tszIdName, ci.pszVal);
							mir_free(ci.pszVal);
							break;
						case CNFT_DWORD:
							mir_sntprintf(text, size, _T("%s: %d"), tszIdName, ci.dVal);
							break;
						}
					}
					else mir_sntprintf(text, size, _T("%s: %s"), tszIdName, TranslateT("<unknown>"));
					mir_free(tszIdName);
				}
				else mir_sntprintf(text, size, TranslateT("Protocol is not loaded."));

				length = lstrlen(text);
				DrawText(lps->hDC, text, -1, &lps->rcItem, DT_LEFT|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);
				GetTextExtentPoint32(lps->hDC, text, length, &sz);
				lps->rcItem.top += sz.cy + 2;
		}	}
		return TRUE;

	case WM_MY_REFRESH:
		{
			HWND hList = GetDlgItem(hwndDlg, IDC_ACCLIST);
			int i = ListBox_GetCurSel(hList);
			PROTOACCOUNT *acc = (i == LB_ERR) ? NULL : (PROTOACCOUNT *)ListBox_GetItemData(hList, i);

			dat->iSelected = -1;
			SendMessage(hList, LB_RESETCONTENT, 0, 0);
			for (i=0; i < accounts.getCount(); ++i) {
				int iItem = SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)accounts[i]->tszAccountName);
				SendMessage(hList, LB_SETITEMDATA, iItem, (LPARAM)accounts[i]);

				if (accounts[i] == acc)
					ListBox_SetCurSel(hList, iItem);
			}

			dat->iSelected = ListBox_GetCurSel(hList); // -1 if error = > nothing selected in our case
			if (dat->iSelected >= 0)
				sttSelectItem(dat, hList, dat->iSelected);
			else if (acc && acc->hwndAccMgrUI)
				ShowWindow(acc->hwndAccMgrUI, SW_HIDE);

			sttUpdateAccountInfo(hwndDlg, dat);
		}
		break;

	case WM_CONTEXTMENU:
		if (GetWindowLongPtr((HWND)wParam, GWL_ID) == IDC_ACCLIST) {
			HWND hwndList = GetDlgItem(hwndDlg, IDC_ACCLIST);
			POINT pt = { (signed short)LOWORD(lParam), (signed short)HIWORD(lParam) };
			int iItem = ListBox_GetCurSel(hwndList);

			if ((pt.x == -1) && (pt.y == -1)) {
				if (iItem != LB_ERR) {
					RECT rc;
					ListBox_GetItemRect(hwndList, iItem, &rc);
					pt.x = rc.left + GetSystemMetrics(SM_CXSMICON) + 4;
					pt.y = rc.top + 4 + max(GetSystemMetrics(SM_CXSMICON), dat->titleHeight);
					ClientToScreen(hwndList, &pt);
				}
			}
			else {
				// menu was activated with mouse = > find item under cursor & set focus to our control.
				POINT ptItem = pt;
				ScreenToClient(hwndList, &ptItem);
				iItem = (short)LOWORD(SendMessage(hwndList, LB_ITEMFROMPOINT, 0, MAKELPARAM(ptItem.x, ptItem.y)));
				if (iItem != LB_ERR) 
                {
				    ListBox_SetCurSel(hwndList, iItem);
                    sttUpdateAccountInfo(hwndDlg, dat);
				    sttSelectItem(dat, hwndList, iItem);
				    SetFocus(hwndList);
                }
			}

			if (iItem != LB_ERR) {
				PROTOACCOUNT* pa = (PROTOACCOUNT*)ListBox_GetItemData(hwndList, iItem);
				HMENU hMenu = CreatePopupMenu();
				if ( !pa->bOldProto && !pa->bDynDisabled)
					AppendMenu(hMenu, MF_STRING, 1, TranslateT("Rename"));

				AppendMenu(hMenu, MF_STRING, 3, TranslateT("Delete"));

				if (Proto_IsAccountEnabled(pa))
					AppendMenu(hMenu, MF_STRING, 4, TranslateT("Configure"));

				if (pa->bOldProto || pa->bDynDisabled)
					AppendMenu(hMenu, MF_STRING, 5, TranslateT("Upgrade"));

				switch (TrackPopupMenu(hMenu, TPM_RETURNCMD, pt.x, pt.y, 0, hwndDlg, NULL)) {
				case 1:
					PostMessage(hwndList, WM_MY_RENAME, 0, 0);
					break;

				case 2:
					sttClickButton(hwndDlg, IDC_EDIT);
					break;

				case 3:
					sttClickButton(hwndDlg, IDC_REMOVE);
					break;

				case 4:
					sttClickButton(hwndDlg, IDC_OPTIONS);
					break;

				case 5:
					sttClickButton(hwndDlg, IDC_UPGRADE);
					break;
				}
				DestroyMenu(hMenu);
			}	
		}
		break;

	case WM_COMMAND:
		switch(LOWORD(wParam)) {
		case IDC_ACCLIST:
			{
				HWND hwndList = GetDlgItem(hwndDlg, IDC_ACCLIST);
				switch (HIWORD(wParam)) {
				case LBN_SELCHANGE:
					sttUpdateAccountInfo(hwndDlg, dat);
					sttSelectItem(dat, hwndList, ListBox_GetCurSel(hwndList));
					SetFocus(hwndList);
					break;

				case LBN_DBLCLK:
					PostMessage(hwndList, WM_MY_RENAME, 0, 0);
					break;

				case LBN_MY_CHECK:
					{
						PROTOACCOUNT *pa = (PROTOACCOUNT *)ListBox_GetItemData(hwndList, lParam);
						if (pa) {
							if (pa->bOldProto || pa->bDynDisabled)
								break;

							pa->bIsEnabled = !pa->bIsEnabled;
							if (pa->bIsEnabled) {
								if (ActivateAccount(pa)) {
									pa->ppro->OnEvent(EV_PROTO_ONLOAD, 0, 0);
									if ( !db_get_b(NULL, "CList", "MoveProtoMenus", TRUE))
										pa->ppro->OnEvent(EV_PROTO_ONMENU, 0, 0);
								}
								else pa->bDynDisabled = TRUE;
							}
							else {
								DWORD dwStatus = CallProtoServiceInt(NULL,pa->szModuleName, PS_GETSTATUS, 0, 0);
								if (dwStatus >= ID_STATUS_ONLINE) {
									if (IDCANCEL == ::MessageBox(hwndDlg, 
										                  TranslateT("Account is online. Disable account?"), 
															   TranslateT("Accounts"), MB_OKCANCEL)) {
										pa->bIsEnabled = 1;	//stay enabled
									}
								}

								if ( !pa->bIsEnabled)
									DeactivateAccount(pa, true, false);
							}

							WriteDbAccounts();
							NotifyEventHooks(hAccListChanged, PRAC_CHECKED, (LPARAM)pa);
							sttUpdateAccountInfo(hwndDlg, dat);
							RedrawWindow(hwndList, NULL, NULL, RDW_INVALIDATE);
					}	}
					break;

				case LBN_MY_RENAME:
					{
						int iItem = ListBox_GetCurSel(hwndList);
						PROTOACCOUNT *pa = (PROTOACCOUNT *)ListBox_GetItemData(hwndList, iItem);
						if (pa) {
							mir_free(pa->tszAccountName);
							pa->tszAccountName = (TCHAR*)lParam;
							WriteDbAccounts();
							NotifyEventHooks(hAccListChanged, PRAC_CHANGED, (LPARAM)pa);

							ListBox_DeleteString(hwndList, iItem);
							iItem = ListBox_AddString(hwndList, pa->tszAccountName);
							ListBox_SetItemData(hwndList, iItem, (LPARAM)pa);
							ListBox_SetCurSel(hwndList, iItem);

							sttSelectItem(dat, hwndList, iItem);

							RedrawWindow(hwndList, NULL, NULL, RDW_INVALIDATE);
						}
						else mir_free((TCHAR*)lParam);
					}
					break;
			}	}
			break;

		case IDC_ADD:
			{
				AccFormDlgParam param = { PRAC_ADDED, NULL };
				if (IDOK == DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_ACCFORM), hwndDlg, AccFormDlgProc, (LPARAM)&param))
					SendMessage(hwndDlg, WM_MY_REFRESH, 0, 0);
			}
			break;

		case IDC_EDIT:
			{
				HWND hList = GetDlgItem(hwndDlg, IDC_ACCLIST);
				int idx = ListBox_GetCurSel(hList);
				if (idx != -1)
					PostMessage(hList, WM_MY_RENAME, 0, 0);
			}
			break;

		case IDC_REMOVE:
			{
				HWND hList = GetDlgItem(hwndDlg, IDC_ACCLIST);
				int idx = ListBox_GetCurSel(hList);
				if (idx != -1) {
					PROTOACCOUNT* pa = (PROTOACCOUNT*)ListBox_GetItemData(hList, idx);
					TCHAR buf[ 200 ];
					mir_sntprintf(buf, SIZEOF(buf), TranslateT("Account %s is being deleted"), pa->tszAccountName);
					if (pa->bOldProto) {
						MessageBox(NULL, TranslateT("You need to disable plugin to delete this account"), buf, 
							MB_ICONERROR | MB_OK);
						break;
					}
					if (IDYES == MessageBox(NULL, TranslateT(errMsg), buf, MB_ICONSTOP | MB_DEFBUTTON2 | MB_YESNO)) {
						// lock controls to avoid changes during remove process
						ListBox_SetCurSel(hList, -1);
						sttUpdateAccountInfo(hwndDlg, dat);
						EnableWindow(hList, FALSE);
						EnableWindow( GetDlgItem(hwndDlg, IDC_ADD), FALSE);

						ListBox_SetItemData(hList, idx, 0);

						accounts.remove(pa);

						CheckProtocolOrder();

						WriteDbAccounts();
						NotifyEventHooks(hAccListChanged, PRAC_REMOVED, (LPARAM)pa);

						UnloadAccount(pa, true, true);
						SendMessage(hwndDlg, WM_MY_REFRESH, 0, 0);

						EnableWindow(hList, TRUE);
						EnableWindow( GetDlgItem(hwndDlg, IDC_ADD), TRUE);
			}	}	}
			break;

		case IDC_OPTIONS:
			{
				HWND hList = GetDlgItem(hwndDlg, IDC_ACCLIST);
				int idx = ListBox_GetCurSel(hList);
				if (idx != -1) {
					PROTOACCOUNT* pa = (PROTOACCOUNT*)ListBox_GetItemData(hList, idx);
					if (pa->bOldProto) {
						OPENOPTIONSDIALOG ood;
						ood.cbSize = sizeof(ood);
						ood.pszGroup = "Network";
						ood.pszPage = pa->szModuleName;
						ood.pszTab = NULL;
						Options_Open(&ood);
					}
					else OpenAccountOptions(pa);
			}	}
			break;

		case IDC_UPGRADE:
			{
				HWND hList = GetDlgItem(hwndDlg, IDC_ACCLIST);
				int idx = ListBox_GetCurSel(hList);
				if (idx != -1) {
					AccFormDlgParam param = { PRAC_UPGRADED, (PROTOACCOUNT*)ListBox_GetItemData(hList, idx) };
					DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_ACCFORM), hwndDlg, AccFormDlgProc, (LPARAM)&param);
			}	}
			break;

		case IDC_LNK_NETWORK:
			{
				PSHNOTIFY pshn = {0};
				pshn.hdr.code = PSN_APPLY;
				pshn.hdr.hwndFrom = hwndDlg;
				SendMessage(hwndDlg, WM_NOTIFY, 0, (LPARAM)&pshn);

				OPENOPTIONSDIALOG ood = {0};
				ood.cbSize = sizeof(ood);
				ood.pszPage = "Network";
				Options_Open(&ood);
				break;
			}

		case IDC_LNK_ADDONS:
			CallService(MS_UTILS_OPENURL, TRUE, (LPARAM)"http://miranda-ng.org/");
			break;

		case IDOK:
			{
				PSHNOTIFY pshn = {0};
				pshn.hdr.code = PSN_APPLY;
				pshn.hdr.hwndFrom = hwndDlg;
				SendMessage(hwndDlg, WM_NOTIFY, 0, (LPARAM)&pshn);
				DestroyWindow(hwndDlg);
				break;
			}

		case IDCANCEL:
			{
				PSHNOTIFY pshn = {0};
				pshn.hdr.code = PSN_RESET;
				pshn.hdr.hwndFrom = hwndDlg;
				SendMessage(hwndDlg, WM_NOTIFY, 0, (LPARAM)&pshn);
				DestroyWindow(hwndDlg);
				break;
			}
		}
	case PSM_CHANGED:
		{
			HWND hList = GetDlgItem(hwndDlg, IDC_ACCLIST);
			int idx = ListBox_GetCurSel(hList);
			if (idx != -1) {
				PROTOACCOUNT *acc = (PROTOACCOUNT *)ListBox_GetItemData(hList, idx);
				if (acc)
				{
					acc->bAccMgrUIChanged = TRUE;
					SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
				}
			}
			break;
		}
	case WM_NOTIFY:
		switch(((LPNMHDR)lParam)->idFrom) {
		case 0:
			switch (((LPNMHDR)lParam)->code) {
				case PSN_APPLY:
				{	
					int i;
					PSHNOTIFY pshn = {0};
					pshn.hdr.code = PSN_APPLY;
					for (i=0; i < accounts.getCount(); ++i) {
						if (accounts[i]->hwndAccMgrUI && accounts[i]->bAccMgrUIChanged) {
							pshn.hdr.hwndFrom = accounts[i]->hwndAccMgrUI;
							SendMessage(accounts[i]->hwndAccMgrUI, WM_NOTIFY, 0, (LPARAM)&pshn);
							accounts[i]->bAccMgrUIChanged = FALSE;
					}	}
					return TRUE;
				}
				case PSN_RESET:
				{	
					int i;
					PSHNOTIFY pshn = {0};
					pshn.hdr.code = PSN_RESET;
					for (i=0; i < accounts.getCount(); ++i) {
						if (accounts[i]->hwndAccMgrUI && accounts[i]->bAccMgrUIChanged) {
							pshn.hdr.hwndFrom = accounts[i]->hwndAccMgrUI;
							SendMessage(accounts[i]->hwndAccMgrUI, WM_NOTIFY, 0, (LPARAM)&pshn);
							accounts[i]->bAccMgrUIChanged = FALSE;
					}	}
					return TRUE;
				}
			}
			break;
		}
		break;
	case WM_DESTROY:
		{
			for (int i=0; i < accounts.getCount(); ++i) {
				accounts[i]->bAccMgrUIChanged = FALSE;
				if (accounts[i]->hwndAccMgrUI) {
					DestroyWindow(accounts[i]->hwndAccMgrUI);
					accounts[i]->hwndAccMgrUI = NULL;
		}	}	}

		Window_FreeIcon_IcoLib(hwndDlg);
		Button_FreeIcon_IcoLib(hwndDlg, IDC_ADD);
		Button_FreeIcon_IcoLib(hwndDlg, IDC_EDIT);
		Button_FreeIcon_IcoLib(hwndDlg, IDC_REMOVE);
		Button_FreeIcon_IcoLib(hwndDlg, IDC_OPTIONS);
		Button_FreeIcon_IcoLib(hwndDlg, IDC_UPGRADE);
		Utils_SaveWindowPosition(hwndDlg, NULL, "AccMgr", "");
		sttSubclassAccList( GetDlgItem(hwndDlg, IDC_ACCLIST), FALSE);
		DeleteObject(dat->hfntTitle);
		DeleteObject(dat->hfntText);
		mir_free(dat);
		hAccMgr = NULL;
		break;
	}

	return FALSE;
}

static INT_PTR OptProtosShow(WPARAM, LPARAM)
{
	if ( !hAccMgr)
		hAccMgr = CreateDialogParam(hInst, MAKEINTRESOURCE(IDD_ACCMGR), NULL, AccMgrDlgProc, 0);

	ShowWindow(hAccMgr, SW_RESTORE);
	SetForegroundWindow(hAccMgr);
	SetActiveWindow(hAccMgr);
	return 0;
}

int OptProtosLoaded(WPARAM, LPARAM)
{
	CLISTMENUITEM mi = { 0 };
	mi.cbSize = sizeof(mi);
	mi.flags = CMIF_ICONFROMICOLIB;
	mi.icolibItem = GetSkinIconHandle(SKINICON_OTHER_ACCMGR);
	mi.position = 1900000000;
	mi.pszName = LPGEN("&Accounts...");
	mi.pszService = MS_PROTO_SHOWACCMGR;
	Menu_AddMainMenuItem(&mi);
	return 0;
}

static int OnAccListChanged(WPARAM eventCode, LPARAM lParam)
{
	PROTOACCOUNT* pa = (PROTOACCOUNT*)lParam;

	switch(eventCode) {
	case PRAC_CHANGED:
		if (pa->ppro) {
			mir_free(pa->ppro->m_tszUserName);
			pa->ppro->m_tszUserName = mir_tstrdup(pa->tszAccountName);
			pa->ppro->OnEvent(EV_PROTO_ONRENAME, 0, lParam);
		}
	}

	return 0;
}

static int ShutdownAccMgr(WPARAM, LPARAM)
{
	if (IsWindow(hAccMgr))
		DestroyWindow(hAccMgr);
	hAccMgr = NULL;
	return 0;
}

int LoadProtoOptions(void)
{
	CreateServiceFunction(MS_PROTO_SHOWACCMGR, OptProtosShow);

	HookEvent(ME_SYSTEM_MODULESLOADED, OptProtosLoaded);
	HookEvent(ME_PROTO_ACCLISTCHANGED, OnAccListChanged);
	HookEvent(ME_SYSTEM_PRESHUTDOWN, ShutdownAccMgr);
	return 0;
}
