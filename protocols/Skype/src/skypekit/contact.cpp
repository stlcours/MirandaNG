#include "contact.h"

CContact::CContact(unsigned int oid, SERootObject* root) : Contact(oid, root) 
{
	this->proto = NULL;
	this->callback == NULL;
}

SEString CContact::GetSid()
{
	SEString result;
	CContact::AVAILABILITY availability;
	this->GetPropAvailability(availability);
	if (availability == CContact::SKYPEOUT)
		this->GetPropPstnnumber(result);
	else
		this->GetPropSkypename(result);
	return result;
}

SEString CContact::GetNick()
{
	SEString result;
	CContact::AVAILABILITY availability;
	this->GetPropAvailability(availability);
	if (availability == CContact::SKYPEOUT)
		this->GetPropPstnnumber(result);
	else
		this->GetPropFullname(result);
	return result;
}

bool CContact::GetFullname(SEString &firstName, SEString &lastName)
{
	SEString fullname;
	this->GetPropFullname(fullname);
	int pos = fullname.find(" ");
	if (pos != -1)
	{
		firstName = fullname.substr(0, pos - 1);
		lastName = fullname.right(fullname.size() - pos - 1);
	} else
		firstName = fullname;
	
	return true;
}

void CContact::SetOnContactChangedCallback(OnContactChanged callback, CSkypeProto* proto)
{
	this->proto = proto;
	this->callback = callback;
}

void CContact::OnChange(int prop)
{
	if (this->proto)
		(proto->*callback)(this->ref(), prop);
}