/* 
Copyright (C) 2012 Mataes

This is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with this file; see the file license.txt.  If
not, write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  
*/

#include "common.h"

int ImportFeedsDialog()
{
	TCHAR FileName[MAX_PATH];
	TCHAR *tszMirDir = Utils_ReplaceVarsT(_T("%miranda_path%"));

	OPENFILENAME ofn = {0};
	ofn.lStructSize = sizeof(ofn);
	TCHAR tmp[MAX_PATH];
	mir_sntprintf(tmp, SIZEOF(tmp), _T("%s (*.opml, *.xml)%c*.opml;*.xml%c%c"), TranslateT("OPML files"), 0, 0, 0);
	ofn.lpstrFilter = tmp;
	ofn.hwndOwner = 0;
	ofn.lpstrFile = FileName;
	ofn.nMaxFile = MAX_PATH;
	ofn.nMaxFileTitle = MAX_PATH;
	ofn.Flags = OFN_HIDEREADONLY;
	ofn.lpstrInitialDir = tszMirDir;
	*FileName = '\0';
	ofn.lpstrDefExt = _T("");

	if (GetOpenFileName(&ofn)) {
		int bytesParsed = 0;
		HXML hXml = xi.parseFile(FileName, &bytesParsed, NULL);
		if(hXml != NULL) {
			BYTE isTitleUTF = 0, isURLUTF = 0, isSiteURLUTF = 0, isGroupUTF = 0;
			HXML node = xi.getChildByPath(hXml, _T("opml/body/outline"), 0);
			if ( !node)
				node = xi.getChildByPath(hXml, _T("body/outline"), 0);
			if (node) {
				while (node) {
					int outlineAttr = xi.getAttrCount(node);
					int outlineChildsCount = xi.getChildCount(node);
					TCHAR *type = (TCHAR *)xi.getAttrValue(node, _T("type"));
					if ( !type && !outlineChildsCount) {
						HXML tmpnode = node;
						node = xi.getNextNode(node);
						if ( !node) {
							do {
								node = tmpnode;
								node = xi.getParent(node);
								tmpnode = node;
								node = xi.getNextNode(node);
								if (node)
									break;
							} while (lstrcmpi(xi.getName(node), _T("body")));
						}
					}
					else if (!type && outlineChildsCount)
						node = xi.getFirstChild(node);
					else if (type) {
						TCHAR *title = NULL, *url = NULL, *siteurl = NULL, *group = NULL, *utfgroup = NULL;
						for (int i = 0; i < outlineAttr; i++) {
							if (!lstrcmpi(xi.getAttrName(node, i), _T("title"))) {
								title = mir_utf8decodeT(_T2A(xi.getAttrValue(node, xi.getAttrName(node, i))));
								if ( !title) {
									isTitleUTF = 0;
									title = (TCHAR *)xi.getAttrValue(node, xi.getAttrName(node, i));
								} else
									isTitleUTF = 1;
								continue;
							}
							if (!lstrcmpi(xi.getAttrName(node, i), _T("xmlUrl"))) {
								url = mir_utf8decodeT(_T2A(xi.getAttrValue(node, xi.getAttrName(node, i))));
								if ( !url) {
									isURLUTF = 0;
									url = (TCHAR *)xi.getAttrValue(node, xi.getAttrName(node, i));
								} else
									isURLUTF = 1;
								continue;
							}
							if (!lstrcmpi(xi.getAttrName(node, i), _T("htmlUrl"))) {
								siteurl = mir_utf8decodeT(_T2A(xi.getAttrValue(node, xi.getAttrName(node, i))));
								if ( !siteurl) {
									isSiteURLUTF = 0;
									siteurl = (TCHAR *)xi.getAttrValue(node, xi.getAttrName(node, i));
								} else
									isSiteURLUTF = 1;
								continue;
							}
							if (title && url && siteurl)
								break;
						}

						HXML parent = xi.getParent(node);
						while (lstrcmpi(xi.getName(parent), _T("body"))) {
							for (int i = 0; i < xi.getAttrCount(parent); i++) {
								if (!lstrcmpi(xi.getAttrName(parent, i), _T("title"))) {
									if ( !group)
										group = (TCHAR *)xi.getAttrValue(parent, xi.getAttrName(parent, i));
									else {
										TCHAR tmpgroup[1024];
										mir_sntprintf(tmpgroup, SIZEOF(tmpgroup), _T("%s\\%s"), xi.getAttrValue(parent, xi.getAttrName(parent, i)), group);
										group = tmpgroup;
									}
									break;
								}
							}
							parent = xi.getParent(parent);
						}

						if (group) {
							utfgroup = mir_utf8decodeT(_T2A(group));
							if ( !utfgroup) {
								isGroupUTF = 0;
								utfgroup = group;
							} else
								isGroupUTF = 1;
						}

						HANDLE hContact = (HANDLE)CallService(MS_DB_CONTACT_ADD, 0, 0);
						CallService(MS_PROTO_ADDTOCONTACT, (WPARAM)hContact, (LPARAM)MODULE);
						db_set_ts(hContact, MODULE, "Nick", title);
						db_set_ts(hContact, MODULE, "URL", url);
						db_set_ts(hContact, MODULE, "Homepage", siteurl);
						db_set_b(hContact, MODULE, "CheckState", 1);
						db_set_dw(hContact, MODULE, "UpdateTime", DEFAULT_UPDATE_TIME);
						db_set_ts(hContact, MODULE, "MsgFormat", _T(TAGSDEFAULT));
						db_set_w(hContact, MODULE, "Status", CallProtoService(MODULE, PS_GETSTATUS, 0, 0));
						if (utfgroup) {
							db_set_ts(hContact, "CList", "Group", utfgroup);
							int hGroup = 1;
							char *group_name;
							BYTE GroupExist = 0;
							do {
								group_name = (char *)CallService(MS_CLIST_GROUPGETNAME, (WPARAM)hGroup, 0);
								if (group_name != NULL && !strcmp(group_name, _T2A(utfgroup))) {
									GroupExist = 1;
									break;
								}
								hGroup++;
							}
								while (group_name);

							if(!GroupExist)
								CallService(MS_CLIST_GROUPCREATE, 0, (LPARAM)utfgroup);
						}
						if (isTitleUTF)
							mir_free(title);
						if (isURLUTF)
							mir_free(url);
						if (isGroupUTF)
							mir_free(utfgroup);
						if (isSiteURLUTF)
							mir_free(siteurl);

						HXML tmpnode = node;
						node = xi.getNextNode(node);
						if ( !node) {
							do {
								node = tmpnode;
								node = xi.getParent(node);
								tmpnode = node;
								node = xi.getNextNode(node);
								if (node)
									break;
							}
								while (lstrcmpi(xi.getName(tmpnode), _T("body")));
						}
					}
				}
			}
			else MessageBox(NULL, TranslateT("Not valid import file."), TranslateT("Error"), MB_OK | MB_ICONERROR);
			xi.destroyNode(hXml);
		}
		else MessageBox(NULL, TranslateT("Not valid import file."), TranslateT("Error"), MB_OK | MB_ICONERROR);

		mir_free(tszMirDir);
		return 0;
	}
	mir_free(tszMirDir);
	return 1;
}

VOID ExportFeedsDialog()
{
	TCHAR FileName[MAX_PATH];
	TCHAR *tszMirDir = Utils_ReplaceVarsT(_T("%miranda_path%"));

	OPENFILENAME ofn = {0};
	ofn.lStructSize = sizeof(ofn);
	TCHAR tmp[MAX_PATH];
	mir_sntprintf(tmp, SIZEOF(tmp), _T("%s (*.opml)%c*.opml%c%c"), TranslateT("OPML files"), 0, 0, 0);
	ofn.lpstrFilter = tmp;
	ofn.hwndOwner = 0;
	ofn.lpstrFile = FileName;
	ofn.nMaxFile = MAX_PATH;
	ofn.nMaxFileTitle = MAX_PATH;
	ofn.Flags = OFN_HIDEREADONLY | OFN_SHAREAWARE | OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
	ofn.lpstrInitialDir = tszMirDir;
	*FileName = '\0';
	ofn.lpstrDefExt = _T("");

	if (GetSaveFileName(&ofn)) {
		HXML hXml = xi.createNode(_T("opml"), NULL, FALSE);
		xi.addAttr(hXml, _T("version"), _T("1.0"));
		HXML header = xi.addChild(hXml, _T("head"), NULL);
		HXML title = xi.addChild(header, _T("title"), _T("Miranda NG NewsAggregator plugin export"));
		header = xi.addChild(hXml, _T("body"), NULL);

		for (HANDLE hContact = db_find_first(MODULE); hContact; hContact = db_find_next(hContact, MODULE)) {
			TCHAR *title = NULL, *url = NULL, *siteurl = NULL, *group = NULL;
			DBVARIANT dbv = {0};
			if (!db_get_ts(hContact, MODULE, "Nick", &dbv)) {
				title = mir_tstrdup(dbv.ptszVal);
				db_free(&dbv);
			}
			if (!db_get_ts(hContact, MODULE, "URL", &dbv)) {
				url = mir_tstrdup(dbv.ptszVal);
				db_free(&dbv);
			}
			if (!db_get_ts(hContact, MODULE, "Homepage", &dbv)) {
				siteurl = mir_tstrdup(dbv.ptszVal);
				db_free(&dbv);
			}
			if (!db_get_ts(hContact, "CList", "Group", &dbv)) {
				group = mir_tstrdup(dbv.ptszVal);
				db_free(&dbv);
			}
			HXML elem = header;
			if (group)
			{
				TCHAR *section = _tcstok(group, _T("\\"));
				while (section != NULL)
				{
					HXML existgroup = xi.getChildByAttrValue(header, _T("outline"), _T("title"), section);
					if ( !existgroup)
					{
						elem = xi.addChild(elem, _T("outline"), NULL);
						xi.addAttr(elem, _T("title"), section);
						xi.addAttr(elem, _T("text"), section);
					} else {
						elem = existgroup;
					}
					section = _tcstok(NULL, _T("\\"));
				}
				elem = xi.addChild(elem, _T("outline"), NULL);
			} else
				elem = xi.addChild(elem, _T("outline"), NULL);
			xi.addAttr(elem, _T("text"), title);
			xi.addAttr(elem, _T("title"), title);
			xi.addAttr(elem, _T("type"), _T("rss"));
			xi.addAttr(elem, _T("xmlUrl"), url);
			xi.addAttr(elem, _T("htmlUrl"), siteurl);

			mir_free(title);
			mir_free(url);
			mir_free(siteurl);
			mir_free(group);
		}
		xi.toFile(hXml, FileName, 1);
		xi.destroyNode(hXml);
	}
}

INT_PTR CALLBACK DlgProcImportOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_INITDIALOG:
		TranslateDialogDefault(hwndDlg);
		SetWindowLongPtr(hwndDlg, GWLP_USERDATA, lParam);
		Utils_RestoreWindowPositionNoSize(hwndDlg, NULL, MODULE, "ImportDlg");
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			{
			}

		case IDCANCEL:
			DestroyWindow(hwndDlg);
			break;

		}
		break;

	case WM_CLOSE:
		DestroyWindow(hwndDlg);
		break;

	case WM_DESTROY:
		Utils_SaveWindowPosition(hwndDlg, NULL, MODULE, "ImportDlg");
		break;
	}

	return FALSE;
}

INT_PTR CALLBACK DlgProcExportOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	HWND FeedsList = GetDlgItem(hwndDlg, IDC_FEEDSLIST);
	HWND FeedsExportList = GetDlgItem(hwndDlg, IDC_FEEDSEXPORTLIST);

	switch (msg) {
	case WM_INITDIALOG:
		TranslateDialogDefault(hwndDlg);
		Utils_RestoreWindowPositionNoSize(hwndDlg, NULL, MODULE, "ExportDlg");
		for (HANDLE hContact = db_find_first(MODULE); hContact; hContact = db_find_next(hContact, MODULE)) {
			DBVARIANT dbVar = {0};
			if (!db_get_ts(hContact, MODULE, "Nick", &dbVar)) {
				SendMessage(FeedsList, LB_ADDSTRING, 0, (LPARAM)dbVar.ptszVal);
				db_free(&dbVar);
			}
		}
		EnableWindow(GetDlgItem(hwndDlg, IDC_REMOVEFEED), FALSE);
		if (SendMessage(FeedsList, LB_GETCURSEL, 0, 0))
		{
			EnableWindow(GetDlgItem(hwndDlg, IDC_ADDFEED), FALSE);
			EnableWindow(GetDlgItem(hwndDlg, IDC_BROWSEEXPORTFILE), FALSE);
			EnableWindow(GetDlgItem(hwndDlg, IDOK), FALSE);
			EnableWindow(GetDlgItem(hwndDlg, IDC_EXPORTFILEPATH), FALSE);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			{
			}

		case IDCANCEL:
			DestroyWindow(hwndDlg);
			break;

		case IDC_ADDFEED:
			{
				if (!IsWindowEnabled(GetDlgItem(hwndDlg, IDC_REMOVEFEED)))
					EnableWindow(GetDlgItem(hwndDlg, IDC_REMOVEFEED), TRUE);
				int cursel = SendMessage(FeedsList, LB_GETCURSEL, 0, 0);
				TCHAR item[MAX_PATH];
				SendMessage(FeedsList, LB_GETTEXT, cursel, (LPARAM)item);
				SendMessage(FeedsExportList, LB_ADDSTRING, 0, (LPARAM)item);
				SendMessage(FeedsList, LB_DELETESTRING, cursel, 0);
				if (!SendMessage(FeedsList, LB_GETCOUNT, 0, 0))
					EnableWindow(GetDlgItem(hwndDlg, IDC_ADDFEED), FALSE);
			}
			break;
	
		case IDC_REMOVEFEED:
			{
				int cursel = SendMessage(FeedsExportList, LB_GETCURSEL, 0, 0);
				TCHAR item[MAX_PATH];
				SendMessage(FeedsExportList, LB_GETTEXT, cursel, (LPARAM)item);
				if (!IsWindowEnabled(GetDlgItem(hwndDlg, IDC_ADDFEED)))
					EnableWindow(GetDlgItem(hwndDlg, IDC_ADDFEED), TRUE);
				SendMessage(FeedsList, LB_ADDSTRING, 0, (LPARAM)item);
				SendMessage(FeedsExportList, LB_DELETESTRING, cursel, 0);
				if (!SendMessage(FeedsExportList, LB_GETCOUNT, 0, 0))
					EnableWindow(GetDlgItem(hwndDlg, IDC_REMOVEFEED), FALSE);
			}
			break;
		}
		break;

	case WM_CLOSE:
		DestroyWindow(hwndDlg);
		break;

	case WM_DESTROY:
		Utils_SaveWindowPosition(hwndDlg, NULL, MODULE, "ExportDlg");
		break;
	}

	return FALSE;
}