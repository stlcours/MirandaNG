#include "common.h"

CToxProto::CToxProto(const char* protoName, const TCHAR* userName) :
	PROTO<CToxProto>(protoName, userName),
	_tox(tox_new(0))
{
	tox_callback_friend_request(_tox, OnFriendRequest, this);
	tox_callback_friend_message(_tox, OnFriendMessage, this);
	tox_callback_friend_action(_tox, OnAction, this);
	tox_callback_name_change(_tox, OnFriendNameChange, this);
	tox_callback_status_message(_tox, OnStatusMessageChanged, this);
	tox_callback_user_status(_tox, OnUserStatusChanged, this);
	tox_callback_connection_status(_tox, OnConnectionStatusChanged, this);

	CreateProtoService(PS_CREATEACCMGRUI, &CToxProto::CreateAccMgrUI);
}

CToxProto::~CToxProto()
{
	tox_kill(_tox);
}

void CALLBACK CToxProto::TimerProc(HWND, UINT, UINT_PTR idEvent, DWORD)
{
	CToxProto *ppro = (CToxProto*)idEvent;

	tox_do(ppro->_tox);
}

MCONTACT __cdecl CToxProto::AddToList(int flags, PROTOSEARCHRESULT* psr) { return 0; }
MCONTACT __cdecl CToxProto::AddToListByEvent(int flags, int iContact, HANDLE hDbEvent) { return 0; }

int __cdecl CToxProto::Authorize(HANDLE hDbEvent) { return 0; }
int __cdecl CToxProto::AuthDeny(HANDLE hDbEvent, const PROTOCHAR* szReason) { return 0; }
int __cdecl CToxProto::AuthRecv(MCONTACT hContact, PROTORECVEVENT*) { return 0; }
int __cdecl CToxProto::AuthRequest(MCONTACT hContact, const PROTOCHAR* szMessage) { return 0; }

HANDLE __cdecl CToxProto::ChangeInfo(int iInfoType, void* pInfoData) { return 0; }

HANDLE __cdecl CToxProto::FileAllow(MCONTACT hContact, HANDLE hTransfer, const PROTOCHAR* szPath) { return 0; }
int __cdecl CToxProto::FileCancel(MCONTACT hContact, HANDLE hTransfer) { return 0; }
int __cdecl CToxProto::FileDeny(MCONTACT hContact, HANDLE hTransfer, const PROTOCHAR* szReason) { return 0; }
int __cdecl CToxProto::FileResume(HANDLE hTransfer, int* action, const PROTOCHAR** szFilename) { return 0; }

DWORD_PTR __cdecl CToxProto::GetCaps(int type, MCONTACT hContact)
{
	switch(type)
	{
	case PFLAGNUM_1:
		return PF1_IM;
	case PFLAGNUM_2:
		return PF2_ONLINE | PF2_SHORTAWAY | PF2_LIGHTDND;
	case PFLAG_UNIQUEIDTEXT:
		return (INT_PTR)"Username";
	case PFLAG_UNIQUEIDSETTING:
		return (DWORD_PTR)"username";
	}

	return 0;
}
int __cdecl CToxProto::GetInfo(MCONTACT hContact, int infoType) { return 0; }

HANDLE __cdecl CToxProto::SearchBasic(const PROTOCHAR* id) { return 0; }
HANDLE __cdecl CToxProto::SearchByEmail(const PROTOCHAR* email) { return 0; }
HANDLE __cdecl CToxProto::SearchByName(const PROTOCHAR* nick, const PROTOCHAR* firstName, const PROTOCHAR* lastName) { return 0; }
HWND __cdecl CToxProto::SearchAdvanced(HWND owner) { return 0; }
HWND __cdecl CToxProto::CreateExtendedSearchUI(HWND owner) { return 0; }

int __cdecl CToxProto::RecvContacts(MCONTACT hContact, PROTORECVEVENT*) { return 0; }
int __cdecl CToxProto::RecvFile(MCONTACT hContact, PROTOFILEEVENT*) { return 0; }
int __cdecl CToxProto::RecvMsg(MCONTACT hContact, PROTORECVEVENT*) { return 0; }
int __cdecl CToxProto::RecvUrl(MCONTACT hContact, PROTORECVEVENT*) { return 0; }

int __cdecl CToxProto::SendContacts(MCONTACT hContact, int flags, int nContacts, MCONTACT* hContactsList) { return 0; }
HANDLE __cdecl CToxProto::SendFile(MCONTACT hContact, const PROTOCHAR* szDescription, PROTOCHAR** ppszFiles) { return 0; }
int __cdecl CToxProto::SendMsg(MCONTACT hContact, int flags, const char* msg) { return 0; }
int __cdecl CToxProto::SendUrl(MCONTACT hContact, int flags, const char* url) { return 0; }

int __cdecl CToxProto::SetApparentMode(MCONTACT hContact, int mode) { return 0; }

int __cdecl CToxProto::SetStatus(int iNewStatus)
{
	if (iNewStatus == this->m_iDesiredStatus)
		return 0;

	int old_status = this->m_iStatus;
	this->m_iDesiredStatus = iNewStatus;

	if (iNewStatus == ID_STATUS_OFFLINE)
	{
		// lgout
		m_iStatus = m_iDesiredStatus = ID_STATUS_OFFLINE;

		ProtoBroadcastAck(NULL, ACKTYPE_STATUS, ACKRESULT_SUCCESS, (HANDLE)old_status, m_iStatus);

		return 0;
	}
	else
	{
		if (old_status == ID_STATUS_OFFLINE/* && !this->IsOnline()*/)
		{
			m_iStatus = ID_STATUS_CONNECTING;

			// login
			//_timer = SetTimer(NULL, (UINT_PTR)this, 30, (TIMERPROC)CToxProto::TimerProc);
		}
		else
		{
			// set tox status
			TOX_USERSTATUS userstatus;
			switch (iNewStatus)
			{
			case ID_STATUS_ONLINE:
				userstatus = TOX_USERSTATUS_NONE;
				break;
			case ID_STATUS_AWAY:
				userstatus = TOX_USERSTATUS_AWAY;
				break;
			case ID_STATUS_OCCUPIED:
				userstatus = TOX_USERSTATUS_BUSY;
				break;
			default:
				userstatus = TOX_USERSTATUS_INVALID;
				break;
			}
			tox_set_user_status(_tox, userstatus);

			ProtoBroadcastAck(NULL, ACKTYPE_STATUS, ACKRESULT_SUCCESS, (HANDLE)old_status, m_iStatus);

			return 0;
		}
	}

	ProtoBroadcastAck(NULL, ACKTYPE_STATUS, ACKRESULT_SUCCESS, (HANDLE)old_status, m_iStatus);

	return 0;
}

HANDLE __cdecl CToxProto::GetAwayMsg(MCONTACT hContact) { return 0; }
int __cdecl CToxProto::RecvAwayMsg(MCONTACT hContact, int mode, PROTORECVEVENT* evt) { return 0; }
int __cdecl CToxProto::SetAwayMsg(int iStatus, const PROTOCHAR* msg) { return 0; }

int __cdecl CToxProto::UserIsTyping(MCONTACT hContact, int type) { return 0; }

int __cdecl CToxProto::OnEvent(PROTOEVENTTYPE iEventType, WPARAM wParam, LPARAM lParam) { return 0; }