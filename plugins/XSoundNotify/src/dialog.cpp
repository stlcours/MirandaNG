/*
Miranda plugin template, originally by Richard Hughes
http://miranda-icq.sourceforge.net/

This file is placed in the public domain. Anybody is free to use or
modify it as they wish with no restriction.
There is no warranty.
*/

#include "stdafx.h"

static INT_PTR CALLBACK DlgProcContactsOptions(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	MCONTACT hContact = (MCONTACT)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
	XSN_Data *p;

	switch (msg) {
	case WM_INITDIALOG:
		{
			TranslateDialogDefault(hwndDlg);
			hContact = lParam;
			SetWindowLongPtr(hwndDlg, GWLP_USERDATA, lParam);
			WindowList_Add(hChangeSoundDlgList, hwndDlg, hContact);
			Utils_RestoreWindowPositionNoSize(hwndDlg, hContact, SETTINGSNAME, "ChangeSoundDlg");
			char* szProto = GetContactProto(hContact);
			PROTOACCOUNT *pa = Proto_GetAccount(szProto);
			char* szUniqueId = (char*)CallProtoService(pa->szModuleName, PS_GETCAPS, PFLAG_UNIQUEIDSETTING, 0);
			if ((INT_PTR)szUniqueId != CALLSERVICE_NOTFOUND && szUniqueId != NULL) {
				DBVARIANT dbvuid = { 0 };
				if (!db_get(hContact, pa->szModuleName, szUniqueId, &dbvuid)) {
					wchar_t uid[MAX_PATH];
					switch (dbvuid.type) {
					case DBVT_DWORD:
						_itow(dbvuid.dVal, uid, 10);
						break;

					case DBVT_ASCIIZ:
						mir_wstrcpy(uid, _A2T(dbvuid.pszVal));
						break;

					case DBVT_UTF8:
						mir_wstrcpy(uid, ptrW(mir_utf8decodeW(dbvuid.pszVal)));
						break;
					}

					wchar_t *nick = (wchar_t *)pcli->pfnGetContactDisplayName(hContact, 0);
					wchar_t value[100];
					mir_snwprintf(value, TranslateT("Custom sound for %s (%s)"), nick, uid);
					SetWindowText(hwndDlg, value);
					db_free(&dbvuid);
				}
			}
			EnableWindow(GetDlgItem(hwndDlg, IDC_CONT_BUTTON_CHOOSE_SOUND), TRUE);
			DBVARIANT dbv = { 0 };
			if (!db_get_ws(hContact, SETTINGSNAME, SETTINGSKEY, &dbv)) {
				EnableWindow(GetDlgItem(hwndDlg, IDC_CONT_BUTTON_TEST_PLAY), TRUE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_CONT_BUTTON_RESET_SOUND), TRUE);
				SetDlgItemText(hwndDlg, IDC_CONT_LABEL_SOUND, PathFindFileName(dbv.ptszVal));
				db_free(&dbv);
			}
			else {
				EnableWindow(GetDlgItem(hwndDlg, IDC_CONT_BUTTON_TEST_PLAY), FALSE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_CONT_BUTTON_RESET_SOUND), FALSE);
				SetDlgItemText(hwndDlg, IDC_CONT_LABEL_SOUND, TranslateT("Not set"));
			}
			EnableWindow(GetDlgItem(hwndDlg, IDC_CONT_IGNORE_SOUND), TRUE);
			CheckDlgButton(hwndDlg, IDC_CONT_IGNORE_SOUND, db_get_b(hContact, SETTINGSNAME, SETTINGSIGNOREKEY, 0) ? BST_CHECKED : BST_UNCHECKED);
			p = XSN_Users.find((XSN_Data *)&hContact);
			if (p == NULL) {
				ptrW name(db_get_wsa(hContact, SETTINGSNAME, SETTINGSKEY));
				if (name != NULL)
					XSN_Users.insert(new XSN_Data(hContact, name, IsDlgButtonChecked(hwndDlg, IDC_CONT_IGNORE_SOUND) ? 1 : 0));
			}
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			p = XSN_Users.find((XSN_Data *)&hContact);
			if (p != NULL) {
				if (mir_wstrcmpi(p->path, L"")) {
					wchar_t shortpath[MAX_PATH];
					PathToRelativeT(p->path, shortpath);
					db_set_ws(hContact, SETTINGSNAME, SETTINGSKEY, shortpath);
				}
				db_set_b(hContact, SETTINGSNAME, SETTINGSIGNOREKEY, p->ignore);
			}

		case IDCANCEL:
			DestroyWindow(hwndDlg);
			break;

		case IDC_CONT_BUTTON_CHOOSE_SOUND:
			{
				wchar_t FileName[MAX_PATH];
				wchar_t *tszMirDir = Utils_ReplaceVarsT(L"%miranda_path%");

				OPENFILENAME ofn = { 0 };
				ofn.lStructSize = sizeof(ofn);
				wchar_t tmp[MAX_PATH];
				if (GetModuleHandle(L"bass_interface.dll"))
					mir_snwprintf(tmp, L"%s (*.wav, *.mp3, *.ogg)%c*.wav;*.mp3;*.ogg%c%c", TranslateT("Sound files"), 0, 0, 0);
				else
					mir_snwprintf(tmp, L"%s (*.wav)%c*.wav%c%c", TranslateT("WAV files"), 0, 0, 0);
				ofn.lpstrFilter = tmp;
				ofn.hwndOwner = 0;
				ofn.lpstrFile = FileName;
				ofn.nMaxFile = MAX_PATH;
				ofn.nMaxFileTitle = MAX_PATH;
				ofn.Flags = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST;
				ofn.lpstrInitialDir = tszMirDir;
				*FileName = '\0';
				ofn.lpstrDefExt = L"";

				if (GetOpenFileName(&ofn)) {
					SetDlgItemText(hwndDlg, IDC_CONT_LABEL_SOUND, PathFindFileName(FileName));
					p = XSN_Users.find((XSN_Data *)&hContact);
					if (p == NULL)
						XSN_Users.insert(new XSN_Data(hContact, FileName, IsDlgButtonChecked(hwndDlg, IDC_CONT_IGNORE_SOUND) ? 1 : 0));
					else {
						wcsncpy(p->path, FileName, _countof(p->path));
						p->ignore = IsDlgButtonChecked(hwndDlg, IDC_CONT_IGNORE_SOUND) ? 1 : 0;
					}
					EnableWindow(GetDlgItem(hwndDlg, IDC_CONT_BUTTON_TEST_PLAY), TRUE);
					EnableWindow(GetDlgItem(hwndDlg, IDC_CONT_BUTTON_RESET_SOUND), TRUE);
				}
				mir_free(tszMirDir);
			}
			break;

		case IDC_CONT_BUTTON_TEST_PLAY:
			p = XSN_Users.find((XSN_Data *)&hContact);
			isIgnoreSound = 0;
			if (p == NULL) {
				DBVARIANT dbv;
				if (!db_get_ws(hContact, SETTINGSNAME, SETTINGSKEY, &dbv)) {
					wchar_t longpath[MAX_PATH] = { 0 };
					PathToAbsoluteW(dbv.ptszVal, longpath);
					SkinPlaySoundFile(longpath);
					db_free(&dbv);
				}
			}
			else {
				wchar_t longpath[MAX_PATH] = { 0 };
				PathToAbsoluteW(p->path, longpath);
				SkinPlaySoundFile(longpath);
			}
			break;

		case IDC_CONT_BUTTON_RESET_SOUND:
			EnableWindow(GetDlgItem(hwndDlg, IDC_CONT_BUTTON_TEST_PLAY), FALSE);
			EnableWindow(GetDlgItem(hwndDlg, IDC_CONT_BUTTON_RESET_SOUND), FALSE);
			CheckDlgButton(hwndDlg, IDC_CONT_IGNORE_SOUND, BST_UNCHECKED);
			SetDlgItemText(hwndDlg, IDC_CONT_LABEL_SOUND, TranslateT("Not set"));

			p = XSN_Users.find((XSN_Data *)&hContact);
			if (p != NULL) {
				XSN_Users.remove(p);
				delete p;
			}
			db_unset(hContact, SETTINGSNAME, SETTINGSKEY);
			db_unset(hContact, SETTINGSNAME, SETTINGSIGNOREKEY);
			break;

		case IDC_CONT_IGNORE_SOUND:
			p = XSN_Users.find((XSN_Data *)&hContact);
			if (p == NULL) {
				DBVARIANT dbv;
				if (!db_get_ws(hContact, SETTINGSNAME, SETTINGSKEY, &dbv)) {
					wchar_t longpath[MAX_PATH];
					PathToAbsoluteW(dbv.ptszVal, longpath);
					XSN_Users.insert(new XSN_Data(hContact, longpath, IsDlgButtonChecked(hwndDlg, IDC_CONT_IGNORE_SOUND) ? 1 : 0));
					db_free(&dbv);
				}
				else XSN_Users.insert(new XSN_Data(hContact, L"", IsDlgButtonChecked(hwndDlg, IDC_CONT_IGNORE_SOUND) ? 1 : 0));
			}
			else p->ignore = IsDlgButtonChecked(hwndDlg, IDC_CONT_IGNORE_SOUND) ? 1 : 0;
		}
		break;

	case WM_CLOSE:
		DestroyWindow(hwndDlg);
		break;

	case WM_DESTROY:
		Utils_SaveWindowPosition(hwndDlg, hContact, SETTINGSNAME, "ChangeSoundDlg");
		WindowList_Remove(hChangeSoundDlgList, hwndDlg);
	}
	return FALSE;
}

INT_PTR ShowDialog(WPARAM wParam, LPARAM)
{
	HWND hChangeSoundDlg = WindowList_Find(hChangeSoundDlgList, wParam);
	if (!hChangeSoundDlg) {
		hChangeSoundDlg = CreateDialogParam(hInst, MAKEINTRESOURCE(IDD_CONTACTS), 0, DlgProcContactsOptions, (LPARAM)wParam);
		ShowWindow(hChangeSoundDlg, SW_SHOW);
	}
	else {
		SetForegroundWindow(hChangeSoundDlg);
		SetFocus(hChangeSoundDlg);
	}
	return 0;
}
