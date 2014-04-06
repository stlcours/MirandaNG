﻿#include "common.h"

INT_PTR CALLBACK CSteamProto::GuardProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	GuardParam *guard = (GuardParam*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

	switch (message)
	{
	case WM_INITDIALOG:
		TranslateDialogDefault(hwnd);
		{
			guard = (GuardParam*)lParam;
			SetWindowLongPtr(hwnd, GWLP_USERDATA, lParam);
			// load steam icon
			char iconName[100];
			mir_snprintf(iconName, SIZEOF(iconName), "%s_%s", MODULE, "main");
			SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)Skin_GetIcon(iconName, 16));
			SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)Skin_GetIcon(iconName, 32));
		}
		Utils_RestoreWindowPositionNoSize(hwnd, 0, "STEAM", "GuardWindow");
		return TRUE;

	case WM_CLOSE:
		Skin_ReleaseIcon((HICON)SendMessage(hwnd, WM_SETICON, ICON_BIG, 0));
		Skin_ReleaseIcon((HICON)SendMessage(hwnd, WM_SETICON, ICON_SMALL, 0));
		Utils_SaveWindowPosition(hwnd, NULL, "STEAM", "GuardWindow");
		EndDialog(hwnd, 0);
		break;

	case WM_COMMAND:
	{
		switch (LOWORD(wParam))
		{
		case IDC_GETDOMAIN:
			CallService(MS_UTILS_OPENURL, 0, (LPARAM)guard->domain);
			SetFocus(GetDlgItem(hwnd, IDC_TEXT));
			break;

		case IDOK:
			GetDlgItemTextA(hwnd, IDC_TEXT, guard->code, sizeof(guard->code));
			EndDialog(hwnd, IDOK);
			break;

		case IDCANCEL:
			EndDialog(hwnd, IDCANCEL);
			break;
		}
	}
		break;
	}

	return FALSE;
}

INT_PTR CALLBACK CSteamProto::CaptchaProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	CaptchaParam *captcha = (CaptchaParam*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

	switch (message)
	{
	case WM_INITDIALOG:
		TranslateDialogDefault(hwnd);
		{
			captcha = (CaptchaParam*)lParam;
			SetWindowLongPtr(hwnd, GWLP_USERDATA, lParam);
		}
		return TRUE;

	case WM_CLOSE:
		EndDialog(hwnd, 0);
		break;

	//case WM_PAINT:
	//	{
	//		/*FI_INTERFACE *fei = 0;

	//		INT_PTR result = CALLSERVICE_NOTFOUND;
	//		if (ServiceExists(MS_IMG_GETINTERFACE))
	//			result = CallService(MS_IMG_GETINTERFACE, FI_IF_VERSION, (LPARAM)&fei);

	//		if (fei == NULL || result != S_OK) {
	//			MessageBox(0, TranslateT("Fatal error, image services not found. Avatar services will be disabled."), TranslateT("Avatar Service"), MB_OK);
	//			return 0;
	//		}*/

	//		PAINTSTRUCT ps;
	//		HDC hDC = BeginPaint(hwnd, &ps);

	//		//FreeImage_Initialise();
	//		
	//		/*FIMEMORY *stream = FreeImage_OpenMemory(captcha->data, captcha->size);
	//		FIBITMAP *dib = FreeImage_LoadFromMemory(FIF_PNG, stream, PNG_DEFAULT);
	//		FreeImage_CloseMemory(stream);

	//		SetStretchBltMode(hDC, COLORONCOLOR);
	//		StretchDIBits(
	//			hDC,
	//			0, 0,
	//			206, 40,
	//			0, 0,
	//			FreeImage_GetWidth(dib), FreeImage_GetHeight(dib),
	//			FreeImage_GetBits(dib),
	//			FreeImage_GetInfo(dib),
	//			DIB_RGB_COLORS, SRCCOPY);

	//		FreeImage_Unload(dib);*/

	//		//FreeImage_DeInitialise();

	//		EndPaint(hwnd, &ps);
	//	}
	//	return 0;

	case WM_COMMAND:
		{
			switch(LOWORD(wParam))
			{
			case IDOK:
				GetDlgItemTextA(hwnd, IDC_TEXT, captcha->text, sizeof(captcha->text));
				EndDialog(hwnd, IDOK);
				break;

			case IDCANCEL:
				EndDialog(hwnd, IDCANCEL);
				break;
			}
		}
		break;
	}

	return FALSE;
}

INT_PTR CALLBACK CSteamProto::MainOptionsProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	CSteamProto *proto = (CSteamProto*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

	switch (message)
	{
	case WM_INITDIALOG:
		TranslateDialogDefault(hwnd);
		{
			proto = (CSteamProto*)lParam;
			SetWindowLongPtr(hwnd, GWLP_USERDATA, lParam);

			ptrW username(proto->getWStringA("Username"));
			SetDlgItemText(hwnd, IDC_USERNAME, username);

			ptrA password(proto->getStringA("Password"));
			SetDlgItemTextA(hwnd, IDC_PASSWORD, password);

			ptrW groupName(proto->getWStringA(NULL, "DefaultGroup"));
			SetDlgItemText(hwnd, IDC_GROUP, groupName);
			SendDlgItemMessage(hwnd, IDC_GROUP, EM_LIMITTEXT, 64, 0);

			if (proto->IsOnline())
			{
				EnableWindow(GetDlgItem(hwnd, IDC_USERNAME), FALSE);
				EnableWindow(GetDlgItem(hwnd, IDC_PASSWORD), FALSE);
				EnableWindow(GetDlgItem(hwnd, IDC_GROUP), FALSE);
			}
		}
		return TRUE;

	case WM_COMMAND:
		{
			switch(LOWORD(wParam))
			{
			case IDC_USERNAME:
				{
					if ((HIWORD(wParam) != EN_CHANGE || (HWND)lParam != GetFocus())) return 0;
					proto->delSetting("SteamID");
					wchar_t username[128];
					GetDlgItemText(hwnd, IDC_USERNAME, username, SIZEOF(username));
					SendMessage(GetParent(hwnd), PSM_CHANGED, 0, 0);
				}
				break;

			case IDC_PASSWORD:
				{
					if ((HIWORD(wParam) != EN_CHANGE || (HWND)lParam != GetFocus())) return 0;
					proto->delSetting("TokenSecret");
					char password[128];
					GetDlgItemTextA(hwnd, IDC_PASSWORD, password, SIZEOF(password));
					SendMessage(GetParent(hwnd), PSM_CHANGED, 0, 0);
				}
				break;

			case IDC_GROUP:
				{
					if ((HIWORD(wParam) != EN_CHANGE || (HWND)lParam != GetFocus()))
						return 0;
					SendMessage(GetParent(hwnd), PSM_CHANGED, 0, 0);
				}
				break;
			}
		}
		break;

	case WM_NOTIFY:
		if (reinterpret_cast<NMHDR*>(lParam)->code == PSN_APPLY)
		{
			if (!proto->IsOnline())
			{
				wchar_t username[128];
				GetDlgItemText(hwnd, IDC_USERNAME, username, SIZEOF(username));
				proto->setWString("Username", username);

				char password[128];
				GetDlgItemTextA(hwnd, IDC_PASSWORD, password, SIZEOF(password));
				proto->setString("Password", password);

				wchar_t groupName[128];
				GetDlgItemText(hwnd, IDC_GROUP, groupName, SIZEOF(groupName));
				if (lstrlen(groupName) > 0)
				{
					proto->setWString(NULL, "DefaultGroup", groupName);
					Clist_CreateGroup(0, groupName);
				}
				else
					proto->delSetting(NULL, "DefaultGroup");
			}

			return TRUE;
		}
		break;
	}

	return FALSE;
}