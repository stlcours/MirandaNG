#include "stdafx.h"

MCONTACT WhatsAppProto::AddToContactList(const std::string &jid, const char *new_name)
{
	if (jid == m_szJid)
		return NULL;

	// First, check if this contact exists
	MCONTACT hContact = ContactIDToHContact(jid);
	if (hContact) {
		if (new_name != NULL) {
			DBVARIANT dbv;
			string oldName;
			if (db_get_utf(hContact, m_szModuleName, WHATSAPP_KEY_NICK, &dbv))
				oldName = jid.c_str();
			else {
				oldName = dbv.pszVal;
				db_free(&dbv);
			}

			if (oldName.compare(string(new_name)) != 0) {
				db_set_utf(hContact, m_szModuleName, WHATSAPP_KEY_NICK, new_name);

				CMStringW tmp(FORMAT, TranslateT("is now known as '%s'"), ptrW(mir_utf8decodeW(new_name)));
				this->NotifyEvent(_A2T(oldName.c_str()), tmp, hContact, WHATSAPP_EVENT_OTHER);
			}
		}

		if (db_get_b(hContact, "CList", "Hidden", 0) > 0)
			db_unset(hContact, "CList", "Hidden");

		return hContact;
	}

	// If not, make a new contact!
	if ((hContact = db_add_contact()) == 0)
		return INVALID_CONTACT_ID;

	Proto_AddToContact(hContact, m_szModuleName);
	setString(hContact, "ID", jid.c_str());
	debugLogA("Added contact %s", jid.c_str());
	setString(hContact, "MirVer", "WhatsApp");
	db_unset(hContact, "CList", "MyHandle");
	db_set_b(hContact, "CList", "NotOnList", 1);
	db_set_ws(hContact, "CList", "Group", m_tszDefaultGroup);

	if (new_name != NULL)
		db_set_utf(hContact, m_szModuleName, WHATSAPP_KEY_NICK, new_name);

	return hContact;
}

MCONTACT WhatsAppProto::ContactIDToHContact(const std::string &phoneNumber)
{
	// Cache
	std::map<string, MCONTACT>::iterator it = m_hContactByJid.find(phoneNumber);
	if (it != m_hContactByJid.end())
		return it->second;

	for (MCONTACT hContact = db_find_first(m_szModuleName); hContact; hContact = db_find_next(hContact, m_szModuleName)) {
		const char *id = isChatRoom(hContact) ? "ChatRoomID" : WHATSAPP_KEY_ID;

		ptrA szId(getStringA(hContact, id));
		if (!mir_strcmp(phoneNumber.c_str(), szId)) {
			m_hContactByJid[phoneNumber] = hContact;
			return hContact;
		}
	}

	return 0;
}

void WhatsAppProto::SetAllContactStatuses(int status, bool reset_client)
{
	for (MCONTACT hContact = db_find_first(m_szModuleName); hContact; hContact = db_find_next(hContact, m_szModuleName)) {
		if (reset_client) {
			ptrW tszMirVer(getWStringA(hContact, "MirVer"));
			if (mir_wstrcmp(tszMirVer, L"WhatsApp"))
				setWString(hContact, "MirVer", L"WhatsApp");

			db_set_ws(hContact, "CList", "StatusMsg", L"");
		}

		if (getWord(hContact, "Status", ID_STATUS_OFFLINE) != status)
			setWord(hContact, "Status", status);
	}
}

void WhatsAppProto::ProcessBuddyList(void*)
{
	// m_pConnection->setFlush(false);

	// for (MCONTACT hContact = db_find_first(m_szModuleName); hContact; hContact = db_find_next(hContact, m_szModuleName)) {
	//		ptrA jid(getStringA(hContact, WHATSAPP_KEY_ID));
	//		if (jid)
	//			m_pConnection->sendQueryLastOnline((char*)jid);
	//	}

	// m_pConnection->setFlush(true);

	try {
		m_pConnection->sendGetGroups();
	}
	CODE_BLOCK_CATCH_ALL
}

void WhatsAppProto::onAvailable(const std::string &paramString, bool paramBoolean, DWORD lastSeenTime)
{
	MCONTACT hContact = AddToContactList(paramString);
	if (hContact != NULL) {
		if (paramBoolean) {
			setWord(hContact, "Status", ID_STATUS_ONLINE);
			setDword(hContact, WHATSAPP_KEY_LAST_SEEN, time(NULL));
		}
		else {
			setWord(hContact, "Status", ID_STATUS_OFFLINE);
			if (lastSeenTime != 0) {
				setDword(hContact, WHATSAPP_KEY_LAST_SEEN, lastSeenTime);
				setByte(hContact, WHATSAPP_KEY_LAST_SEEN_DENIED, 0);
			}
			else
				setByte(hContact, WHATSAPP_KEY_LAST_SEEN_DENIED, 1);
		}
		UpdateStatusMsg(hContact);
	}
}

void WhatsAppProto::UpdateStatusMsg(MCONTACT hContact)
{
	std::wstringstream ss;
	DWORD lastSeen = getDword(hContact, WHATSAPP_KEY_LAST_SEEN, 0);
	WORD status = getWord(hContact, "Status", ID_STATUS_OFFLINE);
	bool denied = getBool(hContact, WHATSAPP_KEY_LAST_SEEN_DENIED, false);
	if (lastSeen > 0) {
		time_t ts = lastSeen;
		wchar_t stzLastSeen[MAX_PATH];
		if (status == ID_STATUS_ONLINE)
			 wcsftime(stzLastSeen, _countof(stzLastSeen), TranslateT("Last online on %x at %X"), localtime(&ts));
		else
			wcsftime(stzLastSeen, _countof(stzLastSeen), denied ? TranslateT("Denied: Last online on %x at %X") : TranslateT("Last seen on %x at %X"), localtime(&ts));

		ss << stzLastSeen;
	}

	db_set_ws(hContact, "CList", "StatusMsg", ss.str().c_str());
}

void WhatsAppProto::onContactChanged(const std::string&, bool)
{
}

void WhatsAppProto::onPictureChanged(const std::string &jid, const std::string&, bool)
{
	if (isOnline())
		m_pConnection->sendGetPicture(jid.c_str(), "image");
}

void WhatsAppProto::onSendGetPicture(const std::string &jid, const std::vector<unsigned char>& data, const std::string &id)
{
	MCONTACT hContact = ContactIDToHContact(jid);
	if (hContact) {
		debugLogA("Updating avatar for jid %s", jid.c_str());

		// Save avatar
		std::wstring filename = GetAvatarFileName(hContact);
		FILE *f = _wfopen(filename.c_str(), L"wb");
		size_t r = fwrite(std::string(data.begin(), data.end()).c_str(), 1, data.size(), f);
		fclose(f);

		PROTO_AVATAR_INFORMATION ai = { 0 };
		ai.hContact = hContact;
		ai.format = PA_FORMAT_JPEG;
		wcsncpy_s(ai.filename, filename.c_str(), _TRUNCATE);

		int ackResult;
		if (r > 0) {
			setString(hContact, WHATSAPP_KEY_AVATAR_ID, id.c_str());
			ackResult = ACKRESULT_SUCCESS;
		}
		else {
			ackResult = ACKRESULT_FAILED;
		}
		ProtoBroadcastAck(ai.hContact, ACKTYPE_AVATAR, ackResult, (HANDLE)&ai, 0);
	}
}

wchar_t* WhatsAppProto::GetContactDisplayName(const string& jid)
{
	MCONTACT hContact = ContactIDToHContact(jid);
	return (hContact) ? pcli->pfnGetContactDisplayName(hContact, 0) : L"none";
}
