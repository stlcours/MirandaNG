#include "common.h"

int CDropbox::OnModulesLoaded(void *obj, WPARAM wParam, LPARAM lParam)
{
	HookEventObj(ME_DB_CONTACT_DELETED, OnContactDeleted, obj);
	HookEventObj(ME_OPT_INITIALISE, OnOptionsInitialized, obj);
	HookEventObj(ME_CLIST_PREBUILDCONTACTMENU, OnPrebuildContactMenu, obj);

	HookEventObj(ME_MSG_WINDOWEVENT, OnSrmmWindowOpened, obj);
	HookEventObj(ME_FILEDLG_CANCELED, OnFileDialogCancelled, obj);
	HookEventObj(ME_FILEDLG_SUCCEEDED, OnFileDialogSuccessed, obj);

	NETLIBUSER nlu = { sizeof(nlu) };
	nlu.flags = NUF_INCOMING | NUF_OUTGOING | NUF_HTTPCONNS | NUF_TCHAR;
	nlu.szSettingsModule = MODULE;
	nlu.szSettingsModule = MODULE;
	nlu.ptszDescriptiveName = L"Dropbox";

	CDropbox *instance = (CDropbox*)obj;

	instance->hNetlibUser = (HANDLE)CallService(MS_NETLIB_REGISTERUSER, 0, (LPARAM)&nlu);

	instance->GetDefaultContact();

	if (ServiceExists(MS_BB_ADDBUTTON))
	{
		BBButton bbd = { sizeof(bbd) };
		bbd.pszModuleName = MODULE;

		bbd.bbbFlags = BBBF_ISIMBUTTON | BBBF_ISRSIDEBUTTON;
		bbd.ptszTooltip = TranslateT("Send files to Dropbox");
		bbd.hIcon = LoadSkinnedIconHandle(SKINICON_EVENT_FILE);
		bbd.dwButtonID = BBB_ID_FILE_SEND;
		bbd.dwDefPos = 100 + bbd.dwButtonID;
		CallService(MS_BB_ADDBUTTON, 0, (LPARAM)&bbd);

		HookEventObj(ME_MSG_BUTTONPRESSED, OnTabSrmmButtonPressed, obj);
	}

	return 0;
}

int CDropbox::OnPreShutdown(void *obj, WPARAM wParam, LPARAM lParam)
{
	if (ServiceExists(MS_BB_ADDBUTTON))
	{
		BBButton bbd = { sizeof(bbd) };
		bbd.pszModuleName = MODULE;

		bbd.dwButtonID = BBB_ID_FILE_SEND;
		CallService(MS_BB_REMOVEBUTTON, 0, (LPARAM)&bbd);
	}

	return 0;
}

int CDropbox::OnContactDeleted(void *obj, WPARAM hContact, LPARAM lParam)
{
	CDropbox *instance = (CDropbox*)obj;

	if (lstrcmpiA(GetContactProto(hContact), MODULE) == 0)
	{
		if (instance->HasAccessToken())
			instance->DestroyAcceessToken();
	}

	return 0;
}

int CDropbox::OnOptionsInitialized(void *obj, WPARAM wParam, LPARAM lParam)
{
	OPTIONSDIALOGPAGE odp = { sizeof(odp) };
	odp.position = 100000000;
	odp.hInstance = g_hInstance;
	odp.flags = ODPF_BOLDGROUPS;
	odp.pszTemplate = MAKEINTRESOURCEA(IDD_OPTIONS_MAIN);
	odp.pszGroup = LPGEN("Network");
	odp.pszTitle = "Dropbox";
	odp.pfnDlgProc = MainOptionsProc;
	odp.dwInitParam = (LPARAM)obj;

	Options_AddPage(wParam, &odp);

	return 0;
}

int CDropbox::OnSrmmWindowOpened(void *obj, WPARAM wParam, LPARAM lParam)
{
	CDropbox *instance = (CDropbox*)obj;

	MessageWindowEventData *ev = (MessageWindowEventData*)lParam;
	if (ev->uType == MSG_WINDOW_EVT_OPENING && ev->hContact)
	{
		char *proto = GetContactProto(ev->hContact);
		WORD status = db_get_w(ev->hContact, proto, "Status", ID_STATUS_OFFLINE);
		bool canSendOffline = (CallProtoService(proto, PS_GETCAPS, PFLAGNUM_4, 0) & PF4_IMSENDOFFLINE) > 0;

		BBButton bbd = { sizeof(bbd) };
		bbd.pszModuleName = MODULE;
		bbd.dwButtonID = BBB_ID_FILE_SEND;
		bbd.bbbFlags = BBSF_RELEASED;
		if (ev->hContact == instance->GetDefaultContact() || !instance->HasAccessToken())
			bbd.bbbFlags = BBSF_HIDDEN | BBSF_DISABLED;
		else if (status == ID_STATUS_OFFLINE && !canSendOffline)
			bbd.bbbFlags = BBSF_DISABLED;

		CallService(MS_BB_SETBUTTONSTATE, ev->hContact, (LPARAM)&bbd);
	}

	return 0;
}

int CDropbox::OnTabSrmmButtonPressed(void *obj, WPARAM wParam, LPARAM lParam)
{
	CDropbox *instance = (CDropbox*)obj;

	CustomButtonClickData *cbc = (CustomButtonClickData *)lParam;
	if (!strcmp(cbc->pszModule, MODULE) && cbc->dwButtonId == BBB_ID_FILE_SEND && cbc->hContact)
	{
		instance->hTransferContact = cbc->hContact;

		HWND hwnd = (HWND)CallService(MS_FILE_SENDFILE, instance->GetDefaultContact(), 0);

		instance->dcftp[hwnd] = cbc->hContact;

		BBButton bbd = { sizeof(bbd) };
		bbd.pszModuleName = MODULE;
		bbd.dwButtonID = BBB_ID_FILE_SEND;
		bbd.bbbFlags = BBSF_DISABLED;

		CallService(MS_BB_SETBUTTONSTATE, cbc->hContact, (LPARAM)&bbd);
	}

	return 0; 
}

void __stdcall EnableTabSrmmButtonAsync(void *arg)
{
	BBButton bbd = { sizeof(bbd) };
	bbd.pszModuleName = MODULE;
	bbd.dwButtonID = BBB_ID_FILE_SEND;
	bbd.bbbFlags = BBSF_RELEASED;
	MCONTACT hContact = (MCONTACT)arg;

	CallService(MS_BB_SETBUTTONSTATE, hContact, (LPARAM)&bbd);
}

int CDropbox::OnFileDialogCancelled(void *obj, WPARAM hContact, LPARAM lParam)
{
	CDropbox *instance = (CDropbox*)obj;

	HWND hwnd = (HWND)lParam;
	if (instance->hTransferContact == instance->dcftp[hwnd])
	{
		CallFunctionAsync(EnableTabSrmmButtonAsync, (void*)instance->hTransferContact);

		instance->dcftp.erase((HWND)lParam);
		instance->hTransferContact = 0;
	}

	return 0;
}

int CDropbox::OnFileDialogSuccessed(void *obj, WPARAM hContact, LPARAM lParam)
{
	CDropbox *instance = (CDropbox*)obj;

	HWND hwnd = (HWND)lParam;
	if (instance->hTransferContact == instance->dcftp[hwnd])
	{
		instance->dcftp.erase((HWND)lParam);

		CallFunctionAsync(EnableTabSrmmButtonAsync, (void*)instance->hTransferContact);
	}

	return 0;
}

int CDropbox::OnSendSuccessed(void *obj, WPARAM hContact, LPARAM lParam)
{
	wchar_t *data = (wchar_t*)lParam;
	CDropbox *instance = (CDropbox*)obj;

	if (instance->hTransferContact)
		instance->hTransferContact = 0;

	if (db_get_b(NULL, MODULE, "UrlAutoSend", 1))
	{
		DBEVENTINFO dbei = { sizeof(dbei) };
		dbei.flags = DBEF_UTF;
		dbei.szModule = MODULE;
		dbei.timestamp = time(NULL);
		dbei.eventType = EVENTTYPE_MESSAGE;
		dbei.cbBlob = wcslen(data);
		dbei.pBlob = (PBYTE)mir_utf8encodeW(data);

		if (hContact != instance->GetDefaultContact())
		{
			if (CallContactService(hContact, PSS_MESSAGE, 0, (LPARAM)data) != ACKRESULT_FAILED)
			{
				dbei.flags = DBEF_SENT | DBEF_READ | DBEF_UTF;
				db_event_add(hContact, &dbei);
			}
			else
				CallServiceSync(MS_MSG_SENDMESSAGEW, (WPARAM)hContact, (LPARAM)data);
		}
		else
			db_event_add(hContact, &dbei);
	}

	CMString urls = data; urls += L"\r\n";

	if (db_get_b(NULL, MODULE, "UrlPasteToMessageInputArea", 0))
		CallServiceSync(MS_MSG_SENDMESSAGEW, (WPARAM)hContact, (LPARAM)urls.GetBuffer());

	if (db_get_b(NULL, MODULE, "UrlCopyToClipboard", 0))
	{
		if (OpenClipboard(NULL))
		{
			EmptyClipboard();
			size_t size = sizeof(wchar_t) * (urls.GetLength() + 1);
			HGLOBAL hClipboardData = GlobalAlloc(NULL, size);
			if (hClipboardData)
			{
				wchar_t* pchData = (wchar_t*)GlobalLock(hClipboardData);
				if (pchData)
				{
					memcpy(pchData, (wchar_t*)urls.GetString(), size);
					GlobalUnlock(hClipboardData);
					SetClipboardData(CF_UNICODETEXT, hClipboardData);
				}
			}
			CloseClipboard();
		}
	}

	return 0;
}

int CDropbox::OnProtoAck(void *obj, WPARAM wParam, LPARAM lParam)
{
	ACKDATA *ack = (ACKDATA*) lParam;

	if ( !strcmp(ack->szModule, MODULE)) {
		return 0; // don't rebroadcast our own acks
	}

	if (ack->type == ACKTYPE_STATUS/* && ((int)ack->lParam != (int)ack->hProcess)*/)
	{
		WORD status = ack->lParam;
		bool canSendOffline = (CallProtoService(ack->szModule, PS_GETCAPS, PFLAGNUM_4, 0) & PF4_IMSENDOFFLINE) > 0;

		MessageWindowInputData msgwi = { sizeof(msgwi) };
		msgwi.uFlags = MSG_WINDOW_UFLAG_MSG_BOTH;

		for (MCONTACT hContact = db_find_first(ack->szModule); hContact; hContact = db_find_next(hContact, ack->szModule))
		{
			msgwi.hContact = hContact;

			MessageWindowData msgw;
			msgw.cbSize = sizeof(msgw);

			if (!CallService(MS_MSG_GETWINDOWDATA, (WPARAM)&msgwi, (LPARAM)&msgw) && msgw.uState & MSG_WINDOW_STATE_EXISTS)
			{
				BBButton bbd = { sizeof(bbd) };
				bbd.pszModuleName = MODULE;
				bbd.dwButtonID = BBB_ID_FILE_SEND;
				bbd.bbbFlags = BBSF_RELEASED;
			
				if (status == ID_STATUS_OFFLINE && !canSendOffline)
					bbd.bbbFlags = BBSF_DISABLED;

				CallService(MS_BB_SETBUTTONSTATE, hContact, (LPARAM)&bbd);
			}
		}
	}

	return 0;
}