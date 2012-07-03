/*

'AutoShutdown'-Plugin for Miranda IM

Copyright 2004-2007 H. Herkenrath

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program (Shutdown-License.txt); if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "common.h"

/* Shutdown Dialog */
static HWND hwndShutdownDlg;
extern HINSTANCE hInst;
/* Services */
static HANDLE hEventOkToShutdown,hEventShutdown;
static HANDLE hServiceShutdown,hServiceIsTypeEnabled,hServiceGetTypeDesc;

/************************* Utils **************************************/

static BOOL WinNT_SetPrivilege(TCHAR *pszPrivName,BOOL bEnable)
{
	BOOL bReturn=FALSE;
	HANDLE hToken;
	TOKEN_PRIVILEGES tkp;
	/* get a token for this process */
	if(OpenProcessToken(GetCurrentProcess(),TOKEN_ADJUST_PRIVILEGES|TOKEN_QUERY,&hToken)) {
		tkp.PrivilegeCount=1; /* one privilege is to set */   
		/* get the LUID for the shutdown privilege */
		if(LookupPrivilegeValue(NULL,pszPrivName,&tkp.Privileges[0].Luid)) {
			tkp.Privileges[0].Attributes=bEnable?SE_PRIVILEGE_ENABLED:0;
			/* get the shutdown privilege for this process */
			bReturn=AdjustTokenPrivileges(hToken,FALSE,&tkp,0,(PTOKEN_PRIVILEGES)NULL,0);
		}
		CloseHandle(hToken);
	}
	return bReturn;
}

static void BroadcastEndSession(DWORD dwRecipients,LPARAM lParam)
{
	LONG (WINAPI *pfnBroadcastSystemMessage)(DWORD,DWORD*,UINT,WPARAM,LPARAM);
#if defined(_UNICODE)
	*(PROC*)&pfnBroadcastSystemMessage=GetProcAddress(GetModuleHandleA("USER32"),"BroadcastSystemMessageW");
#else
	*(PROC*)&pfnBroadcastSystemMessage=GetProcAddress(GetModuleHandleA("USER32"),"BroadcastSystemMessageA");
	if(!pfnBroadcastSystemMessage) /* Win95 has undecorated API */
		*(PROC*)&pfnBroadcastSystemMessage=GetProcAddress(GetModuleHandleA("USER32"),"BroadcastSystemMessage");
#endif
	if(pfnBroadcastSystemMessage)
		pfnBroadcastSystemMessage(BSF_FORCEIFHUNG,&dwRecipients,WM_ENDSESSION,TRUE,lParam);
}

static BOOL WinNT_IsWorkStationLocked(void)
{
	HDESK hDesk;
	TCHAR szName[8];
	DWORD cbName;
	BOOL bLocked;
	hDesk=OpenInputDesktop(0,FALSE,DESKTOP_READOBJECTS);
	if(hDesk==NULL) return TRUE;
	bLocked=(!GetUserObjectInformation(hDesk,UOI_NAME,szName,SIZEOF(szName),&cbName) || lstrcmpi(szName,_T("default"))!=0);
	CloseDesktop(hDesk);
	return bLocked;
} 

#if !defined(_UNICODE)
static void Win9x_TerminateExplorer(void)
{
	HANDLE hProcess;
	DWORD idProcess;
	if(GetWindowThreadProcessId(GetDesktopWindow(),&idProcess)) {
		hProcess=OpenProcess(PROCESS_TERMINATE,FALSE,idProcess);
		if(hProcess!=NULL) {
			TerminateProcess(hProcess,0);
			CloseHandle(hProcess);
		}
	}
}
#endif

/************************* Workers ************************************/

static BOOL IsShutdownTypeEnabled(BYTE shutdownType)
{
	BOOL bReturn=FALSE;
	switch(shutdownType) {
		case SDSDT_HIBERNATE:
		case SDSDT_STANDBY:
			{	HMODULE hPowerDLL=LoadLibraryA("POWRPROF"); /* all ascii */
				if(hPowerDLL!=NULL) {
					BOOLEAN (STDAPICALLTYPE *pfnIsPwrModeAllowed)(void);
					*(PROC*)&pfnIsPwrModeAllowed=GetProcAddress(hPowerDLL,(shutdownType==SDSDT_HIBERNATE)?"IsPwrHibernateAllowed":"IsPwrSuspendAllowed");
					if(pfnIsPwrModeAllowed) bReturn=pfnIsPwrModeAllowed()!=0;
					FreeLibrary(hPowerDLL);
				}
			}
			/* test privilege */
			if(bReturn && IsWinVerNT()) {
				bReturn=WinNT_SetPrivilege(SE_SHUTDOWN_NAME,TRUE);
				if(bReturn) WinNT_SetPrivilege(SE_SHUTDOWN_NAME,FALSE);
			}
			break;
		case SDSDT_LOGOFF:
			{	HKEY hKey;
				DWORD dwSetting,dwSize;
				/* NoLogOff is BINARY on Win9x/ME and DWORD on Win2000+ */
				bReturn=TRUE;
				if(RegOpenKeyEx(HKEY_CURRENT_USER,_T("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer"),0,KEY_QUERY_VALUE,&hKey)==ERROR_SUCCESS) {
					dwSize=sizeof(dwSetting);
					if(RegQueryValueEx(hKey,_T("NoLogOff"),0,NULL,(void*)&dwSetting,&dwSize)==ERROR_SUCCESS)
						if(dwSetting) bReturn=FALSE;
					RegCloseKey(hKey);
				}
			}
			break;
		case SDSDT_LOCKWORKSTATION:
			{	void (WINAPI *pfnLockWorkStation)(void);
				*(PROC*)&pfnLockWorkStation=GetProcAddress(GetModuleHandleA("USER32"),"LockWorkStation");
				if(pfnLockWorkStation) {
					HKEY hKey;
					DWORD dwSize,dwSetting;
					/* DisableLockWorkstation is DWORD on Win2000+ */
					bReturn=TRUE;
					if(RegOpenKeyEx(HKEY_CURRENT_USER,_T("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System"),0,KEY_QUERY_VALUE,&hKey)==ERROR_SUCCESS) {
						dwSize=sizeof(dwSetting);
						if(!RegQueryValueEx(hKey,_T("DisableLockWorkstation"),0,NULL,(void*)&dwSetting,&dwSize))
							if(dwSetting) bReturn=FALSE;
						RegCloseKey(hKey);
					}
				}
				else if(IsWinVerNT()) /* for WinNT4 */
					bReturn=SearchPath(NULL,_T("LOGIN.SCR"),NULL,0,NULL,NULL)!=0;
			}
			break;	
		case SDSDT_CLOSERASCONNECTIONS:
			/* check if RAS installed/available */
			bReturn=SearchPath(NULL,_T("RASAPI32"),_T(".DLL"),0,NULL,NULL)!=0;
			break;
		case SDSDT_SETMIRANDAOFFLINE:
		case SDSDT_CLOSEMIRANDA:
			bReturn=TRUE; /* always possible */
			break;
		case SDSDT_REBOOT:
		case SDSDT_SHUTDOWN: 
			/* test privileges */
			if(IsWinVerNT()) {
				bReturn=WinNT_SetPrivilege(SE_SHUTDOWN_NAME,TRUE);
				if(bReturn) WinNT_SetPrivilege(SE_SHUTDOWN_NAME,FALSE);
			}
			else bReturn=TRUE;
			break;
	}
	return bReturn;
}

static DWORD ShutdownNow(BYTE shutdownType)
{
	DWORD dwErrCode=ERROR_SUCCESS;
	switch(shutdownType) {
		case SDSDT_CLOSEMIRANDA:
			if(!Miranda_Terminated()) {
				/* waiting for short until ready (but not too long...) */
				DWORD dwLastTickCount=GetTickCount();
				while(!CallService(MS_SYSTEM_OKTOEXIT,0,0)) {
					/* infinite loop protection (max 5 sec) */
					if(GetTickCount()-dwLastTickCount>=5000) { /* wraparound works */
						OutputDebugStringA("Timeout (5 sec)\n"); /* tell others, all ascii */
						break;
					}
					SleepEx(1000,TRUE);
					if(Miranda_Terminated()) break; /* someone else did it */
					OutputDebugStringA("Not ready to exit. Waiting...\n"); /* tell others, all ascii */
				}
				/* shutdown service must be called from main thread anyway */
				if(!DestroyWindow((HWND)CallService(MS_CLUI_GETHWND,0,0)))
					dwErrCode=GetLastError();
			}
			break;
		case SDSDT_SETMIRANDAOFFLINE:
			/* set global status mode to offline (is remembered by Miranda on exit) */
			CallService(MS_CLIST_SETSTATUSMODE,(WPARAM)ID_STATUS_OFFLINE,(LPARAM)NULL);
			break;
		case SDSDT_STANDBY:
		case SDSDT_HIBERNATE:
			WinNT_SetPrivilege(SE_SHUTDOWN_NAME,TRUE);
			if(!SetSystemPowerState(shutdownType==SDSDT_STANDBY,TRUE))
				dwErrCode=GetLastError();
			WinNT_SetPrivilege(SE_SHUTDOWN_NAME,FALSE);
			break;
		case SDSDT_LOCKWORKSTATION:
			{	BOOL (WINAPI *pfnLockWorkStation)(void);
				*(PROC*)&pfnLockWorkStation=GetProcAddress(GetModuleHandleA("USER32"),"LockWorkStation");
				if(pfnLockWorkStation!=NULL) /* Win2000+ */
					if(!pfnLockWorkStation() && !WinNT_IsWorkStationLocked())
						dwErrCode=GetLastError();
				else if(IsWinVerNT()) { /* WinNT4 */
					HKEY hKey;
					/* start LOGON.SCR screensaver (locks workstation on NT4) */
					if(!SearchPath(NULL,_T("LOGIN.SCR"),NULL,0,NULL,NULL)) {
						if(RegCreateKeyEx(HKEY_CURRENT_USER,_T("Control Panel\\Desktop"),0,NULL,REG_OPTION_VOLATILE,KEY_QUERY_VALUE|KEY_SET_VALUE,NULL,&hKey,NULL)==ERROR_SUCCESS) {
							TCHAR szScreenSaveActive[2],szScreenSaverIsSecure[2],szScrnsaveExe[MAX_PATH];
							DWORD dwSize;
							/* save old settings */
							dwSize=sizeof(szScreenSaveActive); /* in bytes */
							ZeroMemory(&szScreenSaveActive,dwSize);
							RegQueryValueEx(hKey,_T("ScreenSaveActive"),0,NULL,(void*)szScreenSaveActive,&dwSize);
							dwSize=sizeof(szScreenSaverIsSecure); /* in bytes */
							ZeroMemory(&szScreenSaverIsSecure, dwSize);
							RegQueryValueEx(hKey,_T("ScreenSaverIsSecure"),0,NULL,(void*)szScreenSaverIsSecure,&dwSize);
							dwSize=sizeof(szScrnsaveExe); /* in bytes */
							ZeroMemory(&szScrnsaveExe,dwSize);
							RegQueryValueEx(hKey,_T("SCRNSAVE.EXE"),0,NULL,(void*)szScrnsaveExe,&dwSize);
							/* set LOGON.SCR data */
							if(!RegSetValueEx(hKey,_T("ScreenSaveActive"),0,REG_SZ,(void*)_T("1"),2) &&
							   !RegSetValueEx(hKey,_T("ScreenSaverIsSecure"),0,REG_SZ,(void*)"1",2) &&
							   !RegSetValueEx(hKey,_T("SCRNSAVE.EXE"),0,REG_SZ,(void*)_T("LOGIN.SCR"),10))
									SendMessage(GetForegroundWindow(),WM_SYSCOMMAND,SC_SCREENSAVE,0);
							else dwErrCode=GetLastError();
							/* restore old settings */
							RegSetValueEx(hKey,_T("ScreenSaveActive"),0,REG_SZ,(void*)szScreenSaveActive,(lstrlen(szScreenSaveActive)+1)*sizeof(TCHAR));
							RegSetValueEx(hKey,_T("ScreenSaverIsSecure"),0,REG_SZ,(void*)szScreenSaverIsSecure,(lstrlen(szScreenSaverIsSecure)+1)*sizeof(TCHAR));
							RegSetValueEx(hKey,_T("SCRNSAVE.EXE"),0,REG_SZ,(void*)szScrnsaveExe,(lstrlen(szScrnsaveExe)+1)*sizeof(TCHAR));
							RegCloseKey(hKey);
						} else dwErrCode=GetLastError();
					} else dwErrCode=GetLastError();
				} else dwErrCode=GetLastError();
			}
			break;
		case SDSDT_CLOSERASCONNECTIONS:
			ShutdownNow(SDSDT_SETMIRANDAOFFLINE); /* set Miranda offline */
			/* hang up all ras connections */
			{	HMODULE hRasApiDLL=LoadLibrary(_T("RASAPI32")); /* all ascii */
				if(hRasApiDLL!=NULL) {
					DWORD (APIENTRY *pfnRasEnumConnections)(RASCONN*,DWORD*,DWORD*);
					DWORD (APIENTRY *pfnRasHangUp)(HRASCONN);
					DWORD (APIENTRY *pfnRasGetConnectStatus)(HRASCONN,RASCONNSTATUS*);
					#if defined(_UNICODE)
						*(PROC*)&pfnRasEnumConnections=GetProcAddress(hRasApiDLL,"RasEnumConnectionsW");
						*(PROC*)&pfnRasHangUp=GetProcAddress(hRasApiDLL,"RasHangUpW");
						*(PROC*)&pfnRasGetConnectStatus=GetProcAddress(hRasApiDLL,"RasGetConnectStatusW");
					#else
						*(PROC*)&pfnRasEnumConnections=GetProcAddress(hRasApiDLL,"RasEnumConnectionsA");
						*(PROC*)&pfnRasHangUp=GetProcAddress(hRasApiDLL,"RasHangUpA");
						*(PROC*)&pfnRasGetConnectStatus=GetProcAddress(hRasApiDLL,"RasGetConnectStatusA");
					#endif
					if(pfnRasEnumConnections && pfnRasGetConnectStatus && pfnRasHangUp) {
						RASCONN *paConn;
						RASCONN *paConnBuf;
						DWORD dwConnSize,dwConnItems,dwRetries;
						RASCONNSTATUS rcs;
						DWORD dw,dwLastTickCount;

						dwConnSize=sizeof(RASCONN);
						dwConnItems=0;
						paConn=(RASCONN*)mir_alloc(dwConnSize);
						dwErrCode=ERROR_NOT_ENOUGH_MEMORY;
						if(paConn!=NULL)
							for(dwRetries=5;dwRetries!=0;dwRetries--) { /* prevent infinite loop (rare) */
								ZeroMemory(paConn,dwConnSize);
								paConn[0].dwSize=sizeof(RASCONN);
								dwErrCode=pfnRasEnumConnections(paConn,&dwConnSize,&dwConnItems);
								if(dwErrCode!=ERROR_BUFFER_TOO_SMALL) break;
								paConnBuf=(RASCONN*)mir_realloc(paConn,dwConnSize);
								if(paConnBuf!=NULL) {
									mir_free(paConn);
									paConn=NULL;
									dwErrCode=ERROR_NOT_ENOUGH_MEMORY;
									break;
								}
								paConn=paConnBuf;
							}
						if(dwErrCode==ERROR_SUCCESS || dwErrCode==ERROR_BUFFER_TOO_SMALL) {
							for(dw=0;dw<dwConnItems;++dw) {
								if(dwErrCode) {
									if(pfnRasHangUp(paConn[dw].hrasconn))
										paConn[dw].hrasconn=NULL; /* do not wait for on error */
								} else {
									dwErrCode=pfnRasHangUp(paConn[dw].hrasconn);
									if(!dwErrCode) paConn[dw].hrasconn=NULL; /* do not wait for on error */
								}
							}
							/* RAS does not allow to quit directly after HangUp (see docs) */
							dwLastTickCount=GetTickCount();
							ZeroMemory(&rcs,sizeof(RASCONNSTATUS));
							rcs.dwSize=sizeof(RASCONNSTATUS);
							for(dw=0;dw<dwConnItems;++dw)
								if(paConn[dw].hrasconn!=NULL)
									while(pfnRasGetConnectStatus(paConn[dw].hrasconn, &rcs)!=ERROR_INVALID_HANDLE) {
										Sleep(0); /* give rest of time silce to other threads with equal priority */
										/* infinite loop protection (3000ms defined in docs) */
										dwRetries=GetTickCount();
										if(dwRetries-dwLastTickCount>3000) break; /* wraparound works */
									}
						}
						mir_free(paConn); /* does NULL check */
					} else dwErrCode=GetLastError();
					FreeLibrary(hRasApiDLL);
				} else dwErrCode=GetLastError();
			}
			/* set Miranda to offline again, to remain offline with reconnection plugins */
			ShutdownNow(SDSDT_SETMIRANDAOFFLINE);
			break;
		case SDSDT_REBOOT:
		case SDSDT_SHUTDOWN:
			if(GetSystemMetrics(SM_SHUTTINGDOWN)) { /* Win2000+, 0 on error */
				dwErrCode=ERROR_SHUTDOWN_IN_PROGRESS;
				break;
			}
			/* WinNT4/2000/XP */
			{	BOOL (WINAPI *pfnInitiateSystemShutdownEx)(const TCHAR*,const TCHAR*,DWORD,BOOL,BOOL,DWORD);
				BOOL (WINAPI *pfnInitiateSystemShutdown)(const TCHAR*,const TCHAR*,DWORD,BOOL,BOOL);
				#if defined(_UNICODE)
					*(PROC*)&pfnInitiateSystemShutdownEx=GetProcAddress(GetModuleHandleA("ADVAPI32"),"InitiateSystemShutdownExW");
					*(PROC*)&pfnInitiateSystemShutdown=GetProcAddress(GetModuleHandleA("ADVAPI32"),"InitiateSystemShutdownW");
				#else
					*(PROC*)&pfnInitiateSystemShutdownEx=GetProcAddress(GetModuleHandleA("ADVAPI32"),"InitiateSystemShutdownExA");
					*(PROC*)&pfnInitiateSystemShutdown=GetProcAddress(GetModuleHandleA("ADVAPI32"),"InitiateSystemShutdownA");
				#endif
				if(pfnInitiateSystemShutdownEx!=NULL || pfnInitiateSystemShutdown!=NULL) {
					WinNT_SetPrivilege(SE_SHUTDOWN_NAME,TRUE);
			
					/* does not send out WM_ENDSESSION messages, so we do it manually to
					 * give the applications the chance to save their data */
					WinNT_SetPrivilege(SE_TCB_NAME,TRUE); /* for BSM_ALLDESKTOPS */
					BroadcastEndSession(BSM_APPLICATIONS|BSM_ALLDESKTOPS,ENDSESSION_CLOSEAPP); /* app should close itself */
					WinNT_SetPrivilege(SE_TCB_NAME,FALSE);

					if(pfnInitiateSystemShutdownEx!=NULL) {
						if(!pfnInitiateSystemShutdownEx(NULL,TranslateT("AutoShutdown"),0,TRUE,shutdownType==SDSDT_REBOOT,SHTDN_REASON_MAJOR_OTHER|SHTDN_REASON_MINOR_OTHER|SHTDN_REASON_FLAG_PLANNED))
							dwErrCode=GetLastError();
					} else if(!pfnInitiateSystemShutdown(NULL,TranslateT("AutoShutdown"),0,TRUE,shutdownType==SDSDT_REBOOT))
						dwErrCode=GetLastError();

					/* cleanly close Miranda */
					if(!dwErrCode) ShutdownNow(SDSDT_CLOSEMIRANDA);
					break;
				}
			}
			/* fall through for Win9x */
		case SDSDT_LOGOFF:
			{	UINT flags;
				switch(shutdownType) {
					case SDSDT_LOGOFF: flags=EWX_LOGOFF; break;
					case SDSDT_REBOOT: flags=EWX_REBOOT; break;
					default:           flags=EWX_SHUTDOWN|EWX_POWEROFF;
				}
				if(shutdownType==SDSDT_LOGOFF && IsWinVer2000Plus() && !WinNT_IsWorkStationLocked())
					flags|=EWX_FORCEIFHUNG; /* only considered for WM_ENDSESSION messages */
				else flags|=EWX_FORCE; /* must be used when workstation locked */

				if(flags&EWX_FORCE) {
					/* EWX_FORCE does not send out WM_ENDSESSION messages, so we do it
					 * manually to give the applications the chance to save their data */
					BroadcastEndSession(BSM_APPLICATIONS,(shutdownType==SDSDT_LOGOFF)?ENDSESSION_LOGOFF:0);
				
					/* Windows Me/98/95 (msdn): Because of the design of the shell,
					 * calling ExitWindowsEx with EWX_FORCE fails to completely log off
					 * the user (the system terminates the applications and displays the
					 * Enter Windows Password dialog box, however, the user's desktop remains.)
					 * To log off the user forcibly, terminate the Explorer process before calling
					 * ExitWindowsEx with EWX_LOGOFF and EWX_FORCE. */
					#if !defined(_UNICODE)
						if(shutdownType==SDSDT_LOGOFF && !IsWinVerNT()) Win9x_TerminateExplorer();
					#endif
				}
				if(!ExitWindowsEx(flags,SHTDN_REASON_MAJOR_OTHER|SHTDN_REASON_MINOR_OTHER|SHTDN_REASON_FLAG_PLANNED))
					dwErrCode=GetLastError();
				/* cleanly close Miranda */
				if(!dwErrCode) ShutdownNow(SDSDT_CLOSEMIRANDA);
			}
			break;
	}
	return dwErrCode;
}

/************************* Shutdown Dialog ****************************/

#define M_START_SHUTDOWN    (WM_APP+111)
#define M_UPDATE_COUNTDOWN  (WM_APP+112)
static BOOL CALLBACK ShutdownDlgProc(HWND hwndDlg,UINT msg,WPARAM wParam,LPARAM lParam)
{
	BYTE shutdownType=(BYTE)GetWindowLong(hwndDlg,DWL_USER);
	WORD countdown=(WORD)GetWindowLong(GetDlgItem(hwndDlg,IDC_TEXT_HEADER),GWL_USERDATA);
	static BOOL (WINAPI *pfnLockSetForegroundWindow)(UINT);

	switch(msg) {
		case WM_INITDIALOG:
			hwndShutdownDlg=hwndDlg;
			SetWindowLong(hwndDlg,DWL_USER,(LONG)lParam);
			TranslateDialogDefault(hwndDlg);
			
			if(lParam==SDSDT_SHUTDOWN || lParam==SDSDT_REBOOT || lParam==SDSDT_LOGOFF)
				ShowWindow(GetDlgItem(hwndDlg,IDC_TEXT_UNSAVEDWARNING),SW_SHOW);
			SendDlgItemMessage(hwndDlg,IDC_ICON_HEADER,STM_SETIMAGE,IMAGE_ICON,(LPARAM)IcoLib_GetIcon("AutoShutdown_Header"));
			{	HFONT hBoldFont;
				LOGFONT lf;
				if(GetObject((HFONT)SendDlgItemMessage(hwndDlg,IDC_TEXT_HEADER,WM_GETFONT,0,0),sizeof(lf),&lf)) {
					lf.lfWeight=FW_BOLD;
					hBoldFont=CreateFontIndirect(&lf);
				}
				else hBoldFont=NULL;
				SendDlgItemMessage(hwndDlg,IDC_TEXT_HEADER,WM_SETFONT,(WPARAM)hBoldFont,FALSE);
				SetWindowLong(GetDlgItem(hwndDlg,IDC_TEXT_HEADER),GWL_USERDATA,(LONG)hBoldFont);
			}
			{	WORD countdown=DBGetContactSettingWord(NULL,"AutoShutdown","ConfirmDlgCountdown",SETTING_CONFIRMDLGCOUNTDOWN_DEFAULT);
				if(countdown<3) countdown=SETTING_CONFIRMDLGCOUNTDOWN_DEFAULT;
				SetWindowLong(GetDlgItem(hwndDlg,IDC_TEXT_HEADER),GWL_USERDATA,countdown);
				SendMessage(hwndDlg,M_UPDATE_COUNTDOWN,0,countdown);
			}
			SkinPlaySound("AutoShutdown_Countdown");
			if(!SetTimer(hwndDlg,1,1000,NULL)) PostMessage(hwndDlg,M_START_SHUTDOWN,0,0);
			Utils_RestoreWindowPositionNoSize(hwndDlg,NULL,"AutoShutdown","ConfirmDlg_");
			
			/* disallow foreground window changes (WinMe/2000+) */
			SetForegroundWindow(hwndDlg);
			*(PROC*)&pfnLockSetForegroundWindow=GetProcAddress(GetModuleHandleA("USER32"),"LockSetForegroundWindow");
			if(pfnLockSetForegroundWindow!=NULL)
				pfnLockSetForegroundWindow(LSFW_LOCK);

			SendMessage(hwndDlg,WM_NEXTDLGCTL,(WPARAM)GetDlgItem(hwndDlg,IDCANCEL),TRUE);
			return FALSE; /* focus set on cancel */
		case WM_DESTROY:
		{	HFONT hFont;
			HICON hIcon;
			hwndShutdownDlg=NULL;
			ShowWindow(hwndDlg,SW_HIDE);
			/* reallow foreground window changes (WinMe/2000+) */
			if(pfnLockSetForegroundWindow) pfnLockSetForegroundWindow(LSFW_UNLOCK);
			Utils_SaveWindowPosition(hwndDlg,NULL,"AutoShutdown","ConfirmDlg_");
			hIcon=(HICON)SendDlgItemMessage(hwndDlg,IDC_ICON_HEADER,STM_SETIMAGE,IMAGE_ICON,(LPARAM)NULL);
			IcoLib_ReleaseIcon(hIcon); /* does NULL check */
			hFont=(HFONT)SendDlgItemMessage(hwndDlg,IDC_TEXT_HEADER,WM_GETFONT,0,0);
			SendDlgItemMessage(hwndDlg,IDC_TEXT_HEADER,WM_SETFONT,(WPARAM)NULL,FALSE); /* no return value */
			if(hFont!=NULL) DeleteObject(hFont);
			return TRUE;
		}
		case M_START_SHUTDOWN:
			if(IsWindowEnabled(GetDlgItem(hwndDlg,IDC_BUTTON_SHUTDOWNNOW))) {
				DWORD dwErrCode;
				EnableWindow(GetDlgItem(hwndDlg,IDC_BUTTON_SHUTDOWNNOW),FALSE);
				ShowWindow(hwndDlg,SW_HIDE);  /* get rid of the dialog immediately */
				dwErrCode=ShutdownNow(shutdownType);
				if(dwErrCode!=ERROR_SUCCESS) {
					char *pszErr;
					pszErr=GetWinErrorDescription(dwErrCode);
					ShowInfoMessage(NIIF_ERROR,Translate("Automatic Shutdown Error"),Translate("The shutdown process failed!\nReason: %s"),(pszErr!=NULL)?pszErr:Translate("Unknown"));
					if(pszErr!=NULL) LocalFree(pszErr);
				}
				DestroyWindow(hwndDlg);
			}
			return TRUE;
		case WM_TIMER:
			if(countdown) {
				--countdown;
				SetWindowLong(GetDlgItem(hwndDlg,IDC_TEXT_HEADER),GWL_USERDATA,countdown);
				if(countdown==27 || countdown==24 || countdown==21 || countdown==19 ||
				   countdown==17 || countdown==15 || countdown==13 || countdown==11 ||
				   countdown<=10)
					SkinPlaySound("AutoShutdown_Countdown");
			}
			else KillTimer(hwndDlg,wParam);  /* countdown finished */
			PostMessage(hwndDlg,M_UPDATE_COUNTDOWN,0,countdown);
			return TRUE;
		case M_UPDATE_COUNTDOWN:  /* lParam=(WORD)countdown */
		{	TCHAR szText[256];
			TCHAR *desc[]={_T("Miranda IM is going to be automatically closed in %u second(s)."),
			               _T("All Miranda IM protocols are going to be set to offline in %u second(s)."),
			               _T("You will be logged off automatically in %u second(s)."),
			               _T("The computer will automatically be restarted in %u second(s)."),
			               _T("The computer will automatically be set to standby mode in %u second(s)."),
			               _T("The computer will automatically be set to hibernate mode in %u second(s)."),
			               _T("The workstation will automatically get locked in %u second(s)."),
			               _T("All dialup connections will be closed in %u second(s)."),
			               _T("The computer will automatically be shut down in %u second(s).")};
			mir_sntprintf(szText,SIZEOF(szText),TranslateTS(desc[shutdownType-1]),lParam);
			SetDlgItemText(hwndDlg,IDC_TEXT_HEADER,szText);
			/* countdown finished */
			if(!lParam) PostMessage(hwndDlg,M_START_SHUTDOWN,0,0);
			return TRUE;
		}
		case WM_COMMAND:
			switch(LOWORD(wParam)) {
				case IDC_BUTTON_SHUTDOWNNOW:
					KillTimer(hwndDlg,1);
					SetWindowLong(GetDlgItem(hwndDlg,IDC_TEXT_HEADER),GWL_USERDATA,0);
					SendMessage(hwndDlg,M_UPDATE_COUNTDOWN,0,(LONG)0);
					PostMessage(hwndDlg,M_START_SHUTDOWN,0,0);
					return TRUE;
				case IDCANCEL: /* WM_CLOSE */
					if(countdown) {
						KillTimer(hwndDlg,1);
						DestroyWindow(hwndDlg);
					}
					return TRUE;
			}
			break;
	}
	return FALSE;
}

/************************* Services ***********************************/

int ServiceShutdown(WPARAM wParam,LPARAM lParam)
{
	/* passing 0 as wParam is only to be used internally, undocumented */
	if(!wParam) wParam=DBGetContactSettingByte(NULL,"AutoShutdown","ShutdownType",SETTING_SHUTDOWNTYPE_DEFAULT);
	if(!IsShutdownTypeEnabled((BYTE)wParam)) return 1; /* does shutdownType range check */
	if((BOOL)lParam && hwndShutdownDlg!=NULL) return 2;

	/* ask others if allowed */
	if(NotifyEventHooks(hEventOkToShutdown,wParam,lParam)) {
		OutputDebugStringA("automatic shutdown denied by event hook\n"); /* all ascii */
		return 3;
	}
	/* tell others */
	NotifyEventHooks(hEventShutdown,wParam,lParam);
	/* show dialog */
	if(lParam && DBGetContactSettingByte(NULL,"AutoShutdown","ShowConfirmDlg",SETTING_SHOWCONFIRMDLG_DEFAULT))
		if(CreateDialogParam(hInst,MAKEINTRESOURCE(IDD_SHUTDOWNNOW),NULL,ShutdownDlgProc,(LPARAM)(BYTE)wParam)!=NULL)
			return 0;
	/* show error */
	{	DWORD dwErrCode;
		dwErrCode=ShutdownNow((BYTE)wParam);
		if(dwErrCode!=ERROR_SUCCESS) {
			char *pszErr;
			pszErr=GetWinErrorDescription(dwErrCode);
			ShowInfoMessage(NIIF_ERROR,Translate("Automatic Shutdown Error"),Translate("Inititiating the shutdown process failed!\nReason: %s"),(pszErr!=NULL)?pszErr:Translate("Unknown"));
			if(pszErr!=NULL) LocalFree(pszErr);
			return 4;
		}
	}
	return 0;
}

int ServiceIsTypeEnabled(WPARAM wParam,LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	return IsShutdownTypeEnabled((BYTE)wParam); /* does shutdownType range check */
}

int ServiceGetTypeDescription(WPARAM wParam,LPARAM lParam)
{
	TCHAR *pszDesc;
	const TCHAR *apszShort[]={_T("Close Miranda IM"),_T("Set Miranda IM offline"),_T("Log off user"),
	                          _T("Restart computer"),_T("Shutdown computer"),_T("Standby mode"),_T("Hibernate mode"),
	                          _T("Lock workstation"),_T("Hang up dialup connections"),_T("Close Miranda IM"),
	                          _T("Set Miranda IM offline"),_T("Log off user"),_T("Restart computer"),_T("Shutdown computer"),
	                          _T("Standby mode"),_T("Hibernate mode"),_T("Lock workstation"),_T("Hang up dialup connections")};
	const TCHAR *apszLong[]={_T("Sets all Miranda IM protocols to offline and closes Miranda IM."),
	                         _T("Sets all Miranda IM protocols to offline."),
	                         _T("Logs the current Windows user off so that another user can log in."),
	                         _T("Shuts down Windows and then restarts Windows."),
	                         _T("Closes all running programs and shuts down Windows to a point at which it is safe to turn off the power."),
	                         _T("Saves the current Windows session in memory and sets the system to suspend mode."),
	                         _T("Saves the current Windows session on harddisc, so that the power can be turned off."),
	                         _T("Locks the computer. To unlock the computer, you must log in."),
			                 _T("Sets all protocols to offline and closes all RAS connections.")};
	/* shutdownType range check */
	if(!wParam || (BYTE)wParam>SDSDT_MAX) return (int)NULL;
	/* select description */
	pszDesc=(TCHAR*)((lParam&GSTDF_LONGDESC)?apszLong:apszShort)[wParam-1];
	if(!(lParam&GSTDF_UNTRANSLATED)) pszDesc=TranslateTS(pszDesc);
	/* convert as needed */
	#if defined(_UNICODE)
		if(!(lParam&GSTDF_UNICODE)) {
			static char szConvBuf[128];
			char *buf=u2a(pszDesc);
			if(buf==NULL) return (int)NULL;
			lstrcpynA(szConvBuf,buf,sizeof(szConvBuf));
			mir_free(buf);
			return (int)szConvBuf;
		}
	#else
		if(lParam&GSTDF_UNICODE) {
			static WCHAR szConvBuf[128];
			WCHAR *buf=a2u(pszDesc);
			if(buf==NULL) return (int)NULL;
			lstrcpynW(szConvBuf,buf,SIZEOF(szConvBuf));
			mir_free(buf);
			return (int)szConvBuf;
		}
	#endif
	return (int)pszDesc;
}

/************************* Misc ***************************************/

void InitShutdownSvc(void)
{
	/* Shutdown Dialog */
	hwndShutdownDlg=NULL;
	SkinAddNewSoundBundled("AutoShutdown_Countdown",Translate("Alerts"),Translate("Automatic Shutdown Countdown"),"Sounds\\","countdown.wav");
	/* Services */
	hEventOkToShutdown=CreateHookableEvent(ME_AUTOSHUTDOWN_OKTOSHUTDOWN);
	hEventShutdown=CreateHookableEvent(ME_AUTOSHUTDOWN_SHUTDOWN);
	hServiceShutdown=CreateServiceFunction(MS_AUTOSHUTDOWN_SHUTDOWN,ServiceShutdown);
	hServiceIsTypeEnabled=CreateServiceFunction(MS_AUTOSHUTDOWN_ISTYPEENABLED,ServiceIsTypeEnabled);
	hServiceGetTypeDesc=CreateServiceFunction(MS_AUTOSHUTDOWN_GETTYPEDESCRIPTION,ServiceGetTypeDescription);
}

void UninitShutdownSvc(void)
{
	/* Shutdown Dialog */
	if(hwndShutdownDlg!=NULL) DestroyWindow(hwndShutdownDlg);
	/* Services */
	DestroyServiceFunction(hServiceShutdown);
	DestroyServiceFunction(hServiceIsTypeEnabled);
	DestroyServiceFunction(hServiceGetTypeDesc);
	DestroyHookableEvent(hEventOkToShutdown);
	DestroyHookableEvent(hEventShutdown);
}
