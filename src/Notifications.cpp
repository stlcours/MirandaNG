/* 
Copyright (C) 2010 Mataes

This is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with this file; see the file license.txt. If
not, write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.
*/

#include "common.h"

HWND hwndDialog = NULL;

void unzip(const TCHAR* ptszZipFile, TCHAR* ptszDestPath, TCHAR* ptszBackPath);

void PopupAction(HWND hWnd, BYTE action)
{
	switch (action)
	{
		case PCA_CLOSEPOPUP:
			break;
		case PCA_DONOTHING:
			return;
	}//end* switch
	PUDeletePopUp(hWnd);
}

static INT_PTR CALLBACK PopupDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case UM_POPUPACTION:
		if (HIWORD(wParam) == BN_CLICKED) {
			LPMSGPOPUPDATA pmpd = (LPMSGPOPUPDATA)PUGetPluginData(hDlg);
			if (pmpd) {
				switch (LOWORD(wParam)) {
				case IDYES:
					if (IsWindow(pmpd->hDialog))
						EndDialog(pmpd->hDialog, LOWORD(wParam));
					PUDeletePopUp(hDlg);
					break;
	
				case IDNO:
					if (IsWindow(pmpd->hDialog))
						EndDialog(pmpd->hDialog, LOWORD(wParam));
					PUDeletePopUp(hDlg);
					break;
				}
			}
		}
		break;

	case UM_FREEPLUGINDATA:
		{
			LPMSGPOPUPDATA pmpd = (LPMSGPOPUPDATA)PUGetPluginData(hDlg);
			if (pmpd > 0)
				mir_free(pmpd);
			return TRUE; //TRUE or FALSE is the same, it gets ignored.
		}
		break;
	}

	return DefWindowProc(hDlg, uMsg, wParam, lParam);
}

static INT_PTR CALLBACK PopupDlgProc2(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_COMMAND:
		PopupAction(hDlg, PopupOptions.LeftClickAction);
		break;

	case WM_CONTEXTMENU:
		PopupAction(hDlg, PopupOptions.RightClickAction);
		break;

	case UM_FREEPLUGINDATA:
		{
			LPMSGPOPUPDATA pmpd = (LPMSGPOPUPDATA)PUGetPluginData(hDlg);
			if (pmpd > 0)
				mir_free(pmpd);
			return TRUE; //TRUE or FALSE is the same, it gets ignored.
		}
		break;
	}

	return DefWindowProc(hDlg, uMsg, wParam, lParam);
}

static VOID MakePopupAction(POPUPACTION &pa, INT id)
{
	pa.cbSize = sizeof(POPUPACTION);
	pa.flags = PAF_ENABLED;
	pa.wParam = MAKEWORD(id, BN_CLICKED);
	pa.lParam = 0;
	switch (id) {
	case IDYES:
		pa.lchIcon = Skin_GetIcon("btn_ok");
		strncpy_s(pa.lpzTitle, MODNAME"/Yes", SIZEOF(pa.lpzTitle));
		break;

	case IDNO:
		pa.lchIcon = Skin_GetIcon("btn_cancel");
		strncpy_s(pa.lpzTitle, MODNAME"/No", SIZEOF(pa.lpzTitle));
		break;
	}
}

void ShowPopup(HWND hDlg, LPCTSTR ptszTitle, LPCTSTR ptszText, int Number, int ActType)
{
	if ( !ServiceExists(MS_POPUP_ADDPOPUPEX) || !DBGetContactSettingByte(NULL, "PopUp", "ModuleIsEnabled", 1) || !DBGetContactSettingByte(NULL, MODNAME, "Popups2", DEFAULT_POPUP_ENABLED)) {
		char setting[100];
		mir_snprintf(setting, SIZEOF(setting), "Popups%dM", Number);
		if (DBGetContactSettingByte(NULL, MODNAME, setting, DEFAULT_MESSAGE_ENABLED)) {
			int iMsgType;
			switch( Number ) {
				case 1: iMsgType = MB_ICONSTOP; break;
				case 2: iMsgType = MB_ICONINFORMATION; break;
				default: iMsgType = 0;
			}
			MessageBox(hDlg, TranslateTS(ptszText), TranslateTS(ptszTitle), iMsgType);
		}
		return;
	}

	LPMSGPOPUPDATA	pmpd = (LPMSGPOPUPDATA)mir_alloc(sizeof(MSGPOPUPDATA));
	if (!pmpd)
		return;

	POPUPDATAT_V2 pd = { 0 };
	pd.cbSize = sizeof(POPUPDATAT_V2);
	pd.lchContact = NULL; //(HANDLE)wParam;
	pd.lchIcon = LoadSkinnedIcon(PopupsList[Number].Icon);
	lstrcpyn(pd.lptzText, TranslateTS(ptszText), SIZEOF(pd.lptzText));
	lstrcpyn(pd.lptzContactName, TranslateTS(ptszTitle), SIZEOF(pd.lptzContactName));
	switch (PopupOptions.DefColors) {
	case byCOLOR_WINDOWS:
		pd.colorBack = GetSysColor(COLOR_BTNFACE);
		pd.colorText = GetSysColor(COLOR_WINDOWTEXT);
		break;
	case byCOLOR_OWN:
		pd.colorBack = PopupsList[Number].colorBack;
		pd.colorText = PopupsList[Number].colorText;
		break;
	case byCOLOR_POPUP:
		pd.colorBack = pd.colorText = 0;
		break;
	}
	if (Number == 0 && ActType != 0)
		pd.PluginWindowProc = (WNDPROC)PopupDlgProc;
	else
		pd.PluginWindowProc = (WNDPROC)PopupDlgProc2;
	pd.PluginData = pmpd;
	if (Number == 0)
		pd.iSeconds = -1;
	else
		pd.iSeconds = PopupOptions.Timeout;
	pd.hNotification = NULL;
	pd.lpActions = pmpd->pa;

	pmpd->hDialog = hDlg;
	switch (ActType) {
	case 0:
		break;

	case 1:
		MakePopupAction(pmpd->pa[pd.actionCount++], IDYES);
		MakePopupAction(pmpd->pa[pd.actionCount++], IDNO);
		break;
	}

	CallService(MS_POPUP_ADDPOPUPT, (WPARAM) &pd, APF_NEWDATA);
}

INT_PTR CALLBACK DlgDownload(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {
	case WM_INITDIALOG:
		SetWindowText(GetDlgItem(hDlg, IDC_LABEL), tszDialogMsg);
		SetWindowLongPtr(GetDlgItem(hDlg, IDC_PB), GWL_STYLE, GetWindowLongPtr(GetDlgItem(hDlg, IDC_PB), GWL_STYLE) | PBS_MARQUEE);
		SendMessage(GetDlgItem(hDlg, IDC_PB), PBM_SETMARQUEE, 1, 50);
		return TRUE;
	}
	return FALSE;
}

INT_PTR CALLBACK DlgDownloadPop(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		PopupDataText *temp = (PopupDataText*)lParam;
		ShowPopup(hDlg, temp->Title, temp->Text, 3, 0);
		return TRUE;
	}
	return FALSE;
}

void DlgDownloadProc(FILEURL *pFileUrl, PopupDataText temp)
{
}

void SelectAll(HWND hDlg, bool bEnable)
{
	vector<FILEINFO> &todo = *(vector<FILEINFO> *)GetWindowLongPtr(hDlg, GWLP_USERDATA);
	HWND hwndList = GetDlgItem(hDlg, IDC_LIST_UPDATES);

	for (size_t i=0; i < todo.size(); i++) {
		ListView_SetCheckState(hwndList, i, bEnable);
		todo[i].enabled = bEnable;
	}
}

static void SetStringText(HWND hWnd, size_t i, TCHAR* ptszText)
{
	ListView_SetItemText(hWnd, i, 1, ptszText);
}

static void ApplyUpdates(void* param)
{
	HWND hDlg = (HWND)param;
	HWND hwndList = GetDlgItem(hDlg, IDC_LIST_UPDATES);
	vector<FILEINFO> &todo = *(vector<FILEINFO> *)GetWindowLongPtr(hDlg, GWLP_USERDATA);
	TCHAR tszBuff[2048], tszFileTemp[MAX_PATH], tszFileBack[MAX_PATH];

	mir_sntprintf(tszFileBack, SIZEOF(tszFileBack), _T("%s\\Backups"), tszRoot);
	CreateDirectory(tszFileBack, NULL);

	mir_sntprintf(tszFileTemp, SIZEOF(tszFileTemp), _T("%s\\Temp"), tszRoot);
	CreateDirectory(tszFileTemp, NULL);

	for (size_t i=0; i < todo.size(); ++i) {
		ListView_EnsureVisible(hwndList, i, FALSE);
		if ( !todo[i].enabled) {
			SetStringText(hwndList, i, TranslateT("Skipped"));
			continue;
		}
		
		// download update
		SetStringText(hwndList, i, TranslateT("Downloading..."));

		FILEURL *pFileUrl = &todo[i].File;
		if ( !DownloadFile(pFileUrl->tszDownloadURL, pFileUrl->tszDiskPath))
			SetStringText(hwndList, i, TranslateT("Failed!"));
		else
			SetStringText(hwndList, i, TranslateT("Succeeded."));
	}

	if (todo.size() == 0) {
		DestroyWindow(hDlg);
		return;
	}

	INT rc = -1;
	PopupDataText temp;
	temp.Title = TranslateT("Plugin Updater");
	temp.Text = tszBuff;
	lstrcpyn(tszBuff, TranslateT("Download complete. Start updating? All your data will be saved and Miranda IM will be closed."), SIZEOF(tszBuff));
	if (ServiceExists(MS_POPUP_ADDPOPUPEX) && ServiceExists(MS_POPUP_REGISTERACTIONS) && DBGetContactSettingByte(NULL, "PopUp", "ModuleIsEnabled", 1) && DBGetContactSettingByte(NULL,MODNAME, "Popups0", DEFAULT_POPUP_ENABLED) && (DBGetContactSettingDword(NULL, "PopUp", "Actions", 0) & 1))
		rc = DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_POPUPDUMMI), NULL, DlgMsgPop, (LPARAM)&temp);
	else
		rc = MessageBox(NULL, temp.Text, temp.Title, MB_YESNO | MB_ICONQUESTION);
	if (rc != IDYES) {
		mir_sntprintf(tszBuff, SIZEOF(tszBuff), TranslateT("You have chosen not to install the plugin updates immediately.\nYou can install it manually from this location:\n\n%s"), tszFileTemp);
		ShowPopup(0, LPGENT("Plugin Updater"), tszBuff, 2, 0);
		DestroyWindow(hDlg);
		return;
	}

	TCHAR* tszMirandaPath = Utils_ReplaceVarsT(_T("%miranda_path%"));

	for (size_t i = 0; i < todo.size(); i++) {
		if ( !todo[i].enabled)
			continue;

		FILEINFO& p = todo[i];
		unzip(p.File.tszDiskPath, tszMirandaPath, tszFileBack);
	}

	DestroyWindow(hDlg);
	CallFunctionAsync(RestartMe, 0);
}

INT_PTR CALLBACK DlgUpdate(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	HWND hwndList = GetDlgItem(hDlg, IDC_LIST_UPDATES);

	switch (message) {
	case WM_INITDIALOG:
		hwndDialog = hDlg;
		TranslateDialogDefault( hDlg );
		SendMessage(hwndList, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES);
		{
			RECT r;
			GetClientRect(hwndList, &r);

			LVCOLUMN lvc = {0};
			// Initialize the LVCOLUMN structure.
			// The mask specifies that the format, width, text, and
			// subitem members of the structure are valid.
			lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
			lvc.fmt = LVCFMT_LEFT;

			lvc.iSubItem = 0;
			lvc.pszText = TranslateT("Component Name");
			lvc.cx = 220; // width of column in pixels
			ListView_InsertColumn(hwndList, 0, &lvc);

			lvc.iSubItem = 1;
			lvc.pszText = TranslateT("State");
			lvc.cx = 120 - GetSystemMetrics(SM_CXVSCROLL); // width of column in pixels
			ListView_InsertColumn(hwndList, 1, &lvc);

			//enumerate plugins, fill in list
			//bool one_enabled = false;
			ListView_DeleteAllItems(hwndList);

			// Some code to create the list-view control.
			// Initialize LVITEM members that are common to all
			// items.
			LVITEM lvI = {0};
			lvI.mask = LVIF_TEXT | LVIF_PARAM | LVIF_NORECOMPUTE;// | LVIF_IMAGE;

			vector<FILEINFO> &todo = *(vector<FILEINFO> *)lParam;
			for (int i = 0; i < (int)todo.size(); ++i) {
				lvI.mask = LVIF_TEXT | LVIF_PARAM;// | LVIF_IMAGE;
				lvI.iSubItem = 0;
				lvI.lParam = (LPARAM)&todo[i];
				lvI.pszText = todo[i].tszDescr;
				lvI.iItem = i;
				ListView_InsertItem(hwndList, &lvI);

				// remember whether the user has decided not to update this component with this particular new version
				ListView_SetCheckState(hwndList, lvI.iItem, true);
				todo[i].enabled = true;
			}
			HWND hwOk = GetDlgItem(hDlg, IDOK);
			EnableWindow(hwOk, true/*one_enabled ? TRUE : FALSE*/);
		}
		// do this after filling list - enables 'ITEMCHANGED' below
		SetWindowLongPtr(hDlg, GWLP_USERDATA, lParam);
		Utils_RestoreWindowPositionNoSize(hDlg,0,MODNAME,"ConfirmWindow");
		return TRUE;

	case WM_NOTIFY:
		if (((LPNMHDR) lParam)->hwndFrom == hwndList) {
			switch (((LPNMHDR) lParam)->code) {
			case LVN_ITEMCHANGED:
				if (GetWindowLongPtr(hDlg, GWLP_USERDATA)) {
					NMLISTVIEW *nmlv = (NMLISTVIEW *)lParam;

					LVITEM lvI = {0};
					lvI.iItem = nmlv->iItem;
					lvI.iSubItem = 0;
					lvI.mask = LVIF_PARAM;
					ListView_GetItem(hwndList, &lvI);

					vector<FILEINFO> &todo = *(vector<FILEINFO> *)GetWindowLongPtr(hDlg, GWLP_USERDATA);
					if ((nmlv->uNewState ^ nmlv->uOldState) & LVIS_STATEIMAGEMASK) {
						todo[lvI.iItem].enabled = ListView_GetCheckState(hwndList, nmlv->iItem);

						bool enableOk = false;
						for(int i=0; i<(int)todo.size(); ++i) {
							if(todo[i].enabled) {
								enableOk = true;
								break;
							}
						}
						HWND hwOk = GetDlgItem(hDlg, IDOK);
						EnableWindow(hwOk, enableOk ? TRUE : FALSE);
					}
				}
				break;
			}
		}
		break;

	case WM_COMMAND:
		if (HIWORD( wParam ) == BN_CLICKED) {
			switch(LOWORD(wParam)) {
			case IDOK:
				mir_forkthread(ApplyUpdates, hDlg);
				return TRUE;

			case IDC_SELALL:
				SelectAll(hDlg, true);
				break;

			case IDC_SELNONE:
				SelectAll(hDlg, false);
				break;

			case IDCANCEL:
				Utils_SaveWindowPosition(hDlg, NULL, MODNAME, "ConfirmWindow");
				DestroyWindow(hDlg);
				return TRUE;
			}
		}
		break;

	case WM_DESTROY:
		Utils_SaveWindowPosition(hDlg, NULL, MODNAME, "ConfirmWindow");
		hwndDialog = NULL;
		delete (vector<FILEINFO> *)GetWindowLongPtr(hDlg, GWLP_USERDATA);
		SetWindowLongPtr(hDlg, GWLP_USERDATA, 0);
		break;			
	}

	return FALSE;
}

INT_PTR CALLBACK DlgMsgPop(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		PopupDataText *temp = (PopupDataText*)lParam;
		ShowPopup(hDlg, temp->Title, temp->Text, 0, 1);
		return TRUE;
	}
	ShowWindow(hDlg, SW_HIDE);
	return FALSE;
}
