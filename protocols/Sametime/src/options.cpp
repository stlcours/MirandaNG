#include "StdAfx.h"
#include "sametime.h"


#define DEFAULT_ID		(0x1800)

#define NUM_IDS		20

TCHAR* client_names[NUM_IDS] = {
	_T("Official Binary Library"),
	_T("Official Java Applet"),
	_T("Official Binary Application"), 
	_T("Official Java Application"),
	_T("Notes v6.5"),
	_T("Notes v7.0"),
	_T("ICT"),
	_T("NotesBuddy"),
	_T("NotesBuddy v4.15"),
	_T("Sanity"),
	_T("Perl"),
	_T("PMR Alert"),
	_T("Trillian (SourceForge)"),
	_T("Trillian (IBM)"),
	_T("Meanwhile Library"),
	_T("Meanwhile (Python)"),
	_T("Meanwhile (Gaim)"),
	_T("Meanwhile (Adium)"),
	_T("Meanwhile (Kopete)"),
	_T("Custom")
};
int client_ids[NUM_IDS] = {
	0x1000,
	0x1001,
	0x1002,
	0x1003,
	0x1200,
	0x1210,
	0x1300,
	0x1400,
	0x1405,
	0x1600,
	0x1625,
	0x1650,
	0x16aa,
	0x16bb,
	0x1700,
	0x1701,
	0x1702,
	0x1703,
	0x1704,
	0xFFFF
};

static INT_PTR CALLBACK DlgProcOptNet(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	CSametimeProto* proto = (CSametimeProto*)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);

	switch (msg) {
	case WM_INITDIALOG: {
		TranslateDialogDefault(hwndDlg);

		SetWindowLongPtr(hwndDlg, GWLP_USERDATA, lParam);
		proto = (CSametimeProto*)lParam;

		WORD client_ver = proto->GetClientVersion();
		if (client_ver) {
			TCHAR verbuf[100];
			mir_sntprintf(verbuf, SIZEOF(verbuf), TranslateT("Client proto version: %03d.%03d"), (client_ver & 0xFF00) >> 8, client_ver & 0xFF);
			SetDlgItemText(hwndDlg, IDC_ST_CLIENTVER, verbuf);
		}

		WORD server_ver = proto->GetServerVersion();
		if (server_ver) {
			TCHAR verbuf[100];
			mir_sntprintf(verbuf, SIZEOF(verbuf), TranslateT("Server proto version: %03d.%03d"), (server_ver & 0xFF00) >> 8, server_ver & 0xFF);
			SetDlgItemText(hwndDlg, IDC_ST_SERVERVER, verbuf);
		}

		TCHAR *s = mir_utf8decodeT(proto->options.server_name); SetDlgItemText(hwndDlg, IDC_ED_SNAME, s); mir_free(s);
		s = mir_utf8decodeT(proto->options.id); SetDlgItemText(hwndDlg, IDC_ED_NAME, s); mir_free(s);
		s = mir_utf8decodeT(proto->options.pword); SetDlgItemText(hwndDlg, IDC_ED_PWORD, s); mir_free(s);

		SetDlgItemInt(hwndDlg, IDC_ED_PORT, proto->options.port, FALSE);
		CheckDlgButton(hwndDlg, IDC_CHK_GETSERVERCONTACTS, proto->options.get_server_contacts ? TRUE : FALSE);
		CheckDlgButton(hwndDlg, IDC_CHK_ADDCONTACTS, proto->options.add_contacts ? TRUE : FALSE);
		CheckDlgButton(hwndDlg, IDC_CHK_IDLEAWAY, proto->options.idle_as_away ? TRUE : FALSE);
		CheckDlgButton(hwndDlg, IDC_CHK_OLDDEFAULTVER, proto->options.use_old_default_client_ver ? TRUE : FALSE);

		SendDlgItemMessage(hwndDlg, IDC_CMB_CLIENT, CB_RESETCONTENT, 0, 0);
		int pos = 0;
		bool found = false;

		for (int i = 0; i < NUM_IDS; i++) {
			pos = SendDlgItemMessage(hwndDlg, IDC_CMB_CLIENT, CB_ADDSTRING, -1, (LPARAM)client_names[i]);
			SendDlgItemMessage(hwndDlg, IDC_CMB_CLIENT, CB_SETITEMDATA, pos, client_ids[i]);
			if (client_ids[i] == proto->options.client_id) {
				found = true;
				SendDlgItemMessage(hwndDlg, IDC_CMB_CLIENT, CB_SETCURSEL, pos, 0);
				SetDlgItemInt(hwndDlg, IDC_ED_CLIENTID, client_ids[i], FALSE);
				if (i != sizeof(client_ids) / sizeof(int)-1) {
					HWND hw = GetDlgItem(hwndDlg, IDC_ED_CLIENTID);
					EnableWindow(hw, false);
				}
			}
		}

		if (!found) {
			SendDlgItemMessage(hwndDlg, IDC_CMB_CLIENT, CB_SETCURSEL, pos, 0); // pos is last item, i.e. custom
			SetDlgItemInt(hwndDlg, IDC_ED_CLIENTID, proto->options.client_id, FALSE);
		}

		if (!ServiceExists(MS_POPUP_ADDPOPUP)) {
			HWND hw = GetDlgItem(hwndDlg, IDC_RAD_ERRPOP);
			EnableWindow(hw, FALSE);
		}

		if (!ServiceExists(MS_CLIST_SYSTRAY_NOTIFY)) {
			HWND hw = GetDlgItem(hwndDlg, IDC_RAD_ERRBAL);
			EnableWindow(hw, FALSE);
		}

		switch (proto->options.err_method) {
			case ED_POP: CheckDlgButton(hwndDlg, IDC_RAD_ERRPOP, TRUE); break;
			case ED_MB: CheckDlgButton(hwndDlg, IDC_RAD_ERRMB, TRUE); break;
			case ED_BAL: CheckDlgButton(hwndDlg, IDC_RAD_ERRBAL, TRUE); break;
		}

		if (proto->options.encrypt_session)
			CheckDlgButton(hwndDlg, IDC_RAD_ENC, TRUE);
		else
			CheckDlgButton(hwndDlg, IDC_RAD_NOENC, TRUE);

		return FALSE;
	}

	case WM_COMMAND:
		if (HIWORD(wParam) == EN_CHANGE && (HWND)lParam == GetFocus())
			SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);

		if (HIWORD(wParam) == CBN_SELCHANGE) {
			int sel = SendDlgItemMessage(hwndDlg, IDC_CMB_CLIENT, CB_GETCURSEL, 0, 0);
			int id = SendDlgItemMessage(hwndDlg, IDC_CMB_CLIENT, CB_GETITEMDATA, sel, 0);
			bool custom = (id == client_ids[sizeof(client_ids) / sizeof(int)-1]);

			if (!custom)
				SetDlgItemInt(hwndDlg, IDC_ED_CLIENTID, id, FALSE);
			else
				SetDlgItemInt(hwndDlg, IDC_ED_CLIENTID, DEFAULT_ID, FALSE);

			HWND hw = GetDlgItem(hwndDlg, IDC_ED_CLIENTID);
			EnableWindow(hw, custom);

			SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
		}

		if (HIWORD(wParam) == BN_CLICKED) {
			switch (LOWORD(wParam)) {
			case IDC_BTN_UPLOADCONTACTS:
				{
					EnableWindow(GetDlgItem(hwndDlg, IDC_BTN_UPLOADCONTACTS), FALSE);
					EnableWindow(GetDlgItem(hwndDlg, IDC_BTN_IMPORTCONTACTS), FALSE);

					proto->ExportContactsToServer();

					SendMessage(hwndDlg, WMU_STORECOMPLETE, 0, 0);
				}
				return TRUE;

			case IDC_BTN_IMPORTCONTACTS:
				{
					TCHAR import_filename[MAX_PATH]; import_filename[0] = 0;

					OPENFILENAME ofn = { 0 };
					ofn.lStructSize = sizeof(ofn);
					ofn.lpstrFile = import_filename;
					ofn.hwndOwner = hwndDlg;
					ofn.Flags = CC_FULLOPEN;
					ofn.nMaxFile = MAX_PATH;
					ofn.lpstrFilter = _T("All\0*.*\0");
					ofn.nFilterIndex = 1;
					ofn.lpstrFileTitle = NULL;
					ofn.nMaxFileTitle = 0;
					ofn.lpstrInitialDir = NULL;
					ofn.Flags = OFN_PATHMUSTEXIST;

					if (GetOpenFileName(&ofn) == TRUE) {
						HWND hBut = GetDlgItem(hwndDlg, IDC_BTN_UPLOADCONTACTS);
						EnableWindow(hBut, FALSE);
						hBut = GetDlgItem(hwndDlg, IDC_BTN_IMPORTCONTACTS);
						EnableWindow(hBut, FALSE);

						proto->ImportContactsFromFile(ofn.lpstrFile);

						SendMessage(hwndDlg, WMU_STORECOMPLETE, 0, 0);
					}
				}
				return TRUE;

			case IDC_CHK_GETSERVERCONTACTS:
			case IDC_CHK_ENCMESSAGES:
			case IDC_RAD_ERRMB:
			case IDC_RAD_ERRBAL:
			case IDC_RAD_ERRPOP:
			case IDC_CHK_USERCP:
			case IDC_CHK_ADDCONTACTS:
			case IDC_CHK_IDLEAWAY:
			case IDC_CHK_OLDDEFAULTVER:
			case IDC_RAD_ENC:
			case IDC_RAD_NOENC:
				SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
				return TRUE;
			case IDC_RAD_ANSI:
			case IDC_RAD_UTF8:
			case IDC_RAD_OEM:
			case IDC_RAD_UTF7:
			case IDC_RAD_USERCP:
				HWND hw = GetDlgItem(hwndDlg, IDC_CHK_USERCP);
				EnableWindow(hw, !IsDlgButtonChecked(hwndDlg, IDC_RAD_USERCP));

				SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
				return TRUE;
			}
		}
		break;

	case WMU_STORECOMPLETE:
		EnableWindow(GetDlgItem(hwndDlg, IDC_BTN_UPLOADCONTACTS), TRUE);
		EnableWindow(GetDlgItem(hwndDlg, IDC_BTN_IMPORTCONTACTS), TRUE);
		return TRUE;

	case WM_NOTIFY:
		if (((LPNMHDR)lParam)->code == PSN_APPLY) {
			TCHAR ws[2048];
			char* utf;

			GetDlgItemText(hwndDlg, IDC_ED_SNAME, ws, LSTRINGLEN);
			strcpy(proto->options.server_name, utf = mir_utf8encodeT(ws)); mir_free(utf);
			GetDlgItemText(hwndDlg, IDC_ED_NAME, ws, LSTRINGLEN);
			strcpy(proto->options.id, utf = mir_utf8encodeT(ws)); mir_free(utf);
			GetDlgItemText(hwndDlg, IDC_ED_PWORD, ws, LSTRINGLEN);
			strcpy(proto->options.pword, utf = mir_utf8encodeT(ws)); mir_free(utf);

			BOOL translated;
			int port = GetDlgItemInt(hwndDlg, IDC_ED_PORT, &translated, FALSE);
			if (translated)
				proto->options.port = port;

			proto->options.get_server_contacts = (IsDlgButtonChecked(hwndDlg, IDC_CHK_GETSERVERCONTACTS) != FALSE);

			int sel = SendDlgItemMessage(hwndDlg, IDC_CMB_CLIENT, CB_GETCURSEL, 0, 0);
			int id = SendDlgItemMessage(hwndDlg, IDC_CMB_CLIENT, CB_GETITEMDATA, sel, 0);

			if (id == client_ids[sizeof(client_ids) / sizeof(int)-1]) {
				BOOL trans;
				id = GetDlgItemInt(hwndDlg, IDC_ED_CLIENTID, &trans, FALSE);
				if (trans)
					proto->options.client_id = id;
			}
			else proto->options.client_id = id;

			if (IsDlgButtonChecked(hwndDlg, IDC_RAD_ERRMB)) proto->options.err_method = ED_MB;
			else if (IsDlgButtonChecked(hwndDlg, IDC_RAD_ERRBAL)) proto->options.err_method = ED_BAL;
			else if (IsDlgButtonChecked(hwndDlg, IDC_RAD_ERRPOP)) proto->options.err_method = ED_POP;

			proto->options.add_contacts = (IsDlgButtonChecked(hwndDlg, IDC_CHK_ADDCONTACTS) != FALSE);
			proto->options.encrypt_session = (IsDlgButtonChecked(hwndDlg, IDC_RAD_ENC) != FALSE);
			proto->options.idle_as_away = (IsDlgButtonChecked(hwndDlg, IDC_CHK_IDLEAWAY) != FALSE);

			proto->options.use_old_default_client_ver = (IsDlgButtonChecked(hwndDlg, IDC_CHK_OLDDEFAULTVER) != FALSE);

			proto->SaveOptions();

			return TRUE;
		}
		break;

	case WM_DESTROY:
		break;
	}

	return FALSE;
}

int CSametimeProto::OptInit(WPARAM wParam, LPARAM)
{
	OPTIONSDIALOGPAGE odp = { sizeof(odp) };
	odp.flags = ODPF_BOLDGROUPS | ODPF_TCHAR | ODPF_DONTTRANSLATE;
	odp.hInstance = hInst;
	odp.pszTemplate = MAKEINTRESOURCEA(IDD_OPTNET);
	odp.ptszTitle = m_tszUserName;
	odp.ptszGroup = LPGENT("Network");
	odp.pfnDlgProc = DlgProcOptNet;
	odp.dwInitParam = (LPARAM)this;
	Options_AddPage(wParam, &odp);
	return 0;
}

void CSametimeProto::LoadOptions()
{
	DBVARIANT dbv;

	if (!db_get_utf(0, m_szModuleName, "ServerName", &dbv)) {
		strncpy(options.server_name, dbv.pszVal, LSTRINGLEN);
		db_free(&dbv);
	}
	if (!db_get_utf(0, m_szModuleName, "stid", &dbv)) {
		strncpy(options.id, dbv.pszVal, LSTRINGLEN);
		db_free(&dbv);
	}
	if (!db_get_utf(0, m_szModuleName, "Password", &dbv)) {
		strncpy(options.pword, dbv.pszVal, LSTRINGLEN);
		db_free(&dbv);
	}

	options.port = db_get_dw(0, m_szModuleName, "ServerPort", DEFAULT_PORT);
	options.encrypt_session = (db_get_b(0, m_szModuleName, "EncryptSession", 0) == 1);

	options.client_id = db_get_dw(0, m_szModuleName, "ClientID", DEFAULT_ID);
	options.use_old_default_client_ver = (db_get_b(0, m_szModuleName, "UseOldClientVer", 0) == 1);

	options.get_server_contacts = (db_get_b(0, m_szModuleName, "GetServerContacts", 1) == 1);
	options.add_contacts = (db_get_b(0, m_szModuleName, "AutoAddContacts", 0) == 1);
	options.idle_as_away = (db_get_b(0, m_szModuleName, "IdleAsAway", 1) == 1);

	// if popups not installed, will be changed to 'ED_BAL' (balloons) in main.cpp, modules loaded
	options.err_method = (ErrorDisplay)db_get_b(0, m_szModuleName, "ErrorDisplay", ED_POP);
	// funny logic :) ... try to avoid message boxes
	// if want baloons but no balloons, try popups
	// if want popups but no popups, try baloons
	// if, after that, you want balloons but no balloons, revert to message boxes
	if (options.err_method == ED_BAL && !ServiceExists(MS_CLIST_SYSTRAY_NOTIFY)) options.err_method = ED_POP;
	if (options.err_method == ED_POP && !ServiceExists(MS_POPUP_SHOWMESSAGE)) options.err_method = ED_BAL;
	if (options.err_method == ED_BAL && !ServiceExists(MS_CLIST_SYSTRAY_NOTIFY)) options.err_method = ED_MB;

	debugLog(_T("LoadOptions() loaded: ServerName:len=[%d], id:len=[%d], pword:len=[%d]"), options.server_name == NULL ? -1 : strlen(options.server_name), options.id == NULL ? -1 : strlen(options.id), options.pword == NULL ? -1 : strlen(options.pword));
	debugLog(_T("LoadOptions() loaded: port=[%d], encrypt_session=[%d], client_id=[%d], use_old_default_client_ver=[%d]"), options.port, options.encrypt_session, options.client_id, options.use_old_default_client_ver);
	debugLog(_T("LoadOptions() loaded: get_server_contacts=[%d], add_contacts=[%d], idle_as_away=[%d], err_method=[%d]"), options.get_server_contacts, options.add_contacts, options.idle_as_away, options.err_method);

}

void CSametimeProto::SaveOptions()
{
	db_set_utf(0, m_szModuleName, "ServerName", options.server_name);

	db_set_utf(0, m_szModuleName, "stid", options.id);
	//db_set_s(0, m_szModuleName, "Nick", options.id);
	db_set_utf(0, m_szModuleName, "Password", options.pword);

	db_set_dw(0, m_szModuleName, "ServerPort", options.port);
	db_set_b(0, m_szModuleName, "GetServerContacts", options.get_server_contacts ? 1 : 0);
	db_set_dw(0, m_szModuleName, "ClientID", options.client_id);
	db_set_b(0, m_szModuleName, "ErrorDisplay", options.err_method);

	db_set_b(0, m_szModuleName, "AutoAddContacts", options.add_contacts ? 1 : 0);
	db_set_b(0, m_szModuleName, "EncryptSession", options.encrypt_session ? 1 : 0);
	db_set_b(0, m_szModuleName, "IdleAsAway", options.idle_as_away ? 1 : 0);

	db_set_b(0, m_szModuleName, "UseOldClientVer", options.use_old_default_client_ver ? 1 : 0);
}
