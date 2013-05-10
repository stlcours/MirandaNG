#include "skypekit.h"

CSkype::CSkype(int num_threads) : Skype(num_threads)
{
	this->proto = NULL;
	this->onMessagedCallback = NULL;
}

CAccount* CSkype::newAccount(int oid) 
{ 
	return new CAccount(oid, this); 
}

CContactGroup* CSkype::newContactGroup(int oid)
{ 
	return new CContactGroup(oid, this); 
}

CContact* CSkype::newContact(int oid) 
{ 
	return new CContact(oid, this); 
}

CConversation* CSkype::newConversation(int oid) 
{ 
	return new CConversation(oid, this); 
}

CParticipant* CSkype::newParticipant(int oid) 
{ 
	return new CParticipant(oid, this); 
}

CMessage* CSkype::newMessage(int oid) 
{ 
	return new CMessage(oid, this); 
}

CTransfer* CSkype::newTransfer(int oid) 
{ 
	return new CTransfer(oid, this); 
}

CContactSearch* CSkype::newContactSearch(int oid)
{
	return new CContactSearch(oid, this);
}

bool CSkype::CreateConferenceWithConsumers(ConversationRef &conference, const SEStringList &identities)
{
	if (this->CreateConference(conference))
	{
		conference->SetOption(CConversation::P_OPT_JOINING_ENABLED, true);
		conference->SetOption(CConversation::P_OPT_ENTRY_LEVEL_RANK, CParticipant::WRITER);
		conference->SetOption(CConversation::P_OPT_DISCLOSE_HISTORY, 1);
		conference->AddConsumers(identities);

		return true;
	}

	return false;
}

void CSkype::SetOnMessageCallback(OnMessaged callback, CSkypeProto* proto)
{
	this->proto = proto;
	this->onMessagedCallback = callback;
}

void CSkype::OnMessage (
	const MessageRef & message,
	const bool & changesInboxTimestamp,
	const MessageRef & supersedesHistoryMessage,
	const ConversationRef & conversation)
{
	if (this->proto && this->onMessagedCallback)
		(proto->*onMessagedCallback)(conversation, message);
}
