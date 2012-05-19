/*
	New Away System - plugin for Miranda IM
	Copyright (c) 2005-2007 Chervov Dmitry
	Copyright (c) 2004-2005 Iksaif Entertainment
	Copyright (c) 2002-2003 Goblineye Entertainment

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#pragma once

#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0400

#define MIRANDA_VER 0x0600

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <commdlg.h>
#include <time.h>
#include <shellapi.h>
#include <crtdbg.h>
#include <tchar.h>
#include <stdarg.h>
#include "AggressiveOptimize.h"
#include "resource.h"
#include "newpluginapi.h"
#include "m_clist.h"
#include "m_system.h"
#include "m_database.h"
#include "m_clui.h"
#include "m_langpack.h"
#include "m_protosvc.h"
#include "m_options.h"
#include "..\..\protocols\IcqOscarJ\icq_constants.h"
#include "m_skin.h"
#include "m_plugins.h"
#include "m_awaymsg.h"
#include "m_utils.h"
#include "m_history.h"
#include "m_message.h"
#include "m_userinfo.h"
#include "m_icq.h"
#define THEMEAPI // we don't need no uxtheme defines :-/ they break everything when trying to include tmschema.h later
#include "win2k.h"
#undef THEMEAPI

#include "m_variables.h"
//#include "m_toptoolbar.h"
#include "m_popup.h"
//#include "m_popupw.h"
#include "m_metacontacts.h"
#include "..\CommonLibs\m_LogService.h"
#include "..\CommonLibs\CString.h"
#include "..\CommonLibs\Options.h"


#pragma comment(lib,"comctl32.lib")

#define VAR_AWAYSINCE_TIME "nas_awaysince_time"
#define VAR_AWAYSINCE_DATE "nas_awaysince_date"
#define VAR_STATDESC "nas_statdesc"
#define VAR_MYNICK "nas_mynick"
#define VAR_REQUESTCOUNT "nas_requestcount"
#define VAR_MESSAGENUM "nas_messagecount"
#define VAR_TIMEPASSED "nas_timepassed"
#define VAR_PREDEFINEDMESSAGE "nas_predefinedmessage"
#define VAR_PROTOCOL "nas_protocol"

#define SENDSMSG_EVENT_MSG 0x1
#define SENDSMSG_EVENT_URL 0x2
#define SENDSMSG_EVENT_FILE 0x4

#define AWAY_MSGDATA_MAX 8000

// Flags for status database settings
#define SF_OFF 0x1
#define SF_ONL 0x2
#define SF_AWAY 0x4
#define SF_NA 0x8
#define SF_OCC 0x10
#define SF_DND 0x20
#define SF_FFC 0x40
#define SF_INV 0x80
#define SF_OTP 0x100
#define SF_OTL 0x200
#define SF_OTHER 0x80000000
/*
// Actions on popup click
#define PCA_OPENMESSAGEWND	0	// open message window
#define PCA_CLOSEPOPUP			1	// close popup
#define PCA_OPENDETAILS			2	// open contact details window
#define PCA_OPENMENU				3	// open contact menu
#define PCA_OPENHISTORY			4	// open contact history
#define PCA_OPENLOG					5	// open log file
#define PCA_DONOTHING				6 // do nothing

// Notification options defaults
#define POPUP_DEF_POPUP_FORMAT TranslateT("?cinfo(%subject%,display) (?cinfo(%subject%,id)) is reading your %nas_statdesc% message:\r\n%extratext%")
#define POPUP_DEF_USEPOPUPS 0
#define POPUP_DEF_LCLICKACTION PCA_OPENMESSAGEWND
#define POPUP_DEF_RCLICKACTION PCA_CLOSEPOPUP
#define POPUP_DEF_POPUP_BGCOLOUR 0xFFB5BC
#define POPUP_DEF_POPUP_TEXTCOLOUR 0
#define POPUP_DEF_USEDEFBGCOLOUR 0
#define POPUP_DEF_USEDEFTEXTCOLOUR 0
#define POPUP_DEF_POPUPNOTIFYFLAGS (SF_ONL | SF_AWAY | SF_NA | SF_OCC | SF_DND | SF_FFC | SF_INV | SF_OTP | SF_OTL)
#define POPUP_DEF_POPUPDELAY 0

#define POPUP_MAXPOPUPDELAY 9999
*/
#define MOREOPTDLG_DEF_DONTPOPDLG (SF_ONL | SF_INV)
#define MOREOPTDLG_DEF_USEBYDEFAULT 0

// Event flags (used for "reply on event" options)
#define EF_MSG 1
#define EF_URL 2
#define EF_FILE 4

#define AUTOREPLY_DEF_REPLY 0
#define AUTOREPLY_DEF_REPLYONEVENT (EF_MSG | EF_URL | EF_FILE)
#define AUTOREPLY_DEF_PREFIX TranslateT("Miranda IM autoreply >\r\n%extratext%")
#define AUTOREPLY_DEF_DISABLEREPLY (SF_ONL | SF_INV)

#define AUTOREPLY_IDLE_WINDOWS 0
#define AUTOREPLY_IDLE_MIRANDA 1
#define AUTOREPLY_DEF_IDLEREPLYVALUE AUTOREPLY_IDLE_WINDOWS

#define AUTOREPLY_MAXPREFIXLEN 8000

#define VAL_USEDEFAULT 2 // undefined value for ignore/autoreply/notification settings in the db; must be 2 for proper ContactSettings support

// Set Away Message dialog flags
#define DF_SAM_SHOWMSGTREE 1
#define DF_SAM_SHOWCONTACTTREE 2
#define DF_SAM_DEFDLGFLAGS DF_SAM_SHOWMSGTREE

// WriteAwayMsgInDB option flags:
#define WRITE_LMSG 1
#define WRITE_RMSG 2
#define WRITE_INTERPRET 4
#define WRITE_CMSG 8

#define TOGGLE_SOE_COMMAND LPGENT("Toggle autoreply on/off")
#define DISABLE_SOE_COMMAND LPGENT("Toggle autoreply off")
#define ENABLE_SOE_COMMAND LPGENT("Toggle autoreply on")

#define STR_XSTATUSDESC TranslateT("extended status")

#define MOD_NAME "NewAwaySys"
#define LOG_ID MOD_NAME // LogService log ID
#define LOG_PREFIX MOD_NAME ": " // netlib.log prefix for all NAS' messages

#define DB_SETTINGSVER "SettingsVer"

#ifndef lengthof
#define lengthof(s) (sizeof(s) / sizeof(*s))
#endif

#define MS_NETLIB_LOG "Netlib/Log"

#define UM_ICONSCHANGED (WM_USER + 121)

// IDD_READAWAYMSG user-defined message constants
#define UM_RAM_AWAYMSGACK (WM_USER + 10)

// IDD_SETAWAYMSG user-defined message constants
#define UM_SAM_SPLITTERMOVED (WM_USER + 1)
#define UM_SAM_SAVEDLGSETTINGS (WM_USER + 2)
#define UM_SAM_APPLYANDCLOSE (WM_USER + 3)
#define UM_SAM_KILLTIMER (WM_USER + 4)
#define UM_SAM_REPLYSETTINGCHANGED (WM_USER + 5)
#define UM_SAM_PROTOSTATUSCHANGED (WM_USER + 6) // wParam = (char*)szProto

#define UM_CLICK (WM_USER + 100)

#define SAM_DB_DLGPOSX "SAMDlgPosX"
#define SAM_DB_DLGPOSY "SAMDlgPosY"
#define SAM_DB_DLGSIZEX "SAMDlgSizeX"
#define SAM_DB_DLGSIZEY "SAMDlgSizeY"
#define SAM_DB_MSGSPLITTERPOS "SAMMsgSplitterPos"
#define SAM_DB_CONTACTSPLITTERPOS "SAMContactSplitterPos"

#define DB_MESSAGECOUNT "MessageCount"
#define DB_REQUESTCOUNT "RequestCount"
#define DB_SENDCOUNT "SendCount"
#define MESSAGES_DB_MSGTREEDEF "MsgTreeDef"

#define MSGTREE_RECENT_OTHERGROUP _T("Other")

// GetMsgFormat flags
#define GMF_PERSONAL 1 // is also used to get global status message, when hContact = NULL (szProto = NULL)
#define GMF_PROTOORGLOBAL 2
#define GMF_LASTORDEFAULT 4 // this flag doesn't require hContact or szProto
#define GMF_TEMPORARY 8 // doesn't require status
#define GMF_ANYCURRENT (GMF_TEMPORARY | GMF_PERSONAL | GMF_PROTOORGLOBAL)

// SetMsgFormat flags
#define SMF_PERSONAL 1 // is also used to set global status message, when hContact = NULL (szProto = NULL)
#define SMF_LAST 2
#define SMF_TEMPORARY 4 // doesn't require status

// VAR_PARSE_DATA flags
#define VPF_XSTATUS 1 // use "extended status" instead of the usual status description in %nas_statdesc%, and XStatus message in %nas_message%

// options dialog
#define OPT_TITLE LPGENT("Away System")
#define OPT_MAINGROUP LPGENT("Status")
#define OPT_POPUPGROUP LPGENT("PopUps")

#define MRM_MAX_GENERATED_TITLE_LEN 35 // maximum length of automatically generated title for recent messages

int ICQStatusToGeneralStatus(int bICQStat); // TODO: get rid of these protocol-specific functions, if possible

#define MS_AWAYSYS_SETCONTACTSTATMSG "AwaySys/SetContactStatMsg"

#define MS_AWAYSYS_AUTOREPLY_TOGGLE "AwaySys/AutoreplyToggle"
#define MS_AWAYSYS_AUTOREPLY_ON "AwaySys/AutoreplyOn"
#define MS_AWAYSYS_AUTOREPLY_OFF "AwaySys/AutoreplyOff"
#define MS_AWAYSYS_AUTOREPLY_USEDEFAULT "AwaySys/AutoreplyUseDefault"

#define MS_AWAYSYS_VARIABLESHANDLER "AwaySys/VariablesHandler"
#define MS_AWAYSYS_FREEVARMEM "AwaySys/FreeVarMem"
// these are obsolete AwaySysMod services, though they're still here for compatibility with old plugins
#define MS_AWAYSYS_SETSTATUSMODE "AwaySys/SetStatusMode" // change the status mode. wParam is new mode, lParam is new status message (AwaySys will interpret variables out of it), may be NULL.
#define MS_AWAYSYS_IGNORENEXT "AwaySys/IgnoreNextStatusChange" //ignore nest status change

typedef struct SetAwayMsgData_type
{
	CString szProtocol;
	HANDLE hInitContact; // initial contact (filled by caller)
	TCString Message; // initial message, NULL means default
	bool IsModeless; // means the dialog was created with the CreateDialogParam function, not DialogBoxParam
	int ISW_Flags; // InvokeStatusWindow service flags
} SetAwayMsgData;

typedef struct READAWAYMSGDATA_TYPE
{
	HANDLE hContact; // contact
	HANDLE hSeq; // sequence for stat msg request
	HANDLE hAwayMsgEvent; // hooked
} READAWAYMSGDATA;

typedef struct
{
	char *szProto;
	TCString Message;
	DWORD UIN;
	int Flags; // a combination of VPF_ flags
} VAR_PARSE_DATA;

typedef struct
{
	HANDLE hContact;
	int iStatusMode;
	TCString Proto;
} DYNAMIC_NOTIFY_DATA;

typedef struct
{
	BYTE PopupLClickAction, PopupRClickAction;
	HANDLE hContact;
	HICON hStatusIcon; // needed here to destroy its handle on UM_FREEPLUGINDATA
} PLUGIN_DATA;

typedef struct
{
	int cbSize;
	char *szProto;
	HANDLE hContact;
	char *szMsg;
	WORD status;
} NAS_ISWINFOv1;

#define MTYPE_AUTOONLINE 0xE7 // required to support ICQ Plus online status messages
/*
// additional m_popup.h declarations
#ifdef _UNICODE
	typedef struct
	{
		HANDLE lchContact;
		HICON lchIcon;
		WCHAR lpzContactName[MAX_CONTACTNAME];
		WCHAR lpzText[MAX_SECONDLINE];
		COLORREF colorBack;                   
		COLORREF colorText;
		WNDPROC PluginWindowProc;
		void * PluginData;
		int iSeconds;
		char cZero[16];
	} POPUPDATAT;

	#define MS_POPUP_ADDPOPUPT MS_POPUP_ADDPOPUPW
#else
	#define POPUPDATAT POPUPDATAEX
	#define MS_POPUP_ADDPOPUPT MS_POPUP_ADDPOPUPEX
#endif
*/
// Beware of conflicts between two different windows trying to use the same page at a time!
// Other windows than the owner of the Page must copy the page to their own memory,
// or use GetDBValueCopy to retrieve values
extern COptPage g_MessagesOptPage;
extern COptPage g_AutoreplyOptPage;
//extern COptPage g_PopupOptPage;
extern COptPage g_MoreOptPage;
extern COptPage g_SetAwayMsgPage;

extern HINSTANCE g_hInstance;
extern HANDLE hMainThread;
extern int g_Messages_RecentRootID, g_Messages_PredefinedRootID;
extern VAR_PARSE_DATA VarParseData;
extern bool g_fNoProcessing;
extern int g_bIsIdle;
extern int (*g_OldCallService)(const char *, WPARAM, LPARAM);


// AwaySys.cpp
TCString GetDynamicStatMsg(HANDLE hContact, char *szProto = NULL, DWORD UIN = 0, int iStatus = 0);
int IsAnICQProto(char *szProto);

// Client.cpp
void InitUpdateMsgs();
void ChangeProtoMessages(char* szProto, int iMode, TCString &Msg);
int GetRecentGroupID(int iMode);
TCString VariablesEscape(TCString Str);

// SetAwayMsg.cpp
int CALLBACK SetAwayMsgDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

// ReadAwayMsg.cpp
extern HANDLE g_hReadWndList;
int GetContactStatMsg(WPARAM wParam, LPARAM lParam);

// AwayOpt.cpp
int OptsDlgInit(WPARAM wParam, LPARAM lParam); // called on opening of the options dialog
void InitOptions(); // called once when plugin is loaded

//int ShowPopupNotification(COptPage &PopupNotifyData, HANDLE hContact, int iStatusMode);
void ShowLog(TCString &LogFilePath);
void ShowMsg(TCHAR *szFirstLine, TCHAR *szSecondLine = _T(""), bool IsErrorMsg = false, int Timeout = 0);

#define AWAYSYS_STATUSMSGREQUEST_SOUND "AwaySysStatusMsgRequest"
#define ME_AWAYSYS_WORKAROUND "AwaySys/_CallService"
int _Workaround_CallService(const char *name, WPARAM wParam, LPARAM lParam);

// MsgEventAdded.cpp
int MsgEventAdded(WPARAM wParam, LPARAM lParam);

// buttons
//void UpdateSOEButtons(HANDLE hContact = NULL);
int ToggleSendOnEvent(WPARAM wParam, LPARAM lParam);
//int Create_TopToolbar(WPARAM wParam, LPARAM lParam);


static __inline int LogMessage(const char *Format, ...)
{
	va_list va;
	char szText[8096];
	strcpy(szText, LOG_PREFIX);
	va_start(va, Format);
	mir_vsnprintf(szText + (lengthof(LOG_PREFIX) - 1), sizeof(szText) - (lengthof(LOG_PREFIX) - 1), Format, va);
	va_end(va);
	return CallService(MS_NETLIB_LOG, NULL, (LPARAM)szText);
}

__inline int CallAllowedPS_SETAWAYMSG(const char *szProto, int iMode, const char *szMsg)
{ // we must use this function everywhere we want to call PS_SETAWAYMSG, otherwise NAS won't allow to change the message!
	LogMessage("PS_SETAWAYMSG called by NAS. szProto=%s, Status=%d, Msg:\n%s", szProto, iMode, szMsg ? szMsg : "NULL");
	char str[MAXMODULELABELLENGTH];
	strcpy(str, szProto);
	strcat(str, PS_SETAWAYMSG);
	return g_OldCallService(str, (WPARAM)iMode, (LPARAM)szMsg);
}

static __inline void my_variables_skin_helpbutton(HWND hwndDlg, UINT uIDButton)
{
	HICON hIcon = ServiceExists(MS_VARS_GETSKINITEM) ? (HICON)CallService(MS_VARS_GETSKINITEM, 0, (LPARAM)VSI_HELPICON) : NULL;
	if (hIcon)
	{
		SendDlgItemMessage(hwndDlg, uIDButton, BM_SETIMAGE, IMAGE_ICON, (LPARAM)hIcon);
	}
}

static __inline int my_variables_showhelp(HWND hwndDlg, UINT uIDEdit, int flags = 0, char *szSubjectDesc = NULL, char *szExtraDesc = NULL)
{
	if (ServiceExists(MS_VARS_SHOWHELPEX))
	{
		VARHELPINFO vhi = {0};
		vhi.cbSize = sizeof(VARHELPINFO);
		vhi.flags = flags ? flags : (VHF_FULLDLG | VHF_SETLASTSUBJECT);
		vhi.hwndCtrl = GetDlgItem(hwndDlg, uIDEdit);
		vhi.szSubjectDesc = szSubjectDesc;
		vhi.szExtraTextDesc = szExtraDesc;
		return CallService(MS_VARS_SHOWHELPEX, (WPARAM)hwndDlg, (LPARAM)&vhi);
	} else
	{
		ShowMsg(TranslateT("New Away System"), TranslateT("Variables plugin is not installed"), true);
		return -1;
	}
}
