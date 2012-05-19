/*
	New Away System - plugin for Miranda IM
	Copyright (C) 2005-2007 Chervov Dmitry

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

#include "Common.h"
#include "Properties.h"

CProtoStates g_ProtoStates;


/*char *mystrdup(const char *szStr)
{
	char *szNew = (char *)malloc(lstrlen(szStr) + 1);
	lstrcpy(szNew, szStr);
	return szNew;
}


void *mymalloc(size_t size)
{
	return HeapAlloc(GetProcessHeap(), 0, size);//GlobalAlloc(GPTR, size);
}


void myfree(void *p)
{
	//GlobalFree(p);
	HeapFree(GetProcessHeap(), 0, p);
}


void *__cdecl operator new(unsigned int size)
{
	return (void *)HeapAlloc(GetProcessHeap(), 0, size);//GlobalAlloc(GPTR, size);
}


void __cdecl operator delete(void *p)
{
//	GlobalFree((HGLOBAL)p);
	HeapFree(GetProcessHeap(), 0, p);
}
*/


void ResetContactSettingsOnStatusChange(HANDLE hContact)
{
	DBDeleteContactSetting(hContact, MOD_NAME, DB_REQUESTCOUNT);
	DBDeleteContactSetting(hContact, MOD_NAME, DB_SENDCOUNT);
	DBDeleteContactSetting(hContact, MOD_NAME, DB_MESSAGECOUNT);
}


void ResetSettingsOnStatusChange(const char *szProto = NULL, int bResetPersonalMsgs = false, int Status = 0)
{
	if (bResetPersonalMsgs)
	{ // bResetPersonalMsgs &&= g_MoreOptPage.GetDBValueCopy(IDC_MOREOPTDLG_SAVEPERSONALMSGS);
		bResetPersonalMsgs = !g_MoreOptPage.GetDBValueCopy(IDC_MOREOPTDLG_SAVEPERSONALMSGS);
	}
	HANDLE hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
	while (hContact)
	{
		const char *szCurProto;
		if (!szProto || ((szCurProto = (const char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0)) && !lstrcmpA(szProto, szCurProto)))
		{
			ResetContactSettingsOnStatusChange(hContact);
			if (bResetPersonalMsgs)
			{
				CContactSettings(Status, hContact).SetMsgFormat(SMF_PERSONAL, NULL); // TODO: delete only when SAM dialog opens?
			}
		}
		hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, 0);
	}
}


CProtoState::CStatus& CProtoState::CStatus::operator = (int Status)
{
	_ASSERT(Status >= ID_STATUS_OFFLINE && Status <= ID_STATUS_OUTTOLUNCH);
	if (Status < ID_STATUS_OFFLINE || Status > ID_STATUS_OUTTOLUNCH)
	{
		return *this; // ignore status change if the new status is unknown
	}
	bool bModified = false;
	if (szProto)
	{
		if (this->Status != Status)
		{
			this->Status = Status;
			(*GrandParent)[szProto].AwaySince.Reset();
			ResetSettingsOnStatusChange(szProto, true, Status);
			bModified = true;
		}
		if ((*GrandParent)[szProto].TempMsg.IsSet())
		{
			(*GrandParent)[szProto].TempMsg.Unset();
			bModified = true;
		}
	} else // global status change
	{
		int I;
		int bStatusModified = false;
		for (I = 0; I < GrandParent->GetSize(); I++)
		{
			CProtoState &State = (*GrandParent)[I];
			if (!DBGetContactSettingByte(NULL, State.GetProto(), "LockMainStatus", 0)) // if the protocol isn't locked
			{
				if (State.Status != Status)
				{
					State.Status.Status = Status; // "Status.Status" - changing Status directly to prevent recursive calls to the function
					State.AwaySince.Reset();
					bModified = true;
					bStatusModified = true;
				}
				if (State.TempMsg.IsSet())
				{
					State.TempMsg.Unset();
					bModified = true;
				}
				if (g_MoreOptPage.GetDBValueCopy(IDC_MOREOPTDLG_RESETPROTOMSGS))
				{
					CProtoSettings(State.szProto, Status).SetMsgFormat(SMF_PERSONAL, NULL);
				}
			}
		}
		if (bStatusModified)
		{
			ResetSettingsOnStatusChange(NULL, true, Status);
		}
	}
	if (bModified && g_SetAwayMsgPage.GetWnd())
	{
		SendMessage(g_SetAwayMsgPage.GetWnd(), UM_SAM_PROTOSTATUSCHANGED, (WPARAM)(char*)szProto, 0);
	}
	return *this;
}


void CProtoState::CAwaySince::Reset()
{
	GetLocalTime(&AwaySince);
	if (GrandParent && !szProto)
	{
		int I;
		for (I = 0; I < GrandParent->GetSize(); I++)
		{
			GetLocalTime((*GrandParent)[I].AwaySince);
		}
	}
}


void CContactSettings::SetMsgFormat(int Flags, TCString Message)
{
	if (Flags & SMF_PERSONAL)
	{ // set a personal message for a contact. also it's used to set global status message (hContact = NULL).
	// if Message == NULL, then the function deletes the message.
		CString DBSetting(StatusToDBSetting(Status, DB_STATUSMSG, IDC_MOREOPTDLG_PERSTATUSPERSONAL));
		if (g_AutoreplyOptPage.GetDBValueCopy(IDC_REPLYDLG_RESETCOUNTERWHENSAMEICON) && GetMsgFormat(SMF_PERSONAL) != (const TCHAR*)Message)
		{
			ResetContactSettingsOnStatusChange(hContact);
		}
		if (Message != NULL)
		{
			DBWriteContactSettingTString(hContact, MOD_NAME, DBSetting, Message);
		} else
		{
			DBDeleteContactSetting(hContact, MOD_NAME, DBSetting);
		}
	}
	if (Flags & (SMF_LAST | SMF_TEMPORARY))
	{
		_ASSERT(!hContact);
		CProtoSettings().SetMsgFormat(Flags & (SMF_LAST | SMF_TEMPORARY), Message);
	}
}


TCString CContactSettings::GetMsgFormat(int Flags, int *pOrder, char *szProtoOverride)
// returns the requested message; sets Order to the order of the message in the message tree, if available; or to -1 otherwise.
// szProtoOverride is needed only to get status message of the right protocol when the ICQ contact is on list, but not with the same protocol on which it requests the message - this way we can still get contact details.
{
	TCString Message = NULL;
	if (pOrder)
	{
		*pOrder = -1;
	}
	if (Flags & GMF_PERSONAL)
	{ // try getting personal message (it overrides global)
		Message = DBGetContactSettingString(hContact, MOD_NAME, StatusToDBSetting(Status, DB_STATUSMSG, IDC_MOREOPTDLG_PERSTATUSPERSONAL), (TCHAR*)NULL);
	}
	if (Flags & (GMF_LASTORDEFAULT | GMF_PROTOORGLOBAL | GMF_TEMPORARY) && Message.IsEmpty())
	{
		char *szProto = szProtoOverride ? szProtoOverride : (hContact ? (char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0) : NULL);
		if (Flags & (GMF_LASTORDEFAULT | GMF_PROTOORGLOBAL))
		{ // we mustn't pass here by GMF_TEMPORARY flag, as otherwise we'll handle GMF_TEMPORARY | GMF_PERSONAL combination incorrectly, which is supposed to get only per-contact messages, and at the same time also may be used with NULL contact to get the global status message
			Message = CProtoSettings(szProto).GetMsgFormat(Flags, pOrder);
		} else if (!hContact)
		{ // handle global temporary message too
			if (g_ProtoStates[szProto].TempMsg.IsSet())
			{
				Message = g_ProtoStates[szProto].TempMsg;
			}
		}
	}
	return Message;
}


void CProtoSettings::SetMsgFormat(int Flags, TCString Message)
{
	if (Flags & (SMF_TEMPORARY | SMF_PERSONAL) && g_AutoreplyOptPage.GetDBValueCopy(IDC_REPLYDLG_RESETCOUNTERWHENSAMEICON) && GetMsgFormat(Flags & (SMF_TEMPORARY | SMF_PERSONAL)) != (const TCHAR*)Message)
	{
		ResetSettingsOnStatusChange(szProto);
	}
	if (Flags & SMF_TEMPORARY)
	{
		_ASSERT(!Status || Status == g_ProtoStates[szProto].Status);
		g_ProtoStates[szProto].TempMsg = (szProto || Message != NULL) ? Message : CProtoSettings(NULL, Status).GetMsgFormat(GMF_LASTORDEFAULT);
	}
	if (Flags & SMF_PERSONAL)
	{ // set a "personal" message for a protocol. also it's used to set global status message (hContact = NULL).
	// if Message == NULL, then we'll use the "default" message - i.e. it's either the global message for szProto != NULL (we delete the per-proto DB setting), or it's just a default message for a given status for szProto == NULL.
		g_ProtoStates[szProto].TempMsg.Unset();
		CString DBSetting(ProtoStatusToDBSetting(DB_STATUSMSG, IDC_MOREOPTDLG_PERSTATUSPROTOMSGS));
		if (Message != NULL)
		{
			DBWriteContactSettingTString(NULL, MOD_NAME, DBSetting, Message);
		} else
		{
			if (!szProto)
			{
				DBWriteContactSettingTString(NULL, MOD_NAME, DBSetting, CProtoSettings(NULL, Status).GetMsgFormat(GMF_LASTORDEFAULT)); // global message can't be NULL; we can use an empty string instead if it's really necessary
			} else
			{
				DBDeleteContactSetting(NULL, MOD_NAME, DBSetting);
			}
		}
	}
	if (Flags & SMF_LAST)
	{
		COptPage MsgTreeData(g_MsgTreePage);
		COptItem_TreeCtrl *TreeCtrl = (COptItem_TreeCtrl*)MsgTreeData.Find(IDV_MSGTREE);
		TreeCtrl->DBToMem(CString(MOD_NAME));
		int RecentGroupID = GetRecentGroupID(Status);
		if (RecentGroupID == -1)
		{ // we didn't find the group, it also means that we're using per status messages; so we need to create it
			TreeCtrl->Value.AddElem(CTreeItem(Status ? (const TCHAR*)CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, Status, GCMDF_TCHAR) : MSGTREE_RECENT_OTHERGROUP, g_Messages_RecentRootID, RecentGroupID = TreeCtrl->GenerateID(), TIF_GROUP));
			TreeCtrl->SetModified(true);
		}
		int I;
		TCString Title(_T(""));
		for (I = 0; I < TreeCtrl->Value.GetSize(); I++) // try to find an identical message in the same group (to prevent saving multiple identical messages), or at least if we'll find an identical message somewhere else, then we'll use its title for our new message
		{
			if (!(TreeCtrl->Value[I].Flags & TIF_GROUP) && TreeCtrl->Value[I].User_Str1 == (const TCHAR*)Message)
			{
				if (TreeCtrl->Value[I].ParentID == RecentGroupID)
				{ // found it in the same group
					int GroupOrder = TreeCtrl->IDToOrder(RecentGroupID);
					TreeCtrl->Value.MoveElem(I, (GroupOrder >= 0) ? (GroupOrder + 1) : 0); // now move it to the top of recent messages list
					TreeCtrl->SetModified(true);
					break; // no reason to search for anything else
				} else if (Title.IsEmpty())
				{ // it's not in the same group, but at least we'll use its title
					Title = TreeCtrl->Value[I].Title;
				}
			}
		}
		if (I == TreeCtrl->Value.GetSize())
		{ // we didn't find an identical message in the same group, so we'll add our new message here
			if (Title.IsEmpty())
			{ // didn't find a title for our message either
				if (Message.GetLen() > MRM_MAX_GENERATED_TITLE_LEN)
				{
					Title = Message.Left(MRM_MAX_GENERATED_TITLE_LEN - 3) + _T("...");
				} else
				{
					Title = Message;
				}
				TCHAR *p = Title.GetBuffer();
				while (*p) // remove "garbage"
				{
					if (!(p = _tcspbrk(p, _T("\r\n\t"))))
					{
						break;
					}
					*p++ = ' ';
				}
				Title.ReleaseBuffer();
			}
			int GroupOrder = TreeCtrl->IDToOrder(RecentGroupID);
			TreeCtrl->Value.InsertElem(CTreeItem(Title, RecentGroupID, TreeCtrl->GenerateID(), 0, Message), (GroupOrder >= 0) ? (GroupOrder + 1) : 0);
			TreeCtrl->SetModified(true);
		}
	// now clean up here
		int MRMNum = 0;
		int MaxMRMNum = g_MoreOptPage.GetDBValueCopy(IDC_MOREOPTDLG_RECENTMSGSCOUNT);
		for (I = 0; I < TreeCtrl->Value.GetSize(); I++)
		{
			if (TreeCtrl->Value[I].ParentID == RecentGroupID)
			{ // found a child of our group
				if (TreeCtrl->Value[I].Flags & TIF_GROUP || ++MRMNum > MaxMRMNum) // what groups are doing here?! :))
				{
					TreeCtrl->Value.RemoveElem(I);
					TreeCtrl->SetModified(true);
					I--;
				}
			}
		}
		if (g_MoreOptPage.GetDBValueCopy(IDC_MOREOPTDLG_PERSTATUSMRM))
		{ // if we're saving recent messages per status, then remove any messages that were left at the recent messages' root
			for (I = 0; I < TreeCtrl->Value.GetSize(); I++)
			{
				if (TreeCtrl->Value[I].ParentID == g_Messages_RecentRootID)
				{
					if (!(TreeCtrl->Value[I].Flags & TIF_GROUP))
					{
						TreeCtrl->Value.RemoveElem(I);
						TreeCtrl->SetModified(true);
						I--;
					}
				}
			}
		}
		TreeCtrl->MemToDB(CString(MOD_NAME));
	}
}


TCString CProtoSettings::GetMsgFormat(int Flags, int *pOrder)
// returns the requested message; sets Order to the order of the message in the message tree, if available; or to -1 otherwise.
{
	TCString Message = NULL;
	if (pOrder)
	{
		*pOrder = -1;
	}
	if (Flags & GMF_TEMPORARY)
	{
		_ASSERT(!Status || Status == g_ProtoStates[szProto].Status);
		if (g_ProtoStates[szProto].TempMsg.IsSet())
		{
			Message = g_ProtoStates[szProto].TempMsg;
			Flags &= ~GMF_PERSONAL; // don't allow personal message to overwrite our NULL temporary message
		}
	}
	if (Flags & GMF_PERSONAL && Message == NULL)
	{ // try getting personal message (it overrides global)
		Message = DBGetContactSettingString(NULL, MOD_NAME, ProtoStatusToDBSetting(DB_STATUSMSG, IDC_MOREOPTDLG_PERSTATUSPROTOMSGS), (TCHAR*)NULL);
	}
	if (Flags & GMF_PROTOORGLOBAL && Message == NULL)
	{
		Message = CProtoSettings().GetMsgFormat(GMF_PERSONAL | (Flags & GMF_TEMPORARY), pOrder);
		return (Message == NULL) ? _T("") : Message; // global message can't be NULL
	}
	if (Flags & GMF_LASTORDEFAULT && Message == NULL)
	{ // try to get the last or default message, depending on current settings
		COptPage MsgTreeData(g_MsgTreePage);
		COptItem_TreeCtrl *TreeCtrl = (COptItem_TreeCtrl*)MsgTreeData.Find(IDV_MSGTREE);
		TreeCtrl->DBToMem(CString(MOD_NAME));
		Message = NULL;
		if (g_MoreOptPage.GetDBValueCopy(IDC_MOREOPTDLG_USELASTMSG)) // if using last message by default...
		{
			Message = DBGetContactSettingString(NULL, MOD_NAME, ProtoStatusToDBSetting(DB_STATUSMSG, IDC_MOREOPTDLG_PERSTATUSPROTOMSGS), (TCHAR*)NULL); // try per-protocol message first
			if (Message.IsEmpty())
			{
				Message = NULL; // to be sure it's NULL, not "" - as we're checking 'Message == NULL' later
				int RecentGroupID = GetRecentGroupID(Status);
				if (RecentGroupID != -1)
				{
					int I;
					for (I = 0; I < TreeCtrl->Value.GetSize(); I++) // find first message in the group
					{
						if (TreeCtrl->Value[I].ParentID == RecentGroupID && !(TreeCtrl->Value[I].Flags & TIF_GROUP))
						{
							Message = TreeCtrl->Value[I].User_Str1;
							if (pOrder)
							{
								*pOrder = I;
							}
							break;
						}
					}
				}
			}
		} // else, if using default message by default...
		if (Message == NULL) // ...or we didn't succeed retrieving the last message
		{ // get default message for this status
			int DefMsgID = -1;
			static struct {
				int DBSetting, Status;
			} DefMsgDlgItems[] = {
				IDS_MESSAGEDLG_DEF_ONL, ID_STATUS_ONLINE,
				IDS_MESSAGEDLG_DEF_AWAY, ID_STATUS_AWAY,
				IDS_MESSAGEDLG_DEF_NA, ID_STATUS_NA,
				IDS_MESSAGEDLG_DEF_OCC, ID_STATUS_OCCUPIED,
				IDS_MESSAGEDLG_DEF_DND, ID_STATUS_DND,
				IDS_MESSAGEDLG_DEF_FFC, ID_STATUS_FREECHAT,
				IDS_MESSAGEDLG_DEF_INV, ID_STATUS_INVISIBLE,
				IDS_MESSAGEDLG_DEF_OTP, ID_STATUS_ONTHEPHONE,
				IDS_MESSAGEDLG_DEF_OTL, ID_STATUS_OUTTOLUNCH
			};
			int I;
			for (I = 0; I < lengthof(DefMsgDlgItems); I++)
			{
				if (DefMsgDlgItems[I].Status == Status)
				{
					DefMsgID = MsgTreeData.GetDBValue(DefMsgDlgItems[I].DBSetting);
					break;
				}
			}
			if (DefMsgID == -1)
			{
				DefMsgID = MsgTreeData.GetDBValue(IDS_MESSAGEDLG_DEF_AWAY); // use away message for unknown statuses
			}
			int Order = TreeCtrl->IDToOrder(DefMsgID); // this will return -1 in any case if something goes wrong
			if (Order >= 0)
			{
				Message = TreeCtrl->Value[Order].User_Str1;
			}
			if (pOrder)
			{
				*pOrder = Order;
			}
		}
		if (Message == NULL)
		{
			Message = _T(""); // last or default message can't be NULL.. otherwise ICQ won't reply to status message requests and won't notify us of status message requests at all
		}
	}
	return Message;
}
