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

INT_PTR CALLBACK DlgProcAddFeedOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_INITDIALOG:
		{
			TranslateDialogDefault(hwndDlg);
			SetWindowLongPtr(hwndDlg, GWLP_USERDATA, lParam);
			SetWindowText(hwndDlg, TranslateT("Add Feed"));
			SetDlgItemText(hwndDlg, IDC_FEEDURL, _T("http://"));
			SetDlgItemText(hwndDlg, IDC_TAGSEDIT, _T(TAGSDEFAULT));
			SendDlgItemMessage(hwndDlg, IDC_CHECKTIME, EM_LIMITTEXT, 3, 0);
			SetDlgItemInt(hwndDlg, IDC_CHECKTIME, DEFAULT_UPDATE_TIME, TRUE);
			SendDlgItemMessage(hwndDlg, IDC_TIMEOUT_VALUE_SPIN, UDM_SETRANGE32, 0, 999);	
			Utils_RestoreWindowPositionNoSize(hwndDlg,NULL,MODULE,"AddDlg");
			return TRUE;
		}
		case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
				case IDOK:
					{
						TCHAR str[MAX_PATH];
						if (!GetDlgItemText(hwndDlg, IDC_FEEDTITLE, str, SIZEOF(str)))
						{
							MessageBox(hwndDlg, TranslateT("Enter Feed name"), TranslateT("Error"), MB_OK);
							break;
						}
						else if (!GetDlgItemText(hwndDlg, IDC_FEEDURL, str, SIZEOF(str)) || lstrcmp(str, _T("http://")) == 0)
						{
							MessageBox(hwndDlg, TranslateT("Enter Feed URL"), TranslateT("Error"), MB_OK);
							break;
						}
						else if (GetDlgItemInt(hwndDlg, IDC_CHECKTIME, false, false) < 0)
						{
							MessageBox(hwndDlg, TranslateT("Enter checking interval"), TranslateT("Error"), MB_OK);
							break;
						}
						else if (!GetDlgItemText(hwndDlg, IDC_TAGSEDIT, str, SIZEOF(str)))
						{
							MessageBox(hwndDlg, TranslateT("Enter message format"), TranslateT("Error"), MB_OK);
							break;
						}
						else
						{
							HANDLE hContact = (HANDLE) CallService(MS_DB_CONTACT_ADD, 0, 0);
							CallService(MS_PROTO_ADDTOCONTACT, (WPARAM)hContact, (LPARAM)MODULE);
							GetDlgItemText(hwndDlg, IDC_FEEDTITLE, str, SIZEOF(str));
							DBWriteContactSettingTString(hContact, MODULE, "Nick", str);
							HWND hwndList = (HWND)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
							GetDlgItemText(hwndDlg, IDC_FEEDURL, str, SIZEOF(str));
							DBWriteContactSettingTString(hContact, MODULE, "URL", str);
							DBWriteContactSettingByte(hContact, MODULE, "CheckState", 1);
							DBWriteContactSettingDword(hContact, MODULE, "UpdateTime", GetDlgItemInt(hwndDlg, IDC_CHECKTIME, false, false));
							GetDlgItemText(hwndDlg, IDC_TAGSEDIT, str, SIZEOF(str));
							DBWriteContactSettingTString(hContact, MODULE, "MsgFormat", str);
							DBWriteContactSettingWord(hContact, MODULE, "Status", CallProtoService(MODULE, PS_GETSTATUS, 0, 0));
							if (IsDlgButtonChecked(hwndDlg, IDC_USEAUTH))
							{
								DBWriteContactSettingByte(hContact, MODULE, "UseAuth", 1);
								GetDlgItemText(hwndDlg, IDC_LOGIN, str, SIZEOF(str));
								DBWriteContactSettingTString(hContact, MODULE, "Login", str);
								GetDlgItemText(hwndDlg, IDC_PASSWORD, str, SIZEOF(str));
								DBWriteContactSettingTString(hContact, MODULE, "Password", str);
							}
							DeleteAllItems(hwndList);
							UpdateList(hwndList);
						}
					}

				case IDCANCEL:
					DestroyWindow(hwndDlg);
					break;

				case IDC_USEAUTH:
					{
						if (IsDlgButtonChecked(hwndDlg, IDC_USEAUTH))
						{
							EnableWindow(GetDlgItem(hwndDlg, IDC_LOGIN), TRUE);
							EnableWindow(GetDlgItem(hwndDlg, IDC_PASSWORD), TRUE);
						}
						else
						{
							EnableWindow(GetDlgItem(hwndDlg, IDC_LOGIN), FALSE);
							EnableWindow(GetDlgItem(hwndDlg, IDC_PASSWORD), FALSE);
						}
					}
					break;

				case IDC_TAGHELP:
					{
						TCHAR tszTagHelp[1024];
						mir_sntprintf(tszTagHelp, SIZEOF(tszTagHelp), _T("%s - %s\n%s - %s\n%s - %s\n%s - %s\n%s - %s\n%s - %s\n%s - %s"),
							_T("#<title>#"),		TranslateT("The title of the item."),
							_T("#<description>#"),	TranslateT("The item synopsis."),
							_T("#<link>#"),			TranslateT("The URL of the item."),
							_T("#<author>#"),		TranslateT("Email address of the author of the item."),
							_T("#<comments>#"),		TranslateT("URL of a page for comments relating to the item."),
							_T("#<guid>#"),			TranslateT("A string that uniquely identifies the item."),
							_T("#<category>#"),		TranslateT("Specify one or more categories that the item belongs to.")
						);
						MessageBox(hwndDlg, tszTagHelp, TranslateT("Feed Tag Help"), MB_OK);
					}
					break;

				case IDC_RESET:
					if (MessageBox(hwndDlg, TranslateT("Are you sure?"), TranslateT("Tags Mask Reset"), MB_YESNO | MB_ICONWARNING) == IDYES)
						SetDlgItemText(hwndDlg, IDC_TAGSEDIT, _T(TAGSDEFAULT));
					break;

				case IDC_DISCOVERY:
					{
						EnableWindow(GetDlgItem(hwndDlg, IDC_DISCOVERY), FALSE);
						SetDlgItemText(hwndDlg, IDC_DISCOVERY, TranslateT("Wait..."));
						TCHAR tszURL[MAX_PATH] = {0}, *tszTitle = NULL;
						if (GetDlgItemText(hwndDlg, IDC_FEEDURL, tszURL, SIZEOF(tszURL)) || lstrcmp(tszURL, _T("http://")) != 0)
							tszTitle = CheckFeed(tszURL, hwndDlg);
						else
							MessageBox(hwndDlg, TranslateT("Enter Feed URL"), TranslateT("Error"), MB_OK);
						SetDlgItemText(hwndDlg, IDC_FEEDTITLE, tszTitle);
						EnableWindow(GetDlgItem(hwndDlg, IDC_DISCOVERY), TRUE);
						SetDlgItemText(hwndDlg, IDC_DISCOVERY, TranslateT("Check Feed"));
					}
					break;
			}
			break;
		}
		case WM_CLOSE:
		{
			DestroyWindow(hwndDlg);
			break;
		}
		case WM_DESTROY:
		{
			Utils_SaveWindowPosition(hwndDlg,NULL,MODULE,"AddDlg");
		}
	}

	return FALSE;
}

INT_PTR CALLBACK DlgProcChangeFeedOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_INITDIALOG:
		{
			TranslateDialogDefault(hwndDlg);
			ItemInfo &SelItem = *(ItemInfo*)lParam;
			ItemInfo *nSelItem = new ItemInfo(SelItem);
			SetWindowText(hwndDlg, TranslateT("Change Feed"));
			SendDlgItemMessage(hwndDlg, IDC_CHECKTIME, EM_LIMITTEXT, 3, 0);
			SendDlgItemMessage(hwndDlg, IDC_TIMEOUT_VALUE_SPIN, UDM_SETRANGE32, 0, 999);

			HANDLE hContact = db_find_first();
			while (hContact != NULL)
			{
				if (IsMyContact(hContact))
				{
					DBVARIANT dbNick = {0};
					if (DBGetContactSettingTString(hContact, MODULE, "Nick", &dbNick))
						continue;
					else if (lstrcmp(dbNick.ptszVal, SelItem.nick) == 0)
					{
						DBFreeVariant(&dbNick);
						DBVARIANT dbURL = {0};
						if (DBGetContactSettingTString(hContact, MODULE, "URL", &dbURL))
							continue;
						else if (lstrcmp(dbURL.ptszVal, SelItem.url) == 0)
						{
							DBFreeVariant(&dbURL);
							nSelItem->hContact = hContact;
							SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG)nSelItem);
							SetDlgItemText(hwndDlg, IDC_FEEDURL, SelItem.url);
							SetDlgItemText(hwndDlg, IDC_FEEDTITLE, SelItem.nick);
							SetDlgItemInt(hwndDlg, IDC_CHECKTIME, DBGetContactSettingDword(hContact, MODULE, "UpdateTime", DEFAULT_UPDATE_TIME), TRUE);
							DBVARIANT dbMsg = {0};
							if (!DBGetContactSettingTString(hContact, MODULE, "MsgFormat", &dbMsg))
							{
								SetDlgItemText(hwndDlg, IDC_TAGSEDIT, dbMsg.ptszVal);
								DBFreeVariant(&dbMsg);
							}
							if (DBGetContactSettingByte(hContact, MODULE, "UseAuth", 0))
							{
								CheckDlgButton(hwndDlg, IDC_USEAUTH, BST_CHECKED);
								EnableWindow(GetDlgItem(hwndDlg, IDC_LOGIN), TRUE);
								EnableWindow(GetDlgItem(hwndDlg, IDC_PASSWORD), TRUE);
								DBVARIANT dbLogin = {0};
								if (!DBGetContactSettingTString(hContact, MODULE, "Login", &dbLogin))
								{
									SetDlgItemText(hwndDlg, IDC_LOGIN, dbLogin.ptszVal);
									DBFreeVariant(&dbLogin);
								}
								DBVARIANT dbPass = {0};
								if (!DBGetContactSettingTString(hContact, MODULE, "Password", &dbPass))
								{
									SetDlgItemText(hwndDlg, IDC_PASSWORD, dbPass.ptszVal);
									DBFreeVariant(&dbPass);
								}
							}
							break;
						}
						DBFreeVariant(&dbURL);
					}
					DBFreeVariant(&dbNick);
				}
				hContact = db_find_next(hContact);
			}
			WindowList_Add(hChangeFeedDlgList, hwndDlg, hContact);
			Utils_RestoreWindowPositionNoSize(hwndDlg, hContact, MODULE, "ChangeDlg");
			return TRUE;
		}
		case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
				case IDOK:
					{
						ItemInfo *SelItem = (ItemInfo*)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
						TCHAR str[MAX_PATH];
						if (!GetDlgItemText(hwndDlg, IDC_FEEDTITLE, str, SIZEOF(str)))
						{
							MessageBox(hwndDlg, TranslateT("Enter Feed name"), TranslateT("Error"), MB_OK);
							break;
						}
						else if (!GetDlgItemText(hwndDlg, IDC_FEEDURL, str, SIZEOF(str)) || lstrcmp(str, _T("http://")) == 0)
						{
							MessageBox(hwndDlg, TranslateT("Enter Feed URL"), TranslateT("Error"), MB_OK);
							break;
						}
						else if (GetDlgItemInt(hwndDlg, IDC_CHECKTIME, false, false) < 0)
						{
							MessageBox(hwndDlg, TranslateT("Enter checking interval"), TranslateT("Error"), MB_OK);
							break;
						}
						else if (!GetDlgItemText(hwndDlg, IDC_TAGSEDIT, str, SIZEOF(str)))
						{
							MessageBox(hwndDlg, TranslateT("Enter message format"), TranslateT("Error"), MB_OK);
							break;
						}
						else
						{
							GetDlgItemText(hwndDlg, IDC_FEEDURL, str, SIZEOF(str));
							DBWriteContactSettingTString(SelItem->hContact, MODULE, "URL", str);
							GetDlgItemText(hwndDlg, IDC_FEEDTITLE, str, SIZEOF(str));
							DBWriteContactSettingTString(SelItem->hContact, MODULE, "Nick", str);
							DBWriteContactSettingDword(SelItem->hContact, MODULE, "UpdateTime", GetDlgItemInt(hwndDlg, IDC_CHECKTIME, false, false));
							GetDlgItemText(hwndDlg, IDC_TAGSEDIT, str, SIZEOF(str));
							DBWriteContactSettingTString(SelItem->hContact, MODULE, "MsgFormat", str);
							if (IsDlgButtonChecked(hwndDlg, IDC_USEAUTH))
							{
								DBWriteContactSettingByte(SelItem->hContact, MODULE, "UseAuth", 1);
								GetDlgItemText(hwndDlg, IDC_LOGIN, str, SIZEOF(str));
								DBWriteContactSettingTString(SelItem->hContact, MODULE, "Login", str);
								GetDlgItemText(hwndDlg, IDC_PASSWORD, str, SIZEOF(str));
								DBWriteContactSettingTString(SelItem->hContact, MODULE, "Password", str);
							}
							DeleteAllItems(SelItem->hwndList);
							UpdateList(SelItem->hwndList);
						}
					}

				case IDCANCEL:
					DestroyWindow(hwndDlg);
					break;

				case IDC_USEAUTH:
					{
						if (IsDlgButtonChecked(hwndDlg, IDC_USEAUTH))
						{
							EnableWindow(GetDlgItem(hwndDlg, IDC_LOGIN), TRUE);
							EnableWindow(GetDlgItem(hwndDlg, IDC_PASSWORD), TRUE);
						}
						else
						{
							EnableWindow(GetDlgItem(hwndDlg, IDC_LOGIN), FALSE);
							EnableWindow(GetDlgItem(hwndDlg, IDC_PASSWORD), FALSE);
						}
						break;
					}

				case IDC_TAGHELP:
					{
						TCHAR tszTagHelp[1024];
						mir_sntprintf(tszTagHelp, SIZEOF(tszTagHelp), _T("%s - %s\n%s - %s\n%s - %s\n%s - %s\n%s - %s\n%s - %s\n%s - %s"),
							_T("#<title>#"),		TranslateT("The title of the item."),
							_T("#<description>#"),	TranslateT("The item synopsis."),
							_T("#<link>#"),			TranslateT("The URL of the item."),
							_T("#<author>#"),		TranslateT("Email address of the author of the item."),
							_T("#<comments>#"),		TranslateT("URL of a page for comments relating to the item."),
							_T("#<guid>#"),			TranslateT("A string that uniquely identifies the item."),
							_T("#<category>#"),		TranslateT("Specify one or more categories that the item belongs to.")
						);
						MessageBox(hwndDlg, tszTagHelp, TranslateT("Feed Tag Help"), MB_OK);
					}
					break;

				case IDC_RESET:
					if (MessageBox(hwndDlg, TranslateT("Are you sure?"), TranslateT("Tags Mask Reset"), MB_YESNO | MB_ICONWARNING) == IDYES)
						SetDlgItemText(hwndDlg, IDC_TAGSEDIT, _T(TAGSDEFAULT));
					break;

				case IDC_DISCOVERY:
					{
						TCHAR tszURL[MAX_PATH] = {0};
						if (GetDlgItemText(hwndDlg, IDC_FEEDURL, tszURL, SIZEOF(tszURL)) || lstrcmp(tszURL, _T("http://")) != 0)
						{
							EnableWindow(GetDlgItem(hwndDlg, IDC_DISCOVERY), FALSE);
							SetDlgItemText(hwndDlg, IDC_DISCOVERY, TranslateT("Wait..."));
							TCHAR *tszTitle = CheckFeed(tszURL, hwndDlg);
							SetDlgItemText(hwndDlg, IDC_FEEDTITLE, tszTitle);
							EnableWindow(GetDlgItem(hwndDlg, IDC_DISCOVERY), TRUE);
							SetDlgItemText(hwndDlg, IDC_DISCOVERY, TranslateT("Check Feed"));
						}
						else
							MessageBox(hwndDlg, TranslateT("Enter Feed URL"), TranslateT("Error"), MB_OK);
					}
					break;
			}
			break;
		}

		case WM_CLOSE:
		{
			DestroyWindow(hwndDlg);
			break;
		}

		case WM_DESTROY:
		{
			HANDLE hContact = (HANDLE) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
			Utils_SaveWindowPosition(hwndDlg, hContact, MODULE, "ChangeDlg");
			WindowList_Remove(hChangeFeedDlgList, hwndDlg);
			ItemInfo *SelItem = (ItemInfo*)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
			delete SelItem;
			break;
		}
	}

	return FALSE;
}

INT_PTR CALLBACK DlgProcChangeFeedMenu(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_INITDIALOG:
		{
			TranslateDialogDefault(hwndDlg);
			SetWindowText(hwndDlg, TranslateT("Change Feed"));
			SendDlgItemMessage(hwndDlg, IDC_CHECKTIME, UDM_SETRANGE32, 0, 999);

			HANDLE hContact = (HANDLE)lParam;
			WindowList_Add(hChangeFeedDlgList, hwndDlg, hContact);
			Utils_RestoreWindowPositionNoSize(hwndDlg, hContact, MODULE, "ChangeDlg");
			DBVARIANT dbNick = {0};
			if (!DBGetContactSettingTString(hContact, MODULE, "Nick", &dbNick))
			{
				SetDlgItemText(hwndDlg, IDC_FEEDTITLE, dbNick.ptszVal);
				DBFreeVariant(&dbNick);
				DBVARIANT dbURL = {0};
				if (!DBGetContactSettingTString(hContact, MODULE, "URL", &dbURL))
				{
					SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG)lParam);
					SetDlgItemText(hwndDlg, IDC_FEEDURL, dbURL.ptszVal);
					DBFreeVariant(&dbURL);
					SetDlgItemInt(hwndDlg, IDC_CHECKTIME, DBGetContactSettingDword(hContact, MODULE, "UpdateTime", DEFAULT_UPDATE_TIME), TRUE);
					DBVARIANT dbMsg = {0};
					if (!DBGetContactSettingTString(hContact, MODULE, "MsgFormat", &dbMsg))
					{
						SetDlgItemText(hwndDlg, IDC_TAGSEDIT, dbMsg.ptszVal);
						DBFreeVariant(&dbMsg);
					}
					if (DBGetContactSettingByte(hContact, MODULE, "UseAuth", 0))
					{
						CheckDlgButton(hwndDlg, IDC_USEAUTH, BST_CHECKED);
						EnableWindow(GetDlgItem(hwndDlg, IDC_LOGIN), TRUE);
						EnableWindow(GetDlgItem(hwndDlg, IDC_PASSWORD), TRUE);
						DBVARIANT dbLogin = {0};
						if (!DBGetContactSettingTString(hContact, MODULE, "Login", &dbLogin))
						{
							SetDlgItemText(hwndDlg, IDC_LOGIN, dbLogin.ptszVal);
							DBFreeVariant(&dbLogin);
						}
						DBVARIANT dbPass = {0};
						if (!DBGetContactSettingTString(hContact, MODULE, "Password", &dbPass))
						{
							SetDlgItemText(hwndDlg, IDC_PASSWORD, dbPass.ptszVal);
							DBFreeVariant(&dbPass);
						}
					}
					break;
				}
			}
			return TRUE;
		}
		case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
				case IDOK:
					{
						HANDLE hContact = (HANDLE)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
						TCHAR str[MAX_PATH];
						if (!GetDlgItemText(hwndDlg, IDC_FEEDTITLE, str, SIZEOF(str)))
						{
							MessageBox(hwndDlg, TranslateT("Enter Feed name"), TranslateT("Error"), MB_OK);
							break;
						}
						else if (!GetDlgItemText(hwndDlg, IDC_FEEDURL, str, SIZEOF(str)) || lstrcmp(str, _T("http://")) == 0)
						{
							MessageBox(hwndDlg, TranslateT("Enter Feed URL"), TranslateT("Error"), MB_OK);
							break;
						}
						else if (GetDlgItemInt(hwndDlg, IDC_CHECKTIME, false, false) < 0)
						{
							MessageBox(hwndDlg, TranslateT("Enter checking interval"), TranslateT("Error"), MB_OK);
							break;
						}
						else if (!GetDlgItemText(hwndDlg, IDC_TAGSEDIT, str, SIZEOF(str)))
						{
							MessageBox(hwndDlg, TranslateT("Enter message format"), TranslateT("Error"), MB_OK);
							break;
						}
						else
						{
							GetDlgItemText(hwndDlg, IDC_FEEDURL, str, SIZEOF(str));
							DBWriteContactSettingTString(hContact, MODULE, "URL", str);
							GetDlgItemText(hwndDlg, IDC_FEEDTITLE, str, SIZEOF(str));
							DBWriteContactSettingTString(hContact, MODULE, "Nick", str);
							DBWriteContactSettingDword(hContact, MODULE, "UpdateTime", GetDlgItemInt(hwndDlg, IDC_CHECKTIME, false, false));
							GetDlgItemText(hwndDlg, IDC_TAGSEDIT, str, SIZEOF(str));
							DBWriteContactSettingTString(hContact, MODULE, "MsgFormat", str);
							if (IsDlgButtonChecked(hwndDlg, IDC_USEAUTH))
							{
								DBWriteContactSettingByte(hContact, MODULE, "UseAuth", 1);
								GetDlgItemText(hwndDlg, IDC_LOGIN, str, SIZEOF(str));
								DBWriteContactSettingTString(hContact, MODULE, "Login", str);
								GetDlgItemText(hwndDlg, IDC_PASSWORD, str, SIZEOF(str));
								DBWriteContactSettingTString(hContact, MODULE, "Password", str);
							}
						}
					}

				case IDCANCEL:
					DestroyWindow(hwndDlg);
					break;

				case IDC_USEAUTH:
					{
						if (IsDlgButtonChecked(hwndDlg, IDC_USEAUTH))
						{
							EnableWindow(GetDlgItem(hwndDlg, IDC_LOGIN), TRUE);
							EnableWindow(GetDlgItem(hwndDlg, IDC_PASSWORD), TRUE);
						}
						else
						{
							EnableWindow(GetDlgItem(hwndDlg, IDC_LOGIN), FALSE);
							EnableWindow(GetDlgItem(hwndDlg, IDC_PASSWORD), FALSE);
						}
						break;
					}

				case IDC_TAGHELP:
					{
						TCHAR tszTagHelp[1024];
						mir_sntprintf(tszTagHelp, SIZEOF(tszTagHelp), _T("%s - %s\n%s - %s\n%s - %s\n%s - %s\n%s - %s\n%s - %s\n%s - %s"),
							_T("#<title>#"),		TranslateT("The title of the item."),
							_T("#<description>#"),	TranslateT("The item synopsis."),
							_T("#<link>#"),			TranslateT("The URL of the item."),
							_T("#<author>#"),		TranslateT("Email address of the author of the item."),
							_T("#<comments>#"),		TranslateT("URL of a page for comments relating to the item."),
							_T("#<guid>#"),			TranslateT("A string that uniquely identifies the item."),
							_T("#<category>#"),		TranslateT("Specify one or more categories that the item belongs to.")
						);
						MessageBox(hwndDlg, tszTagHelp, TranslateT("Feed Tag Help"), MB_OK);
					}
					break;

				case IDC_RESET:
					if (MessageBox(hwndDlg, TranslateT("Are you sure?"), TranslateT("Tags Mask Reset"), MB_YESNO | MB_ICONWARNING) == IDYES)
						SetDlgItemText(hwndDlg, IDC_TAGSEDIT, _T(TAGSDEFAULT));
					break;

				case IDC_DISCOVERY:
					{
						TCHAR tszURL[MAX_PATH] = {0};
						if (GetDlgItemText(hwndDlg, IDC_FEEDURL, tszURL, SIZEOF(tszURL)) || lstrcmp(tszURL, _T("http://")) != 0)
						{
							EnableWindow(GetDlgItem(hwndDlg, IDC_DISCOVERY), FALSE);
							SetDlgItemText(hwndDlg, IDC_DISCOVERY, TranslateT("Wait..."));
							TCHAR *tszTitle = CheckFeed(tszURL, hwndDlg);
							SetDlgItemText(hwndDlg, IDC_FEEDTITLE, tszTitle);
							EnableWindow(GetDlgItem(hwndDlg, IDC_DISCOVERY), TRUE);
							SetDlgItemText(hwndDlg, IDC_DISCOVERY, TranslateT("Check Feed"));
						}
						else
							MessageBox(hwndDlg, TranslateT("Enter Feed URL"), TranslateT("Error"), MB_OK);
					}
					break;
			}
			break;
		}

		case WM_CLOSE:
		{
			DestroyWindow(hwndDlg);
			break;
		}
		case WM_DESTROY:
		{
			HANDLE hContact = (HANDLE) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
			Utils_SaveWindowPosition(hwndDlg,hContact,MODULE,"ChangeDlg");
			WindowList_Remove(hChangeFeedDlgList,hwndDlg);
		}
	}

	return FALSE;
}

void CreateCListGroup(TCHAR* szGroupName)
{
	int hGroup;
	CLIST_INTERFACE *clint = NULL;

	if (ServiceExists(MS_CLIST_RETRIEVE_INTERFACE))
		clint = (CLIST_INTERFACE*)CallService(MS_CLIST_RETRIEVE_INTERFACE, 0, 0);
	hGroup = CallService(MS_CLIST_GROUPCREATE, 0, 0);
	TCHAR* usTmp = szGroupName;
	clint->pfnRenameGroup(hGroup, usTmp);
}
 
INT_PTR CALLBACK UpdateNotifyOptsProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	HWND hwndList = GetDlgItem(hwndDlg, IDC_FEEDLIST);
	switch (msg)
	{
	case WM_INITDIALOG:
		{
			TranslateDialogDefault(hwndDlg);
			SetWindowLongPtr(hwndDlg, GWLP_USERDATA, 0);
			CreateList(hwndList);
			UpdateList(hwndList);
			CheckDlgButton(hwndDlg, IDC_STARTUPRETRIEVE, db_get_b(NULL, MODULE, "StartupRetrieve", 1));
			return TRUE;
		}

	case WM_COMMAND:
	{
		switch (LOWORD(wParam))
		{
			case IDC_ADD:
				{
					CreateDialogParam(hInst, MAKEINTRESOURCE(IDD_ADDFEED), hwndDlg, DlgProcAddFeedOpts, (LPARAM)hwndList);
				}
				return FALSE;

			case IDC_CHANGE:
				{
					ItemInfo SelItem = {0};
					int sel = ListView_GetSelectionMark(hwndList);
					ListView_GetItemText(hwndList, sel, 0, SelItem.nick, MAX_PATH);
					ListView_GetItemText(hwndList, sel, 1, SelItem.url, MAX_PATH);
					SelItem.hwndList = hwndList;
					SelItem.SelNumber = sel;
					CreateDialogParam(hInst, MAKEINTRESOURCE(IDD_ADDFEED), hwndDlg, DlgProcChangeFeedOpts, (LPARAM)&SelItem);
				}
				return FALSE;

			case IDC_REMOVE:
				{
					if (MessageBox(hwndDlg, TranslateT("Are you sure?"), TranslateT("Contact deleting"), MB_YESNO | MB_ICONWARNING) == IDYES)
					{
						TCHAR nick[MAX_PATH], url[MAX_PATH];
						int sel = ListView_GetSelectionMark(hwndList);
						ListView_GetItemText(hwndList, sel, 0, nick, MAX_PATH);
						ListView_GetItemText(hwndList, sel, 1, url, MAX_PATH);

						HANDLE hContact = db_find_first();
						while (hContact != NULL)
						{
							if(IsMyContact(hContact))
							{
								DBVARIANT dbNick = {0};
								if (DBGetContactSettingTString(hContact, MODULE, "Nick", &dbNick))
									break;
								else if (lstrcmp(dbNick.ptszVal, nick) == 0)
								{
									DBFreeVariant(&dbNick);
									DBVARIANT dbURL = {0};
									if (DBGetContactSettingTString(hContact, MODULE, "URL", &dbURL))
										break;
									else if (lstrcmp(dbURL.ptszVal, url) == 0)
									{
										DBFreeVariant(&dbURL);
										CallService(MS_DB_CONTACT_DELETE, (WPARAM)hContact, 0);
										ListView_DeleteItem(hwndList, sel);
										break;
									}
									DBFreeVariant(&dbURL);
								}
								DBFreeVariant(&dbNick);
							}
							hContact = db_find_next(hContact);
						}
					}
				}
				return FALSE;

			case IDC_IMPORT:
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

					if (GetOpenFileName(&ofn)) 
					{
						int bytesParsed = 0;
						HXML hXml = xi.parseFile(FileName, &bytesParsed, NULL);
						if(hXml != NULL)
						{
							BYTE isUTF = 0;
							if ( !lstrcmpi(xi.getAttrValue(hXml, _T("encoding")), _T("UTF-8")))
								isUTF = 1;
							HXML node = xi.getChildByPath(hXml, _T("opml/body/outline"), 0);
							if ( !node)
								node = xi.getChildByPath(hXml, _T("body/outline"), 0);
							if (node)
							{
								while (node)
								{
									int outlineAttr = xi.getAttrCount(node);
									int outlineChildsCount = xi.getChildCount(node);
									TCHAR *type = (TCHAR*)xi.getAttrValue(node, _T("type"));
									if ( !type && !outlineChildsCount)
									{
										HXML tmpnode = node;
										node = xi.getNextNode(node);
										if ( !node)
										{
											node = xi.getParent(tmpnode);
											node = xi.getNextNode(node);
										}
									}
									else if (!type && outlineChildsCount)
										node = xi.getFirstChild(node);
									else if (type) {
										TCHAR *title = NULL, *url = NULL, *group = NULL;
										for (int i = 0; i < outlineAttr; i++)
										{
											if (!lstrcmpi(xi.getAttrName(node, i), _T("title")))
											{
												if (isUTF)
													title = mir_utf8decodeT(_T2A(xi.getAttrValue(node, xi.getAttrName(node, i))));
												else
													title = (TCHAR*)xi.getAttrValue(node, xi.getAttrName(node, i));
												continue;
											}
											if (!lstrcmpi(xi.getAttrName(node, i), _T("xmlUrl")))
											{
												if (isUTF)
													url = mir_utf8decodeT(_T2A(xi.getAttrValue(node, xi.getAttrName(node, i))));
												else
													url = (TCHAR*)xi.getAttrValue(node, xi.getAttrName(node, i));
												continue;
											}
											if (title && url)
												break;
										}

										HXML parent = xi.getParent(node);
										LPCTSTR tmp = xi.getName(parent);
										while (lstrcmpi(xi.getName(parent), _T("body")))
										{
											for (int i = 0; i < xi.getAttrCount(parent); i++)
											{
												if (!lstrcmpi(xi.getAttrName(parent, i), _T("title")))
												{
													if ( !group)
														group = (TCHAR*)xi.getAttrValue(parent, xi.getAttrName(parent, i));
													else
													{
														TCHAR tmpgroup[1024];
														mir_sntprintf(tmpgroup, SIZEOF(tmpgroup), _T("%s\\%s"), xi.getAttrValue(parent, xi.getAttrName(parent, i)), group);
														group = tmpgroup;
													}
													break;
												}
											}
											parent = xi.getParent(parent);
										}
										if (isUTF)
											group = mir_utf8decodeT(_T2A(group));

										HANDLE hContact = (HANDLE)CallService(MS_DB_CONTACT_ADD, 0, 0);
										CallService(MS_PROTO_ADDTOCONTACT, (WPARAM)hContact, (LPARAM)MODULE);
										db_set_ts(hContact, MODULE, "Nick", title);
										db_set_ts(hContact, MODULE, "URL", url);
										db_set_b(hContact, MODULE, "CheckState", 1);
										db_set_dw(hContact, MODULE, "UpdateTime", DEFAULT_UPDATE_TIME);
										db_set_ts(hContact, MODULE, "MsgFormat", _T(TAGSDEFAULT));
										db_set_w(hContact, MODULE, "Status", CallProtoService(MODULE, PS_GETSTATUS, 0, 0));
										if (group)
										{
											db_set_ts(hContact, "CList", "Group", group);
											int hGroup = 1;
											char *group_name;
											BYTE GroupExist = 0;
											do {
												group_name = (char *)CallService(MS_CLIST_GROUPGETNAME, (WPARAM)hGroup, 0);
												if (group_name != NULL && !strcmp(group_name, _T2A(group))) {
													GroupExist = 1;
													break;
												}
												hGroup++;
											}
											while (group_name);

											if(!GroupExist)
												CreateCListGroup(group);
										}
										if (isUTF)
										{
											mir_free(title);
											mir_free(url);
											mir_free(group);
										}

										HXML tmpnode = node;
										node = xi.getNextNode(node);
										if ( !node)
										{
											node = xi.getParent(tmpnode);
											node = xi.getNextNode(node);
										}
									}
								}
							} else
								MessageBox(NULL, TranslateT("Not valid import file."), TranslateT("Error"), MB_OK | MB_ICONERROR);
							xi.destroyNode(hXml);
						} else
							MessageBox(NULL, TranslateT("Not valid import file."), TranslateT("Error"), MB_OK | MB_ICONERROR);
	
						DeleteAllItems(hwndList);
						UpdateList(hwndList);
					}
					mir_free(tszMirDir);
				}
				return FALSE;

			case IDC_EXPORT:
				{
				}
				return FALSE;

			case IDC_STARTUPRETRIEVE:
				SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
				break;
		}
		break;
	}
	case WM_NOTIFY:
		{
			NMHDR *hdr = (NMHDR *)lParam;
			switch (hdr->code)
			{
				case PSN_APPLY:
					{
						db_set_b(NULL, MODULE, "StartupRetrieve", IsDlgButtonChecked(hwndDlg, IDC_STARTUPRETRIEVE));
						HANDLE hContact = db_find_first();
						int i = 0;
						while (hContact != NULL)
						{
							if(IsMyContact(hContact))
							{
								DBWriteContactSettingByte(hContact, MODULE, "CheckState", ListView_GetCheckState(hwndList, i));
								if (!ListView_GetCheckState(hwndList, i))
									DBWriteContactSettingByte(hContact, "CList", "Hidden", 1);
								else
									DBDeleteContactSetting(hContact,"CList","Hidden");
								i += 1;
							}
							hContact = db_find_next(hContact);
						}
						break;
					}

				case NM_DBLCLK:
					{
						ItemInfo SelItem = {0};
						int sel = ListView_GetHotItem(hwndList);
						if (sel != -1)
						{
							ListView_GetItemText(hwndList, sel, 0, SelItem.nick, MAX_PATH);
							ListView_GetItemText(hwndList, sel, 1, SelItem.url, MAX_PATH);
							SelItem.hwndList = hwndList;
							SelItem.SelNumber = sel;
							CreateDialogParam(hInst, MAKEINTRESOURCE(IDD_ADDFEED), hwndDlg, DlgProcChangeFeedOpts, (LPARAM)&SelItem);
						}
						break;
					}

			case LVN_ITEMCHANGED:
				{
					NMLISTVIEW *nmlv = (NMLISTVIEW *)lParam;
					if (((nmlv->uNewState ^ nmlv->uOldState) & LVIS_STATEIMAGEMASK) && !UpdateListFlag)
					{
						SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
					}
					break;
				}
			}
		}
	}//end* switch (msg)
	return FALSE;
}

INT OptInit(WPARAM wParam, LPARAM lParam)
{
	OPTIONSDIALOGPAGE odp = {0};
	odp.cbSize = sizeof(odp);
	odp.position = 100000000;
	odp.hInstance = hInst;
	odp.flags = ODPF_BOLDGROUPS;
	odp.pszTemplate = MAKEINTRESOURCEA(IDD_OPTIONS);
	odp.pszGroup = LPGEN("Network");
	odp.pszTitle = LPGEN("News Aggregator");
	odp.pfnDlgProc = UpdateNotifyOptsProc;
	Options_AddPage(wParam, &odp);
	return 0;
}