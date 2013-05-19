/*
 * astyle --force-indent=tab=4 --brackets=linux --indent-switches
 *		  --pad=oper --one-line=keep-blocks  --unpad=paren
 *
 * Miranda NG: the free IM client for Microsoft* Windows*
 *
 * Copyright 2000-2009 Miranda ICQ/IM project,
 * all portions of this codebase are copyrighted to the people
 * listed in contributors.txt.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * you should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * part of tabSRMM messaging plugin for Miranda.
 *
 * (C) 2005-2010 by silvercircle _at_ gmail _dot_ com and contributors
 *
 * Load, setup and shutdown the plugin
 * core plugin messaging services (single IM chats only).
 *
 */

#include "commonheaders.h"

#define IDI_CORE_LOAD	132					// icon id for the "connecting" icon

REOLECallback*		mREOLECallback;
NEN_OPTIONS 		nen_options;
static HANDLE 		hUserPrefsWindowLis = 0;
HMODULE 			g_hIconDLL = 0;

static void 	UnloadIcons();

int     Chat_IconsChanged(WPARAM wp, LPARAM lp);
void    Chat_AddIcons(void);
int     Chat_PreShutdown(WPARAM wParam, LPARAM lParam);

/*
 * fired event when user changes IEView plugin options. Apply them to all open tabs
 */

int IEViewOptionsChanged(WPARAM,LPARAM)
{
	M->BroadcastMessage(DM_IEVIEWOPTIONSCHANGED, 0, 0);
	return 0;
}

/*
 * fired event when user changes smileyadd options. Notify all open tabs about the changes
 */

int SmileyAddOptionsChanged(WPARAM,LPARAM)
{
	M->BroadcastMessage(DM_SMILEYOPTIONSCHANGED, 0, 0);
	if (PluginConfig.m_chat_enabled)
		SM_BroadcastMessage(NULL, DM_SMILEYOPTIONSCHANGED, 0, 0, FALSE);
	return 0;
}

/*
 * Message API 0.0.0.3 services
 */

static INT_PTR GetWindowClass(WPARAM wParam, LPARAM lParam)

{
	char *szBuf = (char*)wParam;
	int size = (int)lParam;
	mir_snprintf(szBuf, size, "tabSRMM");
	return 0;
}

/*
 * service function. retrieves the message window data for a given hcontact or window
 * wParam == hContact of the window to find
 * lParam == window handle (set it to 0 if you want search for hcontact, otherwise it
 * is directly used as the handle for the target window
 */

static INT_PTR GetWindowData(WPARAM wParam, LPARAM lParam)
{
	MessageWindowInputData *mwid = (MessageWindowInputData*)wParam;
	MessageWindowOutputData *mwod = (MessageWindowOutputData*)lParam;

	if (mwid == NULL || mwod == NULL)
		return 1;
	if (mwid->cbSize != sizeof(MessageWindowInputData) || mwod->cbSize != sizeof(MessageWindowOutputData))
		return 1;
	if (mwid->hContact == NULL)
		return 1;
	if (mwid->uFlags != MSG_WINDOW_UFLAG_MSG_BOTH)
		return 1;
	HWND hwnd = M->FindWindow(mwid->hContact);
	if (hwnd) {
		mwod->uFlags = MSG_WINDOW_UFLAG_MSG_BOTH;
		mwod->hwndWindow = hwnd;
		mwod->local = GetParent(GetParent(hwnd));
		SendMessage(hwnd, DM_GETWINDOWSTATE, 0, 0);
		mwod->uState = (int)GetWindowLongPtr(hwnd, DWLP_MSGRESULT);
		return 0;
	}
	else
	{
		SESSION_INFO *si = SM_FindSessionByHCONTACT(mwid->hContact);
		if (si != NULL && si->hWnd != 0) {
			mwod->uFlags = MSG_WINDOW_UFLAG_MSG_BOTH;
			mwod->hwndWindow = si->hWnd;
			mwod->local = GetParent(GetParent(si->hWnd));
			SendMessage(si->hWnd, DM_GETWINDOWSTATE, 0, 0);
			mwod->uState = (int)GetWindowLongPtr(si->hWnd, DWLP_MSGRESULT);
			return 0;
		}
		else {
			mwod->uState = 0;
			mwod->hContact = 0;
			mwod->hwndWindow = 0;
			mwod->uFlags = 0;
		}
	}
	return 1;
}

/*
 * service function. Sets a status bar text for a contact
 */

static INT_PTR SetStatusText(WPARAM wParam, LPARAM lParam)
{
	TContainerData *pContainer;

	HWND hwnd = M->FindWindow((HANDLE)wParam);
	if (hwnd != NULL) {
		TWindowData *dat = (TWindowData*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
		if (dat == NULL || (pContainer = dat->pContainer) == NULL)
			return 1;

		if (lParam == NULL) {
			DM_UpdateLastMessage(dat);
			return 0;
		}

		_tcsncpy(dat->szStatusBar, (TCHAR *)lParam, SIZEOF(dat->szStatusBar));

		if (pContainer->hwndActive != dat->hwnd)
			return 1;
	}
	else {
		SESSION_INFO *si = SM_FindSessionByHCONTACT((HANDLE)wParam);
		if (si == NULL || si->hWnd == 0 || (pContainer = si->pContainer) == NULL || pContainer->hwndActive != si->hWnd)
			return 1;
	}

	SendMessage(pContainer->hwndStatus, SB_SETICON, 0, 0);
	SendMessage(pContainer->hwndStatus, SB_SETTEXT, 0, lParam);
	return 0;
}

/*
 * service function. Invoke the user preferences dialog for the contact given (by handle) in wParam
 */

static INT_PTR SetUserPrefs(WPARAM wParam, LPARAM)
{
	HWND hWnd = WindowList_Find(PluginConfig.hUserPrefsWindowList, (HANDLE)wParam);
	if (hWnd) {
		SetForegroundWindow(hWnd);			// already open, bring it to front
		return 0;
	}
	CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_USERPREFS_FRAME), 0, DlgProcUserPrefsFrame, (LPARAM)wParam);
	return 0;
}

/*
 * service function - open the tray menu from the TTB button
 */

static INT_PTR Service_OpenTrayMenu(WPARAM wParam, LPARAM lParam)
{
	SendMessage(PluginConfig.g_hwndHotkeyHandler, DM_TRAYICONNOTIFY, 101, lParam == 0 ? WM_LBUTTONUP : WM_RBUTTONUP);
	return 0;
}

/*
 * service function. retrieves the message window flags for a given hcontact or window
 * wParam == hContact of the window to find
 * lParam == window handle (set it to 0 if you want search for hcontact, otherwise it
 * is directly used as the handle for the target window
 */

static INT_PTR GetMessageWindowFlags(WPARAM wParam, LPARAM lParam)
{
	HWND hwndTarget = (HWND)lParam;

	if (hwndTarget == 0)
		hwndTarget = M->FindWindow((HANDLE)wParam);

	if (hwndTarget) {
		struct TWindowData *dat = (struct TWindowData *)GetWindowLongPtr(hwndTarget, GWLP_USERDATA);
		if (dat)
			return (dat->dwFlags);
		else
			return 0;
	} else
		return 0;
}

/*
 * return the version of the window api supported
 */

static INT_PTR GetWindowAPI(WPARAM,LPARAM)
{
	return PLUGIN_MAKE_VERSION(0, 0, 0, 2);
}

/*
 * service function finds a message session
 * wParam = contact handle for which we want the window handle
 * thanks to bio for the suggestion of this service
 * if wParam == 0, then lParam is considered to be a valid window handle and
 * the function tests the popup mode of the target container

 * returns the hwnd if there is an open window or tab for the given hcontact (wParam),
 * or (if lParam was specified) the hwnd if the window exists.
 * 0 if there is none (or the popup mode of the target container was configured to "hide"
 * the window..
 */

INT_PTR MessageWindowOpened(WPARAM wParam, LPARAM lParam)
{
	HWND hwnd = 0;
	TContainerData *pContainer = NULL;

	if (wParam)
		hwnd = M->FindWindow((HANDLE)wParam);
	else if (lParam)
		hwnd = (HWND) lParam;
	else
		return NULL;

	if (!hwnd)
		return 0;

	SendMessage(hwnd, DM_QUERYCONTAINER, 0, (LPARAM)&pContainer);
	if (pContainer) {
		if (pContainer->dwFlags & CNT_DONTREPORT) {
			if (IsIconic(pContainer->hwnd))
				return 0;
		}
		if (pContainer->dwFlags & CNT_DONTREPORTUNFOCUSED) {
			if (!IsIconic(pContainer->hwnd) && GetForegroundWindow() != pContainer->hwnd && GetActiveWindow() != pContainer->hwnd)
				return 0;
		}
		if (pContainer->dwFlags & CNT_ALWAYSREPORTINACTIVE) {
			if (pContainer->dwFlags & CNT_DONTREPORTFOCUSED)
				return 0;

			return pContainer->hwndActive == hwnd;
		}
	}
	return 1;
}

/*
 * ReadMessageCommand is executed whenever the user wants to manually open a window.
 * This can happen when double clicking a contact on the clist OR when opening a new
 * message (clicking on a popup, clicking the flashing tray icon and so on).
 */

static INT_PTR ReadMessageCommand(WPARAM, LPARAM lParam)
{
	HANDLE hContact = ((CLISTEVENT *) lParam)->hContact;

	HWND hwndExisting = M->FindWindow(hContact);
	if (hwndExisting != 0)
		SendMessage(hwndExisting, DM_ACTIVATEME, 0, 0);
	else {
		TCHAR szName[CONTAINER_NAMELEN + 1];
		GetContainerNameForContact(hContact, szName, CONTAINER_NAMELEN);
		TContainerData *pContainer = FindContainerByName(szName);
		if (pContainer == NULL)
			pContainer = CreateContainer(szName, FALSE, hContact);
		CreateNewTabForContact(pContainer, hContact, 0, NULL, TRUE, TRUE, FALSE, 0);
	}
	return 0;
}


/*
 * this is the Unicode version of the SendMessageCommand handler. It accepts wchar_t strings
 * for filling the message input box with a passed message
 */

INT_PTR SendMessageCommand_W(WPARAM wParam, LPARAM lParam)
{
	HANDLE hContact = (HANDLE) wParam;
	struct TNewWindowData newData = {
		0
	};
	int isSplit = 1;

	/*
	 * make sure that only the main UI thread will handle window creation
     */
	if (GetCurrentThreadId() != PluginConfig.dwThreadID) {
		if (lParam) {
			unsigned iLen = lstrlenW((wchar_t *)lParam);
			wchar_t *tszText = (wchar_t *)malloc((iLen + 1) * sizeof(wchar_t));
			wcsncpy(tszText, (wchar_t *)lParam, iLen + 1);
			tszText[iLen] = 0;
			PostMessage(PluginConfig.g_hwndHotkeyHandler, DM_SENDMESSAGECOMMANDW, wParam, (LPARAM)tszText);
		} else
			PostMessage(PluginConfig.g_hwndHotkeyHandler, DM_SENDMESSAGECOMMANDW, wParam, 0);
		return 0;
	}

	/* does the HCONTACT's protocol support IM messages? */
	char *szProto = GetContactProto(hContact);
	if (szProto) {
		if (!CallProtoService(szProto, PS_GETCAPS, PFLAGNUM_1, 0) & PF1_IMSEND)
			return 0;
	} else {
		/* unknown contact */
		return 0;
	}
	
	HWND hwnd = M->FindWindow(hContact);
	if (hwnd) {
		if (lParam) {
			HWND hEdit = GetDlgItem(hwnd, IDC_MESSAGE);
			SendMessage(hEdit, EM_SETSEL, -1, SendMessage(hEdit, WM_GETTEXTLENGTH, 0, 0));
			SendMessage(hEdit, EM_REPLACESEL, FALSE, (LPARAM)(TCHAR *) lParam);
		}
		SendMessage(hwnd, DM_ACTIVATEME, 0, (LPARAM)0);
	} else {
		TCHAR szName[CONTAINER_NAMELEN + 1];

		GetContainerNameForContact(hContact, szName, CONTAINER_NAMELEN);
		TContainerData *pContainer = FindContainerByName(szName);
		if (pContainer == NULL)
			pContainer = CreateContainer(szName, FALSE, hContact);
		CreateNewTabForContact(pContainer, hContact, 1, (const char *)lParam, TRUE, TRUE, FALSE, 0);
	}
	return 0;
}

/*
 * the SendMessageCommand() invokes a message session window for the given contact.
 * e.g. it is called when user double clicks a contact on the contact list
 * it is implemented as a service, so external plugins can use it to open a message window.
 * contacts handle must be passed in wParam.
 */

INT_PTR SendMessageCommand(WPARAM wParam, LPARAM lParam)
{
	HANDLE hContact = (HANDLE) wParam;
	struct TNewWindowData newData = {
		0
	};
	int isSplit = 1;

	if (GetCurrentThreadId() != PluginConfig.dwThreadID) {
		if (lParam) {
			unsigned iLen = lstrlenA((char *)lParam);
			char *szText = (char *)malloc(iLen + 1);
			strncpy(szText, (char *)lParam, iLen + 1);
			szText[iLen] = 0;
			PostMessage(PluginConfig.g_hwndHotkeyHandler, DM_SENDMESSAGECOMMAND, wParam, (LPARAM)szText);
		} else
			PostMessage(PluginConfig.g_hwndHotkeyHandler, DM_SENDMESSAGECOMMAND, wParam, 0);
		return 0;
	}

	/* does the HCONTACT's protocol support IM messages? */
	char *szProto = GetContactProto(hContact);
	if (szProto) {
		if (!CallProtoService(szProto, PS_GETCAPS, PFLAGNUM_1, 0) & PF1_IMSEND)
			return 0;
	} else {
		/* unknown contact */
		return 0;
	}
	
	HWND hwnd = M->FindWindow(hContact);
	if (hwnd) {
		if (lParam) {
			HWND hEdit = GetDlgItem(hwnd, IDC_MESSAGE);
			SendMessage(hEdit, EM_SETSEL, -1, SendMessage(hEdit, WM_GETTEXTLENGTH, 0, 0));
			SendMessageA(hEdit, EM_REPLACESEL, FALSE, (LPARAM)(char *) lParam);
		}
		SendMessage(hwnd, DM_ACTIVATEME, 0, 0);          // ask the message window about its parent...
	} else {
		TCHAR szName[CONTAINER_NAMELEN + 1];
		GetContainerNameForContact((HANDLE) wParam, szName, CONTAINER_NAMELEN);
		TContainerData *pContainer = FindContainerByName(szName);
		if (pContainer == NULL)
			pContainer = CreateContainer(szName, FALSE, hContact);
		CreateNewTabForContact(pContainer, hContact, 0, (const char *) lParam, TRUE, TRUE, FALSE, 0);
	}
	return 0;
}

/*
 * open a window when user clicks on the flashing "typing message" tray icon.
 * just calls SendMessageCommand() for the given contact.
 */
static INT_PTR TypingMessageCommand(WPARAM wParam, LPARAM lParam)
{
	CLISTEVENT *cle = (CLISTEVENT *) lParam;

	if (!cle)
		return 0;
	SendMessageCommand((WPARAM)cle->hContact, 0);
	return 0;
}

int SplitmsgShutdown(void)
{
#if defined(__USE_EX_HANDLERS)
	__try {
#endif
		DestroyCursor(PluginConfig.hCurSplitNS);
		DestroyCursor(PluginConfig.hCurHyperlinkHand);
		DestroyCursor(PluginConfig.hCurSplitWE);
		FreeLibrary(GetModuleHandleA("riched20"));
		if (g_hIconDLL)
			FreeLibrary(g_hIconDLL);

		ImageList_RemoveAll(PluginConfig.g_hImageList);
		ImageList_Destroy(PluginConfig.g_hImageList);

		delete Win7Taskbar;
		delete mREOLECallback;

		OleUninitialize();
		DestroyMenu(PluginConfig.g_hMenuContext);
		if (PluginConfig.g_hMenuContainer)
			DestroyMenu(PluginConfig.g_hMenuContainer);
		if (PluginConfig.g_hMenuEncoding)
			DestroyMenu(PluginConfig.g_hMenuEncoding);

		UnloadIcons();
		FreeTabConfig();

		if (Utils::rtf_ctable)
			free(Utils::rtf_ctable);

		UnloadTSButtonModule();

		if (g_hIconDLL) {
			FreeLibrary(g_hIconDLL);
			g_hIconDLL = 0;
		}
#if defined(__USE_EX_HANDLERS)
	}
	__except(CGlobals::Ex_ShowDialog(GetExceptionInformation(), __FILE__, __LINE__, L"SHUTDOWN_STAGE3", false)) {
		return 0;
	}
#endif
	return 0;
}

int MyAvatarChanged(WPARAM wParam, LPARAM lParam)
{
	if (wParam == 0 || IsBadReadPtr((void*)wParam, 4))
		return 0;

	for (TContainerData *p = pFirstContainer; p; p = p->pNext)
		BroadCastContainer(p, DM_MYAVATARCHANGED, wParam, lParam);
	return 0;
}

int AvatarChanged(WPARAM wParam, LPARAM lParam)
{
	struct avatarCacheEntry *ace = (struct avatarCacheEntry *)lParam;
	HWND hwnd = M->FindWindow((HANDLE)wParam);

	if (wParam == 0) {			// protocol picture has changed...
		M->BroadcastMessage(DM_PROTOAVATARCHANGED, wParam, lParam);
		return 0;
	}
	if (hwnd) {
		struct TWindowData *dat = (struct TWindowData *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
		if (dat) {
			dat->ace = ace;
			if (dat->hTaskbarIcon)
				DestroyIcon(dat->hTaskbarIcon);
			dat->hTaskbarIcon = 0;
			DM_RecalcPictureSize(dat);
			if (dat->showPic == 0 || dat->showInfoPic == 0)
				GetAvatarVisibility(hwnd, dat);
			if (dat->hwndPanelPic) {
				dat->panelWidth = -1;				// force new size calculations (not for flash avatars)
				SendMessage(dat->hwnd, WM_SIZE, 0, 1);
			}
				dat->panelWidth = -1;				// force new size calculations (not for flash avatars)
				RedrawWindow(dat->hwnd, NULL, NULL, RDW_INVALIDATE|RDW_UPDATENOW|RDW_ALLCHILDREN);
				SendMessage(dat->hwnd, WM_SIZE, 0, 1);
			ShowPicture(dat, TRUE);
			dat->dwFlagsEx |= MWF_EX_AVATARCHANGED;
			dat->pContainer->SideBar->updateSession(dat);
		}
	}
	return 0;
}

int IcoLibIconsChanged(WPARAM wParam, LPARAM lParam)
{
	LoadFromIconLib();
	CacheMsgLogIcons();
	if (PluginConfig.m_chat_enabled)
		Chat_IconsChanged(wParam, lParam);
	return 0;
}

int IconsChanged(WPARAM wParam, LPARAM lParam)
{
	CreateImageList(FALSE);
	CacheMsgLogIcons();
	M->BroadcastMessage(DM_OPTIONSAPPLIED, 0, 0);
	M->BroadcastMessage(DM_UPDATEWINICON, 0, 0);
	if (PluginConfig.m_chat_enabled)
		Chat_IconsChanged(wParam, lParam);

	return 0;
}

/**
 * initialises the internal API, services, events etc...
 */

static void TSAPI InitAPI()
{
	CreateServiceFunction(MS_MSG_SENDMESSAGE,     SendMessageCommand);
	CreateServiceFunction(MS_MSG_SENDMESSAGE "W", SendMessageCommand_W);
	CreateServiceFunction(MS_MSG_GETWINDOWAPI,    GetWindowAPI);
	CreateServiceFunction(MS_MSG_GETWINDOWCLASS,  GetWindowClass);
	CreateServiceFunction(MS_MSG_GETWINDOWDATA,   GetWindowData);
	CreateServiceFunction(MS_MSG_SETSTATUSTEXT,   SetStatusText);

	CreateServiceFunction("SRMsg/ReadMessage",    ReadMessageCommand);
	CreateServiceFunction("SRMsg/TypingMessage",  TypingMessageCommand);
	CreateServiceFunction(MS_TABMSG_SETUSERPREFS, SetUserPrefs);
	CreateServiceFunction(MS_TABMSG_TRAYSUPPORT,  Service_OpenTrayMenu);
	CreateServiceFunction(MS_TABMSG_SLQMGR,       CSendLater::svcQMgr);

	CreateServiceFunction(MS_MSG_MOD_GETWINDOWFLAGS, GetMessageWindowFlags);
	CreateServiceFunction(MS_MSG_MOD_MESSAGEDIALOGOPENED, MessageWindowOpened);

	SI_InitStatusIcons();
	CB_InitCustomButtons();

	/*
	 * the event API
	 */

	PluginConfig.m_event_MsgWin = CreateHookableEvent(ME_MSG_WINDOWEVENT);
	PluginConfig.m_event_MsgPopup = CreateHookableEvent(ME_MSG_WINDOWPOPUP);
	PluginConfig.m_event_WriteEvent = CreateHookableEvent(ME_MSG_PRECREATEEVENT);
}

int LoadSendRecvMessageModule(void)
{
	if (FIF == 0) {
		MessageBox(0, TranslateT("The image service plugin (advaimg.dll) is not properly installed.\n\nTabSRMM is disabled."), TranslateT("TabSRMM fatal error"), MB_OK | MB_ICONERROR);
		return 1;
	}

	INITCOMMONCONTROLSEX icex;
	icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icex.dwICC  = ICC_COOL_CLASSES | ICC_BAR_CLASSES | ICC_LISTVIEW_CLASSES;;
	InitCommonControlsEx(&icex);

	Utils::loadSystemLibrary(L"\\riched20.dll");

	OleInitialize(NULL);
	mREOLECallback = new REOLECallback;
	Win7Taskbar = new CTaskbarInteract;
	Win7Taskbar->updateMetrics();

	ZeroMemory((void*)&nen_options, sizeof(nen_options));
	M->m_hMessageWindowList = (HANDLE) CallService(MS_UTILS_ALLOCWINDOWLIST, 0, 0);
	PluginConfig.hUserPrefsWindowList = (HANDLE) CallService(MS_UTILS_ALLOCWINDOWLIST, 0, 0);
	sendQueue = new SendQueue;
	Skin = new CSkin;
	sendLater = new CSendLater;

	InitOptions();

	InitAPI();

	PluginConfig.reloadSystemStartup();
	ReloadTabConfig();
	NEN_ReadOptions(&nen_options);

	M->WriteByte(TEMPLATES_MODULE, "setup", 2);
	LoadDefaultTemplates();

	BuildCodePageList();

	return 0;
}

STDMETHODIMP REOLECallback::GetNewStorage(LPSTORAGE FAR *lplpstg)
{
	LPLOCKBYTES lpLockBytes = NULL;
    SCODE sc  = ::CreateILockBytesOnHGlobal(NULL, TRUE, &lpLockBytes);
    if (sc != S_OK)
		return sc;
    sc = ::StgCreateDocfileOnILockBytes(lpLockBytes, STGM_SHARE_EXCLUSIVE|STGM_CREATE|STGM_READWRITE, 0, lplpstg);
    if (sc != S_OK)
		lpLockBytes->Release();
    return sc;
}


/*
 * tabbed mode support functions...
 * (C) by Nightwish
 *
 * this function searches and activates the tab belonging to the given hwnd (which is the
 * hwnd of a message dialog window)
 */

int TSAPI ActivateExistingTab(TContainerData *pContainer, HWND hwndChild)
{
	NMHDR nmhdr;

	struct TWindowData *dat = (struct TWindowData *) GetWindowLongPtr(hwndChild, GWLP_USERDATA);	// needed to obtain the hContact for the message window
	if (dat && pContainer) {
		ZeroMemory((void*)&nmhdr, sizeof(nmhdr));
		nmhdr.code = TCN_SELCHANGE;
		if (TabCtrl_GetItemCount(GetDlgItem(pContainer->hwnd, IDC_MSGTABS)) > 1 && !(pContainer->dwFlags & CNT_DEFERREDTABSELECT)) {
			TabCtrl_SetCurSel(GetDlgItem(pContainer->hwnd, IDC_MSGTABS), GetTabIndexFromHWND(GetDlgItem(pContainer->hwnd, IDC_MSGTABS), hwndChild));
			SendMessage(pContainer->hwnd, WM_NOTIFY, 0, (LPARAM)&nmhdr);	// just select the tab and let WM_NOTIFY do the rest
		}
		if (dat->bType == SESSIONTYPE_IM)
			SendMessage(pContainer->hwnd, DM_UPDATETITLE, (WPARAM)dat->hContact, 0);
		if (IsIconic(pContainer->hwnd)) {
			SendMessage(pContainer->hwnd, WM_SYSCOMMAND, SC_RESTORE, 0);
			SetForegroundWindow(pContainer->hwnd);
		}
		//MaD - hide on close feature
		if (!IsWindowVisible(pContainer->hwnd)) {
			WINDOWPLACEMENT wp={0};
			wp.length = sizeof(wp);
			GetWindowPlacement(pContainer->hwnd, &wp);

			/*
			 * all tabs must re-check the layout on activation because adding a tab while
			 * the container was hidden can make this necessary
             */
			BroadCastContainer(pContainer, DM_CHECKSIZE, 0, 0);
			if (wp.showCmd == SW_SHOWMAXIMIZED)
				ShowWindow(pContainer->hwnd, SW_SHOWMAXIMIZED);
			else {
				ShowWindow(pContainer->hwnd, SW_SHOWNA);
				SetForegroundWindow(pContainer->hwnd);
			}
			SendMessage(pContainer->hwndActive, WM_SIZE, 0, 0);			// make sure the active tab resizes its layout properly
		}
		//MaD_
		else if (GetForegroundWindow() != pContainer->hwnd)
			SetForegroundWindow(pContainer->hwnd);
		if (dat->bType == SESSIONTYPE_IM)
			SetFocus(GetDlgItem(hwndChild, IDC_MESSAGE));
		return TRUE;
	} else
		return FALSE;
}

/*
 * this function creates and activates a new tab within the given container.
 * bActivateTab: make the new tab the active one
 * bPopupContainer: restore container if it was minimized, otherwise flash it...
 */

HWND TSAPI CreateNewTabForContact(TContainerData *pContainer, HANDLE hContact, int isSend, const char *pszInitialText, BOOL bActivateTab, BOOL bPopupContainer, BOOL bWantPopup, HANDLE hdbEvent)
{
	TCHAR  newcontactname[128], tabtitle[128];
	int		newItem;
	DBVARIANT dbv = {0};

	if (M->FindWindow(hContact) != 0) {
		_DebugPopup(hContact, _T("Warning: trying to create duplicate window"));
		return 0;
	}
	// if we have a max # of tabs/container set and want to open something in the default container...
	if (hContact != 0 && M->GetByte("limittabs", 0) &&  !_tcsncmp(pContainer->szName, _T("default"), 6)) {
		if ((pContainer = FindMatchingContainer(_T("default"), hContact)) == NULL) {
			TCHAR szName[CONTAINER_NAMELEN + 1];

			_sntprintf(szName, CONTAINER_NAMELEN, _T("default"));
			pContainer = CreateContainer(szName, CNT_CREATEFLAG_CLONED, hContact);
		}
	}

	struct 	TNewWindowData newData = {0};
	newData.hContact = hContact;
	newData.isWchar = isSend;
	newData.szInitialText = pszInitialText;
	char *szProto = GetContactProto(newData.hContact);

	ZeroMemory((void*)&newData.item, sizeof(newData.item));

	// obtain various status information about the contact
	TCHAR *contactName = (TCHAR *) CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM)newData.hContact, GCDNF_TCHAR);

	/*
	 * cut nickname if larger than x chars...
	 */

	if (contactName && lstrlen(contactName) > 0) {
		if (M->GetByte("cuttitle", 0))
			CutContactName(contactName, newcontactname, SIZEOF(newcontactname));
		else {
			lstrcpyn(newcontactname, contactName, SIZEOF(newcontactname));
			newcontactname[127] = 0;
		}
		//Mad: to fix tab width for nicknames with ampersands
		Utils::DoubleAmpersands(newcontactname);
	} else
		lstrcpyn(newcontactname, _T("_U_"), SIZEOF(newcontactname));

	WORD wStatus = (szProto == NULL ? ID_STATUS_OFFLINE : db_get_w(newData.hContact, szProto, "Status", ID_STATUS_OFFLINE));
	TCHAR *szStatus = (TCHAR *) CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, szProto == NULL ? ID_STATUS_OFFLINE : db_get_w(newData.hContact, szProto, "Status", ID_STATUS_OFFLINE), GSMDF_TCHAR);

	if (M->GetByte("tabstatus", 1))
		mir_sntprintf(tabtitle, SIZEOF(tabtitle), _T("%s (%s)  "), newcontactname, szStatus);
	else
		mir_sntprintf(tabtitle, SIZEOF(tabtitle), _T("%s   "), newcontactname);

	newData.item.pszText = tabtitle;
	newData.item.mask = TCIF_TEXT | TCIF_IMAGE | TCIF_PARAM;
	newData.item.iImage = 0;
	newData.item.cchTextMax = 255;

	HWND hwndTab = GetDlgItem(pContainer->hwnd, IDC_MSGTABS);
	// hide the active tab
	if (pContainer->hwndActive && bActivateTab)
		ShowWindow(pContainer->hwndActive, SW_HIDE);

	{
		int iTabIndex_wanted = M->GetDword(hContact, "tabindex", pContainer->iChilds * 100);
		int iCount = TabCtrl_GetItemCount(hwndTab);
		TCITEM item = {0};
		int relPos;
		int i;

		pContainer->iTabIndex = iCount;
		if (iCount > 0) {
			for (i = iCount - 1; i >= 0; i--) {
				item.mask = TCIF_PARAM;
				TabCtrl_GetItem(hwndTab, i, &item);
				HWND hwnd = (HWND)item.lParam;
				struct TWindowData *dat = (struct TWindowData *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
				if (dat) {
					relPos = M->GetDword(dat->hContact, "tabindex", i * 100);
					if (iTabIndex_wanted <= relPos)
						pContainer->iTabIndex = i;
				}
			}
		}
	}
	newItem = TabCtrl_InsertItem(hwndTab, pContainer->iTabIndex, &newData.item);
	SendMessage(hwndTab, EM_REFRESHWITHOUTCLIP, 0, 0);
	if (bActivateTab)
		TabCtrl_SetCurSel(GetDlgItem(pContainer->hwnd, IDC_MSGTABS), newItem);
	newData.iTabID = newItem;
	newData.iTabImage = newData.item.iImage;
	newData.pContainer = pContainer;
	newData.iActivate = (int) bActivateTab;
	pContainer->iChilds++;
	newData.bWantPopup = bWantPopup;
	newData.hdbEvent = hdbEvent;
	HWND hwndNew = CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_MSGSPLITNEW), GetDlgItem(pContainer->hwnd, IDC_MSGTABS), DlgProcMessage, (LPARAM)& newData);
	/*
	 * switchbar support
	 */
	if (pContainer->dwFlags & CNT_SIDEBAR) {
		TWindowData *dat = (TWindowData *)GetWindowLongPtr(hwndNew, GWLP_USERDATA);
		if (dat)
			pContainer->SideBar->addSession(dat, pContainer->iTabIndex);
	}
	SendMessage(pContainer->hwnd, WM_SIZE, 0, 0);

	// if the container is minimized, then pop it up...

	if (IsIconic(pContainer->hwnd)) {
		if (bPopupContainer) {
			SendMessage(pContainer->hwnd, WM_SYSCOMMAND, SC_RESTORE, 0);
			SetFocus(pContainer->hwndActive);
		} else {
			if (pContainer->dwFlags & CNT_NOFLASH)
				SendMessage(pContainer->hwnd, DM_SETICON, 0, (LPARAM)LoadSkinnedIcon(SKINICON_EVENT_MESSAGE));
			else
				FlashContainer(pContainer, 1, 0);
		}
	}
	if (bActivateTab) {
		ActivateExistingTab(pContainer, hwndNew);
		SetFocus(hwndNew);
		RedrawWindow(pContainer->hwnd, NULL, NULL, RDW_ERASENOW);
		UpdateWindow(pContainer->hwnd);
		if (GetForegroundWindow() != pContainer->hwnd && bPopupContainer == TRUE)
			SetForegroundWindow(pContainer->hwnd);
	}
	else if (!IsIconic(pContainer->hwnd) && IsWindowVisible(pContainer->hwnd)) {
		SendMessage(pContainer->hwndActive, WM_SIZE, 0, 0);
		RedrawWindow(pContainer->hwndActive, NULL, NULL, RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_UPDATENOW);
		RedrawWindow(pContainer->hwndActive, NULL, NULL, RDW_ERASENOW | RDW_UPDATENOW);
	}

	//MaD
	if (PluginConfig.m_HideOnClose&&!IsWindowVisible(pContainer->hwnd)) {
		WINDOWPLACEMENT wp={0};
		wp.length = sizeof(wp);
		GetWindowPlacement(pContainer->hwnd, &wp);

		BroadCastContainer(pContainer, DM_CHECKSIZE, 0, 0);			// make sure all tabs will re-check layout on activation
		if (wp.showCmd == SW_SHOWMAXIMIZED)
			ShowWindow(pContainer->hwnd, SW_SHOWMAXIMIZED);
		else {
			if (bPopupContainer)
				ShowWindow(pContainer->hwnd, SW_SHOWNORMAL);
			else
				ShowWindow(pContainer->hwnd, SW_SHOWMINNOACTIVE);
		}
		SendMessage(pContainer->hwndActive, WM_SIZE, 0, 0);
	}
	if (PluginConfig.m_bIsWin7 && PluginConfig.m_useAeroPeek && CSkin::m_skinEnabled) // && !M->GetByte("forceAeroPeek", 0))
		CWarning::show(CWarning::WARN_AEROPEEK_SKIN, MB_ICONWARNING|MB_OK);

	if (ServiceExists(MS_HPP_EG_EVENT) && ServiceExists(MS_IEVIEW_EVENT) && M->GetByte(0, "HistoryPlusPlus", "IEViewAPI", 0)) {
		if (IDYES == CWarning::show(CWarning::WARN_HPP_APICHECK, MB_ICONWARNING|MB_YESNO))
			M->WriteByte(0, "HistoryPlusPlus", "IEViewAPI", 0);
	}
	return hwndNew;		// return handle of the new dialog
}

/*
 * this is used by the 2nd containermode (limit tabs on default containers).
 * it searches a container with "room" for the new tabs or otherwise creates
 * a new (cloned) one.
 */

TContainerData* TSAPI FindMatchingContainer(const TCHAR *szName, HANDLE hContact)
{
	int iMaxTabs = M->GetDword("maxtabs", 0);
	if (iMaxTabs > 0 && M->GetByte("limittabs", 0) && !_tcsncmp(szName, _T("default"), 6)) {
		// search a "default" with less than iMaxTabs opened...
		for (TContainerData *p = pFirstContainer; p; p = p->pNext)
			if (!_tcsncmp(p->szName, _T("default"), 6) && p->iChilds < iMaxTabs)
				return p;

		return NULL;
	}
	return FindContainerByName(szName);
}

/*
 * load some global icons.
 */

void TSAPI CreateImageList(BOOL bInitial)
{
	int cxIcon = GetSystemMetrics(SM_CXSMICON);
	int cyIcon = GetSystemMetrics(SM_CYSMICON);

	/*
	 * the imagelist is now a fake. It is still needed to provide the tab control with a
	 * image list handle. This will make sure that the tab control will reserve space for
	 * an icon on each tab. This is a blank and empty icon
	 */

	if (bInitial) {
		PluginConfig.g_hImageList = ImageList_Create(16, 16, PluginConfig.m_bIsXP ? ILC_COLOR32 | ILC_MASK : ILC_COLOR8 | ILC_MASK, 2, 0);
		HICON hIcon = CreateIcon(g_hInst, 16, 16, 1, 4, NULL, NULL);
		ImageList_AddIcon(PluginConfig.g_hImageList, hIcon);
		DestroyIcon(hIcon);
	}

	PluginConfig.g_IconFileEvent = LoadSkinnedIcon(SKINICON_EVENT_FILE);
	PluginConfig.g_IconMsgEvent = LoadSkinnedIcon(SKINICON_EVENT_MESSAGE);
	PluginConfig.g_IconMsgEventBig = LoadSkinnedIconBig(SKINICON_EVENT_MESSAGE);
	if ((HICON)CALLSERVICE_NOTFOUND == PluginConfig.g_IconMsgEventBig)
		PluginConfig.g_IconMsgEventBig = 0;
	PluginConfig.g_IconTypingEventBig = LoadSkinnedIconBig(SKINICON_OTHER_TYPING);
	if ((HICON)CALLSERVICE_NOTFOUND == PluginConfig.g_IconTypingEventBig)
		PluginConfig.g_IconTypingEventBig = 0;
	PluginConfig.g_IconSend = PluginConfig.g_buttonBarIcons[9];
	PluginConfig.g_IconTypingEvent = PluginConfig.g_buttonBarIcons[ICON_DEFAULT_TYPING];
}

int TABSRMM_FireEvent(HANDLE hContact, HWND hwnd, unsigned int type, unsigned int subType)
{
	if (hContact == NULL || hwnd == NULL || !M->GetByte("_eventapi", 1))
		return 0;

	struct TWindowData *dat = (struct TWindowData *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	BYTE bType = dat ? dat->bType : SESSIONTYPE_IM;

	MessageWindowEventData mwe = { sizeof(mwe) };
	mwe.hContact = hContact;
	mwe.hwndWindow = hwnd;
	mwe.szModule = "tabSRMsgW";
	mwe.uType = type;
	mwe.hwndInput = GetDlgItem(hwnd, bType == SESSIONTYPE_IM ? IDC_MESSAGE : IDC_CHAT_MESSAGE);
	mwe.hwndLog = GetDlgItem(hwnd, bType == SESSIONTYPE_IM ? IDC_LOG : IDC_CHAT_LOG);

	if (type == MSG_WINDOW_EVT_CUSTOM) {
		TABSRMM_SessionInfo se = { sizeof(se) };
		se.evtCode = HIWORD(subType);
		se.hwnd = hwnd;
		se.extraFlags = (unsigned int)(LOWORD(subType));
		se.local = (void*)dat->sendBuffer;
		mwe.local = (void*) & se;
	}

	return NotifyEventHooks(PluginConfig.m_event_MsgWin, 0, (LPARAM)&mwe);
}

/*
 * standard icon definitions
 */

static TIconDesc _toolbaricons[] = {
	"tabSRMM_mlog", LPGEN("Message Log Options"), &PluginConfig.g_buttonBarIcons[2], -IDI_MSGLOGOPT, 1,
	"tabSRMM_multi", LPGEN("Image tag"), &PluginConfig.g_buttonBarIcons[3], -IDI_IMAGETAG, 1,
	"tabSRMM_quote", LPGEN("Quote text"), &PluginConfig.g_buttonBarIcons[8], -IDI_QUOTE, 1,
	"tabSRMM_save", LPGEN("Save and close"), &PluginConfig.g_buttonBarIcons[7], -IDI_SAVE, 1,
	"tabSRMM_send", LPGEN("Send message"), &PluginConfig.g_buttonBarIcons[9], -IDI_SEND, 1,
	"tabSRMM_avatar", LPGEN("Edit user notes"), &PluginConfig.g_buttonBarIcons[10], -IDI_CONTACTPIC, 1,
	"tabSRMM_close", LPGEN("Close"), &PluginConfig.g_buttonBarIcons[6], -IDI_CLOSEMSGDLG, 1,
	NULL, NULL, NULL, 0, 0
};

static TIconDesc _exttoolbaricons[] = {
	"tabSRMM_emoticon", LPGEN("Smiley button"), &PluginConfig.g_buttonBarIcons[11], -IDI_SMILEYICON, 1,
	"tabSRMM_bold", LPGEN("Format bold"), &PluginConfig.g_buttonBarIcons[17], -IDI_FONTBOLD, 1,
	"tabSRMM_italic", LPGEN("Format italic"), &PluginConfig.g_buttonBarIcons[18], -IDI_FONTITALIC, 1,
	"tabSRMM_underline", LPGEN("Format underline"), &PluginConfig.g_buttonBarIcons[19], -IDI_FONTUNDERLINE, 1,
	"tabSRMM_face", LPGEN("Font face"), &PluginConfig.g_buttonBarIcons[20], -IDI_FONTFACE, 1,
	"tabSRMM_color", LPGEN("Font color"), &PluginConfig.g_buttonBarIcons[21], -IDI_FONTCOLOR, 1,
	"tabSRMM_strikeout", LPGEN("Format strike-through"), &PluginConfig.g_buttonBarIcons[30], -IDI_STRIKEOUT, 1,
	NULL, NULL, NULL, 0, 0
};
//MAD
static TIconDesc _chattoolbaricons[] = {
	"chat_bkgcol",LPGEN("Background colour"), &PluginConfig.g_buttonBarIcons[31] ,-IDI_BKGCOLOR, 1,
	"chat_settings",LPGEN("Room settings"),  &PluginConfig.g_buttonBarIcons[32],-IDI_TOPICBUT, 1,
	"chat_filter",LPGEN("Event filter"), &PluginConfig.g_buttonBarIcons[33] ,-IDI_FILTER2, 1,
	"chat_shownicklist",LPGEN("Nick list"),&PluginConfig.g_buttonBarIcons[35]  ,-IDI_SHOWNICKLIST, 1,
	NULL, NULL, NULL, 0, 0
	};
//
static TIconDesc _logicons[] = {
	"tabSRMM_error", LPGEN("Message delivery error"), &PluginConfig.g_iconErr, -IDI_MSGERROR, 1,
	"tabSRMM_in", LPGEN("Incoming message"), &PluginConfig.g_iconIn, -IDI_ICONIN, 0,
	"tabSRMM_out", LPGEN("Outgoing message"), &PluginConfig.g_iconOut, -IDI_ICONOUT, 0,
	"tabSRMM_status", LPGEN("Statuschange"), &PluginConfig.g_iconStatus, -IDI_STATUSCHANGE, 0,
	NULL, NULL, NULL, 0, 0
};
static TIconDesc _deficons[] = {
	"tabSRMM_container", LPGEN("Static container icon"), &PluginConfig.g_iconContainer, -IDI_CONTAINER, 1,
	"tabSRMM_sounds_on", LPGEN("Sounds (status bar)"), &PluginConfig.g_buttonBarIcons[ICON_DEFAULT_SOUNDS], -IDI_SOUNDSON, 1,
	"tabSRMM_pulldown", LPGEN("Pulldown Arrow"), &PluginConfig.g_buttonBarIcons[ICON_DEFAULT_PULLDOWN], -IDI_PULLDOWNARROW, 1,
	"tabSRMM_Leftarrow", LPGEN("Left Arrow"), &PluginConfig.g_buttonBarIcons[ICON_DEFAULT_LEFT], -IDI_LEFTARROW, 1,
	"tabSRMM_Rightarrow", LPGEN("Right Arrow"), &PluginConfig.g_buttonBarIcons[ICON_DEFAULT_RIGHT], -IDI_RIGHTARROW, 1,
	"tabSRMM_Pulluparrow", LPGEN("Up Arrow"), &PluginConfig.g_buttonBarIcons[ICON_DEFAULT_UP], -IDI_PULLUPARROW, 1,
	"tabSRMM_sb_slist", LPGEN("Session List"), &PluginConfig.g_sideBarIcons[0], -IDI_SESSIONLIST, 1,
	NULL, NULL, NULL, 0, 0
};
static TIconDesc _trayIcon[] = {
	"tabSRMM_frame1", LPGEN("Frame 1"), &PluginConfig.m_AnimTrayIcons[0], -IDI_TRAYANIM1, 1,
	"tabSRMM_frame2", LPGEN("Frame 2"), &PluginConfig.m_AnimTrayIcons[1], -IDI_TRAYANIM2, 1,
	"tabSRMM_frame3", LPGEN("Frame 3"), &PluginConfig.m_AnimTrayIcons[2], -IDI_TRAYANIM3, 1,
	"tabSRMM_frame4", LPGEN("Frame 4"), &PluginConfig.m_AnimTrayIcons[3], -IDI_TRAYANIM4, 1,
	NULL, NULL, NULL, 0, 0
};

static struct _iconblocks {
	char *szSection;
	TIconDesc *idesc;
} ICONBLOCKS[] = {
	LPGEN("Message Sessions")"/"LPGEN("Default"), _deficons,
	LPGEN("Message Sessions")"/"LPGEN("Toolbar"), _toolbaricons,
	LPGEN("Message Sessions")"/"LPGEN("Toolbar"), _exttoolbaricons,
	LPGEN("Message Sessions")"/"LPGEN("Toolbar"), _chattoolbaricons,
	LPGEN("Message Sessions")"/"LPGEN("Message Log"), _logicons,
	LPGEN("Message Sessions")"/"LPGEN("Animated Tray"), _trayIcon,
	NULL, 0
};

static int GetIconPackVersion(HMODULE hDLL)
{
	char szIDString[256];
	int version = 0;

	if (LoadStringA(hDLL, IDS_IDENTIFY, szIDString, sizeof(szIDString)) == 0)
		version = 0;
	else {
		if (!strcmp(szIDString, "__tabSRMM_ICONPACK 1.0__"))
			version = 1;
		else if (!strcmp(szIDString, "__tabSRMM_ICONPACK 2.0__"))
			version = 2;
		else if (!strcmp(szIDString, "__tabSRMM_ICONPACK 3.0__"))
			version = 3;
		else if (!strcmp(szIDString, "__tabSRMM_ICONPACK 3.5__"))
			version = 4;
		else if (!strcmp(szIDString, "__tabSRMM_ICONPACK 5.0__"))
			version = 5;
	}

	if (version < 5)
		CWarning::show(CWarning::WARN_ICONPACK_VERSION, MB_OK|MB_ICONERROR);
	return version;
}
/*
 * setup default icons for the IcoLib service. This needs to be done every time the plugin is loaded
 * default icons are taken from the icon pack in either \icons or \plugins
 */

static int TSAPI SetupIconLibConfig()
{
	int i = 0, j = 2, version = 0, n = 0;

	TCHAR szFilename[MAX_PATH];
	_tcsncpy(szFilename, _T("icons\\tabsrmm_icons.dll"), MAX_PATH);
	g_hIconDLL = LoadLibrary(szFilename);
	if (g_hIconDLL == 0) {
		CWarning::show(CWarning::WARN_ICONPACKMISSING, CWarning::CWF_NOALLOWHIDE|MB_ICONERROR|MB_OK);
		return 0;
	}

	GetModuleFileName(g_hIconDLL, szFilename, MAX_PATH);
	if (PluginConfig.m_chat_enabled)
		Chat_AddIcons();
	version = GetIconPackVersion(g_hIconDLL);
	FreeLibrary(g_hIconDLL);
	g_hIconDLL = 0;

	SKINICONDESC sid = { sizeof(sid) };
	sid.ptszDefaultFile = szFilename;
	sid.flags = SIDF_PATH_TCHAR;

	while (ICONBLOCKS[n].szSection) {
		i = 0;
		sid.pszSection = ICONBLOCKS[n].szSection;
		while (ICONBLOCKS[n].idesc[i].szDesc) {
			sid.pszName = ICONBLOCKS[n].idesc[i].szName;
			sid.pszDescription = ICONBLOCKS[n].idesc[i].szDesc;
			sid.iDefaultIndex = ICONBLOCKS[n].idesc[i].uId == -IDI_HISTORY ? 0 : ICONBLOCKS[n].idesc[i].uId;        // workaround problem /w icoLib and a resource id of 1 (actually, a Windows problem)
			i++;
			if (n > 0 && n < 4)
				PluginConfig.g_buttonBarIconHandles[j++] = Skin_AddIcon(&sid);
			else
				Skin_AddIcon(&sid);
		}
		n++;
	}

	sid.pszSection = LPGEN("Message Sessions")"/"LPGEN("Default");
	sid.pszName = "tabSRMM_clock_symbol";
	sid.pszDescription = LPGEN("Clock symbol (for the info panel clock)");
	sid.iDefaultIndex = -IDI_CLOCK;
	Skin_AddIcon(&sid);

	_tcsncpy(szFilename, _T("plugins\\tabsrmm.dll"), MAX_PATH);

	sid.pszName = "tabSRMM_overlay_disabled";
	sid.pszDescription = LPGEN("Feature disabled (used as overlay)");
	sid.iDefaultIndex = -IDI_FEATURE_DISABLED;
	Skin_AddIcon(&sid);

	sid.pszName = "tabSRMM_overlay_enabled";
	sid.pszDescription = LPGEN("Feature enabled (used as overlay)");
	sid.iDefaultIndex = -IDI_FEATURE_ENABLED;
	Skin_AddIcon(&sid);
	return 1;
}

// load the icon theme from IconLib - check if it exists...

static int TSAPI LoadFromIconLib()
{
	for (int n = 0;ICONBLOCKS[n].szSection;n++) {
		for (int i = 0;ICONBLOCKS[n].idesc[i].szDesc;i++) {
			*(ICONBLOCKS[n].idesc[i].phIcon) = Skin_GetIcon(ICONBLOCKS[n].idesc[i].szName);
		}
	}
	PluginConfig.g_buttonBarIcons[0] = LoadSkinnedIcon(SKINICON_OTHER_ADDCONTACT);
	PluginConfig.g_buttonBarIcons[1] = LoadSkinnedIcon(SKINICON_OTHER_HISTORY);
	PluginConfig.g_buttonBarIconHandles[0] = LoadSkinnedIconHandle(SKINICON_OTHER_HISTORY);
	PluginConfig.g_buttonBarIconHandles[1] = LoadSkinnedIconHandle(SKINICON_OTHER_ADDCONTACT);
	PluginConfig.g_buttonBarIconHandles[20] = LoadSkinnedIconHandle(SKINICON_OTHER_USERDETAILS);

	PluginConfig.g_buttonBarIcons[ICON_DEFAULT_TYPING] = 
		PluginConfig.g_buttonBarIcons[12] = LoadSkinnedIcon(SKINICON_OTHER_TYPING);
	PluginConfig.g_IconChecked = LoadSkinnedIcon(SKINICON_OTHER_TICK);
	PluginConfig.g_IconUnchecked = LoadSkinnedIcon(SKINICON_OTHER_NOTICK);
	PluginConfig.g_IconFolder = LoadSkinnedIcon(SKINICON_OTHER_GROUPOPEN);

	PluginConfig.g_iconOverlayEnabled = Skin_GetIcon("tabSRMM_overlay_enabled");
	PluginConfig.g_iconOverlayDisabled = Skin_GetIcon("tabSRMM_overlay_disabled");

	PluginConfig.g_iconClock = Skin_GetIcon("tabSRMM_clock_symbol");

	CacheMsgLogIcons();
	M->BroadcastMessage(DM_LOADBUTTONBARICONS, 0, 0);
	return 0;
}

/*
 * load icon theme from either icon pack or IcoLib
 */

void TSAPI LoadIconTheme()
{
	if (SetupIconLibConfig() == 0)
		return;
	else
		LoadFromIconLib();

	Skin->setupTabCloseBitmap();
	return;
}

static void UnloadIcons()
{
	for (int n = 0;ICONBLOCKS[n].szSection;n++) {
		for (int i = 0;ICONBLOCKS[n].idesc[i].szDesc;i++) {
			if (*(ICONBLOCKS[n].idesc[i].phIcon) != 0) {
				DestroyIcon(*(ICONBLOCKS[n].idesc[i].phIcon));
				*(ICONBLOCKS[n].idesc[i].phIcon) = 0;
			}
		}
	}
	if (PluginConfig.g_hbmUnknown)
		DeleteObject(PluginConfig.g_hbmUnknown);
	for (int i=0; i < 4; i++) {
		if (PluginConfig.m_AnimTrayIcons[i])
			DestroyIcon(PluginConfig.m_AnimTrayIcons[i]);
	}
}
