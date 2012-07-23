/*
	New Away System - plugin for Miranda IM
	Copyright (c) 2005-2007 Chervov Dmitry

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

#include "Common.h"
#include "MsgTree.h"
#include "ContactList.h"
#include "statusmodes.h"

#define DB_STATUSMSG "StatusMsg"
#define DB_ENABLEREPLY "EnableReply"
#define DB_IGNOREREQUESTS "IgnoreRequests"
//#define DB_POPUPNOTIFY "UsePopups"
#define DB_UNK_CONTACT_PREFIX "Unk" // DB_ENABLEREPLY, DB_IGNOREREQUESTS and DB_POPUPNOTIFY settings prefix for not-on-list contacts

class _CWndUserData
{
public:
	_CWndUserData(): MsgTree(NULL), CList(NULL) {}

	CMsgTree *MsgTree;
	CCList *CList;
};


class CWndUserData
{
public:
	CWndUserData(HWND hWnd): hWnd(hWnd)
	{
		_ASSERT(hWnd);
		dat = (_CWndUserData*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
		if (!dat)
		{
			dat = new _CWndUserData;
			SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)dat);
		}
	}

	~CWndUserData()
	{
		_ASSERT(dat == (_CWndUserData*)GetWindowLongPtr(hWnd, GWLP_USERDATA));
		if (!dat->MsgTree && !dat->CList) // TODO: Uninitialized Memory Read on closing the options dialog - fix it
		{
			SetWindowLongPtr(hWnd, GWLP_USERDATA, NULL);
			delete dat; // TODO: memory leak - this is never executed - fix it
		}
	}

	CMsgTree *GetMsgTree() {return dat->MsgTree;}
	void SetMsgTree(CMsgTree *MsgTree) {dat->MsgTree = MsgTree;}
	CCList *GetCList() {return dat->CList;}
	void SetCList(CCList *CList) {dat->CList = CList;}

private:
	HWND hWnd;
	_CWndUserData *dat;
};

#define IL_SKINICON 0x80000000
#define IL_PROTOICON 0x40000000

#define ILI_NOICON (-1)
#define ILI_EVENT_MESSAGE 0
#define ILI_EVENT_URL 1
#define ILI_EVENT_FILE 2
#define ILI_PROTO_ONL 3
#define ILI_PROTO_AWAY 4
#define ILI_PROTO_NA 5
#define ILI_PROTO_OCC 6
#define ILI_PROTO_DND 7
#define ILI_PROTO_FFC 8
#define ILI_PROTO_INV 9
#define ILI_PROTO_OTP 10
#define ILI_PROTO_OTL 11
#define ILI_DOT 12
#define ILI_MSGICON 13
#define ILI_IGNORE 14
#define ILI_SOE_DISABLED 15
#define ILI_SOE_ENABLED 16
#define ILI_NEWMESSAGE 17
#define ILI_NEWCATEGORY 18
#define ILI_SAVE 19
#define ILI_SAVEASNEW 20
#define ILI_DELETE 21
#define ILI_SETTINGS 22
#define ILI_STATUS_OTHER 23

static int Icons[] = {
	SKINICON_EVENT_MESSAGE | IL_SKINICON, SKINICON_EVENT_URL | IL_SKINICON, SKINICON_EVENT_FILE | IL_SKINICON,
	ID_STATUS_ONLINE | IL_PROTOICON, ID_STATUS_AWAY | IL_PROTOICON, ID_STATUS_NA | IL_PROTOICON, ID_STATUS_OCCUPIED | IL_PROTOICON, ID_STATUS_DND | IL_PROTOICON, ID_STATUS_FREECHAT | IL_PROTOICON, ID_STATUS_INVISIBLE | IL_PROTOICON, ID_STATUS_ONTHEPHONE | IL_PROTOICON, ID_STATUS_OUTTOLUNCH | IL_PROTOICON,
	IDI_DOT, IDI_MSGICON, IDI_IGNORE, IDI_SOE_ENABLED, IDI_SOE_DISABLED, IDI_NEWMESSAGE, IDI_NEWCATEGORY, IDI_SAVE, IDI_SAVEASNEW, IDI_DELETE, IDI_SETTINGS, IDI_STATUS_OTHER
};


class CIconList
{
public:
	~CIconList()
	{
		int I;
		for (I = 0; I < IconList.GetSize(); I++)
		{
			if (IconList[I])
			{
				DestroyIcon(IconList[I]);
			}
		}
	}

	HICON& operator [] (int nIndex) {return IconList[nIndex];}
	void ReloadIcons()
	{
		int cxIcon = GetSystemMetrics(SM_CXSMICON);
		int cyIcon = GetSystemMetrics(SM_CYSMICON);
		int I;
		for (I = 0; I < lengthof(Icons); I++)
		{
			if (IconList.GetSize() > I && IconList[I])
			{
				DestroyIcon(IconList[I]);
			}
			if (Icons[I] & IL_SKINICON)
			{
				IconList.SetAtGrow(I) = (HICON)CopyImage(LoadSkinnedIcon(Icons[I] & ~IL_SKINICON), IMAGE_ICON, cxIcon, cyIcon, LR_COPYFROMRESOURCE);
			} else if (Icons[I] & IL_PROTOICON)
			{
				IconList.SetAtGrow(I) = (HICON)CopyImage(LoadSkinnedProtoIcon(NULL, Icons[I] & ~IL_PROTOICON), IMAGE_ICON, cxIcon, cyIcon, LR_COPYFROMRESOURCE);
			} else
			{
				IconList.SetAtGrow(I) = (HICON)LoadImage(g_hInstance, MAKEINTRESOURCE(Icons[I]), IMAGE_ICON, cxIcon, cyIcon, 0);
			}
		}
	}

private:
	TMyArray<HICON> IconList;
};

extern CIconList g_IconList;


class CProtoStates;

class CProtoState
{
public:
	CProtoState(const char* szProto, CProtoStates* Parent): szProto(szProto), Parent(Parent), Status(szProto, Parent), AwaySince(szProto, Parent) {}

	class CStatus
	{
	public:
		CStatus(const char* szProto, CProtoStates* GrandParent): szProto(szProto), GrandParent(GrandParent), Status(ID_STATUS_OFFLINE) {}
		CStatus& operator = (int Status);
		operator int() {return Status;}
		friend class CProtoState;
	private:
		int Status;
		CString szProto;
		CProtoStates* GrandParent;
	} Status;

	class CAwaySince
	{
	public:
		CAwaySince(const char* szProto, CProtoStates* GrandParent): szProto(szProto), GrandParent(GrandParent) {Reset();}
		void Reset();
		operator LPSYSTEMTIME() {return &AwaySince;}
		friend class CProtoState;
	private:
		SYSTEMTIME AwaySince;
		CString szProto;
		CProtoStates* GrandParent;
	} AwaySince;

	class CCurStatusMsg
	{
	public:
		CCurStatusMsg() {*this = NULL;}
		CCurStatusMsg& operator = (TCString Msg)
		{
			CurStatusMsg = Msg;
			SYSTEMTIME st;
			GetLocalTime(&st);
			SystemTimeToFileTime(&st, (LPFILETIME)&LastUpdateTime); // I'm too lazy to declare FILETIME structure and then copy its data to absolutely equivalent ULARGE_INTEGER structure, so we'll just pass a pointer to the ULARGE_INTEGER directly ;-P
			return *this;
		}
		operator TCString() {return CurStatusMsg;}
		DWORD GetUpdateTimeDifference()
		{
			ULARGE_INTEGER CurTime;
			SYSTEMTIME st;
			GetLocalTime(&st);
			SystemTimeToFileTime(&st, (LPFILETIME)&CurTime);
			return (DWORD)((CurTime.QuadPart - LastUpdateTime.QuadPart) / 10000); // in milliseconds
		}
	private:
		TCString CurStatusMsg;
		ULARGE_INTEGER LastUpdateTime;
	} CurStatusMsg;

	class CTempMsg
	{ // we use temporary messages to keep user-defined per-protocol messages intact, when changing messages through MS_NAS_SETSTATE, or when autoaway becomes active etc.. temporary messages are automatically resetted when protocol status changes
	public:
		CTempMsg(): iIsSet(0) {}
		CTempMsg& operator = (TCString Msg) {this->Msg = Msg; iIsSet = true; return *this;}
		operator TCString()
		{
			_ASSERT(iIsSet);
			return Msg;
		}
		void Unset() {iIsSet = false;}
		int IsSet() {return iIsSet;}

	private:
		int iIsSet; // as we need TempMsg to support every possible value, including NULL and "", we'll use this variable to specify whether TempMsg is set
		TCString Msg;
	} TempMsg;

	void SetParent(CProtoStates* Parent)
	{
		this->Parent = Parent;
		Status.GrandParent = Parent;
		AwaySince.GrandParent = Parent;
	}

	CString &GetProto() {return szProto;}

//NightFox: fix?
//private:
public:
	CString szProto;
	CProtoStates* Parent;
};


class CProtoStates // this class stores all protocols' dynamic data
{
public:
	CProtoStates() {}

	CProtoStates(const CProtoStates &States) {*this = States;}
	CProtoStates& operator = (const CProtoStates& States)
	{
		ProtoStates = States.ProtoStates;
		int I;
		for (I = 0; I < ProtoStates.GetSize(); I++)
		{
			ProtoStates[I].SetParent(this);
		}
		return *this;
	}

	CProtoState& operator[](const char *szProto)
	{
		int I;
		for (I = 0; I < ProtoStates.GetSize(); I++)
		{
			if (ProtoStates[I].GetProto() == szProto)
			{
				return ProtoStates[I];
			}
		}
		if (!szProto) // we need to be sure that we have _all_ protocols in the list, before dealing with global status, so we're adding them here.
		{
			int ProtoCount;
			PROTOCOLDESCRIPTOR **proto;
			CallService(MS_PROTO_ENUMPROTOCOLS, (WPARAM)&ProtoCount, (LPARAM)&proto);
			int I;
			for (I = 0; I < ProtoCount; I++)
			{
				if (proto[I]->type == PROTOTYPE_PROTOCOL)
				{
					(*this)[proto[I]->szName]; // add a protocol if it isn't in the list yet
				}
			}
		}
		return ProtoStates[ProtoStates.AddElem(CProtoState(szProto, this))];
	}

	friend class CProtoState;
	friend class CProtoState::CStatus;
	friend class CProtoState::CAwaySince;

private:
	CProtoState& operator[](int nIndex) {return ProtoStates[nIndex];}
	int GetSize() {return ProtoStates.GetSize();}

	TMyArray<CProtoState> ProtoStates;
};

extern CProtoStates g_ProtoStates;


static struct
{
	int Status;
	char *Setting;
} StatusSettings[] = {
	ID_STATUS_OFFLINE, "Off",
	ID_STATUS_ONLINE, "Onl",
	ID_STATUS_AWAY, "Away",
	ID_STATUS_NA, "Na",
	ID_STATUS_DND, "Dnd",
	ID_STATUS_OCCUPIED, "Occ",
	ID_STATUS_FREECHAT, "Ffc",
	ID_STATUS_INVISIBLE, "Inv",
	ID_STATUS_ONTHEPHONE, "Otp",
	ID_STATUS_OUTTOLUNCH, "Otl",
	ID_STATUS_IDLE, "Idle"
};


class CProtoSettings
{
public:
	CProtoSettings(const char *szProto = NULL, int iStatus = 0): szProto(szProto), Status(iStatus, szProto)
	{
		Autoreply.Parent = this;
	}

	CString ProtoStatusToDBSetting(const char *Prefix, int MoreOpt_PerStatusID = 0)
	{
		if (!MoreOpt_PerStatusID || g_MoreOptPage.GetDBValueCopy(MoreOpt_PerStatusID))
		{
			int I;
			for (I = 0; I < lengthof(StatusSettings); I++)
			{
				if (Status == StatusSettings[I].Status)
				{
					return szProto ? (CString(Prefix) + "_" + szProto + "_" + StatusSettings[I].Setting) : (CString(Prefix) + StatusSettings[I].Setting);
				}
			}
		}
		return szProto ? (CString(Prefix) + "_" + szProto) : CString(Prefix);
	}

	class CAutoreply
	{
	public:
		CAutoreply& operator = (const int Value)
		{
			CString Setting(Parent->szProto ? Parent->ProtoStatusToDBSetting(DB_ENABLEREPLY, IDC_MOREOPTDLG_PERSTATUSPROTOSETTINGS) : DB_ENABLEREPLY);
			if (DBGetContactSettingByte(NULL, MOD_NAME, Setting, VAL_USEDEFAULT) == Value)
			{
				return *this; // prevent deadlocks when calling UpdateSOEButtons
			}
			if (Value != VAL_USEDEFAULT)
			{
				DBWriteContactSettingByte(NULL, MOD_NAME, Setting, Value != 0);
			} else
			{
				DBDeleteContactSetting(NULL, MOD_NAME, Setting);
			}
			/*if (!Parent->szProto)
			{
				UpdateSOEButtons();
			}*/
			return *this;
		}
		operator int() {return DBGetContactSettingByte(NULL, MOD_NAME, Parent->szProto ? Parent->ProtoStatusToDBSetting(DB_ENABLEREPLY, IDC_MOREOPTDLG_PERSTATUSPROTOSETTINGS) : DB_ENABLEREPLY, Parent->szProto ? VAL_USEDEFAULT : AUTOREPLY_DEF_REPLY);}
		int IncludingParents() // takes into account global data also, if per-protocol setting is not defined
		{
			_ASSERT(Parent->szProto);
			int Value = *this;
			return (Value == VAL_USEDEFAULT) ? CProtoSettings(NULL).Autoreply : Value;
		}
		friend class CProtoSettings;
	private:
		CProtoSettings *Parent;
	} Autoreply;

	class CStatus
	{
	public:
		CStatus(int iStatus = 0, const char *szProto = NULL): Status(iStatus), szProto(szProto) {}
		CStatus& operator = (int Status) {this->Status = Status; return *this;}
		operator int()
		{
			if (!Status)
			{
				Status = g_ProtoStates[szProto].Status;
			}
			return Status;
		}
	private:
		int Status;
		const char *szProto;
	} Status;

	void SetMsgFormat(int Flags, TCString Message);
	TCString GetMsgFormat(int Flags, int *pOrder = NULL);

//NightFox: fix?
//private:
public:
	const char *szProto;
};


__inline CString StatusToDBSetting(int Status, const char *Prefix, int MoreOpt_PerStatusID = 0)
{
	if (!MoreOpt_PerStatusID || g_MoreOptPage.GetDBValueCopy(MoreOpt_PerStatusID))
	{
		int I;
		for (I = 0; I < lengthof(StatusSettings); I++)
		{
			if (Status == StatusSettings[I].Status)
			{
				return CString(Prefix) + StatusSettings[I].Setting;
			}
		}
	}
	return CString(Prefix);
}


__inline CString ContactStatusToDBSetting(int Status, const char *Prefix, int MoreOpt_PerStatusID, HANDLE hContact)
{
	if (hContact == INVALID_HANDLE_VALUE)
	{ // it's a not-on-list contact
		return CString(DB_UNK_CONTACT_PREFIX) + Prefix;
	}
	if (hContact)
	{
		StatusToDBSetting(Status, Prefix, MoreOpt_PerStatusID);
	}
	return CString(Prefix);
}


class CContactSettings
{
public:
	CContactSettings(int iStatus = 0, HANDLE hContact = NULL): Status(iStatus, hContact), hContact(hContact)
	{
		Ignore.Parent = this;
		Autoreply.Parent = this;
//		PopupNotify.Parent = this;
	}

	CString ContactStatusToDBSetting(const char *Prefix, int MoreOpt_PerStatusID = 0)
	{
		return ::ContactStatusToDBSetting((hContact != INVALID_HANDLE_VALUE) ? Status : NULL, Prefix, MoreOpt_PerStatusID, hContact);
	}

	class CIgnore
	{
	public:
		CIgnore& operator = (const int Value)
		{
      CString Setting(Parent->ContactStatusToDBSetting(DB_IGNOREREQUESTS, IDC_MOREOPTDLG_PERSTATUSPERSONALSETTINGS));
			HANDLE hContact = (Parent->hContact != INVALID_HANDLE_VALUE) ? Parent->hContact : NULL;
			if (Value)
			{
				DBWriteContactSettingByte(hContact, MOD_NAME, Setting, 1);
			} else
			{
				DBDeleteContactSetting(hContact, MOD_NAME, Setting);
			}
			return *this;
		}
		operator int() {return DBGetContactSettingByte((Parent->hContact != INVALID_HANDLE_VALUE) ? Parent->hContact : NULL, MOD_NAME, Parent->ContactStatusToDBSetting(DB_IGNOREREQUESTS, IDC_MOREOPTDLG_PERSTATUSPERSONALSETTINGS), 0);}
		friend class CContactSettings;
	private:
		CContactSettings *Parent;
	} Ignore;

	class CAutoreply
	{
	public:
		CAutoreply& operator = (const int Value)
		{
			CString Setting(Parent->ContactStatusToDBSetting(DB_ENABLEREPLY, IDC_MOREOPTDLG_PERSTATUSPERSONALSETTINGS));
			HANDLE hContact = (Parent->hContact != INVALID_HANDLE_VALUE) ? Parent->hContact : NULL;
			if (DBGetContactSettingByte(hContact, MOD_NAME, Setting, VAL_USEDEFAULT) == Value)
			{
				return *this; // prevent deadlocks when calling UpdateSOEButtons
			}
			if (Value != VAL_USEDEFAULT)
			{
				DBWriteContactSettingByte(hContact, MOD_NAME, Setting, Value != 0);
			} else
			{
				DBDeleteContactSetting(hContact, MOD_NAME, Setting);
			}
			/*if (Parent->hContact != INVALID_HANDLE_VALUE)
			{
				UpdateSOEButtons(Parent->hContact);
			}*/
			return *this;
		}
		operator int() {return DBGetContactSettingByte((Parent->hContact != INVALID_HANDLE_VALUE) ? Parent->hContact : NULL, MOD_NAME, Parent->ContactStatusToDBSetting(DB_ENABLEREPLY, IDC_MOREOPTDLG_PERSTATUSPERSONALSETTINGS), Parent->hContact ? VAL_USEDEFAULT : AUTOREPLY_DEF_REPLY);}
		int IncludingParents(const char *szProtoOverride = NULL) // takes into account protocol and global data also, if per-contact setting is not defined
		{
			_ASSERT((Parent->hContact && Parent->hContact != INVALID_HANDLE_VALUE) || szProtoOverride); // we need either correct protocol or a correct hContact to determine its protocol
			int Value = *this;
			if (Value == VAL_USEDEFAULT)
			{
				const char *szProto = (Parent->hContact && Parent->hContact != INVALID_HANDLE_VALUE) ? (const char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)Parent->hContact, 0) : szProtoOverride;
				return CProtoSettings(szProto).Autoreply.IncludingParents();
			}
			return Value;
		}
		int GetNextToggleValue()
		{
			switch ((int)*this)
			{
				case VAL_USEDEFAULT: return 0; break;
				case 0: return 1; break;
				default: return Parent->hContact ? VAL_USEDEFAULT : AUTOREPLY_DEF_REPLY; break;
			}
		}
		int Toggle() {return *this = GetNextToggleValue();}
		friend class CContactSettings;
	private:
		CContactSettings *Parent;
	} Autoreply;

/*	class CPopupNotify
	{
	public:
		CPopupNotify& operator = (const int Value)
		{
			//CString Setting(Parent->ContactStatusToDBSetting(DB_POPUPNOTIFY, 0)); //IDC_MOREOPTDLG_PERSTATUSPERSONALSETTINGS
			HANDLE hContact = (Parent->hContact != INVALID_HANDLE_VALUE) ? Parent->hContact : NULL;
			if (DBGetContactSettingByte(hContact, MOD_NAME, Setting, VAL_USEDEFAULT) == Value)
			{
				return *this; // prevent deadlocks when calling UpdateSOEButtons
			}
			if (Value != VAL_USEDEFAULT)
			{
				DBWriteContactSettingByte(hContact, MOD_NAME, Setting, Value != 0);
			} else
			{
				DBDeleteContactSetting(hContact, MOD_NAME, Setting);
			}
			if (!Parent->hContact)
			{
				//UpdateSOEButtons();
			}
			return *this;
		}
		operator int() {return DBGetContactSettingByte((Parent->hContact != INVALID_HANDLE_VALUE) ? Parent->hContact : NULL, MOD_NAME, Parent->ContactStatusToDBSetting(DB_POPUPNOTIFY, 0), Parent->hContact ? VAL_USEDEFAULT : POPUP_DEF_USEPOPUPS);} //IDC_MOREOPTDLG_PERSTATUSPERSONALSETTINGS
		int IncludingParents() // takes into account protocol and global data also, if per-contact setting is not defined
		{
			int Value = *this;
			if (Value == VAL_USEDEFAULT)
			{ // PopupNotify setting is not per-status
				return CContactSettings(ID_STATUS_ONLINE).PopupNotify;
			}
			return Value;
		}
		friend class CContactSettings;
	private:
		CContactSettings *Parent;
	} PopupNotify;
*/
	class CStatus
	{
	public:
		CStatus(int iStatus = 0, HANDLE hContact = NULL): Status(iStatus), hContact(hContact) {}
		CStatus& operator = (int Status) {this->Status = Status; return *this;}
		operator int()
		{
			if (!Status)
			{
				_ASSERT(hContact != INVALID_HANDLE_VALUE);
				char *szProto = hContact ? (char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0) : NULL;
				Status = (szProto || !hContact) ? g_ProtoStates[szProto].Status : ID_STATUS_AWAY;
			}
			return Status;
		}
		friend class CPopupNotify;
		friend class CAutoreply;
	private:
		int Status;
		HANDLE hContact;
	} Status;

	void SetMsgFormat(int Flags, TCString Message);
	TCString GetMsgFormat(int Flags, int *pOrder = NULL, char *szProtoOverride = NULL);

//NightFox: fix?
//private:
public:
	HANDLE hContact;
};
