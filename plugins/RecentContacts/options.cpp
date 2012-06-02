#include "RecentContacts.h"
#include "resource.h"

extern HINSTANCE hInst;
void LoadDBSettings();

LRESULT CALLBACK DlgProcOptions(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	char str[32];

	switch (msg) {
	case WM_INITDIALOG:
		TranslateDialogDefault(hwndDlg);
		CheckDlgButton(hwndDlg, IDC_HIDEOFFLINE, (LastUCOpt.HideOffline ? BST_CHECKED : BST_UNCHECKED));

		mir_snprintf(str, SIZEOF(str), "%d", LastUCOpt.MaxShownContacts);
		SetDlgItemTextA(hwndDlg, IDC_SHOWNCONTACTS, str);

		mir_snprintf(str, SIZEOF(str), "%s", LastUCOpt.DateTimeFormat.c_str());
		SetDlgItemTextA(hwndDlg, IDC_DATETIME, str);

		SetWindowLong(hwndDlg,GWLP_USERDATA,lParam);
		return TRUE;

	case WM_COMMAND:
		switch(HIWORD(wParam)) {
		case EN_CHANGE:
		case BN_CLICKED:
		case CBN_EDITCHANGE:
		case CBN_SELCHANGE:
			SendMessage(GetParent(hwndDlg),PSM_CHANGED,0,0);
		}
		break;

	case WM_NOTIFY:
		{
			LPNMHDR phdr = (LPNMHDR)(lParam);
			if (phdr->idFrom == 0 && phdr->code == PSN_APPLY) {
				BOOL bSuccess = FALSE;

				LastUCOpt.HideOffline = (BOOL)IsDlgButtonChecked(hwndDlg, IDC_HIDEOFFLINE);
				DBWriteContactSettingByte(NULL, dbLastUC_ModuleName, dbLastUC_HideOfflineContacts, (BYTE)LastUCOpt.HideOffline);

				GetDlgItemTextA(hwndDlg, IDC_SHOWNCONTACTS, str, SIZEOF(str));
				LastUCOpt.MaxShownContacts= atoi(str);
				DBWriteContactSettingByte(0,dbLastUC_ModuleName, dbLastUC_MaxShownContacts, LastUCOpt.MaxShownContacts);

				GetDlgItemTextA(hwndDlg, IDC_DATETIME, str, SIZEOF(str));
				DBWriteContactSettingString(0,dbLastUC_ModuleName, dbLastUC_DateTimeFormat, str );

				LoadDBSettings();
				return TRUE;
			}
			break;
		}
	}
	return FALSE;
}

int onOptInitialise(WPARAM wParam, LPARAM lParam)
{
	OPTIONSDIALOGPAGE odp = { 0 };
   odp.cbSize = sizeof(odp);
   odp.position = 0;
   odp.hInstance = hInst;
   odp.pszGroup = "Contact List";
   odp.pszTemplate = MAKEINTRESOURCEA(IDD_LASTUC_OPT);
   odp.pszTitle = Translate(msLastUC_ShowListName);
   odp.pfnDlgProc = DlgProcOptions;
   odp.flags = ODPF_BOLDGROUPS;
   CallService(MS_OPT_ADDPAGE, wParam, (LPARAM) &odp);

   return 0;
}
