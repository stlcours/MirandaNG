/*

Miranda IM: the free IM client for Microsoft* Windows*
Copyright 2000-2009 Miranda ICQ/IM project, 

This file is part of Send Screenshot Plus, a Miranda IM plugin.
Copyright (c) 2010 Ing.U.Horn

Parts of this file based on original sorce code
(c) 2004-2006 S�rgio Vieira Rolanski (portet from Borland C++)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

File name      : $HeadURL: http://merlins-miranda.googlecode.com/svn/trunk/miranda/plugins/SendSSPlus/UMainForm.cpp $
Revision       : $Revision: 22 $
Last change on : $Date: 2010-05-02 21:25:02 +0400 (Вс, 02 май 2010) $
Last change by : $Author: ing.u.horn $

*/

#include "UMainForm.h"
#include "UAboutForm.h"
//#include "UEditForm.h"

//---------------------------------------------------------------------------
INT_PTR CALLBACK TfrmMain::DlgProc_CaptureWindow(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
// main message handling is done inside TfrmMain::DlgTfrmMain
	switch (uMsg) {
	case WM_INITDIALOG:
		Static_SetIcon(GetDlgItem(hDlg, ID_imgTarget), IcoLib_GetIcon(ICO_PLUG_SSTARGET));
		SetDlgItemText(hDlg, ID_edtCaption, _T("Drag&&Drop the target on the desired window."));
		TranslateDialogDefault(hDlg);
		break;
	case WM_CTLCOLOREDIT:		//ctrl is NOT read-only or disabled 
	case WM_CTLCOLORSTATIC:		//ctrl is read-only or disabled
		// make the rectangle on the top white
		switch (GetWindowLongPtr((HWND)lParam, GWL_ID)) {
			case IDC_WHITERECT:
			case ID_chkClientArea:
			case ID_lblDropInfo:
			case ID_edtCaption:
			case ID_edtCaptionLabel:
			case ID_edtSize:
			case ID_edtSizeLabel:
			case ID_bvlTarget:
			case ID_imgTarget:
				SetBkColor((HDC)wParam,GetSysColor(COLOR_WINDOW));
				SetTextColor((HDC)wParam,GetSysColor(COLOR_WINDOWTEXT));

				//SetBkMode((HDC)wParam,OPAQUE);
				//return (INT_PTR)GetSysColorBrush(COLOR_WINDOW);
				return (LRESULT)GetStockObject(WHITE_BRUSH);
				break;
			default:
				SetBkMode((HDC)wParam, TRANSPARENT);
				return (LRESULT)GetStockObject(NULL_BRUSH);
				break;
		}
		break;	//this return false
	case WM_COMMAND:
		SendMessage(GetParent(hDlg), uMsg, wParam, lParam);
		break;
	case WM_MOUSEMOVE:
		SendMessage(GetParent(hDlg), UM_TAB1, uMsg, 0);
		break;
	case WM_NOTIFY:
		SendMessage(GetParent(hDlg), uMsg, wParam, lParam);
		break;
	case WM_DESTROY:
		break;
	}
	return FALSE;
}

//---------------------------------------------------------------------------
INT_PTR CALLBACK TfrmMain::DlgProc_CaptureDesktop(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
// main message handling is done inside TfrmMain::DlgTfrmMain
	switch (uMsg) {
	case WM_INITDIALOG:
		Static_SetIcon(GetDlgItem(hDlg, ID_imgTarget), IcoLib_GetIcon(ICO_PLUG_SSMONITOR));
		break;
	case WM_CTLCOLOREDIT:
	case WM_CTLCOLORSTATIC:
		// make the rectangle on the top white
		switch (GetWindowLongPtr((HWND)lParam, GWL_ID)) {
			case IDC_WHITERECT:
			case ID_lblDropInfo:
			case ID_edtCaption:
			case ID_edtCaptionLabel:
			case ID_edtSize:
			case ID_edtSizeLabel:
			case ID_bvlTarget:
			case ID_imgTarget:
				SetBkColor((HDC)wParam,GetSysColor(COLOR_WINDOW));
				SetTextColor((HDC)wParam,GetSysColor(COLOR_WINDOWTEXT));
				return (LRESULT)GetStockObject(WHITE_BRUSH);
				break;
			default:
				SetBkMode((HDC)wParam, TRANSPARENT);
				return (LRESULT)GetStockObject(NULL_BRUSH);
				break;
		}
		break;
	case WM_COMMAND:
		SendMessage(GetParent(hDlg), uMsg, wParam, lParam);
		break;
	case WM_NOTIFY:
		SendMessage(GetParent(hDlg), uMsg, wParam, lParam);
		break;
	case WM_DESTROY:
		break;
	}
	return FALSE;
}

//---------------------------------------------------------------------------

TfrmMain::CHandleMapping TfrmMain::_HandleMapping;

LRESULT CALLBACK TfrmMain::DlgTfrmMain(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg == WM_CTLCOLOREDIT || msg == WM_CTLCOLORSTATIC) {
		switch ( GetWindowLongPtr(( HWND )lParam, GWL_ID )) {
	/*		case IDC_WHITERECT:*/
			case IDC_HEADERBAR:
				SetTextColor((HDC)wParam,GetSysColor(COLOR_WINDOWTEXT));
				break;
			default:
				return FALSE;
		}
		SetBkColor((HDC)wParam, GetSysColor(COLOR_WINDOW));
		return (LRESULT)GetStockObject(WHITE_BRUSH); 	//GetSysColorBrush(COLOR_WINDOW);
	}

	CHandleMapping::iterator wnd(_HandleMapping.end());
	if (msg == WM_INITDIALOG) {
		wnd = _HandleMapping.insert(CHandleMapping::value_type(hWnd, reinterpret_cast<TfrmMain*>(lParam))).first;
		reinterpret_cast<TfrmMain*>(lParam)->m_hWnd = hWnd;
		return wnd->second->wmInitdialog(wParam, lParam);
	}
	else {
		wnd = _HandleMapping.find(hWnd);
	}
	if (wnd == _HandleMapping.end()) {	// something screwed up
		return FALSE;					//dialog! do not use ::DefWindowProc(hWnd, msg, wParam, lParam);
	}

	switch (msg)
	{
		// case WM_INITDIALOG:	done on top
		case WM_COMMAND:
			return wnd->second->wmCommand(wParam, lParam);
			break;
		case WM_CLOSE:
			return wnd->second->wmClose(wParam, lParam);
			break;
		case WM_DESTROY:
			delete wnd->second;
			break;
		case UM_TAB1:
			return wnd->second->UMTab1(wParam, lParam);
			break;
		case WM_NOTIFY:
			return wnd->second->wmNotify(wParam, lParam);
			break;
		case WM_TIMER:
			return wnd->second->wmTimer(wParam, lParam);
			break;
		case UM_CLOSING:
			return wnd->second->UMClosing(wParam, lParam);
			break;
		case UM_EVENT:
			return wnd->second->UMevent(wParam, lParam);
			break;
	}
	return FALSE;
}

//---------------------------------------------------------------------------
//WM_INITDIALOG:
LRESULT TfrmMain::wmInitdialog(WPARAM wParam, LPARAM lParam) {
	HWND hCtrl;
	//Taskbar and Window icon
	SendMessage(m_hWnd, WM_SETICON, ICON_BIG,	(LPARAM)IcoLib_GetIcon(ICO_PLUG_SSWINDOW1, true));
	SendMessage(m_hWnd, WM_SETICON, ICON_SMALL,	(LPARAM)IcoLib_GetIcon(ICO_PLUG_SSWINDOW2));
	LPTSTR pt = mir_a2t(__PLUGIN_NAME);
	SetWindowText(m_hWnd, pt);
	mir_freeAndNil(pt);

	// Headerbar
	pt = mir_tstrdup((LPTSTR)CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM)m_hContact, (LPARAM)GCDNF_TCHAR));
	if (pt && (m_hContact != 0)) {
		LPTSTR lptString = NULL;
		mir_tcsadd(lptString , TranslateT("Send screenshot to\n"));
		mir_tcsadd(lptString , pt);
		SetDlgItemText(m_hWnd, IDC_HEADERBAR, lptString);
		mir_free(lptString);
	}
	mir_freeAndNil(pt);

	SendMessage(GetDlgItem(m_hWnd, IDC_HEADERBAR), WM_SETICON, 0, (WPARAM)IcoLib_GetIcon(ICO_PLUG_SSWINDOW1, true));

	//Timed controls
	CheckDlgButton(m_hWnd,ID_chkTimed,				m_opt_chkTimed ? BST_CHECKED : BST_UNCHECKED);
	SetDlgItemInt (m_hWnd,ID_edtTimed,				(UINT)m_opt_edtTimed, FALSE);
	SendDlgItemMessage(m_hWnd, ID_upTimed,			UDM_SETRANGE, 0, (LPARAM)MAKELONG(250, 1));
	chkTimedClick();		//enable disable Timed controls

	//create Image list for tab control
	if(m_himlTab == 0){
		//m_himlTab = ImageList_Create(16, 16, PluginConfig.m_bIsXP ? ILC_COLOR32 | ILC_MASK : ILC_COLOR8 | ILC_MASK, 2, 0);
		m_himlTab = ImageList_Create(16, 16, ILC_COLOR32 | ILC_MASK, 2, 0);
		ImageList_AddIcon(m_himlTab, IcoLib_GetIcon(ICO_PLUG_SSWINDOW2));
		ImageList_AddIcon(m_himlTab, IcoLib_GetIcon(ICO_PLUG_SSWINDOW2));
	}

	//create the tab control. 
	{
	TAB_INFO itab;
	RECT rcClient, rcTab;
	m_hwndTab = GetDlgItem(m_hWnd, IDC_CAPTURETAB);
	TabCtrl_SetItemExtra(m_hwndTab, sizeof(TAB_INFO) - sizeof(TCITEMHEADER));

	ZeroMemory(&itab, sizeof(itab));
	itab.hwndMain	= m_hWnd;
	itab.hwndTab	= m_hwndTab;

	GetWindowRect(m_hwndTab, &rcTab);
	GetWindowRect(m_hWnd, &rcClient);
	
	TabCtrl_SetImageList(m_hwndTab, m_himlTab);

	// Add a tab for each of the three child dialog boxes. 
	itab.tcih.mask		= TCIF_PARAM|TCIF_TEXT|TCIF_IMAGE;

	itab.tcih.pszText	= TranslateT("Window");
	itab.tcih.iImage	= 0;
	itab.hwndTabPage	= CreateDialog(hInst,MAKEINTRESOURCE(IDD_UMain_CaptureWindow), m_hWnd,(DLGPROC)DlgProc_CaptureWindow);
	TabCtrl_InsertItem(m_hwndTab, 0, &itab);
	MoveWindow(itab.hwndTabPage, (rcTab.left - rcClient.left)+2, (rcTab.top - rcClient.top), (rcTab.right - rcTab.left) - 2*5, (rcTab.bottom - rcTab.top) - 2*20, TRUE);
	ShowWindow(itab.hwndTabPage, SW_HIDE);
	CheckDlgButton(itab.hwndTabPage, ID_chkClientArea, m_opt_chkClientArea ? BST_CHECKED : BST_UNCHECKED);

	itab.tcih.pszText	= TranslateT("Desktop");
	itab.tcih.iImage	= 1;
	itab.hwndTabPage	= CreateDialog(hInst,MAKEINTRESOURCE(IDD_UMain_CaptureDesktop), m_hWnd, DlgProc_CaptureDesktop);
	TabCtrl_InsertItem(m_hwndTab, 1, &itab);
	MoveWindow(itab.hwndTabPage, (rcTab.left - rcClient.left)+2, (rcTab.top - rcClient.top), (rcTab.right - rcTab.left) - 2*5, (rcTab.bottom - rcTab.top) - 2*20, TRUE);
	ShowWindow(itab.hwndTabPage, SW_HIDE);

	hCtrl = GetDlgItem(itab.hwndTabPage, ID_edtCaption);
	ComboBox_ResetContent(hCtrl);
	ComboBox_SetItemData(hCtrl, ComboBox_AddString(hCtrl, TranslateT("<entire desktop>"))  ,(DWORD)0);
	ComboBox_SetCurSel (hCtrl,0);
	if(m_MonitorCount >1) {
		TCHAR	tszTemp[120];
		for (size_t i = 0; i < m_MonitorCount; ++i) {
			mir_sntprintf(tszTemp, SIZEOF(tszTemp),_T("%i. %s%s"),
				i+1,
				TranslateT("Monitor"),
				(m_Monitors[i].dwFlags & MONITORINFOF_PRIMARY) ? TranslateT(" (primary)") : _T("")
				);
			ComboBox_SetItemData(hCtrl, ComboBox_AddString(hCtrl, (LPCTSTR)tszTemp)  , i+1);
		}
		ComboBox_SelectItemData (hCtrl, -1, m_opt_cboxDesktop);	//use Workaround for MS bug ComboBox_SelectItemData
	}
	PostMessage(m_hWnd, WM_COMMAND, MAKEWPARAM(ID_edtCaption, CBN_SELCHANGE),(LPARAM)hCtrl);

	//select tab and set m_hwndTabPage
	TabCtrl_SetCurSel(m_hwndTab, m_opt_tabCapture);
	ZeroMemory(&itab, sizeof(itab));
	itab.tcih.mask = TCIF_PARAM;
	TabCtrl_GetItem(m_hwndTab,TabCtrl_GetCurSel(m_hwndTab),&itab);
	ShowWindow(itab.hwndTabPage,SW_SHOW);
	m_hwndTabPage = itab.hwndTabPage;
	}
	//init Format combo box
	{
	hCtrl = GetDlgItem(m_hWnd, ID_cboxFormat);
	ComboBox_ResetContent(hCtrl);
	ComboBox_SetItemData(hCtrl, ComboBox_AddString(hCtrl, _T("PNG")),0);
	ComboBox_SetItemData(hCtrl, ComboBox_AddString(hCtrl, _T("JPG")),1);
	ComboBox_SetItemData(hCtrl, ComboBox_AddString(hCtrl, _T("BMP")),2);
	ComboBox_SetItemData(hCtrl, ComboBox_AddString(hCtrl, _T("TIF")),3);
	ComboBox_SetItemData(hCtrl, ComboBox_AddString(hCtrl, _T("GIF")),4);
	ComboBox_SelectItemData (hCtrl, -1, m_opt_cboxFormat);	//use Workaround for MS bug ComboBox_SelectItemData
	}
	//init SendBy combo box
	{
	hCtrl = GetDlgItem(m_hWnd, ID_cboxSendBy);
	ComboBox_ResetContent(hCtrl);
	ComboBox_SetItemData(hCtrl, ComboBox_AddString(hCtrl, TranslateT("<Only save>"))  ,SS_JUSTSAVE);
	if (m_hContact) {
		ComboBox_SetItemData(hCtrl, ComboBox_AddString(hCtrl, TranslateT("File Transfer")),SS_FILESEND);
	}
	else if(m_opt_cboxSendBy == SS_FILESEND) {
		m_opt_cboxSendBy = SS_IMAGESHACK;
	}
	ComboBox_SetItemData(hCtrl, ComboBox_AddString(hCtrl, TranslateT("E-mail"))       ,SS_EMAIL);
	if (myGlobals.PluginHTTPExist) {
		ComboBox_SetItemData(hCtrl, ComboBox_AddString(hCtrl, _T("HTTP Server"))  ,SS_HTTPSERVER);
	}
	else if(m_opt_cboxSendBy == SS_HTTPSERVER) {
		m_opt_cboxSendBy = SS_IMAGESHACK;
	}
	if (myGlobals.PluginFTPExist) {
		ComboBox_SetItemData(hCtrl, ComboBox_AddString(hCtrl, TranslateT("FTP File"))     ,SS_FTPFILE);
	}
	else if(m_opt_cboxSendBy == SS_FTPFILE) {
		m_opt_cboxSendBy = SS_IMAGESHACK;
	}
	ComboBox_SetItemData(hCtrl, ComboBox_AddString(hCtrl, TranslateT("ImageShack"))   ,(BYTE)SS_IMAGESHACK);
	ComboBox_SelectItemData (hCtrl, -1, m_opt_cboxSendBy);	//use Workaround for MS bug ComboBox_SelectItemData
	cboxSendByChange();		//enable disable controls
	}
	//init footer options
	CheckDlgButton(m_hWnd,ID_chkOpenAgain, m_opt_chkOpenAgain ? BST_CHECKED : BST_UNCHECKED);

	if (hCtrl = GetDlgItem(m_hWnd, ID_btnAbout)) {
		SendDlgItemMessage(m_hWnd, ID_btnAbout, BUTTONADDTOOLTIP, (WPARAM)TranslateT("Information"), MBF_TCHAR);
		HICON hIcon = IcoLib_GetIcon(ICO_PLUG_SSHELP);
		SendMessage(hCtrl, BM_SETIMAGE, IMAGE_ICON, (LPARAM)hIcon);
		SetWindowText(hCtrl, hIcon ? _T("") : _T("?"));
	}

	if (hCtrl = GetDlgItem(m_hWnd, ID_btnExplore)) {
		SendDlgItemMessage(m_hWnd, ID_btnExplore, BUTTONADDTOOLTIP, (WPARAM)TranslateT("Open Folder"), MBF_TCHAR);
		HICON hIcon = IcoLib_GetIcon(ICO_PLUG_SSFOLDERO);
		SendMessage(hCtrl, BM_SETIMAGE, IMAGE_ICON, (LPARAM)hIcon);
		SetWindowText(hCtrl, hIcon ? _T("") : _T("..."));
	}

	if (hCtrl = GetDlgItem(m_hWnd, ID_btnDesc)) {
		SendDlgItemMessage(m_hWnd, ID_btnDesc, BUTTONADDTOOLTIP, (WPARAM)TranslateT("Fill description textbox."), MBF_TCHAR);
		HICON hIcon = IcoLib_GetIcon(m_opt_btnDesc ? ICO_PLUG_SSDESKON : ICO_PLUG_SSDESKOFF);
		SendMessage(hCtrl, BM_SETIMAGE, IMAGE_ICON, (LPARAM)hIcon);
		SetWindowText(hCtrl, hIcon ? _T("") : _T("D"));
		SendMessage(hCtrl, BM_SETCHECK, m_opt_btnDesc ? BST_CHECKED : BST_UNCHECKED, NULL);
	}

	if (hCtrl = GetDlgItem(m_hWnd, ID_btnDeleteAfterSend)) {
		SendDlgItemMessage(m_hWnd, ID_btnDeleteAfterSend, BUTTONADDTOOLTIP, (WPARAM)TranslateT("Delete after send"), MBF_TCHAR);
		HICON hIcon = IcoLib_GetIcon(m_opt_btnDeleteAfterSend ? ICO_PLUG_SSDELON : ICO_PLUG_SSDELOFF);
		SendMessage(hCtrl, BM_SETIMAGE, IMAGE_ICON, (LPARAM)hIcon);
		SetWindowText(hCtrl, hIcon ? _T("") : _T("X"));
		SendMessage(hCtrl, BM_SETCHECK, m_opt_btnDeleteAfterSend ? BST_CHECKED : BST_UNCHECKED, NULL);
	}

	if (hCtrl = GetDlgItem(m_hWnd, ID_btnCapture)) {
		SendDlgItemMessage(m_hWnd, ID_btnCapture, BUTTONADDTOOLTIP, (WPARAM)TranslateT("Capture"), MBF_TCHAR);
		HICON hIcon = IcoLib_GetIcon(ICO_BTN_OK);
		SendMessage(hCtrl, BM_SETIMAGE, IMAGE_ICON, (LPARAM)hIcon);
		SetWindowText(hCtrl, TranslateT("&Capture"));
		SendMessage(hCtrl, BUTTONSETDEFAULT, (WPARAM)1, NULL);
	}

//	CheckDlgButton(m_hWnd,ID_chkEditor, m_opt_chkEditor ? BST_CHECKED : BST_UNCHECKED);
	TranslateDialogDefault(m_hWnd);
	return FALSE;
}

//WM_COMMAND:
LRESULT TfrmMain::wmCommand(WPARAM wParam, LPARAM lParam) {
	//---------------------------------------------------------------------------
	int IDControl = LOWORD(wParam);
	switch (HIWORD(wParam)) {
		case BN_CLICKED:		//Button controls
			switch(IDControl) {
				case IDCANCEL:
				case IDCLOSE:
					break;

				case ID_chkTimed:
					m_opt_chkTimed = (BYTE)Button_GetCheck((HWND)lParam);
					TfrmMain::chkTimedClick();
					break;
				case ID_chkClientArea:
					m_opt_chkClientArea = (BYTE)Button_GetCheck((HWND)lParam);
					if(m_hTargetWindow)
						edtSizeUpdate(m_hTargetWindow, m_opt_chkClientArea, GetParent((HWND)lParam), ID_edtSize);
					break;
				case ID_chkOpenAgain:
					m_opt_chkOpenAgain = (BYTE)Button_GetCheck((HWND)lParam);
					break;
				case ID_chkEditor:
					m_opt_chkEditor = (BYTE)Button_GetCheck((HWND)lParam);
					break;

				case ID_btnAbout:
					TfrmMain::btnAboutClick();
					break;
				case ID_btnExplore:
					TfrmMain::btnExploreClick();
					break;
				case ID_btnDesc:
					{
					m_opt_btnDesc = (m_opt_btnDesc == 0);
					HICON hIcon = IcoLib_GetIcon(m_opt_btnDesc ? ICO_PLUG_SSDESKON : ICO_PLUG_SSDESKOFF);
					SendMessage((HWND)lParam, BM_SETIMAGE, IMAGE_ICON, (LPARAM)hIcon);
					}
					break;
				case ID_btnDeleteAfterSend:
					{
					m_opt_btnDeleteAfterSend = (m_opt_btnDeleteAfterSend == 0);
					HICON hIcon = IcoLib_GetIcon(m_opt_btnDeleteAfterSend ? ICO_PLUG_SSDELON : ICO_PLUG_SSDELOFF);
					SendMessage((HWND)lParam, BM_SETIMAGE, IMAGE_ICON, (LPARAM)hIcon);
					if(m_cSend) m_cSend->m_bDeleteAfterSend = m_opt_btnDeleteAfterSend;
					}
					break;
				case ID_btnCapture:
					TfrmMain::btnCaptureClick();
					break;
				default:
					break;
			}
			break;
		case CBN_SELCHANGE:		//ComboBox controls
			switch(IDControl) {
				//lParam = Handle to the control
				case ID_cboxFormat:			//not finish
					m_opt_cboxFormat = (BYTE)ComboBox_GetItemData((HWND)lParam, ComboBox_GetCurSel((HWND)lParam));
					break;
				case ID_cboxSendBy:
					m_opt_cboxSendBy = (BYTE)ComboBox_GetItemData((HWND)lParam, ComboBox_GetCurSel((HWND)lParam));
					cboxSendByChange();
					break;
				case ID_edtCaption:			//cboxDesktopChange
					m_opt_cboxDesktop = (BYTE)ComboBox_GetItemData((HWND)lParam, ComboBox_GetCurSel((HWND)lParam));
					m_hTargetWindow = 0;
					if (m_opt_cboxDesktop > 0) {
						edtSizeUpdate(m_Monitors[m_opt_cboxDesktop-1].rcMonitor, GetParent((HWND)lParam), ID_edtSize);
					}
					else {
						edtSizeUpdate(m_VirtualScreen, GetParent((HWND)lParam), ID_edtSize);
					}
					break;
				default:
					break;
			}
			break;
		case EN_CHANGE:			//Edit controls
			switch(IDControl) {
				//lParam = Handle to the control
				case ID_edtQuality:
					m_opt_edtQuality = (BYTE)GetDlgItemInt(m_hWnd, ID_edtQuality, NULL, FALSE);
					break;
				case ID_edtTimed:
					m_opt_edtTimed = (BYTE)GetDlgItemInt(m_hWnd, ID_edtTimed, NULL, FALSE);
					break;
				default:
					break;
			}
			break;
		default:
			break;
	}
	return FALSE;
}

//WM_CLOSE:
LRESULT TfrmMain::wmClose(WPARAM wParam, LPARAM lParam) {
	DestroyWindow(m_hWnd);
	return FALSE;
}

//WM_TIMER:
LRESULT TfrmMain::wmTimer(WPARAM wParam, LPARAM lParam) {
	if (wParam == ID_bvlTarget) {		// Timer for Target selector
		if (m_hCursor) {			//imgTarget is activ
			//LmouseButton = false
			if (!GetLmouse()) {
				TfrmMain::imgTargetMouseUp();
				return FALSE;
			}
			//Timer action if LmouseButton = true
			m_hLastWin = m_hTargetWindow;
			POINT point={0};
			GetCursorPos(&point);
			HWND hCurrentWin = WindowFromPoint(point);
			while (GetParent(hCurrentWin)) {
				hCurrentWin = GetParent(hCurrentWin);
			}
			if (m_hLastWin != hCurrentWin) {
				LPTSTR lpTitle = NULL;
				if (m_hLastWin) {
					DrawBorderInverted(m_hLastWin);
				}
				int Count = GetWindowTextLength(hCurrentWin)+1;
				if(Count > 1){
					lpTitle = (LPTSTR)mir_alloc(Count*sizeof(TCHAR));
					GetWindowText(hCurrentWin, lpTitle, Count);
				}
				else {
					//no WindowText present, use WindowClass
					lpTitle = (LPTSTR)mir_alloc(MAX_PATH*sizeof(TCHAR));
					RealGetWindowClass(hCurrentWin, lpTitle, MAX_PATH);
				}
				SetDlgItemText(m_hwndTabPage, ID_edtCaption, lpTitle);
				mir_free(lpTitle);
				edtSizeUpdate(hCurrentWin, m_opt_chkClientArea, m_hwndTabPage, ID_edtSize);
				DrawBorderInverted(hCurrentWin);
				m_hTargetWindow = hCurrentWin;
			}
			return FALSE;
		}
		//imgTarget is not activ (check if cursor is over ID_bvlTarget control)
		RECT rc;
		POINT pt;
		GetWindowRect(GetDlgItem(m_hwndTabPage, wParam),&rc);
		GetCursorPos(&pt);
		//check Mouse cursor
		if(!PtInRect(&rc,pt)) {					// mouse must be gone, trigger mouse leave
			//PostMessage(m_hWnd,WM_MOUSELEAVE,wParam,lParam);
			KillTimer(m_hWnd,wParam);
		}
		else if (GetLmouse() && !m_hCursor) {	//mouse hover + LButton
			TfrmMain::imgTargetMouseDown();
		}
	}
	if (wParam == ID_chkTimed) {		// Timer for Screenshot
		#ifdef _DEBUG
			OutputDebugStringA("SS Bitmap Timer Start\r\n" );
		#endif
		if(!m_bCapture) {		//only start once
			if (m_Screenshot) {
				FIP->FI_Unload(m_Screenshot);
				m_Screenshot = NULL;
			}
			m_bCapture = true;
			switch (m_opt_tabCapture) {
				case 0:
					m_Screenshot = CaptureWindow(m_hTargetWindow, (BOOL)m_opt_chkClientArea);
					break;
				case 1:
					m_Screenshot = CaptureMonitor((m_opt_cboxDesktop > 0) ? m_Monitors[m_opt_cboxDesktop-1].szDevice : NULL);
					break;
				default:
					KillTimer(m_hWnd,wParam);
					m_bCapture = false;
					#ifdef _DEBUG
						OutputDebugStringA("SS Bitmap Timer Stop (no tabCapture)\r\n" );
					#endif
					return FALSE;
			}
			if (!m_Screenshot) m_bCapture = false;
		}
		if (m_Screenshot) {
			KillTimer(m_hWnd,wParam);
			m_bCapture = false;
			#ifdef _DEBUG
				OutputDebugStringA("SS Bitmap Timer Stop (CaptureDone)\r\n" );
			#endif
			SendMessage(m_hWnd,UM_EVENT, 0, (LPARAM)EVT_CaptureDone);
		}
	}
	return FALSE;
}

//WM_NOTIFY:
LRESULT TfrmMain::wmNotify(WPARAM wParam, LPARAM lParam) {
	switch(((LPNMHDR)lParam)->idFrom) {
		case IDC_CAPTURETAB:				//TabControl IDC_CAPTURETAB
			switch (((LPNMHDR)lParam)->code) {
				//    HWND hwndFrom;	= member is handle to the tab control
				//    UINT_PTR idFrom;	= member is the child window identifier of the tab control.
				//    UINT code;		= member is TCN_SELCHANGE
				case TCN_SELCHANGING:
					{
					TAB_INFO itab;
					ZeroMemory(&itab, sizeof(itab));
					itab.tcih.mask = TCIF_PARAM;
					TabCtrl_GetItem(m_hwndTab,TabCtrl_GetCurSel(m_hwndTab),&itab);
					ShowWindow(itab.hwndTabPage,SW_HIDE);
					m_hwndTabPage = NULL;
					}
					break;

				case TCN_SELCHANGE:
					{
					TAB_INFO itab;
					ZeroMemory(&itab, sizeof(itab));
					itab.tcih.mask = TCIF_PARAM;
					m_opt_tabCapture = TabCtrl_GetCurSel(m_hwndTab);
					TabCtrl_GetItem(m_hwndTab, m_opt_tabCapture, &itab);
					ShowWindow(itab.hwndTabPage, SW_SHOW);
					m_hwndTabPage = itab.hwndTabPage;
					}
					break;
				default:
					break;
			}
			break;
		default:
			break;
	}
	return FALSE;
}

//UM_CLOSING:
LRESULT TfrmMain::UMClosing(WPARAM wParam, LPARAM lParam) {
	HWND hWnd = (HWND)wParam;
	switch (lParam) {
		case IDD_UAboutForm:
			btnAboutOnCloseWindow(hWnd);
			break;
		case IDD_UEditForm:
			;
			break;
		default:
			break;
	}
	return FALSE;
}

//UM_TAB1:
LRESULT TfrmMain::UMTab1(WPARAM wParam, LPARAM lParam) {
	switch (wParam) {
		case WM_MOUSEMOVE:
			if (m_opt_tabCapture == 0) {
				// Call timer, used to start cheesy TrackMouseEvent faker
				SetTimer(m_hWnd,ID_bvlTarget,BUTTON_POLLDELAY,NULL);
			}
			break;
		default:
			break;
	}
	return FALSE;
}

//UM_EVENT:
LRESULT TfrmMain::UMevent(WPARAM wParam, LPARAM lParam) {
	//HWND hWnd = (HWND)wParam;
	switch (lParam) {
		case EVT_CaptureDone:
			if (!m_Screenshot) {
				TCHAR *err = TranslateT("Cant create a Screenshot");
				MessageBox(m_hWnd,err,ERROR_TITLE,MB_OK|MB_ICONWARNING);
				Show();
				return FALSE;
			}
			if (m_opt_chkEditor) {
			/*	TfrmEdit *frmEdit=new TfrmEdit(this);
				m_bFormEdit = true;

				frmEdit->mniClose->Enabled = !chkJustSaveIt->Checked;
				frmEdit->mniCloseSend->Enabled = frmEdit->mniClose->Enabled;
				frmEdit->OnClose = OnCloseEditWindow;
				frmEdit->InitEditor(Screenshot); // Screenshot is copied to another in-memory bitmap inside this method
				frmEdit->Show();
				delete Screenshot; // This way we can delete it after the method returns
				Screenshot = NULL;
			*/
				return FALSE;
			}
			else {
				FormClose();
			}
			break;
		case EVT_SendFileDone:
			break;
		case EVT_CheckOpenAgain:
			if (m_opt_chkOpenAgain) {
				if (m_Screenshot) {
					FIP->FI_Unload(m_Screenshot);
					m_Screenshot = NULL;
				}
				m_hTargetWindow = m_hLastWin = NULL;
				Show();
			}
			else {
				// Saving Options and close
				SaveOptions();
				Close();
			}
			break;
		default:
			break;
	}
	return FALSE;
}

//---------------------------------------------------------------------------
// Standard konstruktor/destruktor
TfrmMain::TfrmMain() {
	m_hWnd			= NULL;
	m_hContact		= NULL;
	m_hTargetWindow	= NULL;
	m_hCursor		= NULL;
	m_Screenshot	= NULL;
	m_pszFile		= m_pszFileDesc = m_FDestFolder = NULL;
	m_bFormAbout	= m_bFormEdit = m_bDeleteAfterSend = m_bSelectingWindow = false;
	m_cSend			= NULL;
	m_bOnExitSave	= TRUE;
	LoadOptions();
	m_bCapture		= false;
	m_himlTab		= NULL;
	m_Monitors		= NULL;
	m_MonitorCount	= MonitorInfoEnum(m_Monitors, m_VirtualScreen);

}

TfrmMain::~TfrmMain() {
	_HandleMapping.erase(m_hWnd);
	mir_free(m_pszFile);
	mir_free(m_FDestFolder);
	mir_free(m_pszFileDesc);
	mir_free(m_Monitors);
	if (m_Screenshot) FIP->FI_Unload(m_Screenshot);
	if (m_cSend) delete m_cSend;
}

//---------------------------------------------------------------------------
// Load / Saving options from miranda's database
void TfrmMain::LoadOptions(void) {
	DWORD rgb					= DBGetContactSettingDword(NULL, SZ_SENDSS, "AlphaColor", 16777215);
	m_AlphaColor.rgbRed			= GetRValue(rgb);
	m_AlphaColor.rgbGreen		= GetGValue(rgb);
	m_AlphaColor.rgbBlue		= GetBValue(rgb);
	m_AlphaColor.rgbReserved	= 0;

//	m_opt_chkEmulateClick		= DBGetContactSettingByte(NULL, SZ_SENDSS, "AutoSend", 1);
	m_opt_edtQuality			= DBGetContactSettingByte(NULL, SZ_SENDSS, "JpegQuality", 75);

	m_opt_tabCapture			= DBGetContactSettingByte(NULL, SZ_SENDSS, "Capture", 0);
	m_opt_chkClientArea			= DBGetContactSettingByte(NULL, SZ_SENDSS, "ClientArea", 0);
	m_opt_cboxDesktop			= DBGetContactSettingByte(NULL, SZ_SENDSS, "Desktop", 0);

	m_opt_chkTimed				= DBGetContactSettingByte(NULL, SZ_SENDSS, "TimedCap", 0);
	m_opt_edtTimed				= DBGetContactSettingByte(NULL, SZ_SENDSS, "CapTime", 3);
	m_opt_cboxFormat			= DBGetContactSettingByte(NULL, SZ_SENDSS, "OutputFormat", 3);
	m_opt_cboxSendBy			= DBGetContactSettingByte(NULL, SZ_SENDSS, "SendBy", 0);

	m_opt_chkEditor				= DBGetContactSettingByte(NULL, SZ_SENDSS, "Preview", 0);
	m_opt_btnDesc				= DBGetContactSettingByte(NULL, SZ_SENDSS, "AutoDescription", 1);
	m_opt_btnDeleteAfterSend	= DBGetContactSettingByte(NULL, SZ_SENDSS, "DelAfterSend", 1);
	m_opt_chkOpenAgain			= DBGetContactSettingByte(NULL, SZ_SENDSS, "OpenAgain", 0);
}

void TfrmMain::SaveOptions(void) {
	if(m_bOnExitSave) {
		DBWriteContactSettingDword(NULL, SZ_SENDSS, "AlphaColor",
			(DWORD)RGB(m_AlphaColor.rgbRed, m_AlphaColor.rgbGreen, m_AlphaColor.rgbBlue));

//		DBWriteContactSettingByte(NULL, SZ_SENDSS, "AutoSend", m_opt_chkEmulateClick);
		DBWriteContactSettingByte(NULL, SZ_SENDSS, "JpegQuality", m_opt_edtQuality);

		DBWriteContactSettingByte(NULL, SZ_SENDSS, "Capture", m_opt_tabCapture);
		DBWriteContactSettingByte(NULL, SZ_SENDSS, "ClientArea", m_opt_chkClientArea);
		DBWriteContactSettingByte(NULL, SZ_SENDSS, "Desktop", m_opt_cboxDesktop);

		DBWriteContactSettingByte(NULL, SZ_SENDSS, "TimedCap", m_opt_chkTimed);
		DBWriteContactSettingByte(NULL, SZ_SENDSS, "CapTime", m_opt_edtTimed);
		DBWriteContactSettingByte(NULL, SZ_SENDSS, "OutputFormat", m_opt_cboxFormat);
		DBWriteContactSettingByte(NULL, SZ_SENDSS, "SendBy", m_opt_cboxSendBy);

		DBWriteContactSettingByte(NULL, SZ_SENDSS, "AutoDescription", m_opt_btnDesc);
		DBWriteContactSettingByte(NULL, SZ_SENDSS, "DelAfterSend", m_opt_btnDeleteAfterSend);
		DBWriteContactSettingByte(NULL, SZ_SENDSS, "OpenAgain", m_opt_chkOpenAgain);
		DBWriteContactSettingByte(NULL, SZ_SENDSS, "Preview", m_opt_chkEditor);
	}
}

//---------------------------------------------------------------------------
void TfrmMain::Init(LPTSTR DestFolder, HANDLE Contact) {
	if(DestFolder)
	{
		m_FDestFolder = mir_tstrdup(DestFolder);
		m_hContact = Contact;
		if(!m_hContact) m_opt_cboxSendBy = SS_JUSTSAVE;

		// create window
		m_hWnd = CreateDialogParam(hInst, MAKEINTRESOURCE(IDD_UMainForm),0, (DLGPROC)DlgTfrmMain,(LPARAM)this);
		//register object
		_HandleMapping.insert(CHandleMapping::value_type(m_hWnd, this));

		//check Contact
		if(m_cSend) m_cSend->SetContact(Contact);
	}
}

//---------------------------------------------------------------------------
void TfrmMain::btnCaptureClick() {
	m_bFormEdit = false;					//until UEditForm is includet

	if (m_opt_tabCapture == 0 && m_hTargetWindow == 0) {
		TCHAR *err = TranslateT("Select a target window.");
		MessageBox(m_hWnd,err,ERROR_TITLE,MB_OK|MB_ICONWARNING);
		return;
	}
	TfrmMain::Hide();

	if (!m_hTargetWindow) m_hTargetWindow = GetDesktopWindow();

	if (m_opt_chkTimed) {
		SetTimer(m_hWnd, ID_chkTimed, m_opt_edtTimed ? m_opt_edtTimed*1000 : 500, NULL);
	}
	else if (m_opt_tabCapture == 1){
		//desktop need always time to update from TfrmMain::Hide()
		SetTimer(m_hWnd, ID_chkTimed, 500, NULL);
	}
	else {
		m_Screenshot = CaptureWindow(m_hTargetWindow, (BOOL)(m_opt_chkClientArea));
		SendMessage(m_hWnd,UM_EVENT, 0, (LPARAM)EVT_CaptureDone);
	}
}

//---------------------------------------------------------------------------
void TfrmMain::chkTimedClick() {
	Button_Enable(GetDlgItem(m_hWnd, ID_edtTimedLabel), (BOOL)m_opt_chkTimed);
	Button_Enable(GetDlgItem(m_hWnd, ID_edtTimed), (BOOL)m_opt_chkTimed);
	Button_Enable(GetDlgItem(m_hWnd, ID_upTimed), (BOOL)m_opt_chkTimed);
}

//---------------------------------------------------------------------------
void TfrmMain::imgTargetMouseDown() {
	//if (Button != mbLeft) return;
	m_hCursor = CopyCursor(GetCursor()); //backup cursor
	//SetSystemCursor need a copy coz it destroy the handle
	SetSystemCursor(CopyCursor(IcoLib_GetIcon(ICO_PLUG_SSTARGET)),OCR_NORMAL);
	m_bSelectingWindow = true;
	m_hTargetWindow = NULL;
	Hide();
	SetCapture(m_hWnd);
}

//---------------------------------------------------------------------------
void TfrmMain::imgTargetMouseUp() {
	//if (Button == mbLeft && m_bSelectingWindow && TimerCheckFocus->Enabled)
	if (m_bSelectingWindow && m_hCursor) {
		Show();
		ReleaseCapture();
		SetSystemCursor(m_hCursor, OCR_NORMAL);
		m_hCursor = NULL;
		if (m_hTargetWindow){
			DrawBorderInverted(m_hTargetWindow);
		}
		m_bSelectingWindow = false;
	}
}

//---------------------------------------------------------------------------
void TfrmMain::cboxSendByChange() {
	BOOL bState;
	HICON hIcon;
	BYTE itemFlag = SS_DLG_DESCRIPTION;		//SS_DLG_AUTOSEND | SS_DLG_DELETEAFTERSSEND |
	if (m_cSend && !m_cSend->m_bFreeOnExit) {
		delete m_cSend;
		m_cSend = NULL;
	}
	switch(m_opt_cboxSendBy) {
		case SS_FILESEND:		//"File Transfer"
			m_cSend = new CSendFile(m_hWnd, m_hContact, false);
			break;
		case SS_EMAIL:			//"E-mail"
			m_cSend = new CSendEmail(m_hWnd, m_hContact, false);
			break;
		case SS_HTTPSERVER:		//"HTTP Server"
			m_cSend = new CSendHTTPServer(m_hWnd, m_hContact, false);
			break;
		case SS_FTPFILE:		//"FTP File"
			m_cSend = new CSendFTPFile(m_hWnd, m_hContact, false);
			break;
		case SS_IMAGESHACK:		//"ImageShack"
			m_cSend = new CSendImageShack(m_hWnd, m_hContact, false);
			break;
		default:				//SS_JUSTSAVE - "Just save it "
			m_cSend = NULL;
			break;
	}
	if(m_cSend){
		itemFlag = m_cSend->GetEnableItem();
		m_cSend->m_bDeleteAfterSend = m_opt_btnDeleteAfterSend;
	}
	bState = ((itemFlag & SS_DLG_DELETEAFTERSSEND) == SS_DLG_DELETEAFTERSSEND);
	hIcon = IcoLib_GetIcon(m_opt_btnDeleteAfterSend ? ICO_PLUG_SSDELON : ICO_PLUG_SSDELOFF);
	SendMessage(GetDlgItem(m_hWnd, ID_btnDeleteAfterSend), BM_SETIMAGE, IMAGE_ICON, (LPARAM)(bState ? hIcon : 0));
	Button_Enable(GetDlgItem(m_hWnd, ID_btnDeleteAfterSend), bState);

	bState = ((itemFlag & SS_DLG_DESCRIPTION) == SS_DLG_DESCRIPTION);
	hIcon = IcoLib_GetIcon(m_opt_btnDesc ? ICO_PLUG_SSDESKON : ICO_PLUG_SSDESKOFF);
	SendMessage(GetDlgItem(m_hWnd, ID_btnDesc), BM_SETIMAGE, IMAGE_ICON, (LPARAM)(bState ? hIcon : 0));
	Button_Enable(GetDlgItem(m_hWnd, ID_btnDesc), bState);
}

//---------------------------------------------------------------------------
void TfrmMain::btnAboutClick() {
	if (m_bFormAbout) return;

	TfrmAbout *frmAbout=new TfrmAbout(m_hWnd);
	frmAbout->Show();
	m_bFormAbout = true;
}

// Edit window call this event before it closes
void TfrmMain::btnAboutOnCloseWindow(HWND hWnd) {
	m_bFormAbout = false;
}

//---------------------------------------------------------------------------
void TfrmMain::btnExploreClick() {
	if (m_FDestFolder)
		ShellExecute(NULL, _T("explore"), m_FDestFolder, NULL, NULL, SW_SHOW);
}

//---------------------------------------------------------------------------
void TfrmMain::edtSizeUpdate(HWND hWnd, BOOL ClientArea, HWND hTarget, UINT Ctrl) {
	// get window dimensions
	RECT rect = {0};
	RECT cliRect = {0};
	TCHAR B[33], H[16];
	GetWindowRect(hWnd, &rect);
	if (ClientArea) {
		POINT pt = {0};
		GetClientRect(hWnd, &cliRect);
		pt.x = cliRect.left;
		pt.y = cliRect.top;
		ClientToScreen(hWnd, &pt);
		pt.x = pt.x - rect.left;			//offset x for client area
		pt.y = pt.y - rect.top;				//offset y for client area
		rect = cliRect;
	}
//	_itot_s(rect.right - rect.left, B, 33, 10);
	_itot(rect.right - rect.left, B, 10);
//	_itot_s(rect.bottom - rect.top, H, 16, 10);
	_itot(rect.bottom - rect.top, H, 10);
	mir_tcsncat(B, _T("x"), 33);
	mir_tcsncat(B, H, 33);
	SetDlgItemText(hTarget, Ctrl, B);
}

void TfrmMain::edtSizeUpdate(RECT rect, HWND hTarget, UINT Ctrl) {
	TCHAR B[33], H[16];
//	_itot_s(ABS(rect.right - rect.left), B, 33, 10);
	_itot(ABS(rect.right - rect.left), B, 10);
//	_itot_s(ABS(rect.bottom - rect.top), H, 16, 10);
	_itot(ABS(rect.bottom - rect.top), H, 10);
	mir_tcsncat(B, _T("x"), 33);
	mir_tcsncat(B, H, 33);
	SetDlgItemText(hTarget, Ctrl, B);
}

//---------------------------------------------------------------------------
INT_PTR TfrmMain::SaveScreenshot(FIBITMAP* dib) {
	//generate File name
	FREE_IMAGE_FORMAT fif = FIF_UNKNOWN;
	LPTSTR ret = NULL;
	LPTSTR path = NULL;
	LPTSTR pszFilename = NULL;
	LPTSTR pszFileDesc = NULL;
	if (!dib) return 1;		//error

	//Generate FileName
	mir_tcsadd(path, m_FDestFolder);
	if (path[_tcslen(path)-1] != _T('\\')) mir_tcsadd(path, _T("\\"));
	mir_tcsadd(path, _T("shot%.5ld"));
	int FileNumber=DBGetContactSettingDword(NULL, SZ_SENDSS, "FileNumber", 0) + 1;
	// '00000'-'%.5ld'=0 (add more or less len if differ from 5
	size_t len = (_tcslen(path)+0+1);
	pszFilename = (LPTSTR)mir_alloc(sizeof(TCHAR)*(len));
	mir_sntprintf(pszFilename, len, path, FileNumber);
	mir_free(path);

	//Generate a description according to the screenshot
	TCHAR winText[1024];
	mir_tcsadd(pszFileDesc, _T("Screenshot "));
	if (m_opt_tabCapture == 0 && m_opt_chkClientArea) {
		mir_tcsadd(pszFileDesc, _T("for Client area "));
	}
	mir_tcsadd(pszFileDesc, _T("of \""));
	GetDlgItemText(m_hwndTabPage, ID_edtCaption, winText, 1024);
	mir_tcsadd(pszFileDesc, winText);
	mir_tcsadd(pszFileDesc, _T("\" Window"));

	// convert to 32Bits (make shure it is 32bit)
	FIBITMAP *dib_new = FIP->FI_ConvertTo32Bits(dib);
	//RGBQUAD appColor = { 245, 0, 254, 0 };	//bgr0	schwarz
	//FIP->FI_SetBackgroundColor(dib_new, &appColor);
	FIP->FI_SetTransparent(dib_new,TRUE);

	// Investigates the color type of the bitmap (test for RGB or CMYK colour space)
	switch (FREE_IMAGE_COLOR_TYPE ColTye=FIP->FI_GetColorType(dib_new)) {
		case FIC_MINISBLACK:
			//Monochrome bitmap (1-bit) : first palette entry is black.
			//Palletised bitmap (4 or 8-bit) and single channel non standard bitmap: the bitmap has a greyscale palette
		case FIC_MINISWHITE:
			//Monochrome bitmap (1-bit) : first palette entry is white.
			//Palletised bitmap (4 or 8-bit) : the bitmap has an inverted greyscale palette
		case FIC_PALETTE:
			//Palettized bitmap (1, 4 or 8 bit)
		case FIC_RGB:
			//High-color bitmap (16, 24 or 32 bit), RGB16 or RGBF
		case FIC_RGBALPHA:
			//High-color bitmap with an alpha channel (32 bit bitmap, RGBA16 or RGBAF)
		case FIC_CMYK:
			//CMYK bitmap (32 bit only)
		default:
			break;
	}

	
	if ((FIP->FI_GetICCProfile(dib_new)->flags & FIICC_COLOR_IS_CMYK) == FIICC_COLOR_IS_CMYK) {
		// we are in CMYK colour space
		bool bDummy = false;
	}
	else {
		// we are in RGB colour space
		bool bDummy = true;
	}

	FIBITMAP *dib32 = NULL;
	FIBITMAP *dib24 = NULL;
	HWND hwndCombo = GetDlgItem(m_hWnd, ID_cboxFormat);
	switch (ComboBox_GetItemData(hwndCombo, ComboBox_GetCurSel(hwndCombo))) {
		case 0: //PNG
			ret = SaveImage(fif,dib_new, pszFilename, _T("png"));
			break;

		case 1: //JPG
			/*
			#define JPEG_QUALITYSUPERB  0x80	// save with superb quality (100:1)
			#define JPEG_QUALITYGOOD    0x0100	// save with good quality (75:1)
			#define JPEG_QUALITYNORMAL  0x0200	// save with normal quality (50:1)
			#define JPEG_QUALITYAVERAGE 0x0400	// save with average quality (25:1)
			#define JPEG_QUALITYBAD     0x0800	// save with bad quality (10:1)
			#define JPEG_PROGRESSIVE	0x2000	// save as a progressive-JPEG (use | to combine with other save flags)
			*/
			dib32 = FIP->FI_Composite(dib_new,FALSE,&m_AlphaColor,NULL);
			dib24 = FIP->FI_ConvertTo24Bits(dib32);
			FIP->FI_Unload(dib32); dib32 = NULL;
			ret = SaveImage(fif,dib24, pszFilename, _T("jpg"));
			FIP->FI_Unload(dib24); dib24 = NULL;
			break;

		case 2: //BMP
		//	ret = SaveImage(FIF_BMP,dib_new, pszFilename, _T("bmp")); //32bit alpha BMP
			dib32 = FIP->FI_Composite(dib_new,FALSE,&m_AlphaColor,NULL);
			dib24 = FIP->FI_ConvertTo24Bits(dib32);
			FIP->FI_Unload(dib32); dib32 = NULL;
			ret = SaveImage(FIF_BMP,dib24, pszFilename, _T("bmp"));
			FIP->FI_Unload(dib24); dib24 = NULL;
			break;

		case 3: //TIFF (miranda freeimage interface do not support save tiff, we udse GDI+)
			{
			LPTSTR pszFile = NULL;
			mir_tcsadd(pszFile, pszFilename);
			mir_tcsadd(pszFile, _T(".tif"));

			dib32 = FIP->FI_Composite(dib_new,FALSE,&m_AlphaColor,NULL);
			dib24 = FIP->FI_ConvertTo24Bits(dib32);
			FIP->FI_Unload(dib32); dib32 = NULL;

			HBITMAP hBmp = FIP->FI_CreateHBITMAPFromDIB(dib24);
			FIP->FI_Unload(dib24); dib24 = NULL;
			ret = SaveTIF(hBmp, pszFile) ? NULL : pszFile;
			DeleteObject(hBmp);
			}
			break;

		case 4: //GIF
			{
			//dib24 = FIP->FI_ConvertTo8Bits(dib_new);
			//ret = SaveImage(FIF_GIF,dib24, pszFilename, _T("gif"));
			//FIP->FI_Unload(dib24); dib24 = NULL;
			LPTSTR pszFile = NULL;
			mir_tcsadd(pszFile, pszFilename);
			mir_tcsadd(pszFile, _T(".gif"));
			HBITMAP hBmp = FIP->FI_CreateHBITMAPFromDIB(dib_new);
			SaveGIF(hBmp, pszFile);
			ret = pszFile;
			DeleteObject(hBmp);
			}
			break;

		default:
			break;
	}
/*	//load PNG and save file in user format (if differ)
	//this get better result for transparent aereas
	//LPTSTR pszFormat = (LPTSTR)ComboBox_GetItemData(hwndCombo, ComboBox_GetCurSel(hwndCombo));
	TCHAR pszFormat[6];
	ComboBox_GetText(hwndCombo, pszFormat, 6);
	if(ret && (_tcsicmp (pszFormat,_T("png")) != 0)) {
		#if defined(_UNICODE)
			fif = FIP->FI_GetFIFFromFilenameU(ret);
			dib_new = FIP->FI_LoadU(fif, ret,0);
		#else
			fif = FIP->FI_GetFIFFromFilenameU(ret);
			dib_new = FIP->FI_Load(fif, ret,0);
		#endif
		
		if(dib_new) {
			DeleteFile(ret);
			mir_freeAndNil(ret);
			FIBITMAP *dib_save = FIP->FI_ConvertTo24Bits(dib_new);
			ret = SaveImage(FIF_UNKNOWN,dib_save, pszFilename, pszFormat);
			FIP->FI_Unload(dib_new); dib_new = NULL;
			FIP->FI_Unload(dib_save); dib_save = NULL;
		}
	}*/
	FIP->FI_Unload(dib_new); dib_new = NULL;
	mir_freeAndNil(pszFilename);

	if (ret) {
		DBWriteContactSettingDword(NULL, SZ_SENDSS, "FileNumber", (DWORD)FileNumber);
		mir_freeAndNil(m_pszFile);
		mir_freeAndNil(m_pszFileDesc);
		m_pszFile = ret;
		if (IsWindowEnabled(GetDlgItem(m_hWnd, ID_btnDesc)) && m_opt_btnDesc) {
			m_pszFileDesc = pszFileDesc;
		}
		else {
			mir_tcsadd(m_pszFileDesc, _T(""));
		}
		
		if(m_cSend) {
			mir_freeAndNil(m_cSend->m_pszFile);
			mir_freeAndNil(m_cSend->m_pszFileDesc);
			m_cSend->m_pszFile = mir_tstrdup(m_pszFile);
			m_cSend->m_pszFileDesc = mir_tstrdup(m_pszFileDesc);
		}
		return 0;	//OK
	}
	mir_freeAndNil(ret);
	mir_freeAndNil(pszFileDesc);
	return 1;		//error
}

//---------------------------------------------------------------------------
void TfrmMain::FormClose() {

	// Saving the screenshot
	if (SaveScreenshot(m_Screenshot)) {
		Show();		// Error from SaveScreenshot
		return;
	}

	if (m_cSend && m_pszFile && m_hContact && !m_bFormEdit) {
		m_cSend->Send();
		if (m_cSend->m_bFreeOnExit) cboxSendByChange();
//		Not finish delete this if events from m_opt_cboxSendBy implementet
		SendMessage(m_hWnd,UM_EVENT, 0, (LPARAM)EVT_CheckOpenAgain);
	}
	else {
		SendMessage(m_hWnd,UM_EVENT, 0, (LPARAM)EVT_CheckOpenAgain);
	}
}

//---------------------------------------------------------------------------
/*/ Edit window call this event before it closes
void TfrmMain::OnCloseEditWindow(TObject *Sender, TCloseAction &Action) {
	TfrmEdit *form=dynamic_cast<TfrmEdit*>(Sender);
	form->Hide();

	// delete the form automatically,after this event returns
	Action = caFree;

	// This will saves settings, free resources, ...
	form->CallBeforeClose(Action);

	// User selected "Capture" on action menu of edit window
	if (form->ModalResult == mrCancel) {
		this->Show();
	} else {
		Screenshot = form->Screen;
		bFormEdit = form->DontSend;
		this->Close();
	}
}*/

//---------------------------------------------------------------------------
