#include "skype_proto.h"
#include <m_chat.h>
#include <m_message.h>
#include <m_history.h>

wchar_t *CSkypeProto::Roles[] = 
{ 
	L"Creator",
	L"Admin",
	L"Speaker",
	L"Writer",
	L"Retried",
	L"Outlaw"
};

#define SKYPE_CHAT_GROUP_OWNER		0
#define SKYPE_CHAT_GROUP_ADMIN		1
#define SKYPE_CHAT_GROUP_SPEAKER	2
#define SKYPE_CHAT_GROUP_WIRTER		3
#define SKYPE_CHAT_GROUP_RETRIED	4
#define SKYPE_CHAT_GROUP_OUTLAW		5

void CSkypeProto::ChatValidateContact(HANDLE hItem, HWND hwndList, const StringList &contacts)
{
	//HANDLE hContact = (HANDLE)::SendMessage(hwndList, CLM_GETNEXTITEM, CLGN_NEXT, (LPARAM)hItem);
	if (this->IsProtoContact(hItem) && !this->IsChatRoom(hItem))
	{
		mir_ptr<wchar_t> sid( ::db_get_wsa(hItem, this->m_szModuleName, SKYPE_SETTINGS_LOGIN));
		if (sid == NULL || contacts.contains(sid))
			::SendMessage(hwndList, CLM_DELETEITEM, (WPARAM)hItem, 0);
	}
	else
		::SendMessage(hwndList, CLM_DELETEITEM, (WPARAM)hItem, 0);
}

void CSkypeProto::ChatPrepare(HANDLE hItem, HWND hwndList, const StringList &contacts)
{
	if (hItem == NULL)
		hItem = (HANDLE)::SendMessage(hwndList, CLM_GETNEXTITEM, CLGN_ROOT, 0);

	while (hItem) 
	{
		HANDLE hItemN = (HANDLE)::SendMessage(hwndList, CLM_GETNEXTITEM, CLGN_NEXT, (LPARAM)hItem);

		if (IsHContactGroup(hItem))
		{
			HANDLE hItemT = (HANDLE)::SendMessage(hwndList, CLM_GETNEXTITEM, CLGN_CHILD, (LPARAM)hItem);
			if (hItemT) this->ChatPrepare(hItemT, hwndList, contacts);
		}
		else if (IsHContactContact(hItem))
			this->ChatValidateContact(hItem, hwndList, contacts);

		hItem = hItemN;
	}
}

void CSkypeProto::GetInvitedContacts(HANDLE hItem, HWND hwndList, StringList &chatTargets)
{
	if (hItem == NULL)
		hItem = (HANDLE)::SendMessage(hwndList, CLM_GETNEXTITEM, CLGN_ROOT, 0);

	while (hItem)
	{
		if (IsHContactGroup(hItem))
		{
			HANDLE hItemT = (HANDLE)SendMessage(hwndList, CLM_GETNEXTITEM, CLGN_CHILD, (LPARAM)hItem);
			if (hItemT)
				this->GetInvitedContacts(hItemT, hwndList, chatTargets);
		}
		else
		{
			int chk = SendMessage(hwndList, CLM_GETCHECKMARK, (WPARAM)hItem, 0);
			if (chk)
			{
				if (IsHContactInfo(hItem))
				{
					TCHAR buf[128] = _T("");
					SendMessage(hwndList, CLM_GETITEMTEXT, (WPARAM)hItem, (LPARAM)buf);

					if (buf[0]) chatTargets.insert(buf);
				}
				else
				{
					mir_ptr<wchar_t> login( ::db_get_wsa(hItem, this->m_szModuleName, SKYPE_SETTINGS_LOGIN));
					chatTargets.insert(login);
				}
			}
		}
		hItem = (HANDLE)SendMessage(hwndList, CLM_GETNEXTITEM, CLGN_NEXT, (LPARAM)hItem);
	}
}

INT_PTR CALLBACK CSkypeProto::InviteToChatProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	InviteChatParam *param = (InviteChatParam *)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);

	switch (msg) {
	case WM_INITDIALOG:
		TranslateDialogDefault(hwndDlg);

		SetWindowLongPtr(hwndDlg, GWLP_USERDATA, lParam);
		param = (InviteChatParam *)lParam;
		{
			HWND hwndClist = GetDlgItem(hwndDlg, IDC_CCLIST);
			SetWindowLongPtr(hwndClist, GWL_STYLE, GetWindowLongPtr(hwndClist, GWL_STYLE) & ~CLS_HIDEOFFLINE);
		}
		break;

	case WM_CLOSE:
		::EndDialog(hwndDlg, 0);
		break;

	case WM_NOTIFY:
		{
			NMCLISTCONTROL *nmc = (NMCLISTCONTROL *)lParam;
			if (nmc->hdr.idFrom == IDC_CCLIST)
			{
				switch (nmc->hdr.code)
				{
				case CLN_NEWCONTACT:
					if (param && (nmc->flags & (CLNF_ISGROUP | CLNF_ISINFO)) == 0)
					{
						param->ppro->ChatValidateContact(nmc->hItem, nmc->hdr.hwndFrom, param->invitedContacts);
					}
					break;

				case CLN_LISTREBUILT:
					if (param)
					{
						param->ppro->ChatPrepare(NULL, nmc->hdr.hwndFrom, param->invitedContacts);
					}
					break;
				}
			}
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_ADDSCR:
			if (param->ppro->IsOnline())
			{
				wchar_t sid[SKYPE_SID_LIMIT];
				::GetDlgItemText(hwndDlg, IDC_EDITSCR, sid, SIZEOF(sid));

				CLCINFOITEM cii = {0};
				cii.cbSize = sizeof(cii);
				cii.flags = CLCIIF_CHECKBOX | CLCIIF_BELOWCONTACTS;
				cii.pszText = ::wcslwr(sid);

				HANDLE hItem = (HANDLE)::SendDlgItemMessage(
					hwndDlg,
					IDC_CCLIST,
					CLM_ADDINFOITEM,
					0,
					(LPARAM)&cii);
				::SendDlgItemMessage(hwndDlg, IDC_CCLIST, CLM_SETCHECKMARK, (LPARAM)hItem, 1);
			}
			break;

		case IDOK:
			{
				HWND hwndList = ::GetDlgItem(hwndDlg, IDC_CCLIST);

				param->invitedContacts.clear();
				param->ppro->GetInvitedContacts(NULL, hwndList, param->invitedContacts);

				if ( !param->invitedContacts.empty())
				{
					//SetWindowLongPtr(hwndDlg, 0, (LONG_PTR)bct);
					SetWindowLongPtr(hwndDlg, GWLP_USERDATA, lParam);
					::EndDialog(hwndDlg, IDOK);
				}
				else
					param->ppro->ShowNotification(::TranslateT("You did not select any contact"));
			}
			break;

		case IDCANCEL:
			::EndDialog(hwndDlg, IDCANCEL);
			break;
		}
		break;
	}
	return FALSE;
}

bool CSkypeProto::IsChatRoom(HANDLE hContact)
{
	return ::db_get_b(hContact, this->m_szModuleName, "ChatRoom", 0) > 0;
}

HANDLE CSkypeProto::GetChatRoomByCid(const wchar_t *cid)
{
	HANDLE hContact = NULL;

	::EnterCriticalSection(&this->contact_search_lock);

	for (hContact = ::db_find_first(this->m_szModuleName); hContact; hContact = ::db_find_next(hContact, this->m_szModuleName))
	{
		if  (this->IsChatRoom(hContact))
		{
			mir_ptr<wchar_t> chatID(::db_get_wsa(hContact, this->m_szModuleName, "ChatRoomID"));
			if (::lstrcmp(cid, chatID) == 0)
				break;
		}
	}

	::LeaveCriticalSection(&this->contact_search_lock);

	return hContact;
}

HANDLE CSkypeProto::AddChatRoom(CConversation::Ref conversation)
{
	SEString data;

	conversation->GetPropIdentity(data);
	mir_ptr<wchar_t> cid = ::mir_utf8decodeW(data);

	HANDLE hContact = this->GetChatRoomByCid(cid);
	if ( !hContact)
	{
		hContact = (HANDLE)::CallService(MS_DB_CONTACT_ADD, 0, 0);
		::CallService(MS_PROTO_ADDTOCONTACT, (WPARAM)hContact, (LPARAM)this->m_szModuleName);

		conversation->GetPropDisplayname(data);
		mir_ptr<wchar_t> name = ::mir_utf8decodeW(data);

		::db_set_b(hContact, this->m_szModuleName, "ChatRoom", 1);
		::db_set_ws(hContact, this->m_szModuleName, "ChatRoomID", cid);
		::db_set_ws(hContact, this->m_szModuleName, "Nick", name);
		::db_set_w(hContact, this->m_szModuleName, "Status", ID_STATUS_OFFLINE);
		::db_set_w(hContact, this->m_szModuleName, "ApparentMode", ID_STATUS_OFFLINE);
		
		mir_ptr<wchar_t> defaultGroup = ::db_get_wsa(NULL, "Chat", "AddToGroup");
		if (defaultGroup)
		{
			::db_set_ws(hContact, "CList", "Group", defaultGroup);
		}
	}

	return hContact;
}

static const COLORREF crCols[16] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};

void CSkypeProto::InitChat()
{
	GCREGISTER gcr = {0};
	gcr.cbSize = sizeof(gcr);
	gcr.dwFlags = GC_TCHAR | GC_TYPNOTIF | GC_CHANMGR;
	gcr.iMaxText = 0;
	gcr.nColors = 16;
	gcr.pColors = (COLORREF*)crCols;
	gcr.ptszModuleDispName = this->m_tszUserName;
	gcr.pszModule = this->m_szModuleName;
	::CallServiceSync(MS_GC_REGISTER, 0, (LPARAM)&gcr);

	this->HookEvent(ME_GC_EVENT, &CSkypeProto::OnGCEventHook);
	this->HookEvent(ME_GC_BUILDMENU, &CSkypeProto::OnGCMenuHook);
}

void CSkypeProto::CreateChatWindow(CConversation::Ref conversation, bool showWindow)
{
	SEString data;

	conversation->GetPropIdentity(data);
	mir_ptr<wchar_t> cid( ::mir_utf8decodeW(data));

	conversation->GetPropDisplayname(data);
	mir_ptr<wchar_t> chatName( ::mir_utf8decodeW(data));

	GCSESSION gcw = {0};
	gcw.cbSize = sizeof(gcw);
	gcw.iType = GCW_CHATROOM;
	gcw.dwFlags = GC_TCHAR;
	gcw.pszModule = this->m_szModuleName;
	gcw.ptszName = chatName;
	gcw.ptszID = cid;
	::CallServiceSync(MS_GC_NEWSESSION, 0, (LPARAM)&gcw);

	GCDEST gcd = { m_szModuleName, { NULL }, GC_EVENT_ADDGROUP };
	gcd.ptszID = cid;

	GCEVENT gce = {0};
	gce.cbSize = sizeof(GCEVENT);
	gce.dwFlags = GC_TCHAR;
	gce.pDest = &gcd;
	for (int i = 0; i < SIZEOF(CSkypeProto::Roles); i++)
	{
		gce.ptszStatus =:: TranslateW(CSkypeProto::Roles[i]);
		::CallServiceSync(MS_GC_EVENT, NULL, (LPARAM)&gce);
	}

	gcd.iType = GC_EVENT_CONTROL;
	::CallServiceSync(MS_GC_EVENT, showWindow ? SESSION_INITDONE : WINDOW_HIDDEN, (LPARAM)&gce);
	::CallServiceSync(MS_GC_EVENT, SESSION_ONLINE, (LPARAM)&gce);
}

CConversation::Ref CSkypeProto::StartChat(StringList &invitedContacts)
{
	InviteChatParam *param = new InviteChatParam(NULL, invitedContacts, this);

	SEStringList needToAdd;
	for (size_t i = 0; i < param->invitedContacts.size(); i++)
		needToAdd.append(std::string(::mir_utf8encodeW(param->invitedContacts[i])).c_str());
	
	if (::DialogBoxParam(
		g_hInstance, 
		MAKEINTRESOURCE(IDD_CHATROOM_INVITE), 
		NULL, 
		CSkypeProto::InviteToChatProc, 
		(LPARAM)param) == IDOK)
	{
		for (size_t i = 0; i < param->invitedContacts.size(); i++)
		{
			std::string sid(::mir_utf8encodeW(param->invitedContacts[i]));
			if ( !needToAdd.contains(sid.c_str()))
				needToAdd.append(sid.c_str());
		}

		CConversation::Ref conversation;
		if (g_skype->CreateConferenceWithConsumers(conversation, needToAdd))
		{
			delete param;

			return conversation;
		}
	}

	delete param;

	return NULL;
}

CConversation::Ref CSkypeProto::StartChat()
{
	StringList empty;
	return this->StartChat(empty);
}

void CSkypeProto::InviteConactsToChat(CConversation::Ref conversation, const StringList &invitedContacts)
{
	SEString data;

	conversation->GetPropIdentity(data);
	mir_ptr<wchar_t> cid( ::mir_utf8decodeW(data));

	InviteChatParam *param = new InviteChatParam(cid, invitedContacts, this);
	
	if (::DialogBoxParam(
		g_hInstance, 
		MAKEINTRESOURCE(IDD_CHATROOM_INVITE), 
		NULL, 
		CSkypeProto::InviteToChatProc, 
		(LPARAM)param) == IDOK)
	{
		SEStringList needToAdd;
		for (size_t i = 0; i < param->invitedContacts.size(); i++)
			needToAdd.append(std::string(::mir_utf8encodeW(param->invitedContacts[i])).c_str());

		conversation->AddConsumers(needToAdd);
	}

	delete param;
}

void CSkypeProto::JoinToChat(CConversation::Ref conversation, bool showWindow)
{
	conversation->Join();

	SEString data;

	conversation->GetPropIdentity(data);
	mir_ptr<wchar_t> cid( ::mir_utf8decodeW(data));

	this->CreateChatWindow(conversation, showWindow);

	CParticipant::Refs participants;
	conversation->GetParticipants(participants, CConversation::OTHER_CONSUMERS);
	for (uint i = 0; i < participants.size(); i++)
	{
		participants[i]->GetPropIdentity(data);
		std::wstring sid = ::mir_utf8decodeW(data);

		CParticipant::RANK rank;
		participants[i]->GetPropRank(rank);

		CContact::Ref contact;
		g_skype->GetContact(std::string(::mir_utf8encodeW(sid.c_str())).c_str(), contact);

		auto status = Contact::OFFLINE;
		contact->GetPropAvailability(status);

		this->AddChatContact(
			cid,
			sid.c_str(),
			CSkypeProto::Roles[rank],
			CSkypeProto::SkypeToMirandaStatus(status));
	}
}

void CSkypeProto::AddConactsToChat(CConversation::Ref conversation, const StringList &invitedContacts)
{
	SEString data;

	conversation->GetPropIdentity(data);
	wchar_t *cid = ::mir_utf8decodeW(data);

	StringList alreadyInChat(this->GetChatUsers(cid));

	CParticipant::Refs participants;
	conversation->GetParticipants(participants, CConversation::OTHER_CONSUMERS);
	for (uint i = 0; i < participants.size(); i++)
	{
		participants[i]->GetPropIdentity(data);
		mir_ptr<wchar_t> sid = ::mir_utf8decodeW(data);
					
		if (invitedContacts.contains(sid) && !alreadyInChat.contains(sid))
		{
			CContact::Ref contact;
			g_skype->GetContact((char *)mir_ptr<char>(::mir_utf8encodeW(sid)), contact);

			CContact::AVAILABILITY status;
			contact->GetPropAvailability(status);

			CParticipant::RANK rank;
			participants[i]->GetPropRank(rank);

			this->AddChatContact(
				cid, 
				sid, 
				CSkypeProto::Roles[rank],
				status);
		}
	}
}

void CSkypeProto::LeaveChat(const wchar_t *cid)
{
	mir_ptr<wchar_t> chatID( ::mir_wstrdup(cid));

	GCDEST gcd = { m_szModuleName, { NULL }, GC_EVENT_CONTROL };
	gcd.ptszID = chatID;

	GCEVENT gce = {0};
	gce.cbSize = sizeof(GCEVENT);
	gce.dwFlags = GC_TCHAR;
	gce.pDest = &gcd;
	::CallServiceSync(MS_GC_EVENT, SESSION_OFFLINE, (LPARAM)&gce);
	::CallServiceSync(MS_GC_EVENT, SESSION_TERMINATE, (LPARAM)&gce);
}

void CSkypeProto::RaiseChatEvent(const wchar_t *cid, const wchar_t *sid, int evt, DWORD flags, DWORD itemData, const wchar_t *status, const wchar_t *message)
{
	HANDLE hContact = this->GetContactBySid(sid);
	mir_ptr<wchar_t> nick(::db_get_wsa(hContact, this->m_szModuleName, "Nick"));
	if (!nick)
		nick = ::mir_wstrdup(sid);
	/*mir_ptr<wchar_t> nick( hContact ? 
		::mir_a2u((char *)::CallService(MS_CLIST_GETCONTACTDISPLAYNAME, WPARAM(hContact), 0)) : 
		::mir_wstrdup(sid));*/

	GCDEST gcd = { this->m_szModuleName, { NULL }, evt };
	gcd.ptszID = ::mir_wstrdup(cid);

	GCEVENT gce = {0};
	gce.cbSize = sizeof(gce);
	gce.dwFlags = flags | GC_TCHAR;
	gce.pDest = &gcd;
	gce.ptszNick = nick;
	gce.ptszUID = sid;
	gce.bIsMe = ::wcsicmp(sid, this->login) == 0;
	gce.dwItemData = itemData;
	gce.ptszStatus = status;
	gce.ptszText = message;
	gce.time = time(NULL);
	::CallServiceSync(MS_GC_EVENT, 0, (LPARAM)&gce);
}

void CSkypeProto::SendChatMessage(const wchar_t *cid, const wchar_t *sid, const wchar_t *message)
{
	this->RaiseChatEvent(cid, sid, GC_EVENT_MESSAGE, GCEF_ADDTOLOG, 0, NULL, message);
}

void CSkypeProto::AddChatContact(const wchar_t *cid, const wchar_t *sid, const wchar_t *group, const WORD status)
{
	this->RaiseChatEvent(cid, sid, GC_EVENT_JOIN, 0, 0, CSkypeProto::Roles[SKYPE_CHAT_GROUP_WIRTER]);
	this->RaiseChatEvent(cid, sid, GC_EVENT_SETCONTACTSTATUS, 0, status);
}

void CSkypeProto::KickChatContact(const wchar_t *cid, const wchar_t *sid)
{
	this->RaiseChatEvent(cid, sid, GC_EVENT_KICK, GCEF_ADDTOLOG);
}

void CSkypeProto::RemoveChatContact(const wchar_t *cid, const wchar_t *sid)
{
	this->RaiseChatEvent(cid, sid, GC_EVENT_QUIT, GCEF_ADDTOLOG);
}

INT_PTR __cdecl CSkypeProto::OnJoinChat(WPARAM wParam, LPARAM)
{
	HANDLE hContact = (HANDLE)wParam;
	if (hContact)
	{
		mir_ptr<wchar_t> cid( ::db_get_wsa(hContact, this->m_szModuleName, "ChatRoomID"));

		SEString data;
		CConversation::Ref conversation;

		//todo: fixme
		g_skype->GetConversationByIdentity(::mir_utf8encodeW(cid), conversation);
		conversation->GetJoinBlob(data);
		g_skype->GetConversationByBlob(data, conversation, false);
		conversation->Join();

		this->JoinToChat(conversation);
	}

	return 0;
}

INT_PTR __cdecl CSkypeProto::OnLeaveChat(WPARAM wParam, LPARAM)
{
	HANDLE hContact = (HANDLE)wParam;
	if (hContact)
		this->LeaveChat( mir_ptr<wchar_t>( ::db_get_wsa(hContact, this->m_szModuleName, "ChatRoomID")));

	return 0;
}

int __cdecl CSkypeProto::OnGCEventHook(WPARAM, LPARAM lParam)
{
	GCHOOK *gch = (GCHOOK *)lParam;
	if (!gch) return 1;

	if (::strcmp(gch->pDest->pszModule, this->m_szModuleName))
		return 0;

	mir_ptr<wchar_t> cid( ::mir_wstrdup(gch->pDest->ptszID));
	mir_ptr<wchar_t> sid( ::mir_wstrdup(gch->ptszUID));

	CConversation::Ref conversation;
	switch (gch->pDest->iType) {
	case GC_SESSION_TERMINATE:
		if (g_skype->GetConversationByIdentity(::mir_utf8encodeW(cid), conversation, false))
		{
			Participant::Refs participants;
			conversation->GetParticipants(participants, CConversation::MYSELF);
			participants[0]->Retire();
		}
		break;

	case GC_USER_MESSAGE:
		if (gch->ptszText && gch->ptszText[0])
		{
			if (g_skype->GetConversationByIdentity(::mir_utf8encodeW(cid), conversation, false))
			{
				CMessage::Ref message;
				mir_ptr<char> text(::mir_utf8encodeW(gch->ptszText));
				conversation->PostText((char *)text, message);
			}
		}
		break;

	/*case GC_USER_CHANMGR:
		if (g_skype->GetConversationByIdentity(::mir_utf8encodeW(cid), conversation, false))
		{
			StringList invitedContacts(this->GetChatUsers(cid));
			this->InviteConactsToChat(conversation, invitedContacts); 
		}
		break;*/

	case GC_USER_PRIVMESS:
		::CallService(MS_MSG_SENDMESSAGE, (WPARAM)this->GetContactBySid(sid), 0);
		break;

	case GC_USER_LOGMENU:
		switch(gch->dwData) {
		case 10:
			if (g_skype->GetConversationByIdentity(::mir_utf8encodeW(cid), conversation, false))
			{
				StringList invitedContacts(this->GetChatUsers(cid));
				this->InviteConactsToChat(conversation, invitedContacts);
			}
			break;

		case 20:
			this->LeaveChat(cid);
			break;
		}
		break;

	case GC_USER_NICKLISTMENU:
		switch (gch->dwData) {
		case 10:
			::CallService(MS_USERINFO_SHOWDIALOG, (WPARAM)this->GetContactBySid(sid), 0);
			break;

		case 20:
			CallService(MS_HISTORY_SHOWCONTACTHISTORY, (WPARAM)this->GetContactBySid(sid), 0);
			break;

		case 110:
			this->LeaveChat(cid);
			break;
		}
		break;

	case GC_USER_TYPNOTIFY:
		break;
	}
	return 0;
}

int __cdecl CSkypeProto::OnGCMenuHook(WPARAM, LPARAM lParam)
{
	GCMENUITEMS *gcmi = (GCMENUITEMS*) lParam;

	if (gcmi == NULL || ::stricmp(gcmi->pszModule, this->m_szModuleName))
		return 0;

	if (gcmi->Type == MENU_ON_LOG)
	{
		static const struct gc_item Items[] = 
		{
			{ TranslateT("Invite to conference"), 10, MENU_ITEM, FALSE },
			{ TranslateT("&Leave chat session"), 20, MENU_ITEM, FALSE }
		};
		gcmi->nItems = SIZEOF(Items);
		gcmi->Item = (gc_item*)Items;
	}
	else if (gcmi->Type == MENU_ON_NICKLIST)
	{
		static const struct gc_item Items[] = 
		{
			{ TranslateT("User &details"), 10, MENU_ITEM, FALSE },
			{ TranslateT("User &history"), 20, MENU_ITEM, FALSE },
		};
		gcmi->nItems = SIZEOF(Items);
		gcmi->Item = (gc_item*)Items;
	}

	return 0;
}

wchar_t *CSkypeProto::GetChatUsers(const wchar_t *cid)
{
	GC_INFO gci = {0};
	gci.Flags = USERS;
	gci.pszModule = this->m_szModuleName;
	gci.pszID = ::mir_wstrdup(cid);
	::CallService(MS_GC_GETINFO, 0, (LPARAM)(GC_INFO *) &gci);

	::mir_free(gci.pszID);

	// todo: fix me
	return ::mir_a2u(gci.pszUsers);
}

void CSkypeProto::UpdateChatUserStatus(CContact::Ref contact)
{
	CContact::AVAILABILITY availability;
	contact->GetPropAvailability(availability);

	SEString identity;
	contact->GetIdentity(identity);
	mir_ptr<wchar_t> sid( ::mir_utf8decodeW(identity));

	GC_INFO gci = {0};
	gci.Flags = BYINDEX | ID;
	gci.pszModule = this->m_szModuleName;

	int count = ::CallServiceSync(MS_GC_GETSESSIONCOUNT, 0, (LPARAM)this->m_szModuleName);
	for (int i = 0; i < count ; i++)
	{
		gci.iItem = i;
		::CallServiceSync(MS_GC_GETINFO, 0, (LPARAM)&gci);

		this->RaiseChatEvent(
			gci.pszID,
			sid,
			GC_EVENT_SETCONTACTSTATUS,
			0,
			CSkypeProto::SkypeToMirandaStatus(availability));
	}
}