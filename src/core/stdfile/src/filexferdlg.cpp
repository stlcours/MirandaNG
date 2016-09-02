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
#include <io.h>
#include "file.h"

static int CheckVirusScanned(HWND hwnd, FileDlgData *dat, int i)
{
	if (dat->send) return 1;
	if (dat->fileVirusScanned == NULL) return 0;
	if (dat->fileVirusScanned[i]) return 1;
	if (db_get_b(NULL, "SRFile", "WarnBeforeOpening", 1) == 0) return 1;
	return IDYES == MessageBox(hwnd, TranslateT("This file has not yet been scanned for viruses. Are you certain you want to open it?"), TranslateT("File received"), MB_YESNO|MB_DEFBUTTON2);
}

#define M_VIRUSSCANDONE  (WM_USER+100)
struct virusscanthreadstartinfo {
	wchar_t *szFile;
	int returnCode;
	HWND hwndReply;
};

wchar_t* PFTS_StringToTchar(int flags, const wchar_t* s)
{
	if (flags & PFTS_UTF)
		return Utf8DecodeW((char*)s);
	if (flags & PFTS_UNICODE)
		return mir_wstrdup(s);
	return mir_a2u((char*)s);
}

int PFTS_CompareWithTchar(PROTOFILETRANSFERSTATUS *ft, const wchar_t *s, wchar_t *r)
{
	if (ft->flags & PFTS_UTF) {
		wchar_t *ts = Utf8DecodeW((char*)s);
		int res = mir_wstrcmp(ts, r);
		mir_free(ts);
		return res;
	}
	if (ft->flags & PFTS_UNICODE)
		return mir_wstrcmp(s, r);

	wchar_t *ts = mir_a2u((char*)s);
	int res = mir_wstrcmp(ts, r);
	mir_free(ts);
	return res;
}

static void SetOpenFileButtonStyle(HWND hwndButton, int enabled)
{
	EnableWindow(hwndButton, enabled);
}

void FillSendData(FileDlgData *dat, DBEVENTINFO& dbei)
{
	dbei.cbSize = sizeof(dbei);
	dbei.szModule = GetContactProto(dat->hContact);
	dbei.eventType = EVENTTYPE_FILE;
	dbei.flags = DBEF_SENT;
	dbei.timestamp = time(NULL);
	char *szFileNames = Utf8EncodeT(dat->szFilenames), *szMsg = Utf8EncodeT(dat->szMsg);
	dbei.flags |= DBEF_UTF;

	dbei.cbBlob = int(sizeof(DWORD) + mir_strlen(szFileNames) + mir_strlen(szMsg) + 2);
	dbei.pBlob = (PBYTE)mir_alloc(dbei.cbBlob);
	*(PDWORD)dbei.pBlob = 0;
	mir_strcpy((char*)dbei.pBlob + sizeof(DWORD), szFileNames);
	mir_strcpy((char*)dbei.pBlob + sizeof(DWORD) + mir_strlen(szFileNames) + 1, szMsg);

	mir_free(szFileNames), mir_free(szMsg);
}

static void __cdecl RunVirusScannerThread(struct virusscanthreadstartinfo *info)
{
	DBVARIANT dbv;
	if (!db_get_ws(NULL, "SRFile", "ScanCmdLine", &dbv)) {
		if (dbv.ptszVal[0]) {
			STARTUPINFO si = { 0 };
			si.cb = sizeof(si);
			wchar_t *pszReplace = wcsstr(dbv.ptszVal, L"%f");
			wchar_t szCmdLine[768];
			if (pszReplace) {
				if (info->szFile[mir_wstrlen(info->szFile) - 1] == '\\')
					info->szFile[mir_wstrlen(info->szFile) - 1] = '\0';
				*pszReplace = 0;
				mir_snwprintf(szCmdLine, L"%s\"%s\"%s", dbv.ptszVal, info->szFile, pszReplace + 2);
			} else
				wcsncpy_s(szCmdLine, dbv.ptszVal, _TRUNCATE);

			PROCESS_INFORMATION pi;
			if (CreateProcess(NULL, szCmdLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
				if (WaitForSingleObject(pi.hProcess, 3600 * 1000) == WAIT_OBJECT_0)
					PostMessage(info->hwndReply, M_VIRUSSCANDONE, info->returnCode, 0);
				CloseHandle(pi.hProcess);
				CloseHandle(pi.hThread);
			}
		}
		db_free(&dbv);
	}
	mir_free(info->szFile);
	mir_free(info);
}

static void SetFilenameControls(HWND hwndDlg, FileDlgData *dat, PROTOFILETRANSFERSTATUS *fts)
{
	wchar_t msg[MAX_PATH];
	wchar_t *fnbuf = NULL, *fn = NULL;
	SHFILEINFO shfi = { 0 };

	if (fts->tszCurrentFile) {
		fnbuf = mir_wstrdup(fts->tszCurrentFile);
		if ((fn = wcsrchr(fnbuf, '\\')) == NULL)
			fn = fnbuf;
		else fn++;
	}

	if (dat->hIcon) DestroyIcon(dat->hIcon); dat->hIcon = NULL;

	if (fn && (fts->totalFiles > 1)) {
		mir_snwprintf(msg, L"%s: %s (%d %s %d)",
			pcli->pfnGetContactDisplayName(fts->hContact, 0),
			fn, fts->currentFileNumber + 1, TranslateT("of"), fts->totalFiles);

		SHGetFileInfo(fn, FILE_ATTRIBUTE_DIRECTORY, &shfi, sizeof(shfi), SHGFI_USEFILEATTRIBUTES | SHGFI_ICON | SHGFI_SMALLICON);
		dat->hIcon = shfi.hIcon;
	}
	else if (fn) {
		mir_snwprintf(msg, L"%s: %s", pcli->pfnGetContactDisplayName(fts->hContact, 0), fn);

		SHGetFileInfo(fn, FILE_ATTRIBUTE_NORMAL, &shfi, sizeof(shfi), SHGFI_USEFILEATTRIBUTES | SHGFI_ICON | SHGFI_SMALLICON);
		dat->hIcon = shfi.hIcon;
	}
	else {
		mir_wstrncpy(msg, pcli->pfnGetContactDisplayName(fts->hContact, 0), _countof(msg));
		HICON hIcon = Skin_LoadIcon(SKINICON_OTHER_DOWNARROW);
		dat->hIcon = CopyIcon(hIcon);
		IcoLib_ReleaseIcon(hIcon, NULL);
	}

	mir_free(fnbuf);

	SendDlgItemMessage(hwndDlg, IDC_FILEICON, STM_SETIMAGE, IMAGE_ICON, (LPARAM)dat->hIcon);
	SetDlgItemText(hwndDlg, IDC_CONTACTNAME, msg);
}

enum { FTS_TEXT, FTS_PROGRESS, FTS_OPEN };
static void SetFtStatus(HWND hwndDlg, wchar_t *text, int mode)
{
	SetDlgItemText(hwndDlg, IDC_STATUS, TranslateW(text));
	SetDlgItemText(hwndDlg, IDC_TRANSFERCOMPLETED, TranslateW(text));

	ShowWindow(GetDlgItem(hwndDlg, IDC_STATUS), (mode == FTS_TEXT) ? SW_SHOW : SW_HIDE);
	ShowWindow(GetDlgItem(hwndDlg, IDC_ALLFILESPROGRESS), (mode == FTS_PROGRESS) ? SW_SHOW : SW_HIDE);
	ShowWindow(GetDlgItem(hwndDlg, IDC_TRANSFERCOMPLETED), (mode == FTS_OPEN) ? SW_SHOW : SW_HIDE);
	ShowWindow(GetDlgItem(hwndDlg, IDC_FILEICON), (mode == FTS_OPEN) ? SW_SHOW : SW_HIDE);
}

static void HideProgressControls(HWND hwndDlg)
{
	RECT rc;
	char buf[64];

	GetWindowRect(GetDlgItem(hwndDlg, IDC_ALLPRECENTS), &rc);
	MapWindowPoints(NULL, hwndDlg, (LPPOINT)&rc, 2);
	SetWindowPos(hwndDlg, NULL, 0, 0, 100, rc.bottom + 3, SWP_NOMOVE | SWP_NOZORDER);
	ShowWindow(GetDlgItem(hwndDlg, IDC_ALLTRANSFERRED), SW_HIDE);
	ShowWindow(GetDlgItem(hwndDlg, IDC_ALLSPEED), SW_HIDE);

	_strtime(buf);
	SetDlgItemTextA(hwndDlg, IDC_ALLPRECENTS, buf);

	PostMessage(GetParent(hwndDlg), WM_FT_RESIZE, 0, (LPARAM)hwndDlg);
}

static int FileTransferDlgResizer(HWND, LPARAM, UTILRESIZECONTROL *urc)
{
	switch (urc->wId) {
	case IDC_CONTACTNAME:
	case IDC_STATUS:
	case IDC_ALLFILESPROGRESS:
	case IDC_TRANSFERCOMPLETED:
		return RD_ANCHORX_WIDTH | RD_ANCHORY_TOP;
	case IDC_FRAME:
		return RD_ANCHORX_WIDTH | RD_ANCHORY_BOTTOM;
	case IDC_ALLPRECENTS:
	case IDCANCEL:
	case IDC_OPENFILE:
	case IDC_OPENFOLDER:
		return RD_ANCHORX_RIGHT | RD_ANCHORY_TOP;

	case IDC_ALLTRANSFERRED:
		urc->rcItem.right = urc->rcItem.left + (urc->rcItem.right - urc->rcItem.left - urc->dlgOriginalSize.cx + urc->dlgNewSize.cx) / 3;
		return RD_ANCHORX_CUSTOM | RD_ANCHORY_CUSTOM;

	case IDC_ALLSPEED:
		urc->rcItem.right = urc->rcItem.right - urc->dlgOriginalSize.cx + urc->dlgNewSize.cx;
		urc->rcItem.left = urc->rcItem.left + (urc->rcItem.right - urc->rcItem.left) / 3;
		return RD_ANCHORX_CUSTOM | RD_ANCHORY_CUSTOM;
	}
	return RD_ANCHORX_LEFT | RD_ANCHORY_TOP;
}

INT_PTR CALLBACK DlgProcFileTransfer(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	FileDlgData *dat = (FileDlgData*)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);

	switch (msg) {
	case WM_INITDIALOG:
		TranslateDialogDefault(hwndDlg);
		dat = (FileDlgData*)lParam;
		SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR)dat);
		dat->hNotifyEvent = HookEventMessage(ME_PROTO_ACK, hwndDlg, HM_RECVEVENT);
		dat->transferStatus.currentFileNumber = -1;
		if (dat->send) {
			if (db_mc_isMeta(dat->hContact))
				dat->hContact = db_mc_getMostOnline(dat->hContact);
			dat->fs = (HANDLE)ProtoChainSend(dat->hContact, PSS_FILE, (WPARAM)dat->szMsg, (LPARAM)dat->files);
			SetFtStatus(hwndDlg, LPGENW("Request sent, waiting for acceptance..."), FTS_TEXT);
			SetOpenFileButtonStyle(GetDlgItem(hwndDlg, IDC_OPENFILE), 1);
			dat->waitingForAcceptance = 1;
			// hide "open" button since it may cause potential access violations...
			ShowWindow(GetDlgItem(hwndDlg, IDC_OPENFILE), SW_HIDE);
			ShowWindow(GetDlgItem(hwndDlg, IDC_OPENFOLDER), SW_HIDE);
		}
		else {	//recv
			CreateDirectoryTreeW(dat->szSavePath);
			dat->fs = (HANDLE)ProtoChainSend(dat->hContact, PSS_FILEALLOW, (WPARAM)dat->fs, (LPARAM)dat->szSavePath);
			dat->transferStatus.tszWorkingDir = mir_wstrdup(dat->szSavePath);
			if (db_get_b(dat->hContact, "CList", "NotOnList", 0)) dat->resumeBehaviour = FILERESUME_ASK;
			else dat->resumeBehaviour = db_get_b(NULL, "SRFile", "IfExists", FILERESUME_ASK);
			SetFtStatus(hwndDlg, LPGENW("Waiting for connection..."), FTS_TEXT);
		}

		/* check we actually got an fs handle back from the protocol */
		if (!dat->fs) {
			SetFtStatus(hwndDlg, LPGENW("Unable to initiate transfer."), FTS_TEXT);
			dat->waitingForAcceptance = 0;
		}
		{
			LOGFONT lf;
			HFONT hFont = (HFONT)SendDlgItemMessage(hwndDlg, IDC_CONTACTNAME, WM_GETFONT, 0, 0);
			GetObject(hFont, sizeof(lf), &lf);
			lf.lfWeight = FW_BOLD;
			hFont = CreateFontIndirect(&lf);
			SendDlgItemMessage(hwndDlg, IDC_CONTACTNAME, WM_SETFONT, (WPARAM)hFont, 0);

			SHFILEINFO shfi = { 0 };
			SHGetFileInfo(L"", FILE_ATTRIBUTE_DIRECTORY, &shfi, sizeof(shfi), SHGFI_USEFILEATTRIBUTES | SHGFI_ICON | SHGFI_SMALLICON);
			dat->hIconFolder = shfi.hIcon;
		}
		dat->hIcon = NULL;
		{
			char *szProto = GetContactProto(dat->hContact);
			WORD status = db_get_w(dat->hContact, szProto, "Status", ID_STATUS_ONLINE);
			SendDlgItemMessage(hwndDlg, IDC_CONTACT, BM_SETIMAGE, IMAGE_ICON, (LPARAM)Skin_LoadProtoIcon(szProto, status));
		}

		SendDlgItemMessage(hwndDlg, IDC_CONTACT, BUTTONADDTOOLTIP, (WPARAM)LPGEN("Contact menu"), 0);
		SendDlgItemMessage(hwndDlg, IDC_CONTACT, BUTTONSETASFLATBTN, TRUE, 0);

		Button_SetIcon_IcoLib(hwndDlg, IDC_OPENFILE, SKINICON_OTHER_DOWNARROW, LPGEN("Open..."));
		SendDlgItemMessage(hwndDlg, IDC_OPENFILE, BUTTONSETASPUSHBTN, TRUE, 0);

		SendDlgItemMessage(hwndDlg, IDC_OPENFOLDER, BM_SETIMAGE, IMAGE_ICON, (LPARAM)dat->hIconFolder);
		SendDlgItemMessage(hwndDlg, IDC_OPENFOLDER, BUTTONADDTOOLTIP, (WPARAM)LPGEN("Open folder"), 0);
		SendDlgItemMessage(hwndDlg, IDC_OPENFOLDER, BUTTONSETASFLATBTN, TRUE, 0);

		Button_SetIcon_IcoLib(hwndDlg, IDCANCEL, SKINICON_OTHER_DELETE, LPGEN("Cancel"));

		SetDlgItemText(hwndDlg, IDC_CONTACTNAME, pcli->pfnGetContactDisplayName(dat->hContact, 0));

		if (!dat->waitingForAcceptance) SetTimer(hwndDlg, 1, 1000, NULL);
		return TRUE;

	case WM_TIMER:
		memmove(dat->bytesRecvedHistory + 1, dat->bytesRecvedHistory, sizeof(dat->bytesRecvedHistory) - sizeof(dat->bytesRecvedHistory[0]));
		dat->bytesRecvedHistory[0] = dat->transferStatus.totalProgress;
		if (dat->bytesRecvedHistorySize < _countof(dat->bytesRecvedHistory))
			dat->bytesRecvedHistorySize++;

		{
			wchar_t szSpeed[32], szTime[32], szDisplay[96];
			SYSTEMTIME st;
			ULARGE_INTEGER li;
			FILETIME ft;

			GetSensiblyFormattedSize((dat->bytesRecvedHistory[0] - dat->bytesRecvedHistory[dat->bytesRecvedHistorySize - 1]) / dat->bytesRecvedHistorySize, szSpeed, _countof(szSpeed), 0, 1, NULL);
			if (dat->bytesRecvedHistory[0] == dat->bytesRecvedHistory[dat->bytesRecvedHistorySize - 1])
				mir_wstrcpy(szTime, L"??:??:??");
			else {
				li.QuadPart = BIGI(10000000)*(dat->transferStatus.currentFileSize - dat->transferStatus.currentFileProgress)*dat->bytesRecvedHistorySize / (dat->bytesRecvedHistory[0] - dat->bytesRecvedHistory[dat->bytesRecvedHistorySize - 1]);
				ft.dwHighDateTime = li.HighPart; ft.dwLowDateTime = li.LowPart;
				FileTimeToSystemTime(&ft, &st);
				GetTimeFormat(LOCALE_USER_DEFAULT, TIME_FORCE24HOURFORMAT | TIME_NOTIMEMARKER, &st, NULL, szTime, _countof(szTime));
			}
			if (dat->bytesRecvedHistory[0] != dat->bytesRecvedHistory[dat->bytesRecvedHistorySize - 1]) {
				li.QuadPart = BIGI(10000000)*(dat->transferStatus.totalBytes - dat->transferStatus.totalProgress)*dat->bytesRecvedHistorySize / (dat->bytesRecvedHistory[0] - dat->bytesRecvedHistory[dat->bytesRecvedHistorySize - 1]);
				ft.dwHighDateTime = li.HighPart; ft.dwLowDateTime = li.LowPart;
				FileTimeToSystemTime(&ft, &st);
				GetTimeFormat(LOCALE_USER_DEFAULT, TIME_FORCE24HOURFORMAT | TIME_NOTIMEMARKER, &st, NULL, szTime, _countof(szTime));
			}

			mir_snwprintf(szDisplay, L"%s/%s  (%s %s)", szSpeed, TranslateT("sec"), szTime, TranslateT("remaining"));
			SetDlgItemText(hwndDlg, IDC_ALLSPEED, szDisplay);
		}
		break;

	case WM_MEASUREITEM:
		return Menu_MeasureItem((LPMEASUREITEMSTRUCT)lParam);

	case WM_DRAWITEM:
		return Menu_DrawItem((LPDRAWITEMSTRUCT)lParam);

	case WM_FT_CLEANUP:
		if (!dat->fs) {
			PostMessage(GetParent(hwndDlg), WM_FT_REMOVE, 0, (LPARAM)hwndDlg);
			DestroyWindow(hwndDlg);
		}
		break;

	case WM_COMMAND:
		if (!dat)
			break;

		if (Clist_MenuProcessCommand(LOWORD(wParam), MPCF_CONTACTMENU, dat->hContact))
			break;

		switch (LOWORD(wParam)) {
		case IDOK:
		case IDCANCEL:
			PostMessage(GetParent(hwndDlg), WM_FT_REMOVE, 0, (LPARAM)hwndDlg);
			DestroyWindow(hwndDlg);
			break;

		case IDC_CONTACT:
			{
				RECT rc;
				HMENU hMenu = Menu_BuildContactMenu(dat->hContact);
				GetWindowRect((HWND)lParam, &rc);
				TrackPopupMenu(hMenu, 0, rc.left, rc.bottom, 0, hwndDlg, NULL);
				DestroyMenu(hMenu);
			}
			break;

		case IDC_TRANSFERCOMPLETED:
			if (dat->transferStatus.currentFileNumber <= 1 && CheckVirusScanned(hwndDlg, dat, 0)) {
				ShellExecute(NULL, NULL, dat->files[0], NULL, NULL, SW_SHOW);
				break;
			}

		case IDC_OPENFOLDER:
			{
				wchar_t *path = dat->transferStatus.tszWorkingDir;
				if (!path || !path[0]) {
					path = NEWWSTR_ALLOCA(dat->transferStatus.tszCurrentFile);
					wchar_t *p = wcsrchr(path, '\\'); if (p) *p = 0;
				}

				if (path) ShellExecute(NULL, L"open", path, NULL, NULL, SW_SHOW);
			}
			break;

		case IDC_OPENFILE:
			wchar_t **files;
			if (dat->send) {
				if (dat->files == NULL)
					files = dat->transferStatus.ptszFiles;
				else
					files = dat->files;
			}
			else files = dat->files;

			HMENU hMenu = CreatePopupMenu();
			AppendMenu(hMenu, MF_STRING, 1, TranslateT("Open folder"));
			AppendMenu(hMenu, MF_SEPARATOR, 0, 0);

			if (files && *files) {
				int limit;
				wchar_t *pszFilename, *pszNewFileName;

				if (dat->send)
					limit = dat->transferStatus.totalFiles;
				else
					limit = dat->transferStatus.currentFileNumber;

				// Loop over all transfered files and add them to the menu
				for (int i = 0; i < limit; i++) {
					pszFilename = wcsrchr(files[i], '\\');
					if (pszFilename == NULL)
						pszFilename = files[i];
					else
						pszFilename++;

					if (pszFilename) {
						size_t cbFileNameLen = mir_wstrlen(pszFilename);

						pszNewFileName = (wchar_t*)mir_alloc(cbFileNameLen * 2 * sizeof(wchar_t));
						wchar_t *p = pszNewFileName;
						for (size_t pszlen = 0; pszlen < cbFileNameLen; pszlen++) {
							*p++ = pszFilename[pszlen];
							if (pszFilename[pszlen] == '&')
								*p++ = '&';
						}
						*p = '\0';
						AppendMenu(hMenu, MF_STRING, i + 10, pszNewFileName);
						mir_free(pszNewFileName);
					}
				}
			}

			RECT rc;
			GetWindowRect((HWND)lParam, &rc);
			CheckDlgButton(hwndDlg, IDC_OPENFILE, BST_CHECKED);
			int ret = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_RIGHTALIGN, rc.right, rc.bottom, 0, hwndDlg, NULL);
			CheckDlgButton(hwndDlg, IDC_OPENFILE, BST_UNCHECKED);
			DestroyMenu(hMenu);

			if (ret == 1) {
				wchar_t *path = dat->transferStatus.tszWorkingDir;
				if (!path || !path[0]) {
					path = NEWWSTR_ALLOCA(dat->transferStatus.tszCurrentFile);
					wchar_t *p = wcsrchr(path, '\\');
					if (p)
						*p = 0;
				}

				if (path) ShellExecute(NULL, L"open", path, NULL, NULL, SW_SHOW);
			}
			else if (ret && CheckVirusScanned(hwndDlg, dat, ret))
				ShellExecute(NULL, NULL, files[ret - 10], NULL, NULL, SW_SHOW);
		}
		break;
	
	case M_FILEEXISTSDLGREPLY:
		EnableWindow(hwndDlg, TRUE);
		{
			PROTOFILERESUME *pfr = (PROTOFILERESUME*)lParam;
			wchar_t *szOriginalFilename = (wchar_t*)wParam;
			char *szProto = GetContactProto(dat->hContact);

			switch (pfr->action) {
			case FILERESUME_CANCEL:
				if (dat->fs) ProtoChainSend(dat->hContact, PSS_FILECANCEL, (WPARAM)dat->fs, 0);
				dat->fs = NULL;
				mir_free(szOriginalFilename);
				if (pfr->szFilename) mir_free((char*)pfr->szFilename);
				mir_free(pfr);
				return 0;
			case FILERESUME_RESUMEALL:
			case FILERESUME_OVERWRITEALL:
				dat->resumeBehaviour = pfr->action;
				pfr->action &= ~FILERESUMEF_ALL;
				break;
			case FILERESUME_RENAMEALL:
				pfr->action = FILERESUME_RENAME;
				{
					wchar_t *pszExtension, *pszFilename;
					if ((pszFilename = wcsrchr(szOriginalFilename, '\\')) == NULL) pszFilename = szOriginalFilename;
					if ((pszExtension = wcsrchr(pszFilename + 1, '.')) == NULL) pszExtension = pszFilename + mir_wstrlen(pszFilename);
					if (pfr->szFilename) mir_free((wchar_t*)pfr->szFilename);
					size_t size = (pszExtension - szOriginalFilename) + 21 + mir_wstrlen(pszExtension);
					pfr->szFilename = (wchar_t*)mir_alloc(sizeof(wchar_t)*size);
					for (int i = 1;; i++) {
						mir_snwprintf((wchar_t*)pfr->szFilename, size, L"%.*s (%u)%s", pszExtension - szOriginalFilename, szOriginalFilename, i, pszExtension);
						if (_waccess(pfr->szFilename, 0) != 0)
							break;
					}
				}
				break;
			}
			mir_free(szOriginalFilename);
			CallProtoService(szProto, PS_FILERESUME, (WPARAM)dat->fs, (LPARAM)pfr);
			if (pfr->szFilename) mir_free((char*)pfr->szFilename);
			mir_free(pfr);
		}
		break;

	case HM_RECVEVENT:
		{
			ACKDATA *ack = (ACKDATA*)lParam;
			if (ack->hProcess != dat->fs) break;
			if (ack->type != ACKTYPE_FILE) break;
			if (ack->hContact != dat->hContact) break;

			if (dat->waitingForAcceptance) {
				SetTimer(hwndDlg, 1, 1000, NULL);
				dat->waitingForAcceptance = 0;
			}

			switch (ack->result) {
			case ACKRESULT_SENTREQUEST: SetFtStatus(hwndDlg, LPGENW("Decision sent"), FTS_TEXT); break;
			case ACKRESULT_CONNECTING: SetFtStatus(hwndDlg, LPGENW("Connecting..."), FTS_TEXT); break;
			case ACKRESULT_CONNECTPROXY: SetFtStatus(hwndDlg, LPGENW("Connecting to proxy..."), FTS_TEXT); break;
			case ACKRESULT_CONNECTED: SetFtStatus(hwndDlg, LPGENW("Connected"), FTS_TEXT); break;
			case ACKRESULT_LISTENING: SetFtStatus(hwndDlg, LPGENW("Waiting for connection..."), FTS_TEXT); break;
			case ACKRESULT_INITIALISING: SetFtStatus(hwndDlg, LPGENW("Initializing..."), FTS_TEXT); break;
			case ACKRESULT_NEXTFILE:
				SetFtStatus(hwndDlg, LPGENW("Moving to next file..."), FTS_TEXT);
				SetDlgItemTextA(hwndDlg, IDC_FILENAME, "");
				if (dat->transferStatus.currentFileNumber == 1 && dat->transferStatus.totalFiles > 1 && !dat->send)
					SetOpenFileButtonStyle(GetDlgItem(hwndDlg, IDC_OPENFILE), 1);
				if (dat->transferStatus.currentFileNumber != -1 && dat->files && !dat->send && db_get_b(NULL, "SRFile", "UseScanner", VIRUSSCAN_DISABLE) == VIRUSSCAN_DURINGDL) {
					if (GetFileAttributes(dat->files[dat->transferStatus.currentFileNumber])&FILE_ATTRIBUTE_DIRECTORY)
						PostMessage(hwndDlg, M_VIRUSSCANDONE, dat->transferStatus.currentFileNumber, 0);
					else {
						virusscanthreadstartinfo *vstsi;
						vstsi = (struct virusscanthreadstartinfo*)mir_alloc(sizeof(struct virusscanthreadstartinfo));
						vstsi->hwndReply = hwndDlg;
						vstsi->szFile = mir_wstrdup(dat->files[dat->transferStatus.currentFileNumber]);
						vstsi->returnCode = dat->transferStatus.currentFileNumber;
						mir_forkthread((void(*)(void*))RunVirusScannerThread, vstsi);
					}
				}
				break;

			case ACKRESULT_FILERESUME:
				UpdateProtoFileTransferStatus(&dat->transferStatus, (PROTOFILETRANSFERSTATUS*)ack->lParam);
				{
					PROTOFILETRANSFERSTATUS *fts = &dat->transferStatus;
					SetFilenameControls(hwndDlg, dat, fts);
					if (_waccess(fts->tszCurrentFile, 0))
						break;

					SetFtStatus(hwndDlg, LPGENW("File already exists"), FTS_TEXT);
					if (dat->resumeBehaviour == FILERESUME_ASK) {
						TDlgProcFileExistsParam param = { hwndDlg, fts };
						ShowWindow(hwndDlg, SW_SHOWNORMAL);
						CreateDialogParam(hInst, MAKEINTRESOURCE(IDD_FILEEXISTS), hwndDlg, DlgProcFileExists, (LPARAM)&param);
						EnableWindow(hwndDlg, FALSE);
					}
					else {
						PROTOFILERESUME *pfr = (PROTOFILERESUME*)mir_alloc(sizeof(PROTOFILERESUME));
						pfr->action = dat->resumeBehaviour;
						pfr->szFilename = NULL;
						PostMessage(hwndDlg, M_FILEEXISTSDLGREPLY, (WPARAM)mir_wstrdup(fts->tszCurrentFile), (LPARAM)pfr);
					}
				}
				SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, 1);
				return TRUE;

			case ACKRESULT_DATA:
				{
					PROTOFILETRANSFERSTATUS *fts = (PROTOFILETRANSFERSTATUS*)ack->lParam;
					wchar_t str[64], str2[64], szSizeDone[32], szSizeTotal[32];//, *contactName;

					if (dat->fileVirusScanned == NULL)
						dat->fileVirusScanned = (int*)mir_calloc(sizeof(int) * fts->totalFiles);

					// This needs to be here - otherwise we get holes in the files array
					if (!dat->send) {
						if (dat->files == NULL)
							dat->files = (wchar_t**)mir_calloc((fts->totalFiles + 1) * sizeof(wchar_t*));
						if (fts->currentFileNumber < fts->totalFiles && dat->files[fts->currentFileNumber] == NULL)
							dat->files[fts->currentFileNumber] = PFTS_StringToTchar(fts->flags, fts->tszCurrentFile);
					}

					/* HACK: for 0.3.3, limit updates to around 1.1 ack per second */
					if (fts->totalProgress != fts->totalBytes && GetTickCount() < (dat->dwTicks + 650))
						break; // the last update was less than a second ago!
					dat->dwTicks = GetTickCount();

					// Update local transfer status with data from protocol
					UpdateProtoFileTransferStatus(&dat->transferStatus, fts);
					fts = &dat->transferStatus;

					bool firstTime = false;
					if ((GetWindowLongPtr(GetDlgItem(hwndDlg, IDC_ALLFILESPROGRESS), GWL_STYLE) & WS_VISIBLE) == 0) {
						SetFtStatus(hwndDlg, (fts->flags & PFTS_SENDING) ? LPGENW("Sending...") : LPGENW("Receiving..."), FTS_PROGRESS);
						SetFilenameControls(hwndDlg, dat, fts);
						firstTime = true;
					}

					const unsigned long lastPos = SendDlgItemMessage(hwndDlg, IDC_ALLFILESPROGRESS, PBM_GETPOS, 0, 0);
					const unsigned long nextPos = fts->totalBytes ? (BIGI(100) * fts->totalProgress / fts->totalBytes) : 0;
					if (lastPos != nextPos || firstTime) {
						SendDlgItemMessage(hwndDlg, IDC_ALLFILESPROGRESS, PBM_SETPOS, nextPos, 0);
						mir_snwprintf(str, L"%u%%", nextPos);
						SetDlgItemText(hwndDlg, IDC_ALLPRECENTS, str);
					}

					int units;
					GetSensiblyFormattedSize(fts->totalBytes, szSizeTotal, _countof(szSizeTotal), 0, 1, &units);
					GetSensiblyFormattedSize(fts->totalProgress, szSizeDone, _countof(szSizeDone), units, 0, NULL);
					mir_snwprintf(str, L"%s/%s", szSizeDone, szSizeTotal);
					str2[0] = 0;
					GetDlgItemText(hwndDlg, IDC_ALLTRANSFERRED, str2, _countof(str2));
					if (mir_wstrcmp(str, str2))
						SetDlgItemText(hwndDlg, IDC_ALLTRANSFERRED, str);
				}
				break;

			case ACKRESULT_SUCCESS:
			case ACKRESULT_FAILED:
			case ACKRESULT_DENIED:
				HideProgressControls(hwndDlg);
				KillTimer(hwndDlg, 1);
				if (!dat->send)
					SetOpenFileButtonStyle(GetDlgItem(hwndDlg, IDC_OPENFILE), 1);
				SetDlgItemText(hwndDlg, IDCANCEL, TranslateT("Close"));
				if (dat->hNotifyEvent)
					UnhookEvent(dat->hNotifyEvent);
				dat->hNotifyEvent = NULL;

				if (ack->result == ACKRESULT_DENIED) {
					dat->fs = NULL; /* protocol will free structure */
					SkinPlaySound("FileDenied");
					SetFtStatus(hwndDlg, LPGENW("File transfer denied"), FTS_TEXT);
				}
				else if (ack->result == ACKRESULT_FAILED) {
					dat->fs = NULL; /* protocol will free structure */
					SkinPlaySound("FileFailed");
					SetFtStatus(hwndDlg, LPGENW("File transfer failed"), FTS_TEXT);
				}
				else {
					SkinPlaySound("FileDone");
					if (dat->send) {
						dat->fs = NULL; /* protocol will free structure */
						SetFtStatus(hwndDlg, LPGENW("Transfer completed."), FTS_TEXT);

						DBEVENTINFO dbei = { 0 };
						FillSendData(dat, dbei);
						db_event_add(dat->hContact, &dbei);
						if (dbei.pBlob)
							mir_free(dbei.pBlob);
						dat->files = NULL;   //protocol library frees this
					}
					else {
						SetFtStatus(hwndDlg,
							(dat->transferStatus.totalFiles == 1) ?
							LPGENW("Transfer completed, open file.") :
							LPGENW("Transfer completed, open folder."),
							FTS_OPEN);

						int useScanner = db_get_b(NULL, "SRFile", "UseScanner", VIRUSSCAN_DISABLE);
						if (useScanner != VIRUSSCAN_DISABLE) {
							struct virusscanthreadstartinfo *vstsi;
							vstsi = (struct virusscanthreadstartinfo*)mir_alloc(sizeof(struct virusscanthreadstartinfo));
							vstsi->hwndReply = hwndDlg;
							if (useScanner == VIRUSSCAN_DURINGDL) {
								vstsi->returnCode = dat->transferStatus.currentFileNumber;
								if (GetFileAttributes(dat->files[dat->transferStatus.currentFileNumber])&FILE_ATTRIBUTE_DIRECTORY) {
									PostMessage(hwndDlg, M_VIRUSSCANDONE, vstsi->returnCode, 0);
									mir_free(vstsi);
									vstsi = NULL;
								}
								else vstsi->szFile = mir_wstrdup(dat->files[dat->transferStatus.currentFileNumber]);
							}
							else {
								vstsi->szFile = mir_wstrdup(dat->transferStatus.tszWorkingDir);
								vstsi->returnCode = -1;
							}
							SetFtStatus(hwndDlg, LPGENW("Scanning for viruses..."), FTS_TEXT);
							if (vstsi)
								mir_forkthread((void(*)(void*))RunVirusScannerThread, vstsi);
						}
						else dat->fs = NULL; /* protocol will free structure */

						dat->transferStatus.currentFileNumber = dat->transferStatus.totalFiles;
					} // else dat->send

				} // else ack->result

				PostMessage(GetParent(hwndDlg), WM_FT_COMPLETED, ack->result, (LPARAM)hwndDlg);
				break;
			} // switch ack->result
		} // case HM_RECVEVENT
		break; 

	case M_VIRUSSCANDONE:
		{
			int done = 1;
			if ((int)wParam == -1) {
				for (int i = 0; i < dat->transferStatus.totalFiles; i++)
					dat->fileVirusScanned[i] = 1;
			}
			else {
				dat->fileVirusScanned[wParam] = 1;
				for (int i = 0; i < dat->transferStatus.totalFiles; i++) 
					if (!dat->fileVirusScanned[i]) {
						done = 0;
						break;
					}
			}
			if (done) {
				dat->fs = NULL; /* protocol will free structure */
				SetFtStatus(hwndDlg, LPGENW("Transfer and virus scan complete"), FTS_TEXT);
			}
		}
		break;

	case WM_SIZE:
		Utils_ResizeDialog(hwndDlg, hInst, MAKEINTRESOURCEA(IDD_FILETRANSFERINFO), FileTransferDlgResizer);

		RedrawWindow(GetDlgItem(hwndDlg, IDC_ALLTRANSFERRED), NULL, NULL, RDW_INVALIDATE | RDW_NOERASE);
		RedrawWindow(GetDlgItem(hwndDlg, IDC_ALLSPEED), NULL, NULL, RDW_INVALIDATE | RDW_NOERASE);
		RedrawWindow(GetDlgItem(hwndDlg, IDC_CONTACTNAME), NULL, NULL, RDW_INVALIDATE | RDW_NOERASE);
		RedrawWindow(GetDlgItem(hwndDlg, IDC_STATUS), NULL, NULL, RDW_INVALIDATE | RDW_NOERASE);
		break;

	case WM_DESTROY:
		KillTimer(hwndDlg, 1);

		HFONT hFont = (HFONT)SendDlgItemMessage(hwndDlg, IDC_CONTACTNAME, WM_GETFONT, 0, 0);
		DeleteObject(hFont);

		Button_FreeIcon_IcoLib(hwndDlg, IDC_CONTACT);
		Button_FreeIcon_IcoLib(hwndDlg, IDC_OPENFILE);
		Button_FreeIcon_IcoLib(hwndDlg, IDCANCEL);

		FreeFileDlgData(dat);
		SetWindowLongPtr(hwndDlg, GWLP_USERDATA, 0);
		break;
	}
	return FALSE;
}

void FreeFileDlgData(FileDlgData* dat)
{
	if (dat == NULL)
		return;

	if (dat->fs)
		ProtoChainSend(dat->hContact, PSS_FILECANCEL, (WPARAM)dat->fs, 0);
	if (dat->hPreshutdownEvent)
		UnhookEvent(dat->hPreshutdownEvent);
	if (dat->hNotifyEvent)
		UnhookEvent(dat->hNotifyEvent);

	FreeProtoFileTransferStatus(&dat->transferStatus);
	FreeFilesMatrix(&dat->files);

	mir_free(dat->fileVirusScanned);
	if (dat->hIcon)
		DestroyIcon(dat->hIcon);
	if (dat->hIconFolder)
		DestroyIcon(dat->hIconFolder);
	mir_free(dat);
}
