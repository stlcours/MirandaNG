﻿#include "skype_proto.h"

CSkypeProto::CSkypeProto(const char* protoName, const TCHAR* userName) : 
	instanceHookList(1),
	instanceServiceList(1)
{
	::ProtoConstructor(this, protoName, userName);

	this->rememberPassword = false;

	this->SetAllContactStatus(ID_STATUS_OFFLINE);

	DBEVENTTYPEDESCR dbEventType = { sizeof(dbEventType) };
	dbEventType.module = m_szModuleName;
	dbEventType.eventType = SKYPE_DB_EVENT_TYPE_CALL;
	dbEventType.descr = "Skype call";
	::CallService(MS_DB_EVENT_REGISTERTYPE, 0, (LPARAM)&dbEventType);

	this->HookEvent(ME_MSG_PRECREATEEVENT, &CSkypeProto::OnMessagePreCreate);

	this->InitInstanceServiceList();
}

CSkypeProto::~CSkypeProto()
{
	::mir_free(this->login);
	if (this->password)
	{
		::mir_free(this->password);
		this->password = NULL;
	}

	this->UninitInstanceServiceList();

	::ProtoDestructor(this);
}

HANDLE __cdecl CSkypeProto::AddToList(int flags, PROTOSEARCHRESULT* psr)
{
	//fixme
	CContact::Ref contact;
	g_skype->GetContact(::mir_u2a(psr->id), contact);
	return this->AddContact(contact);
	return 0;
}

HANDLE __cdecl CSkypeProto::AddToListByEvent(int flags, int iContact, HANDLE hDbEvent)
{
	DBEVENTINFO dbei = {0};
	dbei.cbSize = sizeof(dbei);

	/*if ((dbei.cbBlob = CallService(MS_DB_EVENT_GETBLOBSIZE, (WPARAM)hDbEvent, 0)) != -1)
	{
		dbei.pBlob = (PBYTE)alloca(dbei.cbBlob);
		if (CallService(MS_DB_EVENT_GET, (WPARAM)hDbEvent, (LPARAM)&dbei) == 0 &&
				!strcmp(dbei.szModule, m_szModuleName) &&
				(dbei.eventType == EVENTTYPE_AUTHREQUEST || dbei.eventType == EVENTTYPE_CONTACTS))
		{
			char *nick = (char*)(dbei.pBlob + sizeof(DWORD) * 2);
			char *firstName = nick + strlen(nick) + 1;
			char *lastName = firstName + strlen(firstName) + 1;
			char *skypeName = lastName + strlen(lastName) + 1;
			return AddContactBySkypeName(::mir_a2u(skypeName), ::mir_a2u(nick), 0);
		}
	}*/
	return 0;
}

int __cdecl CSkypeProto::Authorize(HANDLE hDbEvent)
{
	if (this->IsOnline() && hDbEvent)
	{
		HANDLE hContact = this->GetContactFromAuthEvent(hDbEvent);
		if (hContact == INVALID_HANDLE_VALUE)
			return 1;

		return CSkypeProto::GrantAuth((WPARAM)hContact, NULL);
	}

	return 1;
}

int __cdecl CSkypeProto::AuthDeny(HANDLE hDbEvent, const TCHAR* szReason)
{
	if (this->IsOnline())
	{
		HANDLE hContact = this->GetContactFromAuthEvent(hDbEvent);
		if (hContact == INVALID_HANDLE_VALUE)
			return 1;

		return CSkypeProto::RevokeAuth((WPARAM)hContact, NULL);
	}

	return 1;
}

int __cdecl CSkypeProto::AuthRecv(HANDLE hContact, PROTORECVEVENT* pre)
{
	DWORD flags = 0;

	if (pre->flags & PREF_CREATEREAD)
		flags |= DBEF_READ;

	if (pre->flags & PREF_UTF)
		flags |= DBEF_UTF;

	this->AddDBEvent(
		hContact,
		EVENTTYPE_AUTHREQUEST,
		pre->timestamp,
		flags,
		pre->lParam,
		(PBYTE)pre->szMessage);

	return 0;
}

int __cdecl CSkypeProto::AuthRequest(HANDLE hContact, const TCHAR* szMessage)
{
	if (this->IsOnline() && hContact)
	{
		CContact::Ref contact;
		SEString sid(::mir_u2a(::db_get_wsa(hContact, this->m_szModuleName, SKYPE_SETTINGS_LOGIN)));
		if (g_skype->GetContact(sid, contact))
		{
			contact->SetBuddyStatus(Contact::AUTHORIZED_BY_ME);
			contact->SendAuthRequest(::mir_utf8encodeW(szMessage));
		}

		return 0;
	}

	return 1;
}

HANDLE __cdecl CSkypeProto::ChangeInfo( int iInfoType, void* pInfoData ) { return 0; }

HANDLE __cdecl CSkypeProto::FileAllow( HANDLE hContact, HANDLE hTransfer, const TCHAR* szPath ) 
{ 
	uint oid = (uint)hTransfer;

	CMessage *message = new CMessage(oid, g_skype);

	CTransfer::Refs transfers;
	message->GetTransfers(transfers);
	for (uint i = 0; i < transfers.size(); i++)
	{
		bool success;
		SEString name;
		wchar_t fullPath[MAX_PATH] = {0};
		transfers[i]->GetPropFilename(name);
		::mir_sntprintf(fullPath, MAX_PATH, L"%s%s", szPath, ::mir_utf8decodeW(name));
		if (!transfers[i]->Accept(::mir_u2a(fullPath), success) || !success)
		{
			// todo: write to log!
			delete message;
			return 0;
		}
	}
	
	delete message;

	return hTransfer; 
}

int    __cdecl CSkypeProto::FileCancel( HANDLE hContact, HANDLE hTransfer ) 
{
	uint oid = (uint)hTransfer;

	MessageRef message(oid);

	CTransfer::Refs transfers;
	message->GetTransfers(transfers);
	for (uint i = 0; i < transfers.size(); i++)
	{
		if (!transfers[i]->Cancel())
		{
			// todo: write to log!
			return 0;
		}
	}

	return 1; 
}

int    __cdecl CSkypeProto::FileDeny( HANDLE hContact, HANDLE hTransfer, const TCHAR* szReason ) 
{ 
	uint oid = (uint)hTransfer;

	MessageRef message(oid);

	CTransfer::Refs transfers;
	message->GetTransfers(transfers);
	for (uint i = 0; i < transfers.size(); i++)
	{
		if (!transfers[i]->Cancel())
		{
			// todo: write to log!
			return 0;
		}
	}

	return 1; 
}

int    __cdecl CSkypeProto::FileResume( HANDLE hTransfer, int* action, const TCHAR** szFilename ) 
{ 
	return 0; 
}

DWORD_PTR __cdecl CSkypeProto:: GetCaps(int type, HANDLE hContact)
{
	switch(type)
	{
	case PFLAGNUM_1:
		return PF1_IM | PF1_FILE | PF1_BASICSEARCH | PF1_ADDSEARCHRES | PF1_SEARCHBYEMAIL/* | PF1_SEARCHBYNAME*/;
	case PFLAGNUM_2:
	case PFLAGNUM_3:
		return PF2_ONLINE | PF2_SHORTAWAY | PF2_HEAVYDND | PF2_INVISIBLE;
	case PFLAGNUM_4:
		return PF4_FORCEAUTH | PF4_FORCEADDED | PF4_SUPPORTTYPING | PF4_AVATARS |
			PF4_OFFLINEFILES | PF4_IMSENDUTF | PF4_IMSENDOFFLINE;
	case PFLAG_UNIQUEIDTEXT:
		return (DWORD_PTR)::Translate("g_skype Name");
	case PFLAG_UNIQUEIDSETTING:
		return (DWORD_PTR)SKYPE_SETTINGS_LOGIN;
	default:
		return 0;
	}
}

int    __cdecl CSkypeProto::GetInfo( HANDLE hContact, int infoType ) { return 0; }

HANDLE __cdecl CSkypeProto::SearchBasic(const TCHAR* id)
{
	if ( !this->IsOnline())
		return 0;

	this->ForkThread(&CSkypeProto::SearchBySidAsync, ::mir_tstrdup(id));

	return (HANDLE)SKYPE_SEARCH_BYSID;
}

HANDLE __cdecl CSkypeProto::SearchByEmail(const TCHAR* email)
{
	if ( !this->IsOnline())
		return 0;

	this->ForkThread(&CSkypeProto::SearchByEmailAsync, ::mir_tstrdup(email));

	return (HANDLE)SKYPE_SEARCH_BYEMAIL;
}

HANDLE __cdecl CSkypeProto::SearchByName(const TCHAR* nick, const TCHAR* firstName, const TCHAR* lastName)
{
	PROTOSEARCHRESULT psr = {0};
	psr.cbSize = sizeof(psr);
	psr.flags = PSR_TCHAR;
	psr.nick = ::mir_wstrdup(nick);
	psr.firstName = ::mir_wstrdup(firstName);
	psr.lastName = ::mir_wstrdup(lastName);

	this->ForkThread(&CSkypeProto::SearchByNamesAsync, &psr);

	return (HANDLE)SKYPE_SEARCH_BYNAMES;
}

HWND   __cdecl CSkypeProto::SearchAdvanced( HWND owner ) { return 0; }

HWND   __cdecl CSkypeProto::CreateExtendedSearchUI( HWND owner ){ return 0; }

int    __cdecl CSkypeProto::RecvContacts( HANDLE hContact, PROTORECVEVENT* ) { return 0; }

int    __cdecl CSkypeProto::RecvFile( HANDLE hContact, PROTORECVFILET* evt) 
{ 
	::db_unset(hContact, "CList", "Hidden");
	return ::Proto_RecvFile(hContact, evt);
}

int __cdecl CSkypeProto::RecvMsg(HANDLE hContact, PROTORECVEVENT* pre)
{
	::db_unset(hContact, "CList", "Hidden");
	this->UserIsTyping(hContact, PROTOTYPE_SELFTYPING_OFF);

	char *guid = (char *)pre->lParam;
	int guidLen = (int)::strlen(guid);

	char *message = (char *)pre->szMessage;
	int msgLen = (int)::strlen(message) + 1;

	message = (char *)::mir_realloc(message, msgLen + guidLen);
	::memcpy((char *)&message[msgLen], guid, guidLen);

	return (INT_PTR)this->AddDBEvent(
		hContact,
		EVENTTYPE_MESSAGE,
		pre->timestamp,
		DBEF_UTF | ((pre->flags & PREF_CREATEREAD) ? DBEF_READ : 0),
		msgLen + guidLen,
		(PBYTE)message);

	//return ::Proto_RecvMessage(hContact, pre);
}

int    __cdecl CSkypeProto::RecvUrl( HANDLE hContact, PROTORECVEVENT* ) { return 0; }

int    __cdecl CSkypeProto::SendContacts( HANDLE hContact, int flags, int nContacts, HANDLE* hContactsList ) { return 0; }

HANDLE __cdecl CSkypeProto::SendFile( HANDLE hContact, const TCHAR* szDescription, TCHAR** ppszFiles ) 
{
	if (this->IsOnline() && hContact && ppszFiles)
	{
		SEStringList targets;
		mir_ptr<wchar_t> sid(::db_get_wsa(hContact, this->m_szModuleName, SKYPE_SETTINGS_LOGIN));
		targets.append((char *)mir_ptr<char>(::mir_utf8encodeW(sid)));

		CConversation::Ref conversation;
		g_skype->GetConversationByParticipants(targets, conversation);

		SEFilenameList fileList;
		for (int i = 0; ppszFiles[i]; i++)
			fileList.append((char *)mir_ptr<char>(::mir_utf8encodeW(ppszFiles[i])));

		auto error = TRANSFER_OPEN_SUCCESS;
		SEFilename errFile; MessageRef msgRef;
		//(char *)mir_ptr<char>(::mir_utf8encodeW(szDescription))
		if ( !conversation->PostFiles(fileList, " ", error, errFile, msgRef) || error)
			return 0;

		CTransfer::Refs transfers;
		if (msgRef->GetTransfers(transfers))
		{
			for (uint i = 0; i < transfers.size(); i++)
			{
				auto transfer = transfers[i];
				transfer.fetch();
				transfer->SetOnTransferCallback((CTransfer::OnTransfer)&CSkypeProto::OnTransferChanged, this);
				this->transferList.append(transfer);
			}
		}

		return (HANDLE)msgRef->getOID();
	}

	return 0; 
}

int    __cdecl CSkypeProto::SendMsg(HANDLE hContact, int flags, const char* msg)
{
	SEStringList targets;
	wchar_t * sid = ::db_get_wsa(hContact, this->m_szModuleName, SKYPE_SETTINGS_LOGIN);
	SEString identity = ::mir_u2a(sid);
	targets.append(identity);

	CConversation::Ref conversation;
	g_skype->GetConversationByParticipants(targets, conversation);

	if (conversation)
	{
		CMessage::Ref message;
		if (!conversation->PostText(msg, message))
			return 0;

		return message->getOID();
	}

	return 0;
}

int    __cdecl CSkypeProto::SendUrl( HANDLE hContact, int flags, const char* url ) { return 0; }

int    __cdecl CSkypeProto::SetApparentMode( HANDLE hContact, int mode ) { return 0; }

int CSkypeProto::SetStatus(int new_status)
{
	if (new_status == this->m_iDesiredStatus)
		return 0;

	int old_status = this->m_iStatus;
	this->m_iDesiredStatus = new_status;

	if (new_status == ID_STATUS_OFFLINE)
	{
		this->LogOut();
		this->m_iStatus = this->m_iDesiredStatus = ID_STATUS_OFFLINE;
	}
	else
	{
		if (old_status == ID_STATUS_OFFLINE && !this->IsOnline())
		{
			this->m_iStatus = ID_STATUS_CONNECTING;
			if ( !this->LogIn()) 
				return 0;
		}
		else
		{
			if ( this->account->IsOnline()) {
				SetServerStatus(new_status);
				return 0;
			}
			
			SendBroadcast(NULL, ACKTYPE_STATUS, ACKRESULT_SUCCESS, (HANDLE)old_status, m_iStatus);
		}
	}

	this->SendBroadcast(NULL, ACKTYPE_STATUS, ACKRESULT_SUCCESS, (HANDLE)old_status, m_iStatus);
	return 0;
}

HANDLE __cdecl CSkypeProto::GetAwayMsg(HANDLE hContact) { return 0; }
int    __cdecl CSkypeProto::RecvAwayMsg( HANDLE hContact, int mode, PROTORECVEVENT* evt ) { return 0; }
int    __cdecl CSkypeProto::SendAwayMsg( HANDLE hContact, HANDLE hProcess, const char* msg ) { return 0; }
int    __cdecl CSkypeProto::SetAwayMsg( int m_iStatus, const TCHAR* msg ) { return 0; }

int    __cdecl CSkypeProto::UserIsTyping( HANDLE hContact, int type )
{
	if (hContact && this->IsOnline() && this->m_iStatus != ID_STATUS_INVISIBLE)
	{
		mir_ptr<wchar_t> sid(::db_get_wsa(hContact, this->m_szModuleName, SKYPE_SETTINGS_LOGIN));
		if (::wcsicmp(sid, this->login) != 0)
		{
			SEStringList targets;
			mir_ptr<wchar_t> sid(::db_get_wsa(hContact, this->m_szModuleName, SKYPE_SETTINGS_LOGIN));
			targets.append((char *)mir_ptr<char>(::mir_utf8encodeW(sid)));

			CConversation::Ref conversation;
			g_skype->GetConversationByParticipants(targets, conversation);

			if (conversation)
			{
				switch (type)
				{
					case PROTOTYPE_SELFTYPING_ON:
						conversation->SetMyTextStatusTo(Participant::WRITING);
						return 0;

					case PROTOTYPE_SELFTYPING_OFF:
						conversation->SetMyTextStatusTo(Participant::READING); //todo: mb TEXT_UNKNOWN?
						return 0;
				}
			}
		}
	}

	return 1;
}

int    __cdecl CSkypeProto::OnEvent(PROTOEVENTTYPE eventType, WPARAM wParam, LPARAM lParam)
{
	switch (eventType)
	{
	case EV_PROTO_ONLOAD:
		return this->OnModulesLoaded(wParam, lParam);

	case EV_PROTO_ONEXIT:
		return this->OnPreShutdown(wParam, lParam);

	case EV_PROTO_ONMENU:
		this->OnInitStatusMenu();

	case EV_PROTO_ONCONTACTDELETED:
		return this->OnContactDeleted(wParam, lParam);
	}

	return 1;
}