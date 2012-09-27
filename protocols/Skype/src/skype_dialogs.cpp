#include "skype_proto.h"

INT_PTR CALLBACK CSkypeProto::SkypeAccountProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	CSkypeProto *proto;

	switch (message)
	{
	case WM_INITDIALOG:
		TranslateDialogDefault(hwnd);

		proto = reinterpret_cast<CSkypeProto*>(lparam);
		SetWindowLongPtr(hwnd, GWLP_USERDATA, lparam);

		DBVARIANT dbv;
		if ( !DBGetContactSettingWString(0, proto->ModuleName(), SKYPE_SETTINGS_LOGIN, &dbv))
		{
			SetDlgItemText(hwnd, IDC_SL, dbv.ptszVal);
			DBFreeVariant(&dbv);
		}

		if ( !DBGetContactSettingWString(0, proto->ModuleName(), SKYPE_SETTINGS_PASSWORD, &dbv))
		{
			CallService(
				MS_DB_CRYPT_DECODESTRING,
				wcslen(dbv.ptszVal) + 1,
				reinterpret_cast<LPARAM>(dbv.ptszVal));
			SetDlgItemText(hwnd, IDC_PW, dbv.ptszVal);
			DBFreeVariant(&dbv);
		}

		if ( !proto->IsOffline()) 
		{
			SendMessage(GetDlgItem(hwnd, IDC_SL), EM_SETREADONLY, 1, 0);
			SendMessage(GetDlgItem(hwnd, IDC_PW), EM_SETREADONLY, 1, 0); 
		}

		return TRUE;

	case WM_COMMAND:
		if (HIWORD(wparam) == EN_CHANGE && reinterpret_cast<HWND>(lparam) == GetFocus())
		{
			switch(LOWORD(wparam))
			{
			case IDC_SL:
			case IDC_PW:
				SendMessage(GetParent(hwnd), PSM_CHANGED, 0, 0);
			}
		}
		break;

	case WM_NOTIFY:
		if (reinterpret_cast<NMHDR*>(lparam)->code == PSN_APPLY)
		{
			proto = reinterpret_cast<CSkypeProto*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
			TCHAR str[128];

			GetDlgItemText(hwnd, IDC_SL, str, sizeof(str));
			DBWriteContactSettingWString(0, proto->ModuleName(), SKYPE_SETTINGS_LOGIN, str);

			GetDlgItemText(hwnd, IDC_PW, str, sizeof(str));
			CallService(MS_DB_CRYPT_ENCODESTRING, sizeof(str), reinterpret_cast<LPARAM>(str));
			DBWriteContactSettingWString(0, proto->ModuleName(), SKYPE_SETTINGS_PASSWORD, str);

			return TRUE;
		}
		break;

	}

	return FALSE;
}

INT_PTR CALLBACK CSkypeProto::SkypeOptionsProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	CSkypeProto *proto;

	switch (message)
	{
	case WM_INITDIALOG:
	{
		TranslateDialogDefault(hwnd);

		proto = reinterpret_cast<CSkypeProto*>(lparam);
		SetWindowLongPtr(hwnd, GWLP_USERDATA, lparam);

		DBVARIANT dbv;
		if ( !DBGetContactSettingString(0, proto->ModuleName(), SKYPE_SETTINGS_LOGIN, &dbv))
		{
			SetDlgItemText(hwnd, IDC_SL, dbv.ptszVal);
			DBFreeVariant(&dbv);
		}

		if ( !DBGetContactSettingString(0, proto->ModuleName(), SKYPE_SETTINGS_PASSWORD, &dbv))
		{
			CallService(
				MS_DB_CRYPT_DECODESTRING,
				wcslen(dbv.ptszVal) + 1,
				reinterpret_cast<LPARAM>(dbv.ptszVal));
			SetDlgItemText(hwnd, IDC_PW, dbv.ptszVal);
			DBFreeVariant(&dbv);
		}

		if ( !proto->IsOffline()) 
		{
			SendMessage(GetDlgItem(hwnd, IDC_SL), EM_SETREADONLY, 1, 0);
			SendMessage(GetDlgItem(hwnd, IDC_PW), EM_SETREADONLY, 1, 0); 
		}
	}
	return TRUE;

	case WM_COMMAND: 
	{
		if (HIWORD(wparam) == EN_CHANGE && reinterpret_cast<HWND>(lparam) == GetFocus())
		{
			switch(LOWORD(wparam))
			{
			case IDC_SL:
			case IDC_PW:
				SendMessage(GetParent(hwnd), PSM_CHANGED, 0, 0);
			}
		}
		break;

	case WM_NOTIFY:
		if (reinterpret_cast<NMHDR*>(lparam)->code == PSN_APPLY)
		{
			proto = reinterpret_cast<CSkypeProto*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
			TCHAR str[128];

			GetDlgItemText(hwnd, IDC_SL, str, sizeof(str));
			DBWriteContactSettingTString(0, proto->ModuleName(), SKYPE_SETTINGS_LOGIN, str);

			GetDlgItemText(hwnd, IDC_PW, str, sizeof(str));
			CallService(MS_DB_CRYPT_ENCODESTRING, sizeof(str), reinterpret_cast<LPARAM>(str));
			DBWriteContactSettingTString(0, proto->ModuleName(), SKYPE_SETTINGS_PASSWORD, str);

			return TRUE;
		}
	}
	break;
	}

	return FALSE;
}

int __cdecl CSkypeProto::OnAccountManagerInit(WPARAM wParam, LPARAM lParam)
{
	return (int)CreateDialogParam(
		g_hInstance, 
		MAKEINTRESOURCE(IDD_SKYPEACCOUNT), 
		(HWND)lParam, 
		&CSkypeProto::SkypeAccountProc, (LPARAM)this);
}

int __cdecl CSkypeProto::OnOptionsInit(WPARAM wParam, LPARAM lParam)
{
	OPTIONSDIALOGPAGE odp = {0};
	odp.cbSize = sizeof(odp);
	odp.hInstance   = g_hInstance;
	odp.ptszTitle   = m_tszUserName;
	odp.dwInitParam = LPARAM(this);
	odp.flags       = ODPF_BOLDGROUPS | ODPF_TCHAR;

	odp.position    = 271828;
	odp.ptszGroup   = LPGENT("Network");
	odp.ptszTab     = LPGENT("Account");
	odp.pszTemplate = MAKEINTRESOURCEA(IDD_OPTIONS);
	odp.pfnDlgProc  = SkypeOptionsProc;
	Options_AddPage(wParam, &odp);
	
	return 0;
}
