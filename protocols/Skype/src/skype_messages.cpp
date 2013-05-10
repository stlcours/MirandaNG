#include "skype_proto.h"

int CSkypeProto::OnMessagePreCreate(WPARAM, LPARAM lParam)
{
	MessageWindowEvent *evt = (MessageWindowEvent *)lParam;
	if ( strcmp(GetContactProto(evt->hContact), this->m_szModuleName))
		return 0;

	MessageRef message(evt->seq);
	SEBinary guid;
	if (message->GetPropGuid(guid))
	{
		CMessage::TYPE messageType;
		message->GetPropType(messageType);

		if (messageType == CMessage::POSTED_TEXT)
		{
			evt->dbei->pBlob = (PBYTE)::mir_realloc(evt->dbei->pBlob, evt->dbei->cbBlob + guid.size());
			::memcpy((char *)&evt->dbei->pBlob[evt->dbei->cbBlob], guid.data(), guid.size());
			evt->dbei->cbBlob += (DWORD)guid.size();
		}
		else if (messageType == CMessage::POSTED_EMOTE)
		{
			evt->dbei->pBlob = (PBYTE)::mir_realloc(evt->dbei->pBlob, evt->dbei->cbBlob + guid.size() - 4);
			::memcpy((char *)&evt->dbei->pBlob[0], (char *)&evt->dbei->pBlob[4], evt->dbei->cbBlob - 4);
			::memcpy((char *)&evt->dbei->pBlob[evt->dbei->cbBlob - 4], guid.data(), guid.size());
			evt->dbei->cbBlob += (DWORD)guid.size() - 4;

			evt->dbei->eventType = SKYPE_DB_EVENT_TYPE_EMOTE;
		}
	}

	return 1;
}

void CSkypeProto::OnMessageReceived(CConversation::Ref &conversation, CMessage::Ref &message)
{
	SEString data;

	CMessage::TYPE messageType;
	message->GetPropType(messageType);

	uint timestamp;
	message->GetPropTimestamp(timestamp);

	CMessage::CONSUMPTION_STATUS status;
	message->GetPropConsumptionStatus(status);
		
	message->GetPropBodyXml(data);
	char *text = CSkypeProto::RemoveHtml(data);

	message->GetPropAuthor(data);			
		
	CContact::Ref author;
	g_skype->GetContact(data, author);

	HANDLE hContact = this->AddContact(author);
	this->UserIsTyping(hContact, PROTOTYPE_SELFTYPING_OFF);

	SEBinary guid;
	message->GetPropGuid(guid);
	ReadMessageParam param = { guid, messageType };

	if (status != CMessage::UNCONSUMED_NORMAL)
		if (this->IsMessageInDB(hContact, timestamp, guid))
			return;

	PROTORECVEVENT recv;
	recv.flags = PREF_UTF;
	recv.lParam = (LPARAM)&param;
	recv.timestamp = timestamp;
	recv.szMessage = ::mir_strdup(text);
	::ProtoChainRecvMsg(hContact, &recv);
}

/////////////////////////////////////////////////////////////////////////////////////////

void CSkypeProto::OnMessageSent(CConversation::Ref &conversation, CMessage::Ref &message)
{
	SEString data;

	CMessage::TYPE messageType;
	message->GetPropType(messageType);

	uint timestamp;
	message->GetPropTimestamp(timestamp);

	CMessage::SENDING_STATUS sstatus;
	message->GetPropSendingStatus(sstatus);

	CMessage::CONSUMPTION_STATUS status;
	message->GetPropConsumptionStatus(status);

	message->GetPropBodyXml(data);
	char *text = CSkypeProto::RemoveHtml(data);

	CParticipant::Refs participants;
	conversation->GetParticipants(participants, CConversation::OTHER_CONSUMERS);
	participants[0]->GetPropIdentity(data);
		
	CContact::Ref receiver;
	g_skype->GetContact(data, receiver);

	HANDLE hContact = this->AddContact(receiver);
	this->SendBroadcast(
		hContact,
		ACKTYPE_MESSAGE,
		sstatus == CMessage::FAILED_TO_SEND ? ACKRESULT_FAILED : ACKRESULT_SUCCESS,
		(HANDLE)message->getOID(), 0);
			
	SEBinary guid;
	message->GetPropGuid(guid);

	this->RaiseMessageSentEvent(
		hContact,
		timestamp,
		guid,
		text,
		status == CMessage::UNCONSUMED_NORMAL);
}

void CSkypeProto::OnMessageEvent(CConversation::Ref conversation, CMessage::Ref message)
{
	CMessage::TYPE messageType;
	message->GetPropType(messageType);

	switch (messageType)
	{
	case CMessage::POSTED_EMOTE:
	case CMessage::POSTED_TEXT:
		{
			SEString author;
			message->GetPropAuthor(author);
			
			if (::wcsicmp(mir_ptr<wchar_t>(::mir_utf8decodeW(author)), this->login) == 0)
				this->OnMessageSent(conversation, message);
			else
				this->OnMessageReceived(conversation, message);
		}
		break;

	case CMessage::STARTED_LIVESESSION:
		{
			conversation->LeaveLiveSession();

			uint timestamp;
			message->GetPropTimestamp(timestamp);

			SEString identity;
			message->GetPropAuthor(identity);

			CContact::Ref author;
			g_skype->GetContact(identity, author);

			HANDLE hContact = this->AddContact(author);
		
			char *message = ::mir_utf8encode(::Translate("Incoming call"));
		
			this->AddDBEvent(
				hContact,
				SKYPE_DB_EVENT_TYPE_CALL,
				timestamp,
				DBEF_UTF,
				(DWORD)::strlen(message) + 1,
				(PBYTE)message);
		}
	}
}