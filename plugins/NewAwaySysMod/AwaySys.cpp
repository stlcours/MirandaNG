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

/*
  Thanx to Faith Healer for icons and help, Unregistered for his plugins (variables, AAA :p),
   mistag for GamerStatus,  BigMuscle for his code to use AAA
  Thanx to Tornado, orignal developer of AwaySys.
  Please note that some code from the Miranda's original away module (SRAway) is used around
  AwaySys. I tried to mention it wherever possible, but I might have forgotten a few. Kudos to Miranda's authors.
  The Read-Away-Msg part was practically copied from Miranda, not proud of it, but since I really can't see how can I make it better, there
  was no point in rewriting it all.
*/

#include "Common.h"
#include "m_genmenu.h"
#include "m_idle.h"
#include "m_statusplugins.h"
#include "m_updater.h"
#include "m_NewAwaySys.h"
#include "m_ContactSettings.h"
#include "MsgTree.h"
#include "ContactList.h"
#include "Properties.h"
#include "Path.h"
#include "Services.h"
#include "VersionNo.h"

//NightFox
#include <m_modernopt.h>
#include <process.h> // needed for MSVC 7 msvcr7*.dll patch

HINSTANCE g_hInstance;
PLUGINLINK *pluginLink;
MM_INTERFACE mmi;
int hLangpack = 0;
TMyArray<HANDLE> hHooks, hServices;
HANDLE g_hContactMenuItem = NULL, g_hReadStatMenuItem = NULL, /*g_hTopToolbarbutton = NULL, */g_hToggleSOEMenuItem = NULL, g_hToggleSOEContactMenuItem = NULL, g_hAutoreplyOnContactMenuItem = NULL, g_hAutoreplyOffContactMenuItem = NULL, g_hAutoreplyUseDefaultContactMenuItem = NULL;
bool g_fNoProcessing = false; // tells the status change proc not to do anything
int g_bIsIdle = false;
HANDLE hMainThread;
int g_CSProtoCount = 0; // CommonStatus - StartupStatus and AdvancedAutoAway
VAR_PARSE_DATA VarParseData;
INT_PTR (*g_OldCallService)(const char *, WPARAM, LPARAM) = NULL;

static struct
{
	int Status, DisableReplyCtlID, DontShowDialogCtlID;
} StatusModeList[] = {
	ID_STATUS_ONLINE, IDC_REPLYDLG_DISABLE_ONL, IDC_MOREOPTDLG_DONTPOPDLG_ONL,
	ID_STATUS_AWAY, IDC_REPLYDLG_DISABLE_AWAY, IDC_MOREOPTDLG_DONTPOPDLG_AWAY,
	ID_STATUS_NA, IDC_REPLYDLG_DISABLE_NA, IDC_MOREOPTDLG_DONTPOPDLG_NA,
	ID_STATUS_OCCUPIED, IDC_REPLYDLG_DISABLE_OCC, IDC_MOREOPTDLG_DONTPOPDLG_OCC,
	ID_STATUS_DND, IDC_REPLYDLG_DISABLE_DND, IDC_MOREOPTDLG_DONTPOPDLG_DND,
	ID_STATUS_FREECHAT, IDC_REPLYDLG_DISABLE_FFC, IDC_MOREOPTDLG_DONTPOPDLG_FFC,
	ID_STATUS_INVISIBLE, IDC_REPLYDLG_DISABLE_INV, IDC_MOREOPTDLG_DONTPOPDLG_INV,
	ID_STATUS_ONTHEPHONE, IDC_REPLYDLG_DISABLE_OTP, IDC_MOREOPTDLG_DONTPOPDLG_OTP,
	ID_STATUS_OUTTOLUNCH, IDC_REPLYDLG_DISABLE_OTL, IDC_MOREOPTDLG_DONTPOPDLG_OTL
};

// took this nice idea from MetaContacts plugin ;)
// my_make_version is required to break up #define PRODUCTVER from VersionNo.h
DWORD my_make_version(const int a, const int b, const int c, const int d)
{
	return PLUGIN_MAKE_VERSION(a, b, c, d);
}

PLUGININFOEX pluginInfo = {
	sizeof(PLUGININFOEX),
	"New Away System Mod",
	my_make_version(PRODUCTVER), // see VersionNo.h
	"New Away System Mod plugin for Miranda IM.",
	"NightFox; Deathdemon; XF007; Goblineye Entertainment",
	"NightFox@myied.org",
	"� 2010 NightFox; � 2005-2007 Chervov Dmitry; � 2004-2005 Iksaif; � 2002-2003 Goblineye Entertainment",
	"http://MyiEd.org/packs",
	UNICODE_AWARE,
	DEFMOD_SRAWAY,		// mwawawawa.
	{0xb2dd9270, 0xce5e, 0x11df, {0xbd, 0x3d, 0x8, 0x0, 0x20, 0xc, 0x9a, 0x66}}
};

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	g_hInstance = hinstDLL;
	return TRUE;
}

static const MUUID interfaces[] = {MIID_SRAWAY, MIID_LAST}; // TODO: add MIID_WHOISREADING here if there'll be any some time in future..
extern "C" __declspec(dllexport) const MUUID *MirandaPluginInterfaces(void)
{
	return interfaces;
}

extern "C" __declspec(dllexport) PLUGININFOEX *MirandaPluginInfoEx(DWORD mirandaVersion)
{
	return &pluginInfo;
}

//NightFox
int ModernOptInitialise(WPARAM wParam,LPARAM lParam);

TCString GetDynamicStatMsg(HANDLE hContact, char *szProto, DWORD UIN, int iStatus)
{
// hContact is the contact that requests the status message
	if (hContact != INVALID_HANDLE_VALUE)
	{
		VarParseData.Message = CContactSettings(iStatus, hContact).GetMsgFormat(GMF_ANYCURRENT, NULL, szProto);
	} else
	{ // contact is unknown
		VarParseData.Message = CProtoSettings(szProto, iStatus).GetMsgFormat(iStatus ? GMF_LASTORDEFAULT : GMF_ANYCURRENT);
	}
	TCString sTime;
	VarParseData.szProto = szProto ? szProto : ((hContact && hContact != INVALID_HANDLE_VALUE) ? (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0) : NULL);
	VarParseData.UIN = UIN;
	VarParseData.Flags = 0;
	if (ServiceExists(MS_VARS_FORMATSTRING) && !g_SetAwayMsgPage.GetDBValueCopy(IDS_SAWAYMSG_DISABLEVARIABLES))
	{
		FORMATINFO fi = {0};
		fi.cbSize = sizeof(fi);
		fi.tszFormat = VarParseData.Message;
		fi.hContact = hContact;
		fi.flags = FIF_TCHAR;
		TCHAR *szResult = (TCHAR*)CallService(MS_VARS_FORMATSTRING, (WPARAM)&fi, 0);
		if (szResult)
		{
			VarParseData.Message = szResult;
			CallService(MS_VARS_FREEMEMORY, (WPARAM)szResult, 0);
		}
	}
	return VarParseData.Message = VarParseData.Message.Left(AWAY_MSGDATA_MAX);
}


int StatusMsgReq(WPARAM wParam, LPARAM lParam, CString &szProto)
{
	_ASSERT(szProto != NULL);
	LogMessage("ME_ICQ_STATUSMSGREQ called. szProto=%s, Status=%d, UIN=%d", (char*)szProto, wParam, lParam);
// find the contact
	char *szFoundProto;
	HANDLE hFoundContact = NULL; // if we'll find the contact only on some other protocol, but not on szProto, then we'll use that hContact.
	HANDLE hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
	while (hContact)
	{	
		char *szCurProto = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
		if (DBGetContactSettingDword(hContact, szCurProto, "UIN", 0) == lParam)
		{
			szFoundProto = szCurProto;
			hFoundContact = hContact;
			if (!strcmp(szCurProto, szProto))
			{
				break;
			}
		}
		hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, 0);
	}
	int iMode = ICQStatusToGeneralStatus(wParam);
	if (!hFoundContact)
	{
		hFoundContact = INVALID_HANDLE_VALUE;
	} else if (iMode >= ID_STATUS_ONLINE && iMode <= ID_STATUS_OUTTOLUNCH)
	{ // don't count xstatus requests
		DBWriteContactSettingWord(hFoundContact, MOD_NAME, DB_REQUESTCOUNT, DBGetContactSettingWord(hFoundContact, MOD_NAME, DB_REQUESTCOUNT, 0) + 1);
	}
	HANDLE hContactForSettings = hFoundContact; // used to take into account not-on-list contacts when getting contact settings, but at the same time allows to get correct contact info for contacts that are in the DB
	if (hContactForSettings != INVALID_HANDLE_VALUE && DBGetContactSettingByte(hContactForSettings, "CList", "NotOnList", 0))
	{
		hContactForSettings = INVALID_HANDLE_VALUE; // INVALID_HANDLE_VALUE means the contact is not-on-list
	}
	if (g_SetAwayMsgPage.GetWnd())
	{
		CallAllowedPS_SETAWAYMSG(szProto, iMode, NULL); // we can set status messages to NULL here, as they'll be changed again when the SAM dialog closes.
		return 0;
	}
	if (CContactSettings(iMode, hContactForSettings).Ignore)
	{
		CallAllowedPS_SETAWAYMSG(szProto, iMode, ""); // currently NULL makes ICQ to ignore _any_ further status message requests until the next PS_SETAWAYMSG, so I can't use it here..
		return 0; // move along, sir
	}

	if (iMode)
	{ // if it's not an xstatus message request
		CallAllowedPS_SETAWAYMSG(szProto, iMode, (char*)TCHAR2ANSI(GetDynamicStatMsg(hFoundContact, szProto, lParam)));
	}
//	COptPage PopupNotifyData(g_PopupOptPage);
//	PopupNotifyData.DBToMem();
	VarParseData.szProto = szProto;
	VarParseData.UIN = lParam;
	VarParseData.Flags = 0;
	if (!iMode)
	{
		VarParseData.Flags |= VPF_XSTATUS;
	}
/*
	int ShowPopup = ((iMode == ID_STATUS_ONLINE && PopupNotifyData.GetValue(IDC_POPUPOPTDLG_ONLNOTIFY)) ||
		(iMode == ID_STATUS_AWAY && PopupNotifyData.GetValue(IDC_POPUPOPTDLG_AWAYNOTIFY)) ||
		(iMode == ID_STATUS_DND && PopupNotifyData.GetValue(IDC_POPUPOPTDLG_DNDNOTIFY)) ||
		(iMode == ID_STATUS_NA && PopupNotifyData.GetValue(IDC_POPUPOPTDLG_NANOTIFY)) ||
		(iMode == ID_STATUS_OCCUPIED && PopupNotifyData.GetValue(IDC_POPUPOPTDLG_OCCNOTIFY)) ||
		(iMode == ID_STATUS_FREECHAT && PopupNotifyData.GetValue(IDC_POPUPOPTDLG_FFCNOTIFY)) ||
		(!iMode && PopupNotifyData.GetValue(IDC_POPUPOPTDLG_OTHERNOTIFY))) &&
		CContactSettings(iMode, hContactForSettings).PopupNotify.IncludingParents();

// Show popup and play a sound
	if (ShowPopup)
	{
		ShowPopup = ShowPopupNotification(PopupNotifyData, hFoundContact, iMode); // we need ShowPopup also to determine whether to log to file or not
	}
*/
// Log status message request to a file
//	if (!PopupNotifyData.GetValue(IDC_POPUPOPTDLG_LOGONLYWITHPOPUP) || ShowPopup)
//	{
		TCString LogMsg;
		if (!iMode)
		{ // if it's an xstatus message request
			LogMsg = DBGetContactSettingString(NULL, szProto, "XStatusName", _T(""));
			TCString XMsg(DBGetContactSettingString(NULL, szProto, "XStatusMsg", _T("")));
			if (XMsg.GetLen())
			{
				if (LogMsg.GetLen())
				{
					LogMsg += _T("\r\n");
				}
				LogMsg += XMsg;
			}
		} else
		{
			LogMsg = VarParseData.Message;
		}
		if (ServiceExists(MS_VARS_FORMATSTRING))
		{
			logservice_log(LOG_ID, hFoundContact, LogMsg);
		} else
		{
			TCString szUIN;
			_ultot(lParam, szUIN.GetBuffer(16), 10);
			szUIN.ReleaseBuffer();
			TCHAR *szStatDesc = iMode ? (TCHAR*)CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, iMode, 0) : STR_XSTATUSDESC;
			if (!szStatDesc)
			{
				_ASSERT(0);
				szStatDesc = _T("");
			}
			logservice_log(LOG_ID, hFoundContact, TCString((TCHAR*)CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM)hFoundContact, GCDNF_TCHAR)) + _T(" (") + szUIN + TranslateT(") read your ") + szStatDesc + TranslateT(" message:\r\n") + LogMsg);
		}
//	}
	return 0;
}

// Here is an ugly workaround to support multiple ICQ accounts
// hope 5 icq accounts will be sufficient for everyone ;)
#define MAXICQACCOUNTS 5
CString ICQProtoList[MAXICQACCOUNTS];
#define StatusMsgReqN(N) int StatusMsgReq##N(WPARAM wParam, LPARAM lParam) {return StatusMsgReq(wParam, lParam, ICQProtoList[N - 1]);}
StatusMsgReqN(1) StatusMsgReqN(2) StatusMsgReqN(3) StatusMsgReqN(4) StatusMsgReqN(5)
MIRANDAHOOK StatusMsgReqHooks[] = {StatusMsgReq1, StatusMsgReq2, StatusMsgReq3, StatusMsgReq4, StatusMsgReq5};

int IsAnICQProto(char *szProto)
{
	int I;
	for (I = 0; I < MAXICQACCOUNTS; I++)
	{
		if (ICQProtoList[I] == (const char*)szProto)
		{
			return true;
		}
	}
	return false;
}


int StatusChanged(WPARAM wParam, LPARAM lParam)
{
// wParam = iMode
// lParam = (char*)szProto
	LogMessage("MS_CLIST_SETSTATUSMODE called. szProto=%s, Status=%d", lParam ? (char*)lParam : "NULL", wParam);
	g_ProtoStates[(char*)lParam].Status = wParam;
// let's check if we handle this thingy
	if (g_fNoProcessing) // we're told not to do anything
	{
		g_fNoProcessing = false; // take it off
		return 0;
	}
	DWORD Flag1 = 0;
	DWORD Flag3 = 0;
	if (lParam)
	{
		Flag1 = CallProtoService((char*)lParam, PS_GETCAPS, PFLAGNUM_1, 0);
		Flag3 = CallProtoService((char*)lParam, PS_GETCAPS, PFLAGNUM_3, 0);
	} else
	{
		PROTOCOLDESCRIPTOR **proto;
		int iProtoCount = 0;
		CallService(MS_PROTO_ENUMPROTOCOLS, (WPARAM)&iProtoCount, (LPARAM)&proto);
		int I;
		for (I = 0; I < iProtoCount; I++)
		{
			if (proto[I]->type == PROTOTYPE_PROTOCOL)
			{
				Flag1 |= CallProtoService(proto[I]->szName, PS_GETCAPS, PFLAGNUM_1, 0);
				Flag3 |= CallProtoService(proto[I]->szName, PS_GETCAPS, PFLAGNUM_3, 0);
			}
		}
	}
	if (!(Flag1 & PF1_MODEMSGSEND || Flag3 & Proto_Status2Flag(wParam) || (Flag1 & PF1_IM) == PF1_IM))
	{
		return 0; // there are no protocols with changed status that support autoreply or away messages for this status
	}
	if (g_SetAwayMsgPage.GetWnd())
	{
		SetForegroundWindow(g_SetAwayMsgPage.GetWnd());
		return 0;
	}
	int I;
	for (I = lengthof(StatusModeList) - 1; I >= 0; I--)
	{
		if (wParam == StatusModeList[I].Status)
		{
			break;
		}
	}
	if (I < 0)
	{
		return 0;
	}
	BOOL bScreenSaverRunning;
	SystemParametersInfo(SPI_GETSCREENSAVERRUNNING, 0, &bScreenSaverRunning, 0);
	if (bScreenSaverRunning || g_MoreOptPage.GetDBValueCopy(StatusModeList[I].DontShowDialogCtlID))
	{
		CProtoSettings((char*)lParam).SetMsgFormat(SMF_PERSONAL, CProtoSettings((char*)lParam).GetMsgFormat(GMF_LASTORDEFAULT));
		ChangeProtoMessages((char*)lParam, wParam, TCString());
	} else
	{
		SetAwayMsgData *dat = new SetAwayMsgData;
		ZeroMemory(dat, sizeof(SetAwayMsgData));
		dat->szProtocol = (char*)lParam;
		dat->IsModeless = false;
		DialogBoxParam(g_hInstance, MAKEINTRESOURCE(IDD_SETAWAYMSG), NULL, SetAwayMsgDlgProc, (LPARAM)dat);
	}
	return 0;
}


#define ID_STATUS_LAST 40081 // yes, 40081 means internal CommonStatus' ID_STATUS_LAST here, not ID_STATUS_IDLE :-S
#define ID_STATUS_CURRENT 40082
#define ID_STATUS_DISABLED 41083
int CSStatusChange(WPARAM wParam, LPARAM lParam) // CommonStatus plugins (StartupStatus and AdvancedAutoAway)
{
// wParam = PROTOCOLSETTINGEX** protoSettings
	PROTOCOLSETTINGEX** ps = *(PROTOCOLSETTINGEX***)wParam;	
	if (!ps)
	{
		return -1;
	}
	LogMessage("ME_CS_STATUSCHANGEEX event:");
	int I;
	for (I = 0; I < g_CSProtoCount; I++)
	{
		LogMessage("%d: cbSize=%d, szProto=%s, status=%d, lastStatus=%d, szMsg:", I + 1, ps[I]->cbSize, ps[I]->szName ? (char*)ps[I]->szName : "NULL", ps[I]->status, ps[I]->lastStatus, ps[I]->szMsg ? ps[I]->szMsg : "NULL");
		if (ps[I]->status != ID_STATUS_DISABLED)
		{
			if (ps[I]->status != ID_STATUS_CURRENT)
			{
				g_ProtoStates[ps[I]->szName].Status = (ps[I]->status == ID_STATUS_LAST) ? ps[I]->lastStatus : ps[I]->status;
			}
			CProtoSettings(ps[I]->szName).SetMsgFormat(SMF_TEMPORARY, ps[I]->szMsg ? ANSI2TCHAR(ps[I]->szMsg) : CProtoSettings(ps[I]->szName).GetMsgFormat(GMF_LASTORDEFAULT));
		}
	}
	return 0;
}


static int IdleChangeEvent(WPARAM wParam, LPARAM lParam)
{
	LogMessage("ME_IDLE_CHANGED event. lParam=0x%x", lParam); // yes, we don't do anything with status message changes on idle.. there seems to be no any good solution for the wrong status message issue :(
	g_bIsIdle = lParam & IDF_ISIDLE;
	return 0;
}


int CSModuleLoaded(WPARAM wParam, LPARAM lParam) // StartupStatus and AdvancedAutoAway
{
// wParam = ProtoCount
	g_CSProtoCount = wParam;
	return 0;
}


int PreBuildContactMenu(WPARAM wParam, LPARAM lParam)
{
	HANDLE hContact = (HANDLE)wParam;
	char *szProto = (char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
	CLISTMENUITEM miSetMsg = {0};
	miSetMsg.cbSize = sizeof(miSetMsg);
	miSetMsg.flags = CMIM_FLAGS | CMIF_TCHAR | CMIF_HIDDEN;
	CLISTMENUITEM miReadMsg = {0};
	miReadMsg.cbSize = sizeof(miReadMsg);
	miReadMsg.flags = CMIM_FLAGS | CMIF_TCHAR | CMIF_HIDDEN;
	int iMode = szProto ? CallProtoService(szProto, PS_GETSTATUS, 0, 0) : 0;
	int Flag1 = szProto ? CallProtoService(szProto, PS_GETCAPS, PFLAGNUM_1, 0) : 0;
	int iContactMode = DBGetContactSettingWord(hContact, szProto, "Status", ID_STATUS_OFFLINE);
	TCHAR szSetStr[256], szReadStr[256];
	if (szProto)
	{
		int I;
		for (I = lengthof(StatusModeList) - 1; I >= 0; I--)
		{
			if (iMode == StatusModeList[I].Status)
			{
				break;
			}
		}
		if ((Flag1 & PF1_MODEMSGSEND && CallProtoService(szProto, PS_GETCAPS, PFLAGNUM_3, 0) & Proto_Status2Flag(iMode)) || ((Flag1 & PF1_IM) == PF1_IM && (I < 0 || !g_AutoreplyOptPage.GetDBValueCopy(StatusModeList[I].DisableReplyCtlID))))
		{ // the protocol supports status message sending for current status, or autoreplying
			mir_sntprintf(szSetStr, SIZEOF(szSetStr), TranslateT("Set %s message for the contact"), CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, iMode, GSMDF_TCHAR), CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM)hContact, GCDNF_TCHAR));
			miSetMsg.ptszName = szSetStr;
			miSetMsg.flags = CMIM_FLAGS | CMIF_TCHAR | CMIM_NAME;
		}
		if (Flag1 & PF1_MODEMSGRECV && CallProtoService(szProto, PS_GETCAPS, PFLAGNUM_3, 0) & Proto_Status2Flag(iContactMode))
		{ // the protocol supports status message reading for contact's status
			mir_sntprintf(szReadStr, SIZEOF(szReadStr), TranslateT("Re&ad %s Message"), CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, iContactMode, GSMDF_TCHAR));
			miReadMsg.ptszName = szReadStr;
			miReadMsg.flags = CMIM_FLAGS | CMIF_TCHAR | CMIM_NAME | CMIM_ICON;
			miReadMsg.hIcon = LoadSkinnedProtoIcon(szProto, iContactMode);
		}
	}
	if (g_hContactMenuItem)
	{
		CallService(MS_CLIST_MODIFYMENUITEM, (WPARAM)g_hContactMenuItem, (LPARAM)&miSetMsg);

		if ((Flag1 & PF1_IM) == PF1_IM)
		{ // if this contact supports sending/receiving messages
			int iAutoreply = CContactSettings(g_ProtoStates[szProto].Status, hContact).Autoreply;
			CLISTMENUITEM mi = {0};
			mi.cbSize = sizeof(mi);
			mi.flags = CMIM_ICON | CMIM_FLAGS | CMIF_TCHAR;
			switch (iAutoreply)
			{
				case VAL_USEDEFAULT: mi.hIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_DOT)); break;
				case 0: mi.hIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_SOE_DISABLED)); break;
				default: iAutoreply = 1; mi.hIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_SOE_ENABLED)); break;
			}
			CallService(MS_CLIST_MODIFYMENUITEM, (WPARAM)g_hToggleSOEContactMenuItem, (LPARAM)&mi);
			mi.flags = CMIM_FLAGS | CMIF_TCHAR | (iAutoreply == 1 ? CMIF_CHECKED : 0);
			CallService(MS_CLIST_MODIFYMENUITEM, (WPARAM)g_hAutoreplyOnContactMenuItem, (LPARAM)&mi);
			mi.flags = CMIM_FLAGS | CMIF_TCHAR | (iAutoreply == 0 ? CMIF_CHECKED : 0);
			CallService(MS_CLIST_MODIFYMENUITEM, (WPARAM)g_hAutoreplyOffContactMenuItem, (LPARAM)&mi);
			mi.flags = CMIM_FLAGS | CMIF_TCHAR | (iAutoreply == VAL_USEDEFAULT ? CMIF_CHECKED : 0);
			CallService(MS_CLIST_MODIFYMENUITEM, (WPARAM)g_hAutoreplyUseDefaultContactMenuItem, (LPARAM)&mi);
		} else
		{ // hide the Autoreply menu item
			CLISTMENUITEM mi = {0};
			mi.cbSize = sizeof(mi);
			mi.flags = CMIM_FLAGS | CMIF_TCHAR | CMIF_HIDDEN;
			CallService(MS_CLIST_MODIFYMENUITEM, (WPARAM)g_hToggleSOEContactMenuItem, (LPARAM)&mi);
		}
	}
	CallService(MS_CLIST_MODIFYMENUITEM, (WPARAM)g_hReadStatMenuItem, (LPARAM)&miReadMsg);
	return 0;
}


static INT_PTR SetContactStatMsg(WPARAM wParam, LPARAM lParam)
{
	if (g_SetAwayMsgPage.GetWnd()) // already setting something
	{
		SetForegroundWindow(g_SetAwayMsgPage.GetWnd()); 
		return 0;
	}
	SetAwayMsgData *dat = new SetAwayMsgData;
	ZeroMemory(dat, sizeof(SetAwayMsgData));
	dat->hInitContact = (HANDLE)wParam;
	dat->szProtocol = (char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, wParam, 0);
	dat->IsModeless = false;
	DialogBoxParam(g_hInstance, MAKEINTRESOURCE(IDD_SETAWAYMSG), NULL, SetAwayMsgDlgProc, (LPARAM)dat);
	return 0;
}

/* //NightFox: deleted used-to-be support
void UpdateSOEButtons(HANDLE hContact)
{
	if (!hContact)
	{
		int SendOnEvent = CContactSettings(g_ProtoStates[(char*)NULL].Status).Autoreply;
		CallService(MS_TTB_SETBUTTONSTATE, (WPARAM)g_hTopToolbarbutton, SendOnEvent ? TTBST_PUSHED : TTBST_RELEASED);
		CLISTMENUITEM mi = {0};
		mi.cbSize = sizeof(mi);
		mi.position = 1000020000;
		mi.flags = CMIF_TCHAR | CMIM_NAME | CMIM_ICON; // strange, but CMIF_TCHAR is still necessary even without CMIM_FLAGS
		mi.hIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(SendOnEvent ? IDI_SOE_ENABLED : IDI_SOE_DISABLED));
		mi.ptszName = SendOnEvent ? DISABLE_SOE_COMMAND : ENABLE_SOE_COMMAND;
		CallService(MS_CLIST_MODIFYMENUITEM, (WPARAM)g_hToggleSOEMenuItem, (LPARAM)&mi);
	}
	if (g_SetAwayMsgPage.GetWnd())
	{
		SendMessage(g_SetAwayMsgPage.GetWnd(), UM_SAM_REPLYSETTINGCHANGED, (WPARAM)hContact, 0);
	}
}
*/

INT_PTR ToggleSendOnEvent(WPARAM wParam, LPARAM lParam)
{ // used only for the global setting
	HANDLE hContact = (HANDLE)wParam;
	CContactSettings(g_ProtoStates[hContact ? (char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0) : (char*)NULL].Status, hContact).Autoreply.Toggle();
	//UpdateSOEButtons();
	return 0;
}


INT_PTR srvAutoreplyOn(WPARAM wParam, LPARAM lParam)
{ // wParam = hContact
	HANDLE hContact = (HANDLE)wParam;
	CContactSettings(g_ProtoStates[(char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0)].Status, hContact).Autoreply = 1;
	//UpdateSOEButtons(hContact);
	return 0;
}


INT_PTR srvAutoreplyOff(WPARAM wParam, LPARAM lParam)
{ // wParam = hContact
	HANDLE hContact = (HANDLE)wParam;
	CContactSettings(g_ProtoStates[(char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0)].Status, hContact).Autoreply = 0;
	//UpdateSOEButtons(hContact);
	return 0;
}


INT_PTR srvAutoreplyUseDefault(WPARAM wParam, LPARAM lParam)
{ // wParam = hContact
	HANDLE hContact = (HANDLE)wParam;
	CContactSettings(g_ProtoStates[(char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0)].Status, hContact).Autoreply = VAL_USEDEFAULT;
	//UpdateSOEButtons(hContact);
	return 0;
}

/* //NightFox: deleted used-to-be support
int Create_TopToolbar(WPARAM wParam, LPARAM lParam)
{	
	int SendOnEvent = CContactSettings(g_ProtoStates[(char*)NULL].Status).Autoreply;
	if (ServiceExists(MS_TTB_ADDBUTTON))
	{
		TTBButton ttbb = {0};
		ttbb.cbSize = sizeof(ttbb);
		ttbb.hbBitmapUp = LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_SOE_DISABLED));
		ttbb.hbBitmapDown = LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_SOE_ENABLED));
		ttbb.pszServiceUp = MS_AWAYSYS_AUTOREPLY_TOGGLE;
		ttbb.pszServiceDown = MS_AWAYSYS_AUTOREPLY_TOGGLE;
		ttbb.dwFlags = TTBBF_VISIBLE | TTBBF_SHOWTOOLTIP;
		ttbb.name = Translate("Toggle autoreply on/off");
		g_hTopToolbarbutton = (HANDLE)CallService(MS_TTB_ADDBUTTON, (WPARAM)&ttbb, 0);
		CallService(MS_TTB_SETBUTTONSTATE, (WPARAM)g_hTopToolbarbutton, SendOnEvent ? TTBST_PUSHED : TTBST_RELEASED);
	}
	return 0;
}
*/

static int IconsChanged(WPARAM wParam, LPARAM lParam)
{
	g_IconList.ReloadIcons();
	if (g_MessagesOptPage.GetWnd())
	{
		SendMessage(g_MessagesOptPage.GetWnd(), UM_ICONSCHANGED, 0, 0);
	}
	if (g_MoreOptPage.GetWnd())
	{
		SendMessage(g_MoreOptPage.GetWnd(), UM_ICONSCHANGED, 0, 0);
	}
	if (g_AutoreplyOptPage.GetWnd())
	{
		SendMessage(g_AutoreplyOptPage.GetWnd(), UM_ICONSCHANGED, 0, 0);
	}
/*	if (g_PopupOptPage.GetWnd())
	{
		SendMessage(g_PopupOptPage.GetWnd(), UM_ICONSCHANGED, 0, 0);
	}*/
	return 0;
}


static int ContactSettingsInit(WPARAM wParam, LPARAM lParam)
{
	CONTACTSETTINGSINIT *csi = (CONTACTSETTINGSINIT*)wParam;
	char *szProto = (csi->Type == CSIT_CONTACT) ? (char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)csi->hContact, 0) : NULL;
	if ((csi->Type == CSIT_GROUP) || szProto)
	{
		int Flag1 = (csi->Type == CSIT_CONTACT) ? CallProtoService(szProto, PS_GETCAPS, PFLAGNUM_1, 0) : PF1_IM; // we assume that there can be some contacts in the group with PF1_IM capability
		if ((Flag1 & PF1_IM) == PF1_IM || Flag1 & PF1_INDIVMODEMSG)
		{ // does contact's protocol supports message sending/receiving or individual status messages?
			CONTACTSETTINGSCONTROL csc = {0};
			csc.cbSize = sizeof(csc);
			csc.cbStateSize = sizeof(CSCONTROLSTATE);
			csc.Position = CSPOS_SORTBYALPHABET;
			csc.ControlType = CSCT_CHECKBOX;
			csc.szModule = MOD_NAME;
			csc.StateNum = 3;
			csc.DefState = 2; // these settings are used for all controls below

			/*if ((csi->Type == CSIT_GROUP) || IsAnICQProto(szProto))
			{
				csc.Flags = CSCF_TCHAR;
				csc.ptszTitle = LPGENT("New Away System: Status message request notifications");
				csc.ptszGroup = CSGROUP_NOTIFICATIONS;
				csc.ptszTooltip = NULL;
				csc.szSetting = DB_POPUPNOTIFY;
				CallService(MS_CONTACTSETTINGS_ADDCONTROL, wParam, (LPARAM)&csc);
			}*/
			int StatusMode = 0;
			if (csi->Type == CSIT_CONTACT)
			{
				CContactSettings CSettings(0, csi->hContact);
				StatusMode = CSettings.Status;
			} else
			{
				_ASSERT(csi->Type == CSIT_GROUP);
				StatusMode = g_ProtoStates[(char*)NULL].Status;
			}
			if (StatusMode == ID_STATUS_OFFLINE)
			{
				StatusMode = ID_STATUS_AWAY;
			}
			CString Setting;
			TCHAR Title[128];
			csc.Flags = CSCF_TCHAR | CSCF_DONT_TRANSLATE_STRINGS; // these Flags and ptszGroup are used for both controls below
			csc.ptszGroup = TranslateT("New Away System");

			if (g_MoreOptPage.GetDBValueCopy(IDC_MOREOPTDLG_PERSTATUSPERSONALSETTINGS))
			{
				mir_sntprintf(Title, SIZEOF(Title), TranslateT("Enable autoreply when you are %s"), (TCHAR*)CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, StatusMode, GSMDF_TCHAR));
				csc.ptszTitle = Title;
				csc.ptszTooltip = TranslateT("\"Store contact autoreply/ignore settings for each status separately\" is enabled, so this setting is per-contact AND per-status.");
			} else
			{
				csc.ptszTitle = TranslateT("Enable autoreply");
				csc.ptszTooltip = NULL;
			}
			Setting = StatusToDBSetting(StatusMode, DB_ENABLEREPLY, IDC_MOREOPTDLG_PERSTATUSPERSONALSETTINGS);
			csc.szSetting = Setting;
			CallService(MS_CONTACTSETTINGS_ADDCONTROL, wParam, (LPARAM)&csc);

			if (g_MoreOptPage.GetDBValueCopy(IDC_MOREOPTDLG_PERSTATUSPERSONALSETTINGS))
			{
				mir_sntprintf(Title, SIZEOF(Title), TranslateT("Don't send status message when you are %s"), (TCHAR*)CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, StatusMode, GSMDF_TCHAR));
				csc.ptszTitle = Title;
				csc.ptszTooltip = TranslateT("Ignore status message requests from this contact and don't send an autoreply.\r\n\"Store contact autoreply/ignore settings for each status separately\" is enabled, so this setting is per-contact AND per-status.");
			} else
			{
				csc.ptszTitle = TranslateT("Don't send status message");
				csc.ptszTooltip = TranslateT("Ignore status message requests from this contact and don't send an autoreply");
			}
			Setting = StatusToDBSetting(StatusMode, DB_IGNOREREQUESTS, IDC_MOREOPTDLG_PERSTATUSPERSONALSETTINGS);
			csc.szSetting = Setting;
			CallService(MS_CONTACTSETTINGS_ADDCONTROL, wParam, (LPARAM)&csc);
		}
	}
	return 0;
}


INT_PTR srvVariablesHandler(WPARAM wParam, LPARAM lParam)
{
	ARGUMENTSINFO *ai = (ARGUMENTSINFO*)lParam;
	ai->flags = AIF_DONTPARSE;
	TCString Result;
	if (!lstrcmp(ai->targv[0], _T(VAR_AWAYSINCE_TIME)))
	{
		GetTimeFormat(MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT), 0, g_ProtoStates[VarParseData.szProto].AwaySince, (ai->argc > 1 && *ai->targv[1]) ? ai->targv[1] : _T("H:mm"), Result.GetBuffer(256), 256);
		Result.ReleaseBuffer();
	} else if (!lstrcmp(ai->targv[0], _T(VAR_AWAYSINCE_DATE)))
	{
		GetDateFormat(MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT), 0, g_ProtoStates[VarParseData.szProto].AwaySince, (ai->argc > 1 && *ai->targv[1]) ? ai->targv[1] : NULL, Result.GetBuffer(256), 256);
		Result.ReleaseBuffer();
	} else if (!lstrcmp(ai->targv[0], _T(VAR_STATDESC)))
	{
		Result = (VarParseData.Flags & VPF_XSTATUS) ? STR_XSTATUSDESC : (TCHAR*)CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, g_ProtoStates[VarParseData.szProto].Status, GSMDF_TCHAR);
	} else if (!lstrcmp(ai->targv[0], _T(VAR_MYNICK)))
	{
		if (g_MoreOptPage.GetDBValueCopy(IDC_MOREOPTDLG_MYNICKPERPROTO) && VarParseData.szProto)
		{
			Result = DBGetContactSettingString(NULL, VarParseData.szProto, "Nick", (TCHAR*)NULL);
		}
		if (Result == NULL)
		{
			Result = (TCHAR*)CallService(MS_CLIST_GETCONTACTDISPLAYNAME, NULL, GCDNF_TCHAR);
		}
		if (Result == NULL)
		{
			Result = TranslateT("Stranger");
		}
	} else if (!lstrcmp(ai->targv[0], _T(VAR_REQUESTCOUNT)))
	{
		mir_sntprintf(Result.GetBuffer(16), 16, _T("%d"), DBGetContactSettingWord(ai->fi->hContact, MOD_NAME, DB_REQUESTCOUNT, 0));
		Result.ReleaseBuffer();
	} else if (!lstrcmp(ai->targv[0], _T(VAR_MESSAGENUM)))
	{
		mir_sntprintf(Result.GetBuffer(16), 16, _T("%d"), DBGetContactSettingWord(ai->fi->hContact, MOD_NAME, DB_MESSAGECOUNT, 0));
		Result.ReleaseBuffer();
	} else if (!lstrcmp(ai->targv[0], _T(VAR_TIMEPASSED)))
	{
		ULARGE_INTEGER ul_AwaySince, ul_Now;
		SYSTEMTIME st;
		GetLocalTime(&st);
		SystemTimeToFileTime(&st, (LPFILETIME)&ul_Now);
		SystemTimeToFileTime(g_ProtoStates[VarParseData.szProto].AwaySince, (LPFILETIME)&ul_AwaySince);
		ul_Now.QuadPart -= ul_AwaySince.QuadPart;
		ul_Now.QuadPart /= 10000000; // now it's in seconds
		Result.GetBuffer(256);
		if (ul_Now.LowPart >= 7200) // more than 2 hours
		{
			mir_sntprintf(Result, 256, TranslateT("%d hours"), ul_Now.LowPart / 3600);
		} else if (ul_Now.LowPart >= 120) // more than 2 minutes
		{
			mir_sntprintf(Result, 256, TranslateT("%d minutes"), ul_Now.LowPart / 60);
		} else
		{
			mir_sntprintf(Result, 256, TranslateT("%d seconds"), ul_Now.LowPart);
		}
		Result.ReleaseBuffer();
	} else if (!lstrcmp(ai->targv[0], _T(VAR_PREDEFINEDMESSAGE)))
	{
		ai->flags = 0; // reset AIF_DONTPARSE flag
		if (ai->argc != 2)
		{
			return NULL;
		}
		COptPage MsgTreeData(g_MsgTreePage);
		COptItem_TreeCtrl *TreeCtrl = (COptItem_TreeCtrl*)MsgTreeData.Find(IDV_MSGTREE);
		TreeCtrl->DBToMem(CString(MOD_NAME));
		int I;
		for (I = 0; I < TreeCtrl->Value.GetSize(); I++)
		{
			if (!(TreeCtrl->Value[I].Flags & TIF_GROUP) && !_tcsicmp(TreeCtrl->Value[I].Title, ai->targv[1]))
			{
				Result = TreeCtrl->Value[I].User_Str1;
				break;
			}
		}
		if (Result == NULL)
		{ // if we didn't find a message with specified title
			return NULL; // return it now, as later we change NULL to ""
		}
	} else if (!lstrcmp(ai->targv[0], _T(VAR_PROTOCOL)))
	{
		if (VarParseData.szProto)
		{
			CString AnsiResult;
			CallProtoService(VarParseData.szProto, PS_GETNAME, 256, (LPARAM)AnsiResult.GetBuffer(256));
			AnsiResult.ReleaseBuffer();
			Result = ANSI2TCHAR(AnsiResult);
		}
		if (Result == NULL)
		{ // if we didn't find a message with specified title
			return NULL; // return it now, as later we change NULL to ""
		}
	}
	TCHAR *szResult;
	if (!(szResult = (TCHAR*)malloc((Result.GetLen() + 1) * sizeof(TCHAR))))
	{
		return NULL;
	}
	_tcscpy(szResult, (Result != NULL) ? Result : _T(""));
	return (int)szResult;
}


INT_PTR srvFreeVarMem(WPARAM wParam, LPARAM lParam)
{
	if (!lParam)
	{
		return -1;
	}
	free((void*)lParam);
	return 0;
}


static INT_PTR MyCallService(const char *name, WPARAM wParam, LPARAM lParam)
{
	if (name && wParam <= ID_STATUS_OUTTOLUNCH && wParam >= ID_STATUS_OFFLINE) // wParam conditions here are distinctive "features" of PS_SETSTATUS and PS_SETAWAYMSG services, so if wParam does not suit them, we'll pass the control to the old CallService function as soon as possible
	{
		const char *pProtoNameEnd = strrchr(name, '/');
		if (pProtoNameEnd)
		{
			if (!lstrcmpA(pProtoNameEnd, PS_SETSTATUS))
			{
			// it's PS_SETSTATUS service; wParam = status; lParam = 0
			// returns 0 on success, nonzero on failure
				CString Proto("");
				Proto.DiffCat(name, pProtoNameEnd);
				if (wParam != g_ProtoStates[Proto].Status)
				{
					g_ProtoStates[Proto].Status = wParam;
					TCString Msg(CProtoSettings(Proto).GetMsgFormat(GMF_LASTORDEFAULT));
					LogMessage("Detected a PS_SETSTATUS call with Status different from the one known to NAS. szProto=%s, NewStatus=%d, NewMsg:\n%s", (char*)Proto, wParam, (Msg != NULL) ? TCHAR2ANSI(Msg) : "NULL");
					CProtoSettings(Proto).SetMsgFormat(SMF_TEMPORARY, Msg);
				}
			} else if (!lstrcmpA(pProtoNameEnd, PS_SETAWAYMSG))
			{
			// PS_SETAWAYMSG service; wParam = status; lParam = (const char*)szMessage
			// returns 0 on success, nonzero on failure
        CString Proto("");
				Proto.DiffCat(name, pProtoNameEnd);
				LogMessage("Someone else than NAS called PS_SETAWAYMSG. szProto=%s, Status=%d, Msg:\n%s", (char*)Proto, wParam, lParam ? (char*)lParam : "NULL");
				CProtoSettings(Proto).SetMsgFormat(SMF_TEMPORARY, lParam ? ((ServiceExists(MS_VARS_FORMATSTRING) && !g_SetAwayMsgPage.GetDBValueCopy(IDS_SAWAYMSG_DISABLEVARIABLES)) ? VariablesEscape(ANSI2TCHAR((char*)lParam)) : ANSI2TCHAR((char*)lParam)) : TCString(_T("")));
				ChangeProtoMessages(Proto, wParam, TCString());
				return 0;
			}
		}
	}
	return g_OldCallService(name, wParam, lParam);
}


int MirandaLoaded(WPARAM wParam, LPARAM lParam)
{
	LoadMsgTreeModule();
	LoadCListModule();
	InitUpdateMsgs();
	g_IconList.ReloadIcons();

	PROTOCOLDESCRIPTOR **proto;
	DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &hMainThread, THREAD_SET_CONTEXT, false, 0);
	int iProtoCount = 0;
	CallService(MS_PROTO_ENUMPROTOCOLS, (WPARAM)&iProtoCount, (LPARAM)&proto);
	int I;
	int CurProtoIndex;
	for (I = 0, CurProtoIndex = 0; I < iProtoCount && CurProtoIndex < MAXICQACCOUNTS; I++)
	{
		if (proto[I]->type == PROTOTYPE_PROTOCOL)
		{
			HANDLE hHook = HookEvent(CString(proto[I]->szName) + ME_ICQ_STATUSMSGREQ, StatusMsgReqHooks[CurProtoIndex]);
			if (hHook)
			{
				hHooks.AddElem(hHook);
				ICQProtoList[CurProtoIndex] = proto[I]->szName;
				CurProtoIndex++;
			}
		}
	}
	hServices.AddElem(CreateServiceFunction(MS_AWAYSYS_SETCONTACTSTATMSG, SetContactStatMsg));
	hServices.AddElem(CreateServiceFunction(MS_AWAYSYS_AUTOREPLY_TOGGLE, ToggleSendOnEvent));
	hServices.AddElem(CreateServiceFunction(MS_AWAYSYS_AUTOREPLY_ON, srvAutoreplyOn));
	hServices.AddElem(CreateServiceFunction(MS_AWAYSYS_AUTOREPLY_OFF, srvAutoreplyOff));
	hServices.AddElem(CreateServiceFunction(MS_AWAYSYS_AUTOREPLY_USEDEFAULT, srvAutoreplyUseDefault));
	hServices.AddElem(CreateServiceFunction(MS_NAS_GETSTATEA, GetStateA));
	hServices.AddElem(CreateServiceFunction(MS_NAS_SETSTATEA, SetStateA));
	hServices.AddElem(CreateServiceFunction(MS_NAS_GETSTATEW, GetStateW));
	hServices.AddElem(CreateServiceFunction(MS_NAS_SETSTATEW, SetStateW));
	hServices.AddElem(CreateServiceFunction(MS_NAS_INVOKESTATUSWINDOW, InvokeStatusWindow));
	hServices.AddElem(CreateServiceFunction(MS_AWAYMSG_GETSTATUSMSG, GetStatusMsg));
// and old AwaySysMod service, for compatibility reasons
	hServices.AddElem(CreateServiceFunction(MS_AWAYSYS_SETSTATUSMODE, SetStatusMode));
//NightFox: none;
//	hHooks.AddElem(HookEvent(ME_TTB_MODULELOADED, Create_TopToolbar));
	hHooks.AddElem(HookEvent(ME_OPT_INITIALISE, OptsDlgInit));
	hHooks.AddElem(HookEvent(ME_CLIST_STATUSMODECHANGE, StatusChanged));
	hHooks.AddElem(HookEvent(ME_CS_STATUSCHANGEEX, CSStatusChange)); // for compatibility with StartupStatus and AdvancedAutoAway
	hHooks.AddElem(HookEvent(ME_DB_EVENT_FILTER_ADD, MsgEventAdded));
	hHooks.AddElem(HookEvent(ME_CLIST_PREBUILDCONTACTMENU, PreBuildContactMenu));
	hHooks.AddElem(HookEvent(ME_SKIN_ICONSCHANGED, IconsChanged));
	hHooks.AddElem(HookEvent(ME_IDLE_CHANGED, IdleChangeEvent));
	hHooks.AddElem(HookEvent(ME_CONTACTSETTINGS_INITIALISE, ContactSettingsInit));
	g_hReadWndList = (HANDLE)CallService(MS_UTILS_ALLOCWINDOWLIST, 0, 0);
	int SendOnEvent = CContactSettings(g_ProtoStates[(char*)NULL].Status).Autoreply;

	CLISTMENUITEM mi = {0};
	mi.cbSize = sizeof(mi);
	mi.position = 1000020000;
	mi.flags = CMIF_TCHAR | CMIF_NOTOFFLINE;
	mi.hIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(SendOnEvent ? IDI_SOE_ENABLED : IDI_SOE_DISABLED));
	mi.ptszName = SendOnEvent ? DISABLE_SOE_COMMAND : ENABLE_SOE_COMMAND;
	mi.pszService = MS_AWAYSYS_AUTOREPLY_TOGGLE;
	g_hToggleSOEMenuItem = Menu_AddMainMenuItem(&mi);

	ZeroMemory(&mi, sizeof(mi));
	mi.cbSize = sizeof(mi);
	mi.position = -2000005000;
	mi.flags = CMIF_TCHAR | CMIF_NOTOFFLINE | CMIF_HIDDEN;
	mi.ptszName = LPGENT("Read status message"); // never seen...
	mi.pszService = MS_AWAYMSG_SHOWAWAYMSG;
	g_hReadStatMenuItem = Menu_AddContactMenuItem(&mi);
	if (g_MoreOptPage.GetDBValueCopy(IDC_MOREOPTDLG_USEMENUITEM)) {
		ZeroMemory(&mi, sizeof(mi));
		mi.cbSize = sizeof(mi);
		mi.flags = CMIF_TCHAR | CMIF_HIDDEN;
		mi.ptszName = LPGENT("Set status message"); // will never be shown
		mi.position = 1000020000;
		mi.hIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_MSGICON));
		mi.pszService = MS_AWAYSYS_SETCONTACTSTATMSG;
		g_hContactMenuItem = Menu_AddContactMenuItem(&mi);

		ZeroMemory(&mi, sizeof(mi));
		mi.cbSize = sizeof(mi);
		mi.flags = CMIF_TCHAR | CMIF_ROOTPOPUP;
		mi.hIcon = NULL;
		mi.pszPopupName = (char*)-1;
		mi.position = 1000020000;
		mi.ptszName = LPGENT("Autoreply");
		g_hToggleSOEContactMenuItem = Menu_AddContactMenuItem(&mi);

		mi.flags = CMIF_TCHAR | CMIF_CHILDPOPUP;
		mi.pszPopupName = (char*)g_hToggleSOEContactMenuItem;
		mi.popupPosition = 1000020000;
		mi.position = 1000020000;

		mi.hIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_SOE_ENABLED));
		mi.ptszName = LPGENT("On");
		mi.pszService = MS_AWAYSYS_AUTOREPLY_ON;
		g_hAutoreplyOnContactMenuItem = Menu_AddContactMenuItem(&mi);

		mi.hIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_SOE_DISABLED));
		mi.ptszName = LPGENT("Off");
		mi.pszService = MS_AWAYSYS_AUTOREPLY_OFF;
		g_hAutoreplyOffContactMenuItem = Menu_AddContactMenuItem(&mi);

		mi.hIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_DOT));
		mi.ptszName = LPGENT("Use the default setting");
		mi.pszService = MS_AWAYSYS_AUTOREPLY_USEDEFAULT;
		g_hAutoreplyUseDefaultContactMenuItem = Menu_AddContactMenuItem(&mi);
	}
	// add that funky thingy (just tweaked a bit, was spotted in Miranda's src code)
	// we have to read the status message from contacts too... err
	hServices.AddElem(CreateServiceFunction(MS_AWAYMSG_SHOWAWAYMSG, GetContactStatMsg));

	SkinAddNewSoundEx(AWAYSYS_STATUSMSGREQUEST_SOUND, NULL, LPGEN("NewAwaySys: Incoming status message request"));

	if (ServiceExists(MS_VARS_REGISTERTOKEN)) {
		struct
		{
			TCHAR *Name;
			char *Descr;
			int Flags;
		} Variables[] = {
			_T(VAR_AWAYSINCE_TIME), LPGEN("New Away System\t(x)\tAway since time in default format; ?nas_awaysince_time(x) in format x"), TRF_FIELD | TRF_FUNCTION,
			_T(VAR_AWAYSINCE_DATE), LPGEN("New Away System\t(x)\tAway since date in default format; ?nas_awaysince_date(x) in format x"), TRF_FIELD | TRF_FUNCTION,
			_T(VAR_STATDESC), LPGEN("New Away System\tStatus description"), TRF_FIELD | TRF_FUNCTION,
			_T(VAR_MYNICK), LPGEN("New Away System\tYour nick for current protocol"), TRF_FIELD | TRF_FUNCTION,
			_T(VAR_REQUESTCOUNT), LPGEN("New Away System\tNumber of status message requests from the contact"), TRF_FIELD | TRF_FUNCTION,
			_T(VAR_MESSAGENUM), LPGEN("New Away System\tNumber of messages from the contact"), TRF_FIELD | TRF_FUNCTION,
			_T(VAR_TIMEPASSED), LPGEN("New Away System\tTime passed until request"), TRF_FIELD | TRF_FUNCTION,
			_T(VAR_PREDEFINEDMESSAGE), LPGEN("New Away System\t(x)\tReturns one of your predefined messages by its title: ?nas_predefinedmessage(creepy)"), TRF_FUNCTION,
			_T(VAR_PROTOCOL), LPGEN("New Away System\tCurrent protocol name"), TRF_FIELD | TRF_FUNCTION
		};
		hServices.AddElem(CreateServiceFunction(MS_AWAYSYS_FREEVARMEM, srvFreeVarMem));
		hServices.AddElem(CreateServiceFunction(MS_AWAYSYS_VARIABLESHANDLER, srvVariablesHandler));
		TOKENREGISTER tr = {0};
		tr.cbSize = sizeof(TOKENREGISTER);
		tr.szService = MS_AWAYSYS_VARIABLESHANDLER;
		tr.szCleanupService = MS_AWAYSYS_FREEVARMEM;
		tr.memType = TR_MEM_OWNER;
		int I;
		for (I = 0; I < lengthof(Variables); I++)
		{
			tr.flags = Variables[I].Flags | TRF_CALLSVC | TRF_TCHAR;
			tr.tszTokenString = Variables[I].Name;
			tr.szHelpText = Variables[I].Descr;
			CallService(MS_VARS_REGISTERTOKEN, 0, (LPARAM)&tr);
		}
	}
// updater plugin support
	Update update = {0};
	char szVersion[16];
	update.cbSize = sizeof(Update);
	update.szComponentName = pluginInfo.shortName;
	update.pbVersion = (BYTE*)CreateVersionString(my_make_version(PRODUCTVER), szVersion);
	update.cpbVersion = strlen((char*)update.pbVersion);
	update.szUpdateURL = "http://myied.org/packs/NAS"

		"W"

		".zip";
	update.szVersionURL = "http://myied.org/packs/NAS/updaterinfo.php";
	update.pbVersionPrefix = (BYTE*)"New Away System Mod"

		" Unicode"

		" version ";
	update.cpbVersionPrefix = strlen((char*)update.pbVersionPrefix);
	CallService(MS_UPDATE_REGISTER, 0, (WPARAM)&update);
	
	//NightFox
	HookEvent(ME_MODERNOPT_INITIALIZE, ModernOptInitialise);	
	
	
	return 0;
}


extern "C" int __declspec(dllexport) Load(PLUGINLINK *link)
{
	pluginLink = link;
	mir_getMMI( &mmi );
	mir_getLP( &pluginInfo );

	hHooks.AddElem(HookEvent(ME_SYSTEM_MODULESLOADED, MirandaLoaded));
	if (DBGetContactSettingString(NULL, "KnownModules", "New Away System", (char*)NULL) == NULL)
		DBWriteContactSettingString(NULL, "KnownModules", "New Away System", MOD_NAME);

	InitCommonControls();
	InitOptions(); // must be called before we hook CallService
	g_OldCallService = pluginLink->CallService; // looks like this hack is currently the only way to handle all calls to PS_SETSTATUS and PS_SETAWAYMSG properly. I must know when someone calls it, and there's no any "standard" way to know the real value passed as wParam to it (protocol module often replaces it by another, _supported_ status; for example, Jabber replaces Occupied by DND), so we're intercepting the service here
// and we need to know the real wParam value just to set proper status message (an example: changing the global status to Out to lunch makes Jabber go Away. I guess, the user wants Jabber status message to be "I've been having lunch..." too, like for other protocols, rather than the default Away message "Gone since...")
	pluginLink->CallService = MyCallService;

	logservice_register(LOG_ID, LPGENT("New Away System"), _T("NewAwaySys?puts(p,?dbsetting(%subject%,Protocol,p))?if2(_?dbsetting(,?get(p),?pinfo(?get(p),uidsetting)),).log"), TranslateTS(_T("`[`!cdate()-!ctime()`]`  ?cinfo(%subject%,display) (?cinfo(%subject%,id)) read your %") _T(VAR_STATDESC) _T("% message:\r\n%extratext%\r\n\r\n")));

	if (DBGetContactSettingByte(NULL, MOD_NAME, DB_SETTINGSVER, 0) < 1)
	{ // change all %nas_message% variables to %extratext% if it wasn't done before
		TCString Str;
		Str = DBGetContactSettingString(NULL, MOD_NAME, "PopupsFormat", _T(""));
		if (Str.GetLen())
		{
			DBWriteContactSettingTString(NULL, MOD_NAME, "PopupsFormat", Str.Replace(_T("nas_message"), _T("extratext")));
		}
		Str = DBGetContactSettingString(NULL, MOD_NAME, "ReplyPrefix", _T(""));
		if (Str.GetLen())
		{
			DBWriteContactSettingTString(NULL, MOD_NAME, "ReplyPrefix", Str.Replace(_T("nas_message"), _T("extratext")));
		}
	}
	if (DBGetContactSettingByte(NULL, MOD_NAME, DB_SETTINGSVER, 0) < 2)
	{ // disable autoreply for not-on-list contacts, as such contact may be a spam bot
		DBWriteContactSettingByte(NULL, MOD_NAME, ContactStatusToDBSetting(0, DB_ENABLEREPLY, 0, INVALID_HANDLE_VALUE), 0);
		DBWriteContactSettingByte(NULL, MOD_NAME, DB_SETTINGSVER, 2);
	}
	return 0;
}


extern "C" int __declspec(dllexport) Unload()
{
	_ASSERT(pluginLink->CallService == MyCallService);
	if (pluginLink->CallService == MyCallService)
	{
		pluginLink->CallService = g_OldCallService;
	}
	CloseHandle(hMainThread);
	int I;
	for (I = 0; I < hHooks.GetSize(); I++)
	{
		if (hHooks[I])
		{
			UnhookEvent(hHooks[I]);
		}
	}
	for (I = 0; I < hServices.GetSize(); I++)
	{
		if (hServices[I])
		{
			DestroyServiceFunction(hServices[I]);
		}
	}
	return 0;
}
