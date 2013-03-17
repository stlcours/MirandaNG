#include "common.h"

HANDLE hMenuShowHistory, hMenuToggleOnOff;

void StripBBCodesInPlace(wchar_t *text)
{
	if (text == 0 || db_get_b(0, MODULE, "StripBBCodes", 1) == 0)
		return;

	int read = 0, write = 0;
	int len = (int)wcslen(text);

	while(read <= len) { // copy terminating null too
		while(read <= len && text[read] != L'[') {
			if (text[read] != text[write]) text[write] = text[read];
			read++; write++;
		}
		if (read > len) break;

		if (len - read >= 3 && (_wcsnicmp(text + read, L"[b]", 3) == 0 || _wcsnicmp(text + read, L"[i]", 3) == 0))
			read += 3;
		else if (len - read >= 4 && (_wcsnicmp(text + read, L"[/b]", 4) == 0 || _wcsnicmp(text + read, L"[/i]", 4) == 0))
			read += 4;
		else if (len - read >= 6 && (_wcsnicmp(text + read, L"[color", 6) == 0)) {
			while(read < len && text[read] != L']') read++; 
			read++;// skip the ']'
		} else if (len - read >= 8 && (_wcsnicmp(text + read, L"[/color]", 8) == 0))
			read += 8;
		else if (len - read >= 5 && (_wcsnicmp(text + read, L"[size", 5) == 0)) {
			while(read < len && text[read] != L']') read++; 
			read++;// skip the ']'
		} else if (len - read >= 7 && (_wcsnicmp(text + read, L"[/size]", 7) == 0))
			read += 7;
		else {
			if (text[read] != text[write]) text[write] = text[read];
			read++; write++;
		}
	}
}

static INT_PTR CreatePopup(WPARAM wParam, LPARAM lParam)
{
	POPUPDATA *pd_in = (POPUPDATA *)wParam;
	PopupData *pd_out = (PopupData *)mir_calloc(sizeof(PopupData));

	pd_out->cbSize = sizeof(PopupData);
	pd_out->flags = PDF_UNICODE;
	pd_out->pwzTitle = mir_a2u(pd_in->lpzContactName);
	pd_out->pwzText = mir_a2u(pd_in->lpzText);
	StripBBCodesInPlace(pd_out->pwzTitle);
	StripBBCodesInPlace(pd_out->pwzText);

	pd_out->hContact = pd_in->lchContact;
	pd_out->SetIcon(pd_in->lchIcon);
	if (pd_in->colorBack == 0xffffffff) // that's the old #define for 'skinned bg'
		pd_out->colorBack = pd_out->colorText = 0;
	else {
		pd_out->colorBack = pd_in->colorBack & 0xFFFFFF;
		pd_out->colorText = pd_in->colorText & 0xFFFFFF;
	}
	pd_out->windowProc = pd_in->PluginWindowProc;
	pd_out->opaque = pd_in->PluginData;
	pd_out->timeout = pd_in->iSeconds;

	lstPopupHistory.Add(pd_out->pwzTitle, pd_out->pwzText, time(0));
	if (!db_get_b(0, MODULE, "Enabled", 1)) {
		mir_free(pd_out->pwzTitle);
		mir_free(pd_out->pwzText);
		mir_free(pd_out);
		return -1;
	}

	PostMPMessage(MUM_CREATEPOPUP, 0, (LPARAM)pd_out);
	return 0;
}

static INT_PTR CreatePopupW(WPARAM wParam, LPARAM lParam)
{
	POPUPDATAW *pd_in = (POPUPDATAW *)wParam;
	PopupData *pd_out = (PopupData *)mir_calloc(sizeof(PopupData));

	pd_out->cbSize = sizeof(PopupData);
	pd_out->flags = PDF_UNICODE;
	pd_out->pwzTitle = mir_wstrdup(pd_in->lpwzContactName);
	pd_out->pwzText = mir_wstrdup(pd_in->lpwzText);
	StripBBCodesInPlace(pd_out->pwzTitle);
	StripBBCodesInPlace(pd_out->pwzText);

	pd_out->hContact = pd_in->lchContact;
	pd_out->SetIcon(pd_in->lchIcon);
	if (pd_in->colorBack == 0xffffffff) // that's the old #define for 'skinned bg'
		pd_out->colorBack = pd_out->colorText = 0;
	else {
		pd_out->colorBack = pd_in->colorBack & 0xFFFFFF;
		pd_out->colorText = pd_in->colorText & 0xFFFFFF;
	}
	pd_out->windowProc = pd_in->PluginWindowProc;
	pd_out->opaque = pd_in->PluginData;
	pd_out->timeout = pd_in->iSeconds;

	lstPopupHistory.Add(pd_out->pwzTitle, pd_out->pwzText, time(0));
	if (!db_get_b(0, MODULE, "Enabled", 1)) {
		mir_free(pd_out->pwzTitle);
		mir_free(pd_out->pwzText);
		mir_free(pd_out);
		return -1;
	}

	PostMPMessage(MUM_CREATEPOPUP, 0, (LPARAM)pd_out);
	return 0;
}

static INT_PTR ChangeTextW(WPARAM wParam, LPARAM lParam)
{
	HWND hwndPop = (HWND)wParam;
	wchar_t *newText = NEWWSTR_ALLOCA((wchar_t *)lParam);
	StripBBCodesInPlace(newText);

	if (IsWindow(hwndPop))
		SendMessage(hwndPop, PUM_SETTEXT, 0, (LPARAM)newText);
	return 0;
}

void ShowPopup(PopupData &pd_in) 
{
	PopupData *pd_out = (PopupData *)mir_alloc(sizeof(PopupData));
	*pd_out = pd_in;
	if (pd_in.flags & PDF_UNICODE) {
		pd_out->pwzTitle = mir_wstrdup(pd_in.pwzTitle);
		pd_out->pwzText = mir_wstrdup(pd_in.pwzText);
	} else {
		pd_out->flags |= PDF_UNICODE;
		pd_out->pwzTitle = mir_a2u(pd_in.pszTitle);
		pd_out->pwzText = mir_a2u(pd_in.pszText); 
	}
	StripBBCodesInPlace(pd_out->pwzTitle);
	StripBBCodesInPlace(pd_out->pwzText);

	lstPopupHistory.Add(pd_out->pwzTitle, pd_out->pwzText, time(0));

	if (!db_get_b(0, MODULE, "Enabled", 1)) 
	{
		mir_free(pd_out->pwzTitle);
		mir_free(pd_out->pwzText);
		mir_free(pd_out);
	}
	else
		PostMPMessage(MUM_CREATEPOPUP, 0, (LPARAM)pd_out);
}

static INT_PTR GetContact(WPARAM wParam, LPARAM lParam)
{
	HWND hwndPop = (HWND)wParam;
	HANDLE hContact;
	if (GetCurrentThreadId() == message_pump_thread_id) {
		SendMessage(hwndPop, PUM_GETCONTACT, (WPARAM)&hContact, 0);
	} else {
		HANDLE hEvent = CreateEvent(0, 0, 0, 0);
		PostMessage(hwndPop, PUM_GETCONTACT, (WPARAM)&hContact, (LPARAM)hEvent);
		MsgWaitForMultipleObjectsEx(1, &hEvent, INFINITE, 0, 0);
		CloseHandle(hEvent);
	}

	return (INT_PTR)hContact;
}

static INT_PTR GetOpaque(WPARAM wParam, LPARAM lParam)
{
	HWND hwndPop = (HWND)wParam;
	void *data = 0;
	if (GetCurrentThreadId() == message_pump_thread_id) {
		SendMessage(hwndPop, PUM_GETOPAQUE, (WPARAM)&data, 0);
	} else {
		HANDLE hEvent = CreateEvent(0, 0, 0, 0);
		PostMessage(hwndPop, PUM_GETOPAQUE, (WPARAM)&data, (LPARAM)hEvent);
		MsgWaitForMultipleObjectsEx(1, &hEvent, INFINITE, 0, 0);
		CloseHandle(hEvent);
	}

	return (INT_PTR)data;
}

static INT_PTR IsSecondLineShown(WPARAM wParam, LPARAM lParam)
{
	return TRUE;
}

void UpdateMenu()
{
	CLISTMENUITEM mi = { sizeof(mi) };
	mi.pszName = (char*)(db_get_b(0, MODULE, "Enabled", 1) == 1 ? LPGEN("Disable Popups") : LPGEN("Enable Popups"));
	mi.flags = CMIM_NAME;// | CMIM_ICON;
	CallService(MS_CLIST_MODIFYMENUITEM, (WPARAM)hMenuToggleOnOff, (LPARAM)&mi);
}

INT_PTR PopupQuery(WPARAM wParam, LPARAM lParam) {
	switch(wParam) {
	case PUQS_ENABLEPOPUPS:
		{
			bool enabled = db_get_b(0, MODULE, "Enabled", 1) != 0;
			if (!enabled) db_set_b(0, MODULE, "Enabled", 1);
			return !enabled;
		}
		break;
	case PUQS_DISABLEPOPUPS:
		{
			bool enabled = db_get_b(0, MODULE, "Enabled", 1) != 0;
			if (enabled) db_set_b(0, MODULE, "Enabled", 0);
			return enabled;
		}
		break;

	case PUQS_GETSTATUS:
		return db_get_b(0, MODULE, "Enabled", 1);

	default:
		return 1;
	}
	UpdateMenu();
	return 0;
}

static INT_PTR TogglePopups(WPARAM wParam, LPARAM lParam)
{
	BYTE val = db_get_b(0, MODULE, "Enabled", 1);
	db_set_b(0, MODULE, "Enabled", !val);
	UpdateMenu();
	return 0;
}

static INT_PTR PopupChangeW(WPARAM wParam, LPARAM lParam)
{
	HWND hwndPop = (HWND)wParam;
	POPUPDATAW *pd_in = (POPUPDATAW *)lParam;

	if (IsWindow(hwndPop)) {
		PopupData pd_out;
		pd_out.cbSize = sizeof(PopupData);
		pd_out.flags = PDF_UNICODE;

		pd_out.pwzTitle = mir_wstrdup(pd_in->lpwzContactName);
		pd_out.pwzText = mir_wstrdup(pd_in->lpwzText);
		StripBBCodesInPlace(pd_out.pwzTitle);
		StripBBCodesInPlace(pd_out.pwzText);

		pd_out.hContact = pd_in->lchContact;
		pd_out.SetIcon(pd_in->lchIcon);
		if (pd_in->colorBack == 0xffffffff) // that's the old #define for 'skinned bg'
			pd_out.colorBack = pd_out.colorText = 0;
		else {
			pd_out.colorBack = pd_in->colorBack & 0xFFFFFF;
			pd_out.colorText = pd_in->colorText & 0xFFFFFF;
		}
		pd_out.colorBack = pd_in->colorBack;
		pd_out.colorText = pd_in->colorText;
		pd_out.windowProc = pd_in->PluginWindowProc;
		pd_out.opaque = pd_in->PluginData;
		pd_out.timeout = pd_in->iSeconds;

		lstPopupHistory.Add(pd_out.pwzTitle, pd_out.pwzText, time(0));
	
		SendMessage(hwndPop, PUM_CHANGE, 0, (LPARAM)&pd_out);
	}
	return 0;
}

static INT_PTR ShowMessage(WPARAM wParam, LPARAM lParam)
{
	if ( !db_get_b(0, MODULE, "Enabled", 1)) return 0;

	POPUPDATAT pd = {0};
	_tcscpy(pd.lptzContactName, lParam == SM_WARNING ? _T("Warning") : _T("Notification"));
	pd.lchIcon = LoadIcon(0, lParam == SM_WARNING ? IDI_WARNING : IDI_INFORMATION);
	_tcsncpy(pd.lptzText, _A2T((char *)wParam), MAX_SECONDLINE); pd.lptzText[MAX_SECONDLINE-1] = 0;
	CallService(MS_POPUP_ADDPOPUPT, (WPARAM)&pd, 0);
	return 0;
}

static INT_PTR ShowMessageW(WPARAM wParam, LPARAM lParam)
{
	if ( !db_get_b(0, MODULE, "Enabled", 1)) return 0;

	POPUPDATAW pd = {0};
	wcscpy(pd.lpwzContactName, lParam == SM_WARNING ? L"Warning" : L"Notification");
	pd.lchIcon = LoadIcon(0, lParam == SM_WARNING ? IDI_WARNING : IDI_INFORMATION);
	wcsncpy(pd.lpwzText, (wchar_t *)wParam, MAX_SECONDLINE);
	CallService(MS_POPUP_ADDPOPUPW, (WPARAM)&pd, 0);
	return 0;
}

//=====PopUp/ShowHistory

INT_PTR PopUp_ShowHistory(WPARAM wParam, LPARAM lParam)
{
	if (!hHistoryWindow)
		hHistoryWindow = CreateDialog(hInst, MAKEINTRESOURCE(IDD_LST_HISTORY), NULL, DlgProcHistLst);

	ShowWindow(hHistoryWindow, SW_SHOW);
	return 0;
}

LIST<POPUPCLASS> arClasses(3);

static INT_PTR RegisterPopupClass(WPARAM wParam, LPARAM lParam)
{
	POPUPCLASS *pc = (POPUPCLASS*)mir_alloc( sizeof(POPUPCLASS));
	memcpy(pc, (PVOID)lParam, sizeof(POPUPCLASS));

	pc->pszName = mir_strdup(pc->pszName);
	if (pc->flags & PCF_UNICODE)
		pc->pwszDescription = mir_wstrdup(pc->pwszDescription);
	else
		pc->pszDescription = mir_strdup(pc->pszDescription);
	
	char setting[256];
	mir_snprintf(setting, 256, "%s/Timeout", pc->pszName);
	pc->iSeconds = DBGetContactSettingWord(0, MODULE, setting, pc->iSeconds);
	if (pc->iSeconds == (WORD)-1) pc->iSeconds = -1;
	mir_snprintf(setting, 256, "%s/TextCol", pc->pszName);
	pc->colorText = (COLORREF)db_get_dw(0, MODULE, setting, (DWORD)pc->colorText);
	mir_snprintf(setting, 256, "%s/BgCol", pc->pszName);
	pc->colorBack = (COLORREF)db_get_dw(0, MODULE, setting, (DWORD)pc->colorBack);

	arClasses.insert(pc);
	return (INT_PTR)pc;
}

static void FreePopupClass(POPUPCLASS *pc)
{
	mir_free(pc->pszName);
	mir_free(pc->pszDescription);
	mir_free(pc);
}

static INT_PTR UnregisterPopupClass(WPARAM wParam, LPARAM lParam)
{
	POPUPCLASS *pc = (POPUPCLASS*)lParam;
	if (pc == NULL)
		return 1;

	for (int i=0; i < arClasses.getCount(); i++)
		if (arClasses[i] == pc) {
			arClasses.remove(i);
			FreePopupClass(pc);
			return 0;
		}

	return 1;
}

static INT_PTR CreateClassPopup(WPARAM wParam, LPARAM lParam)
{
	POPUPDATACLASS *pdc = (POPUPDATACLASS *)lParam;
	if (pdc->cbSize < sizeof(POPUPDATACLASS)) return 1;

	POPUPCLASS *pc = 0;
	if (wParam) 
		pc = (POPUPCLASS *)wParam;
	else {
		for (int i = 0; i < arClasses.getCount(); i++) {
			if ( strcmp(arClasses[i]->pszName, pdc->pszClassName) == 0) {
				pc = arClasses[i];
				break;
			}
		}
	}
	if (pc) {
		PopupData pd = {sizeof(PopupData)};
		if (pc->flags & PCF_UNICODE) pd.flags |= PDF_UNICODE;
		pd.colorBack = pc->colorBack;
		pd.colorText = pc->colorText;
		pd.SetIcon(pc->hIcon);
		pd.timeout = pc->iSeconds;
		pd.windowProc = pc->PluginWindowProc;

		pd.hContact = pdc->hContact;
		pd.opaque = pdc->PluginData;
		pd.pszTitle = (char *)pdc->pszTitle;
		pd.pszText = (char *)pdc->pszText;
	
		ShowPopup(pd);
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////////////

void InitServices() 
{
	CreateServiceFunction(MS_POPUP_REGISTERCLASS, RegisterPopupClass);
	CreateServiceFunction(MS_POPUP_UNREGISTERCLASS, UnregisterPopupClass);

	CreateServiceFunction(MS_POPUP_ADDPOPUPCLASS, CreateClassPopup);
	CreateServiceFunction(MS_POPUP_ADDPOPUP, CreatePopup);
	CreateServiceFunction(MS_POPUP_ADDPOPUPW, CreatePopupW);
	CreateServiceFunction(MS_POPUP_CHANGETEXTW, ChangeTextW);
	CreateServiceFunction(MS_POPUP_CHANGEW, PopupChangeW);
	CreateServiceFunction(MS_POPUP_GETCONTACT, GetContact);
	CreateServiceFunction(MS_POPUP_GETPLUGINDATA, GetOpaque);
	CreateServiceFunction(MS_POPUP_ISSECONDLINESHOWN, IsSecondLineShown);
	CreateServiceFunction(MS_POPUP_QUERY, PopupQuery);

	CreateServiceFunction(MS_POPUP_SHOWMESSAGE, ShowMessage);
	CreateServiceFunction(MS_POPUP_SHOWMESSAGE"W", ShowMessageW);

	CreateServiceFunction(MS_POPUP_SHOWHISTORY, PopUp_ShowHistory);
	CreateServiceFunction("PopUp/ToggleEnabled", TogglePopups);

	////////////////////////////////////////////////////////////////////////////
	// Menus

	CLISTMENUITEM mi = { sizeof(mi) };
	mi.flags = CMIM_ALL;
	mi.position = 500010000;
	mi.pszPopupName = LPGEN("PopUps");

	hiPopupHistory = LoadIcon(hInst, MAKEINTRESOURCE(IDI_POPUP_HISTORY));
	mi.hIcon = hiPopupHistory;
	mi.pszService= MS_POPUP_SHOWHISTORY;
	mi.pszName = LPGEN("Popup History");
	hMenuShowHistory = Menu_AddMainMenuItem(&mi);
	
	mi.hIcon = NULL;
	mi.pszService = "PopUp/ToggleEnabled";
	mi.pszName = (char*)(db_get_b(0, MODULE, "Enabled", 1) ? LPGEN("Disable Popups") : LPGEN("Enable Popups"));
	hMenuToggleOnOff = Menu_AddMainMenuItem(&mi);
}

void DeinitServices()
{
	for (int i = 0; i < arClasses.getCount(); i++)
		FreePopupClass(arClasses[i]);
	arClasses.destroy();
}
