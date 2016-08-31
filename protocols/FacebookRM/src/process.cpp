/*

Facebook plugin for Miranda Instant Messenger
_____________________________________________

Copyright � 2009-11 Michal Zelinka, 2011-16 Robert P�sel

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "stdafx.h"

/**
 * Helper function for loading name from database (or use default one specified as parameter), used for title of few notifications.
 */
std::string getContactName(FacebookProto *proto, MCONTACT hContact, const char *defaultName)
{
	std::string name = defaultName;

	DBVARIANT dbv;
	if (!proto->getStringUtf(hContact, FACEBOOK_KEY_NICK, &dbv)) {
		name = dbv.pszVal;
		db_free(&dbv);
	}

	return name;
}

void FacebookProto::ProcessFriendList(void*)
{
	if (isOffline())
		return;

	facy.handle_entry("load_friends");

	// Get friends list
	std::string data = "__user=" + facy.self_.user_id;
	data += "&__dyn=" + facy.__dyn();
	data += "&__req=" + facy.__req();
	data += "&fb_dtsg=" + facy.dtsg_;
	data += "&ttstamp=" + facy.ttstamp_;
	data += "&__rev=" + facy.__rev();
	data += "&__pc=PHASED:DEFAULT&__be=-1&__a=1";

	http::response resp = facy.flap(REQUEST_USER_INFO_ALL, &data); // NOTE: Request revised 17.8.2016

	if (resp.code != HTTP_CODE_OK) {
		facy.handle_error("load_friends");
		return;
	}

	debugLogA("*** Starting processing friend list");

	CODE_BLOCK_TRY

		std::map<std::string, facebook_user*> friends;

	facebook_json_parser* p = new facebook_json_parser(this);
	p->parse_friends(&resp.data, &friends);
	delete p;


	// Check and update old contacts
	for (MCONTACT hContact = db_find_first(m_szModuleName); hContact; hContact = db_find_next(hContact, m_szModuleName)) {
		if (isChatRoom(hContact))
			continue;

		// TODO RM: change name of "Deleted" key to "DeletedTS", remove this code in some next version
		int deletedTS = getDword(hContact, "Deleted", 0);
		if (deletedTS != 0) {
			delSetting(hContact, "Deleted");
			setDword(hContact, FACEBOOK_KEY_DELETED, deletedTS);
		}

		facebook_user *fbu;
		ptrA id(getStringA(hContact, FACEBOOK_KEY_ID));
		if (id != NULL) {
			std::map< std::string, facebook_user* >::iterator iter;

			if ((iter = friends.find(std::string(id))) != friends.end()) {
				// Found contact, update it and remove from map
				fbu = iter->second;

				// TODO RM: remove, because contacts cant change it, so its only for "first run"
				// - but what with contacts, that was added after logon?
				// Update gender
				if (getByte(hContact, "Gender", 0) != (int)fbu->gender)
					setByte(hContact, "Gender", fbu->gender);

				// TODO: remove this in some future version?
				// Remove old useless "RealName" field
				ptrA realname(getStringA(hContact, "RealName"));
				if (realname != NULL) {
					delSetting(hContact, "RealName");
				}

				// Update real name and nick
				if (!fbu->real_name.empty()) {
					SaveName(hContact, fbu);
				}

				// Update username
				ptrA username(getStringA(hContact, FACEBOOK_KEY_USERNAME));
				if (!username || mir_strcmp(username, fbu->username.c_str())) {
					if (!fbu->username.empty())
						setString(hContact, FACEBOOK_KEY_USERNAME, fbu->username.c_str());
					else
						delSetting(hContact, FACEBOOK_KEY_USERNAME);
				}

				// Update contact type
				if (getByte(hContact, FACEBOOK_KEY_CONTACT_TYPE) != fbu->type) {
					setByte(hContact, FACEBOOK_KEY_CONTACT_TYPE, fbu->type);
					// TODO: remove that popup and use "Contact added you" event?
				}

				// Wasn't contact removed from "server-list" someday?
				if (getDword(hContact, FACEBOOK_KEY_DELETED, 0)) {
					delSetting(hContact, FACEBOOK_KEY_DELETED);

					// Notify it, if user wants to be notified
					if (getByte(FACEBOOK_KEY_EVENT_FRIENDSHIP_ENABLE, DEFAULT_EVENT_FRIENDSHIP_ENABLE)) {
						std::string url = FACEBOOK_URL_PROFILE + fbu->user_id;
						std::string contactname = getContactName(this, hContact, !fbu->real_name.empty() ? fbu->real_name.c_str() : fbu->user_id.c_str());

						ptrW szTitle(mir_utf8decodeW(contactname.c_str()));
						NotifyEvent(szTitle, TranslateT("Contact is back on server-list."), hContact, FACEBOOK_EVENT_FRIENDSHIP, &url);
					}
				}

				// Check avatar change
				CheckAvatarChange(hContact, fbu->image_url);

				// Mark this contact as deleted ("processed") and delete them later (as there may be some duplicit contacts to use)
				fbu->deleted = true;
			}
			else {
				// Contact was removed from "server-list", notify it

				// Wasn't we already been notified about this contact? And was this real friend?
				if (!getDword(hContact, FACEBOOK_KEY_DELETED, 0) && getByte(hContact, FACEBOOK_KEY_CONTACT_TYPE, 0) == CONTACT_FRIEND) {
					setDword(hContact, FACEBOOK_KEY_DELETED, ::time(NULL));
					setByte(hContact, FACEBOOK_KEY_CONTACT_TYPE, CONTACT_NONE);
					
					// Notify it, if user wants to be notified
					if (getByte(FACEBOOK_KEY_EVENT_FRIENDSHIP_ENABLE, DEFAULT_EVENT_FRIENDSHIP_ENABLE)) {
						std::string url = FACEBOOK_URL_PROFILE + std::string(id);
						std::string contactname = getContactName(this, hContact, id);

						ptrW szTitle(mir_utf8decodeW(contactname.c_str()));
						NotifyEvent(szTitle, TranslateT("Contact is no longer on server-list."), hContact, FACEBOOK_EVENT_FRIENDSHIP, &url);
					}
				}
			}
		}
	}

	// Check remaining contacts in map and add them to contact list
	for (std::map< std::string, facebook_user* >::iterator it = friends.begin(); it != friends.end();) {
		if (!it->second->deleted)
			AddToContactList(it->second, true); // we know this contact doesn't exists, so we force add it

		delete it->second;
		it = friends.erase(it);
	}
	friends.clear();

	debugLogA("*** Friend list processed");

	CODE_BLOCK_CATCH

		debugLogA("*** Error processing friend list: %s", e.what());

	CODE_BLOCK_END
}

void FacebookProto::ProcessUnreadMessages(void*)
{
	if (isOffline())
		return;

	facy.handle_entry("ProcessUnreadMessages");

	std::string data = "folders[0]=inbox&folders[1]=other"; // TODO: "other" is probably unused, and there is now "pending" instead
	data += "&client=mercury";
	data += "&__user=" + facy.self_.user_id;
	data += "&fb_dtsg=" + facy.dtsg_;
	data += "&__a=1&__dyn=&__req=&ttstamp=" + facy.ttstamp_;
	data += "&__rev=" + facy.__rev();
	data += "&__pc=PHASED:DEFAULT&__be=-1";

	http::response resp = facy.flap(REQUEST_UNREAD_THREADS, &data);

	if (resp.code != HTTP_CODE_OK) {
		facy.handle_error("ProcessUnreadMessages");
		return;
	}

	CODE_BLOCK_TRY

		std::vector<std::string> threads;

		facebook_json_parser* p = new facebook_json_parser(this);
		p->parse_unread_threads(&resp.data, &threads);
		delete p;

		ForkThread(&FacebookProto::ProcessUnreadMessage, new std::vector<std::string>(threads));

		debugLogA("*** Unread threads list processed");

	CODE_BLOCK_CATCH

		debugLogA("*** Error processing unread threads list: %s", e.what());

	CODE_BLOCK_END

		facy.handle_success("ProcessUnreadMessages");
}

void FacebookProto::ProcessUnreadMessage(void *pParam)
{
	if (pParam == NULL)
		return;

	std::vector<std::string> *threads = (std::vector<std::string>*)pParam;

	if (isOffline()) {
		delete threads;
		return;
	}

	facy.handle_entry("ProcessUnreadMessage");

	int offset = 0;
	int limit = 21;

	http::response resp;

	// TODO: First load info about amount of unread messages, then load exactly this amount for each thread

	while (!threads->empty()) {		
		std::string data = "client=mercury";
		data += "&__user=" + facy.self_.user_id;
		data += "&__dyn=" + facy.__dyn();
		data += "&__req=" + facy.__req();
		data += "&fb_dtsg=" + facy.dtsg_;
		data += "&ttstamp=" + facy.ttstamp_;
		data += "&__rev=" + facy.__rev();
		data += "&__pc=PHASED:DEFAULT&__be=-1&__a=1";

		for (std::vector<std::string>::size_type i = 0; i < threads->size(); i++) {
			std::string thread_id = utils::url::encode(threads->at(i));

			// request messages from thread
			data += "&messages[thread_ids][" + thread_id;
			data += "][offset]=" + utils::conversion::to_string(&offset, UTILS_CONV_SIGNED_NUMBER);
			data += "&messages[thread_ids][" + thread_id;
			data += "][limit]=" + utils::conversion::to_string(&limit, UTILS_CONV_SIGNED_NUMBER);

			// request info about thread
			data += "&threads[thread_ids][" + utils::conversion::to_string(&i, UTILS_CONV_UNSIGNED_NUMBER);
			data += "]=" + thread_id;
		}

		resp = facy.flap(REQUEST_THREAD_INFO, &data); // NOTE: Request revised 17.8.2016

		if (resp.code == HTTP_CODE_OK) {

			CODE_BLOCK_TRY

			std::vector<facebook_message> messages;
			std::map<std::string, facebook_chatroom*> chatrooms;

			facebook_json_parser* p = new facebook_json_parser(this);
			p->parse_thread_messages(&resp.data, &messages, &chatrooms, false);
			delete p;

			for (std::map<std::string, facebook_chatroom*>::iterator it = chatrooms.begin(); it != chatrooms.end();) {

				// TODO: refactor this too!
				// TODO: have all chatrooms in facy, in memory, and then handle them as needed... somehow think about it...
				/*	facebook_chatroom *room = it->second;
				MCONTACT hChatContact = NULL;
				ptrA users(GetChatUsers(room->thread_id.c_str()));
				if (users == NULL) {
				AddChat(room->thread_id.c_str(), room->chat_name.c_str());
				hChatContact = ChatIDToHContact(room->thread_id);
				// Set thread id (TID) for later
				setWString(hChatContact, FACEBOOK_KEY_TID, room->thread_id.c_str());

				for (std::map<std::string, std::string>::iterator jt = room->participants.begin(); jt != room->participants.end(); ) {
				AddChatContact(room->thread_id.c_str(), jt->first.c_str(), jt->second.c_str());
				++jt;
				}
				}

				if (!hChatContact)
				hChatContact = ChatIDToHContact(room->thread_id);

				ForkThread(&FacebookProto::ReadMessageWorker, (void*)hChatContact);*/

				delete it->second;
				it = chatrooms.erase(it);
			}
			chatrooms.clear();

			ReceiveMessages(messages, true);

			debugLogA("*** Unread messages processed");

			CODE_BLOCK_CATCH

			debugLogA("*** Error processing unread messages: %s", e.what());

			CODE_BLOCK_END

			facy.handle_success("ProcessUnreadMessage");
		}
		else {
			facy.handle_error("ProcessUnreadMessage");
		}

		offset += limit;
		limit = 20; // TODO: use better limits?

		threads->clear(); // TODO: if we have limit messages from one user, there may be more unread messages... continue with it... otherwise remove that threadd from threads list -- or do it in json parser? hm			 = allow more than "limit" unread messages to be parsed
	}

	delete threads;
}

void FacebookProto::LoadLastMessages(void *pParam)
{
	if (pParam == NULL)
		return;

	MCONTACT hContact = *(MCONTACT*)pParam;
	delete (MCONTACT*)pParam;

	if (isOffline())
		return;

	facy.handle_entry("LoadLastMessages");
	if (!isOnline())
		return;

	std::string data = "client=mercury";
	data += "&__user=" + facy.self_.user_id;
	data += "&__dyn=" + facy.__dyn();
	data += "&__req=" + facy.__req();
	data += "&fb_dtsg=" + facy.dtsg_;
	data += "&ttstamp=" + facy.ttstamp_;
	data += "&__rev=" + facy.__rev();
	data += "&__pc=PHASED:DEFAULT&__be=-1&__a=1";

	bool isChat = isChatRoom(hContact);

	if (isChat && (!m_enableChat || IsSpecialChatRoom(hContact))) // disabled chats or special chatroom (e.g. nofitications)
		return;

	ptrA item_id(getStringA(hContact, isChat ? FACEBOOK_KEY_TID : FACEBOOK_KEY_ID));
	if (item_id == NULL) {
		debugLogA("!!! LoadLastMessages(): Contact has no TID/ID");
		return;
	}

	std::string id = utils::url::encode(std::string(item_id));
	std::string type = isChat ? "thread_ids" : "user_ids";
	int count = getByte(FACEBOOK_KEY_MESSAGES_ON_OPEN_COUNT, DEFAULT_MESSAGES_ON_OPEN_COUNT);
	count = min(count, FACEBOOK_MESSAGES_ON_OPEN_LIMIT);

	// request messages from thread
	data += "&messages[" + type + "][" + id;
	data += "][offset]=0";
	data += "&messages[" + type + "][" + id;
	data += "][limit]=" + utils::conversion::to_string(&count, UTILS_CONV_UNSIGNED_NUMBER);

	// request info about thread
	data += "&threads[" + type + "][0]=" + id;

	http::response resp = facy.flap(REQUEST_THREAD_INFO, &data); // NOTE: Request revised 17.8.2016

	if (resp.code != HTTP_CODE_OK || resp.data.empty()) {
		facy.handle_error("LoadLastMessages");
		return;
	}

	// Temporarily disable marking messages as read for this contact
	facy.ignore_read.insert(hContact);

	CODE_BLOCK_TRY

	std::vector<facebook_message> messages;
	std::map<std::string, facebook_chatroom*> chatrooms;

	facebook_json_parser* p = new facebook_json_parser(this);
	p->parse_thread_messages(&resp.data, &messages, &chatrooms, false);
	delete p;

	// TODO: do something with this, chat is loading somewhere else... (in receiveMessages method right now)
	/*for (std::map<std::string, facebook_chatroom*>::iterator it = chatrooms.begin(); it != chatrooms.end();) {

		facebook_chatroom *room = it->second;
		MCONTACT hChatContact = NULL;
		ptrA users(GetChatUsers(room->thread_id.c_str()));
		if (users == NULL) {
		AddChat(room->thread_id.c_str(), room->chat_name.c_str());
		hChatContact = ChatIDToHContact(room->thread_id);
		// Set thread id (TID) for later
		setWString(hChatContact, FACEBOOK_KEY_TID, room->thread_id.c_str());

		for (std::map<std::string, std::string>::iterator jt = room->participants.begin(); jt != room->participants.end();) {
		AddChatContact(room->thread_id.c_str(), jt->first.c_str(), jt->second.c_str());
		++jt;
		}
		}

		if (!hChatContact)
		hChatContact = ChatIDToHContact(room->thread_id);

		ForkThread(&FacebookProto::ReadMessageWorker, (void*)hChatContact);

		delete it->second;
		it = chatrooms.erase(it);
		}
		chatrooms.clear();*/

	ReceiveMessages(messages, true);

	debugLogA("*** Thread messages processed");

	CODE_BLOCK_CATCH

		debugLogA("*** Error processing thread messages: %s", e.what());

	CODE_BLOCK_END

		facy.handle_success("LoadLastMessages");

	// Enable marking messages as read for this contact
	facy.ignore_read.erase(hContact);

	// And force mark read
	OnDbEventRead(hContact, NULL);
}

void FacebookProto::LoadHistory(void *pParam)
{
	if (pParam == NULL)
		return;

	MCONTACT hContact = *(MCONTACT*)pParam;
	delete (MCONTACT*)pParam;

	if (!isOnline())
		return;

	facy.handle_entry("LoadHistory");

	std::string data = "client=mercury";
	data += "&__user=" + facy.self_.user_id;
	data += "&__dyn=" + facy.__dyn();
	data += "&__req=" + facy.__req();
	data += "&fb_dtsg=" + facy.dtsg_;
	data += "&ttstamp=" + facy.ttstamp_;
	data += "&__rev=" + facy.__rev();
	data += "&__pc=PHASED:DEFAULT&__be=-1&__a=1";

	bool isChat = isChatRoom(hContact);
	if (isChat) // TODO: Support chats?
		return;
	/*if (isChat && (!m_enableChat || IsSpecialChatRoom(hContact))) // disabled chats or special chatroom (e.g. nofitications)
		return;*/

	ptrA item_id(getStringA(hContact, isChat ? FACEBOOK_KEY_TID : FACEBOOK_KEY_ID));
	if (item_id == NULL) {
		debugLogA("!!! LoadHistory(): Contact has no TID/ID");
		return;
	}

	std::string id = utils::url::encode(std::string(item_id));
	std::string type = isChat ? "thread_ids" : "user_ids";

	// first get info about this thread and how many messages is there

	// request info about thread
	data += "&threads[" + type + "][0]=" + id;

	http::response resp = facy.flap(REQUEST_THREAD_INFO, &data); // NOTE: Request revised 17.8.2016
	if (resp.code != HTTP_CODE_OK || resp.data.empty()) {
		facy.handle_error("LoadHistory");
		return;
	}

	int messagesCount = -1;
	int unreadCount = -1;

	facebook_json_parser* p = new facebook_json_parser(this);
	if (p->parse_messages_count(&resp.data, &messagesCount, &unreadCount) == EXIT_FAILURE) {
		delete p;
		facy.handle_error("LoadHistory");
		return;
	}

	// Temporarily disable marking messages as read for this contact
	facy.ignore_read.insert(hContact);

	POPUPDATAW pd = { sizeof(pd) };
	pd.iSeconds = 5;
	pd.lchContact = hContact;
	pd.lchIcon = IcoLib_GetIconByHandle(GetIconHandle("conversation")); // TODO: Use better icon
	wcsncpy(pd.lptzContactName, m_tszUserName, MAX_CONTACTNAME);
	wcsncpy(pd.lptzText, TranslateT("Loading history started."), MAX_SECONDLINE);

	HWND popupHwnd = NULL;
	if (ServiceExists(MS_POPUP_ADDPOPUPW)) {
		popupHwnd = (HWND)CallService(MS_POPUP_ADDPOPUPW, (WPARAM)&pd, (LPARAM)APF_RETURN_HWND);
	}

	std::vector<facebook_message> messages;
	std::string firstTimestamp = "";
	std::string firstMessageId = "";
	std::string lastMessageId = "";
	int loadedMessages = 0;
	int messagesPerBatch = messagesCount > 10000 ? 500 : 100;
	for (int batch = 0; batch < messagesCount; batch += messagesPerBatch) {
		if (!isOnline())
			break;

		// Request batch of messages from thread
		std::string data = "client=mercury";
		data += "&__user=" + facy.self_.user_id;
		data += "&__dyn=" + facy.__dyn();
		data += "&__req=" + facy.__req();
		data += "&fb_dtsg=" + facy.dtsg_;
		data += "&ttstamp=" + facy.ttstamp_;
		data += "&__rev=" + facy.__rev();
		data += "&__pc=PHASED:DEFAULT&__be=-1&__a=1";

		// Grrr, offset doesn't work at all, we need to use timestamps to get back in history...
		// And we don't know, what's timestamp of first message, so we need to get from latest to oldest
		data += "&messages[" + type + "][" + id;
		data += "][offset]=" + utils::conversion::to_string(&batch, UTILS_CONV_UNSIGNED_NUMBER);
		data += "&messages[" + type + "][" + id;
		data += "][timestamp]=" + firstTimestamp;
		data += "&messages[" + type + "][" + id;
		data += "][limit]=" + utils::conversion::to_string(&messagesPerBatch, UTILS_CONV_UNSIGNED_NUMBER);

		resp = facy.flap(REQUEST_THREAD_INFO, &data); // NOTE: Request revised 17.8.2016
		if (resp.code != HTTP_CODE_OK || resp.data.empty()) {
			facy.handle_error("LoadHistory");
			break;
		}

		// Parse the result
		CODE_BLOCK_TRY

		messages.clear();

		p->parse_history(&resp.data, &messages, &firstTimestamp);

		// Receive messages
		std::string previousFirstMessageId = firstMessageId;
		for (std::vector<facebook_message*>::size_type i = 0; i < messages.size(); i++) {
			facebook_message &msg = messages[i];
			
			// First message might overlap (as we are using it's timestamp for the next loading), so we need to check for it
			if (i == 0) {
				firstMessageId = msg.message_id;
			}
			if (previousFirstMessageId == msg.message_id) {
				continue;
			}			
			lastMessageId = msg.message_id;

			if (msg.isIncoming && msg.isUnread && msg.type == MESSAGE) {
				PROTORECVEVENT recv = { 0 };
				recv.szMessage = const_cast<char*>(msg.message_text.c_str());
				recv.timestamp = msg.time;
				ProtoChainRecvMsg(hContact, &recv);
			}
			else {
				DBEVENTINFO dbei = { 0 };
				dbei.cbSize = sizeof(dbei);

				if (msg.type == CALL)
					dbei.eventType = FACEBOOK_EVENTTYPE_CALL;
				else
					dbei.eventType = EVENTTYPE_MESSAGE;

				dbei.flags = DBEF_UTF;

				if (!msg.isIncoming)
					dbei.flags |= DBEF_SENT;

				if (!msg.isUnread)
					dbei.flags |= DBEF_READ;

				dbei.szModule = m_szModuleName;
				dbei.timestamp = msg.time;
				dbei.cbBlob = (DWORD)msg.message_text.length() + 1;
				dbei.pBlob = (PBYTE)msg.message_text.c_str();
				db_event_add(hContact, &dbei);
			}

			loadedMessages++;
		}

		// Save last message id of first batch which is latest message completely, because we're going backwards
		if (batch == 0 && !lastMessageId.empty()) {
			setString(hContact, FACEBOOK_KEY_MESSAGE_ID, lastMessageId.c_str());
		}

		debugLogA("*** Load history messages processed");

		CODE_BLOCK_CATCH

		debugLogA("*** Error processing load history messages: %s", e.what());

		break;

		CODE_BLOCK_END

		// Update progress popup
		CMStringW text;
		text.AppendFormat(TranslateT("Loading messages: %d/%d"), loadedMessages, messagesCount);

		if (ServiceExists(MS_POPUP_CHANGETEXTW) && popupHwnd) {
			PUChangeTextW(popupHwnd, text);
		}
		else if (ServiceExists(MS_POPUP_ADDPOPUPW)) {
			wcsncpy(pd.lptzText, text, MAX_SECONDLINE);
			pd.iSeconds = 1;
			popupHwnd = (HWND)CallService(MS_POPUP_ADDPOPUPW, (WPARAM)&pd, (LPARAM)0);
		}

		// There is no more messages
		if (messages.empty() || loadedMessages > messagesCount) {
			break;
		}
	}

	delete p;

	facy.handle_success("LoadHistory");

	// Enable marking messages as read for this contact
	facy.ignore_read.erase(hContact);

	if (ServiceExists(MS_POPUP_CHANGETEXTW) && popupHwnd) {
		PUChangeTextW(popupHwnd, TranslateT("Loading history completed."));
	} else if (ServiceExists(MS_POPUP_ADDPOPUPW)) {
		pd.iSeconds = 5;
		wcsncpy(pd.lptzText, TranslateT("Loading history completed."), MAX_SECONDLINE);
		popupHwnd = (HWND)CallService(MS_POPUP_ADDPOPUPW, (WPARAM)&pd, (LPARAM)0);
	}
	// PUDeletePopup(popupHwnd);
}

std::string truncateUtf8(std::string &text, size_t maxLength) {
	// To not split some unicode character we need to transform it to wchar_t first, then split it, and then convert it back, because we want std::string as result
	// TODO: Probably there is much simpler and nicer way
	std::wstring ttext = ptrW(mir_utf8decodeW(text.c_str()));
	if (ttext.length() > maxLength) {
		ttext = ttext.substr(0, maxLength) + L"\x2026"; // unicode ellipsis
		return std::string(_T2A(ttext.c_str(), CP_UTF8));
	}
	// It's not longer, return given string
	return text;
}

void parseFeeds(const std::string &text, std::vector<facebook_newsfeed *> &news, DWORD &last_post_time, bool filterAds = true) {
	std::string::size_type pos = 0;
	UINT limit = 0;

	DWORD new_time = last_post_time;

	while ((pos = text.find("<div class=\"userContentWrapper", pos)) != std::string::npos && limit <= 25)
	{
		std::string post = text.substr(pos, text.find("</form>", pos) - pos);
		pos += 5;

		std::string post_header = utils::text::source_get_value(&post, 3, "<h5", ">", "</h5>");
		std::string post_message = utils::text::source_get_value(&post, 3, " userContent\"", ">", "<form");
		std::string post_link = utils::text::source_get_value(&post, 4, "</h5>", "<a", "href=\"", "\"");
		std::string post_time = utils::text::source_get_value(&post, 3, "</h5>", "<abbr", "</a>");

		std::string time = utils::text::source_get_value(&post_time, 2, "data-utime=\"", "\"");
		std::string time_text = utils::text::source_get_value(&post_time, 2, ">", "</abbr>");

		if (time.empty()) {
			// alternative parsing (probably page like or advertisement)
			time = utils::text::source_get_value(&post, 2, "content_timestamp&quot;:&quot;", "&quot;");
		}

		DWORD ttime;
		if (!utils::conversion::from_string<DWORD>(ttime, time, std::dec)) {
			//debugLogA("!!! - Newsfeed with wrong/empty time (probably wrong parsing)\n%s", post.c_str());
			continue;
		}

		if (ttime > new_time) {
			new_time = ttime; // remember newest time from all these posts
			//debugLogA("    - Newsfeed time: %d (new)", ttime);
		}
		else if (ttime <= last_post_time) {
			//debugLogA("    - Newsfeed time: %d (ignored)", ttime);
			continue; // ignore posts older than newest post of previous check
		}
		else {
			//debugLogA("    - Newsfeed time: %d (normal)", ttime);
		}

		std::string timeandloc = utils::text::trim(utils::text::html_entities_decode(utils::text::remove_html(time_text)));
		std::string post_place = utils::text::source_get_value(&post, 4, "</abbr>", "<a", ">", "</a>");
		post_place = utils::text::trim(utils::text::remove_html(post_place));
		if (!post_place.empty()) {
			timeandloc += " · " + post_place;
		}

		// in title keep only name, end of events like "X shared link" put into message
		std::string::size_type pos2 = post_header.find("</a>");
		std::string header_author = utils::text::trim(
			utils::text::html_entities_decode(
			utils::text::remove_html(
			post_header.substr(0, pos2))));
		std::string header_rest = utils::text::trim(
			utils::text::html_entities_decode(
			utils::text::remove_html(
			post_header.substr(pos2, post_header.length() - pos2))));

		// Strip "Translate" link
		pos2 = post_message.find("role=\"button\">");
		if (pos2 != std::string::npos) {
			post_message = post_message.substr(0, pos2 + 14);
		}

		// Process attachment (if present)
		std::string post_attachment = "";
		pos2 = post_message.find("class=\"mtm\">");
		if (pos2 != std::string::npos) {
			pos2 += 12;
			post_attachment = post_message.substr(pos2, post_message.length() - pos2);
			post_message = post_message.substr(0, pos2);

			// Add new lines between some elements to improve formatting
			utils::text::replace_all(&post_attachment, "</a>", "</a>\n");
			utils::text::replace_all(&post_attachment, "ellipsis\">", "ellipsis\">\n");

			post_attachment = utils::text::trim(
				utils::text::html_entities_decode(
				utils::text::remove_html(post_attachment)));

			post_attachment = truncateUtf8(post_attachment, MAX_LINK_DESCRIPTION_LEN);

			if (post_attachment.empty()) {
				// This is some textless attachment, so mention it
				post_attachment = Translate("<attachment without text>");
			}
		}

		post_message = utils::text::trim(
			utils::text::html_entities_decode(
			utils::text::remove_html(post_message)));

		// Truncate text of newsfeed when it's too long
		post_message = truncateUtf8(post_message, MAX_NEWSFEED_LEN);

		std::string content = "";
		if (header_rest.length() > 2)
			content += TEXT_ELLIPSIS + header_rest + "\n";
		if (!post_message.empty())
			content += post_message + "\n";
		if (!post_attachment.empty())
			content += TEXT_EMOJI_LINK" " + post_attachment + "\n";
		if (!timeandloc.empty())
			content += TEXT_EMOJI_CLOCK" " + timeandloc;

		facebook_newsfeed* nf = new facebook_newsfeed;

		nf->title = header_author;

		nf->user_id = utils::text::source_get_value(&post_header, 2, "user.php?id=", "&amp;");

		nf->link = utils::text::html_entities_decode(post_link);

		// Check if we don't want to show ads posts
		bool filtered = filterAds && (nf->link.find("/about/ads") != std::string::npos
			|| post.find("class=\"uiStreamSponsoredLink\"") != std::string::npos
			|| post.find("href=\"/about/ads\"") != std::string::npos);

		nf->text = utils::text::trim(content);

		if (filtered || nf->title.empty() || nf->text.empty()) {
			//debugLogA("    \\ Newsfeed (time: %d) is filtered: %s", ttime, filtered ? "advertisement" : (nf->title.empty() ? "title empty" : "text empty"));
			delete nf;
			continue;
		}
		else {
			//debugLogA("    Got newsfeed (time: %d)", ttime);
		}

		news.push_back(nf);
		pos++;
		limit++;
	}

	last_post_time = new_time;
}

void FacebookProto::ProcessMemories(void*)
{
	if (isOffline())
		return;

	facy.handle_entry(__FUNCTION__);

	time_t timestamp = ::time(NULL);
	
	std::string get_data = "&start_index=0&num_stories=20&last_section_header=0";
	get_data += "&timestamp=" + utils::conversion::to_string((void*)&timestamp, UTILS_CONV_TIME_T);
	get_data += "&__dyn=&__req=&__rev=&__user=" + facy.self_.user_id;

	http::response resp = facy.flap(REQUEST_ON_THIS_DAY, NULL, &get_data);

	if (resp.code != HTTP_CODE_OK) {
		facy.handle_error(__FUNCTION__);
		return;
	}

	std::string jsonData = resp.data.substr(9);
	JSONNode root = JSONNode::parse(jsonData.c_str());
	if (root) {
		const JSONNode &html_ = root["domops"].at((json_index_t)0).at((json_index_t)3).at("__html");
		if (html_) {
			std::string html = utils::text::html_entities_decode(utils::text::slashu_to_utf8(html_.as_string()));

			std::vector<facebook_newsfeed *> news;
			DWORD new_time = 0;
			parseFeeds(html, news, new_time, true);

			if (!news.empty()) {
				SkinPlaySound("Memories");
			}

			for (std::vector<facebook_newsfeed*>::size_type i = 0; i < news.size(); i++)
			{
				ptrW tszTitle(mir_utf8decodeW(news[i]->title.c_str()));
				ptrW tszText(mir_utf8decodeW(news[i]->text.c_str()));

				NotifyEvent(TranslateT("On this day"), tszText, NULL, FACEBOOK_EVENT_ON_THIS_DAY, &news[i]->link);
				delete news[i];
			}
			news.clear();
		}
	}

	facy.handle_success(__FUNCTION__);
}

void FacebookProto::ReceiveMessages(std::vector<facebook_message> &messages, bool check_duplicates)
{
	bool naseemsSpamMode = getBool(FACEBOOK_KEY_NASEEMS_SPAM_MODE, false);

	// TODO: make this checking more lightweight as now it is not effective at all...
	if (check_duplicates) {
		// 1. check if there are some message that we already have (compare FACEBOOK_KEY_MESSAGE_ID = last received message ID)
		for (size_t i = 0; i < messages.size(); i++) {
			facebook_message &msg = messages[i];

			MCONTACT hContact = msg.isChat ? ChatIDToHContact(msg.thread_id) : ContactIDToHContact(msg.user_id);
			if (hContact == NULL)
				continue;

			ptrA lastId(getStringA(hContact, FACEBOOK_KEY_MESSAGE_ID));
			if (lastId == NULL)
				continue;

			if (!msg.message_id.compare(lastId)) {
				// Equal, ignore all older messages (including this) from same contact
				for (std::vector<facebook_message*>::size_type j = 0; j < messages.size(); j++) {
					bool equalsId = msg.isChat
						? (messages[j].thread_id == msg.thread_id)
						: (messages[j].user_id == msg.user_id);

					if (equalsId && messages[j].time <= msg.time)
						messages[j].flag_ = 1;
				}
			}
		}

		// 2. remove all marked messages from list
		for (std::vector<facebook_message>::iterator it = messages.begin(); it != messages.end();) {
			if ((*it).flag_ == 1)
				it = messages.erase(it);
			else
				++it;
		}
	}

	std::set<MCONTACT> *hChatContacts = new std::set<MCONTACT>();

	for (std::vector<facebook_message*>::size_type i = 0; i < messages.size(); i++) {
		facebook_message &msg = messages[i];
		if (msg.isChat) {
			if (!m_enableChat)
				continue;

			// Multi-user message
			debugLogA("  < Got chat message ID: %s", msg.message_id.c_str());

			facebook_chatroom *fbc;
			std::string thread_id = msg.thread_id.c_str();

			auto it = facy.chat_rooms.find(thread_id);
			if (it != facy.chat_rooms.end()) {
				fbc = it->second;
			}
			else {
				// In Naseem's spam mode we ignore outgoing messages sent from other instances
				if (naseemsSpamMode && !msg.isIncoming)
					continue;

				// We don't have this chat loaded in memory yet, lets load some info (name, list of users)
				fbc = new facebook_chatroom(thread_id);
				LoadChatInfo(fbc);
				facy.chat_rooms.insert(std::make_pair(thread_id, fbc));
			}

			MCONTACT hChatContact = NULL;
			// RM TODO: better use check if chatroom exists/is in db/is online... no?
			// like: if (ChatIDToHContact(thread_id) == NULL) {
			ptrA users(GetChatUsers(fbc->thread_id.c_str()));
			if (users == NULL) {
				AddChat(fbc->thread_id.c_str(), fbc->chat_name.c_str());
				hChatContact = ChatIDToHContact(fbc->thread_id);
				// Set thread id (TID) for later
				setString(hChatContact, FACEBOOK_KEY_TID, fbc->thread_id.c_str());

				for (auto jt = fbc->participants.begin(); jt != fbc->participants.end(); ++jt) {
					AddChatContact(fbc->thread_id.c_str(), jt->second, false);
				}
			}

			if (!hChatContact)
				hChatContact = ChatIDToHContact(fbc->thread_id);

			if (!hChatContact) {
				// hopefully shouldn't happen, but who knows?
				debugLogW(L"!!! No hChatContact for %s", fbc->thread_id.c_str());
				continue;
			}

			// We don't want to save (this) message ID for chatrooms
			// setString(hChatContact, FACEBOOK_KEY_MESSAGE_ID, msg.message_id.c_str());
			setDword(FACEBOOK_KEY_LAST_ACTION_TS, msg.time);

			// Save TID if not exists already
			ptrA tid(getStringA(hChatContact, FACEBOOK_KEY_TID));
			if (!tid || mir_strcmp(tid, msg.thread_id.c_str()))
				setString(hChatContact, FACEBOOK_KEY_TID, msg.thread_id.c_str());

			// Get name of this chat participant
			std::string name = msg.user_id; // fallback to numeric id

			auto jt = fbc->participants.find(msg.user_id);
			if (jt != fbc->participants.end()) {
				name = jt->second.nick;
			}
			else {
				// TODO: Load info about this participant from server, and add it manually to participants list (maybe only if this is SUBSCRIBE type)
			}

			switch (msg.type) {
			default:
			case MESSAGE:
				UpdateChat(fbc->thread_id.c_str(), msg.user_id.c_str(), name.c_str(), msg.message_text.c_str(), msg.time);
				break;
			case ADMIN_TEXT:
				UpdateChat(thread_id.c_str(), NULL, NULL, msg.message_text.c_str());
				break;
			case SUBSCRIBE:
				// TODO: We must get data with list of added users (their ids) from json to work properly, so for now we just print it as admin message
				UpdateChat(thread_id.c_str(), NULL, NULL, msg.message_text.c_str());
				/*if (jt == fbc->participants.end()) {
					// We don't have this user there yet, so load info about him and then add him
					chatroom_participant participant;
					participant.is_former = false;
					participant.loaded = false;
					participant.nick = msg.data;
					participant.user_id = msg.data;
					AddChatContact(thread_id.c_str(), participant, msg.isUnread);
				}*/
				break;
			case UNSUBSCRIBE:
				RemoveChatContact(thread_id.c_str(), msg.user_id.c_str(), name.c_str());
				break;
			case THREAD_NAME:
				// proto->RenameChat(thread_id.c_str(), msg.data.c_str()); // this don't work, why?
				setStringUtf(hChatContact, FACEBOOK_KEY_NICK, msg.data.c_str());
				UpdateChat(thread_id.c_str(), NULL, NULL, msg.message_text.c_str());
				break;
			case THREAD_IMAGE:
				UpdateChat(thread_id.c_str(), NULL, NULL, msg.message_text.c_str());
				break;
			}

			// Automatically mark message as read because chatroom doesn't support onRead event (yet)
			hChatContacts->insert(hChatContact); // std::set checks duplicates at insert automatically
		}
		else {
			// Single-user message
			debugLogA("  < Got message ID: %s", msg.message_id.c_str());

			facebook_user fbu;
			fbu.user_id = msg.user_id;

			MCONTACT hContact = ContactIDToHContact(fbu.user_id);
			if (hContact == NULL) {
				// In Naseem's spam mode we ignore outgoing messages sent from other instances
				if (naseemsSpamMode && !msg.isIncoming)
					continue;

				// We don't have this contact, lets load info about him
				LoadContactInfo(&fbu);

				hContact = AddToContactList(&fbu);
			}

			if (!hContact) {
				// hopefully shouldn't happen, but who knows?
				debugLogA("!!! No hContact for %s", msg.user_id.c_str());
				continue;
			}

			// Save last (this) message ID
			setString(hContact, FACEBOOK_KEY_MESSAGE_ID, msg.message_id.c_str());

			// Save TID if not exists already
			ptrA tid(getStringA(hContact, FACEBOOK_KEY_TID));
			if ((!tid || mir_strcmp(tid, msg.thread_id.c_str())) && !msg.thread_id.empty())
				setString(hContact, FACEBOOK_KEY_TID, msg.thread_id.c_str());

			if (msg.isIncoming && msg.isUnread && msg.type == MESSAGE) {
				PROTORECVEVENT recv = { 0 };
				recv.szMessage = const_cast<char*>(msg.message_text.c_str());
				recv.timestamp = msg.time;
				ProtoChainRecvMsg(hContact, &recv);
			}
			else {
				DBEVENTINFO dbei = { 0 };
				dbei.cbSize = sizeof(dbei);

				if (msg.type == MESSAGE)
					dbei.eventType = EVENTTYPE_MESSAGE;
				else if (msg.type == CALL)
					dbei.eventType = FACEBOOK_EVENTTYPE_CALL;
				else
					dbei.eventType = EVENTTYPE_URL; // FIXME: Use better and specific type for our other event types.

				dbei.flags = DBEF_UTF;

				if (!msg.isIncoming)
					dbei.flags |= DBEF_SENT;

				if (!msg.isUnread)
					dbei.flags |= DBEF_READ;

				dbei.szModule = m_szModuleName;
				dbei.timestamp = msg.time;
				dbei.cbBlob = (DWORD)msg.message_text.length() + 1;
				dbei.pBlob = (PBYTE)msg.message_text.c_str();
				db_event_add(hContact, &dbei);
			}
		}
	}

	if (!hChatContacts->empty()) {
		ForkThread(&FacebookProto::ReadMessageWorker, (void*)hChatContacts);
	}
	else {
		delete hChatContacts;
	}
}

void FacebookProto::ProcessMessages(void* data)
{
	if (data == NULL)
		return;

	std::string* resp = (std::string*)data;

	if (isOffline()) {
		delete resp;
		return;
	}

	debugLogA("*** Starting processing messages");

	CODE_BLOCK_TRY

	std::vector<facebook_message> messages;

		facebook_json_parser* p = new facebook_json_parser(this);
		p->parse_messages(resp, &messages, &facy.notifications);
		delete p;

		ReceiveMessages(messages);

		if (getBool(FACEBOOK_KEY_EVENT_NOTIFICATIONS_ENABLE, DEFAULT_EVENT_NOTIFICATIONS_ENABLE))
			ShowNotifications();

		debugLogA("*** Messages processed");

	CODE_BLOCK_CATCH

		debugLogA("*** Error processing messages: %s", e.what());

	CODE_BLOCK_END

	delete resp;
}

void FacebookProto::ShowNotifications() {
	ScopedLock s(facy.notifications_lock_);

	// Show popups for unseen notifications and/or write them to chatroom
	for (std::map<std::string, facebook_notification*>::iterator it = facy.notifications.begin(); it != facy.notifications.end(); ++it) {
		facebook_notification *notification = it->second;
		if (notification != NULL && !notification->seen) {
			debugLogA("    Showing popup for notification ID: %s", notification->id.c_str());
			ptrW szText(mir_utf8decodeW(notification->text.c_str()));
			MCONTACT hContact = (notification->user_id.empty() ? NULL : ContactIDToHContact(notification->user_id));
			notification->hWndPopup = NotifyEvent(m_tszUserName, szText, hContact, FACEBOOK_EVENT_NOTIFICATION, &notification->link, &notification->id, notification->icon);
			notification->seen = true;
		}
	}
}

void FacebookProto::ProcessNotifications(void*)
{
	if (isOffline())
		return;

	facy.handle_entry("notifications");

	int count = FACEBOOK_NOTIFICATIONS_LOAD_COUNT;

	std::string data = "__user=" + facy.self_.user_id;
	data += "&fb_dtsg=" + facy.dtsg_;
	data += "&cursor="; // when loading more
	data += "&length=" + utils::conversion::to_string(&count, UTILS_CONV_UNSIGNED_NUMBER); // number of items to load
	data += "&businessID="; // probably for pages?
	data += "&ttstamp=" + facy.ttstamp_;
	data += "&__dyn=" + facy.__dyn();
	data += "&__req=" + facy.__req();
	data += "&__rev=" + facy.__rev();
	data += "&__pc=PHASED:DEFAULT&__be=-1&__a=1";

	// Get notifications
	http::response resp = facy.flap(REQUEST_NOTIFICATIONS, &data); // NOTE: Request revised 17.8.2016

	if (resp.code != HTTP_CODE_OK) {
		facy.handle_error("notifications");
		return;
	}

	// Process notifications
	debugLogA("*** Starting processing notifications");

	CODE_BLOCK_TRY

	facebook_json_parser* p = new facebook_json_parser(this);
	p->parse_notifications(&(resp.data), &facy.notifications);
	delete p;

	ShowNotifications();

	debugLogA("*** Notifications processed");

	CODE_BLOCK_CATCH

		debugLogA("*** Error processing notifications: %s", e.what());

	CODE_BLOCK_END
}

void FacebookProto::ProcessFriendRequests(void*)
{
	if (isOffline())
		return;

	facy.handle_entry("friendRequests");

	// Get notifications
	http::response resp = facy.flap(REQUEST_LOAD_FRIENDSHIPS);

	// Workaround not working "mbasic." website for some people
	if (!resp.isValid()) {
		// Remember it didn't worked and try it again (internally it will try "m." this time)
		facy.mbasicWorks = false;
		resp = facy.flap(REQUEST_LOAD_FRIENDSHIPS);
	}

	if (resp.code != HTTP_CODE_OK) {
		facy.handle_error("friendRequests");
		return;
	}

	// Parse it
	std::string reqs = utils::text::source_get_value(&resp.data, 3, "id=\"friends_center_main\"", "</h3>", "/friends/center/suggestions/");

	std::string::size_type pos = 0;
	std::string::size_type pos2 = 0;
	bool last = (reqs.find("seenrequesttime=") == std::string::npos); // false when there are some requests

	while (!last && !reqs.empty()) {
		std::string req;
		if ((pos2 = reqs.find("</table>", pos)) != std::string::npos) {
			req = reqs.substr(pos, pos2 - pos);
			pos = pos2 + 8;
		} else {
			req = reqs.substr(pos);
			last = true;
		}
		
		std::string get = utils::text::source_get_value(&req, 2, "notifications.php?", "\"");
		std::string time = utils::text::source_get_value2(&get, "seenrequesttime=", "&\"");
		std::string reason = utils::text::remove_html(utils::text::source_get_value(&req, 4, "</a>", "<div", ">", "</div>"));

		facebook_user fbu;
		fbu.real_name = utils::text::remove_html(utils::text::source_get_value(&req, 3, "<a", ">", "</a>"));
		fbu.user_id = utils::text::source_get_value2(&get, "confirm=", "&\"");
		fbu.type = CONTACT_APPROVE;

		if (!fbu.user_id.empty() && !fbu.real_name.empty()) {
			MCONTACT hContact = AddToContactList(&fbu);
			setByte(hContact, FACEBOOK_KEY_CONTACT_TYPE, CONTACT_APPROVE);

			bool isNew = false;
			ptrA oldTime(getStringA(hContact, "RequestTime"));
			if (oldTime == NULL || mir_strcmp(oldTime, time.c_str())) {
				// This is new request
				isNew = true;
				setString(hContact, "RequestTime", time.c_str());

				//blob is: uin(DWORD), hContact(HANDLE), nick(ASCIIZ), first(ASCIIZ), last(ASCIIZ), email(ASCIIZ), reason(ASCIIZ)
				//blob is: 0(DWORD), hContact(HANDLE), nick(ASCIIZ), ""(ASCIIZ), ""(ASCIIZ), email(ASCIIZ), reason(ASCIIZ)
				DBEVENTINFO dbei = { 0 };
				dbei.cbSize = sizeof(DBEVENTINFO);
				dbei.szModule = m_szModuleName;
				dbei.timestamp = ::time(NULL);
				dbei.flags = DBEF_UTF;
				dbei.eventType = EVENTTYPE_AUTHREQUEST;
				dbei.cbBlob = (DWORD)(sizeof(DWORD) * 2 + fbu.real_name.length() + fbu.user_id.length() + reason.length() + 5);

				PBYTE pCurBlob = dbei.pBlob = (PBYTE)mir_alloc(dbei.cbBlob);
				*(PDWORD)pCurBlob = 0; pCurBlob += sizeof(DWORD);                    // UID
				*(PDWORD)pCurBlob = (DWORD)hContact; pCurBlob += sizeof(DWORD);      // Contact Handle
				mir_strcpy((char*)pCurBlob, fbu.real_name.data()); pCurBlob += fbu.real_name.length() + 1;	// Nickname
				*pCurBlob = '\0'; pCurBlob++;                                        // First Name
				*pCurBlob = '\0'; pCurBlob++;                                        // Last Name
				mir_strcpy((char*)pCurBlob, fbu.user_id.data()); pCurBlob += fbu.user_id.length() + 1;	// E-mail (we use it for string ID)
				mir_strcpy((char*)pCurBlob, reason.data()); pCurBlob += reason.length() + 1;	// Reason (we use it for info about common friends)

				db_event_add(0, &dbei);
			}
			debugLogA("  < (%s) Friendship request [%s]", (isNew ? "New" : "Old"), time.c_str());
		} else {
			debugLogA("!!! Wrong friendship request:\n%s", req.c_str());
		}
	}

	facy.handle_success("friendRequests");
}

void FacebookProto::ProcessFeeds(void*)
{
	if (!isOnline())
		return;

	facy.handle_entry("feeds");

	// Get feeds
	http::response resp = facy.flap(REQUEST_FEEDS);

	if (resp.code != HTTP_CODE_OK || resp.data.empty()) {
		facy.handle_error("feeds");
		return;
	}

	std::vector<facebook_newsfeed *> news;
	DWORD new_time = facy.last_feeds_update_;
	bool filterAds = getBool(FACEBOOK_KEY_FILTER_ADS, DEFAULT_FILTER_ADS);

	parseFeeds(resp.data, news, new_time, filterAds);

	if (!news.empty()) {
		SkinPlaySound("NewsFeed");
	}

	for (std::vector<facebook_newsfeed*>::size_type i = 0; i < news.size(); i++)
	{
		ptrW tszTitle(mir_utf8decodeW(news[i]->title.c_str()));
		ptrW tszText(mir_utf8decodeW(news[i]->text.c_str()));
		MCONTACT hContact = ContactIDToHContact(news[i]->user_id);

		NotifyEvent(tszTitle, tszText, hContact, FACEBOOK_EVENT_NEWSFEED, &news[i]->link);
		delete news[i];
	}
	news.clear();

	// Set time of last update to time of newest post
	this->facy.last_feeds_update_ = new_time;

	facy.handle_success("feeds");
}

void FacebookProto::ProcessPages(void*)
{
	if (isOffline() || !getByte(FACEBOOK_KEY_LOAD_PAGES, DEFAULT_LOAD_PAGES))
		return;

	facy.handle_entry("load_pages");

	// Get feeds
	http::response resp = facy.flap(REQUEST_PAGES);

	if (resp.code != HTTP_CODE_OK) {
		facy.handle_error("load_pages");
		return;
	}

	std::string content = utils::text::source_get_value(&resp.data, 2, "id=\"bookmarksSeeAllSection\"", "</code>");

	std::string::size_type start, end;
	start = content.find("<li", 0);
	while (start != std::string::npos) {
		end = content.find("<li", start + 1);
		if (end == std::string::npos)
			end = content.length();

		std::string item = content.substr(start, end - start);
		//item = utils::text::source_get_value(&item, 2, "data-gt=", ">");

		start = content.find("<li", start + 1);

		std::string id = utils::text::source_get_value(&item, 3, "data-gt=", "bmid&quot;:&quot;", "&quot;");
		std::string title = utils::text::slashu_to_utf8(utils::text::source_get_value(&item, 3, "data-gt=", "title=\"", "\""));
		std::string href = utils::text::source_get_value(&item, 3, "data-gt=", "href=\"", "\"");

		// Ignore pages channel
		if (href.find("/pages/feed") != std::string::npos)
			continue;

		if (id.empty() || title.empty())
			continue;

		debugLogA("    Got page ID: %s", id.c_str());
		facy.pages[id] = title;
	}

	facy.handle_success("load_pages");
}

void FacebookProto::SearchAckThread(void *targ)
{
	facy.handle_entry("searchAckThread");

	int count = 0;

	std::string search = utils::url::encode(T2Utf((wchar_t *)targ).str());
	std::string ssid;

	while (count < 50 && !isOffline())
	{
		std::string get_data = search + "&s=" + utils::conversion::to_string(&count, UTILS_CONV_UNSIGNED_NUMBER);
		if (!ssid.empty())
			get_data += "&ssid=" + ssid;

		http::response resp = facy.flap(REQUEST_SEARCH, NULL, &get_data);

		if (resp.code == HTTP_CODE_OK)
		{
			std::string items = utils::text::source_get_value(&resp.data, 4, "<body", "name=\"charset_test\"", "<table", "</body>");

			std::string::size_type pos = 0;
			std::string::size_type pos2 = 0;

			while ((pos = items.find("<tr", pos)) != std::string::npos) {
				std::string item = items.substr(pos, items.find("</tr>", pos) - pos);
				pos++; count++;

				std::string id = utils::text::source_get_value2(&item, "?id=", "&\"");
				if (id.empty())
					id = utils::text::source_get_value2(&item, "?ids=", "&\"");

				std::string name = utils::text::source_get_value(&item, 3, "<a", ">", "</");
				std::string surname;
				std::string nick;
				std::string common = utils::text::source_get_value(&item, 4, "</a>", "<span", ">", "</span>");

				if ((pos2 = name.find("<span class=\"alternate_name\">")) != std::string::npos) {
					nick = name.substr(pos2 + 30, name.length() - pos2 - 31); // also remove brackets around nickname
					name = name.substr(0, pos2 - 1);
				}

				if ((pos2 = name.find(" ")) != std::string::npos) {
					surname = name.substr(pos2 + 1, name.length() - pos2 - 1);
					name = name.substr(0, pos2);
				}

				// ignore self contact and empty ids
				if (id.empty() || id == facy.self_.user_id)
					continue;

				ptrW tid(mir_utf8decodeW(id.c_str()));
				ptrW tname(mir_utf8decodeW(utils::text::html_entities_decode(name).c_str()));
				ptrW tsurname(mir_utf8decodeW(utils::text::html_entities_decode(surname).c_str()));
				ptrW tnick(mir_utf8decodeW(utils::text::html_entities_decode(nick).c_str()));
				ptrW tcommon(mir_utf8decodeW(utils::text::html_entities_decode(common).c_str()));

				PROTOSEARCHRESULT psr = { 0 };
				psr.cbSize = sizeof(psr);
				psr.flags = PSR_UNICODE;
				psr.id.w = tid;
				psr.nick.w = tnick;
				psr.firstName.w = tname;
				psr.lastName.w = tsurname;
				psr.email.w = tcommon;
				ProtoBroadcastAck(NULL, ACKTYPE_SEARCH, ACKRESULT_DATA, targ, (LPARAM)&psr);
			}

			ssid = utils::text::source_get_value(&items, 3, "id=\"more_objects\"", "ssid=", "&");
			if (ssid.empty())
				break; // No more results
		}
	}

	ProtoBroadcastAck(NULL, ACKTYPE_SEARCH, ACKRESULT_SUCCESS, targ, 0);

	facy.handle_success("searchAckThread");

	mir_free(targ);
}

void FacebookProto::SearchIdAckThread(void *targ)
{
	facy.handle_entry("searchIdAckThread");

	std::string search = utils::url::encode(T2Utf((wchar_t*)targ).str()) + "?";

	if (!isOffline())
	{
		http::response resp = facy.flap(REQUEST_USER_INFO_MOBILE, NULL, &search);

		if (resp.code == HTTP_CODE_FOUND && resp.headers.find("Location") != resp.headers.end()) {
			search = utils::text::source_get_value(&resp.headers["Location"], 2, FACEBOOK_SERVER_MBASIC"/", "_rdr", true);
			resp = facy.flap(REQUEST_USER_INFO_MOBILE, NULL, &search);
		}

		if (resp.code == HTTP_CODE_OK)
		{
			std::string about = utils::text::source_get_value(&resp.data, 2, "<div id=\"root\"", "</body>");

			std::string id = utils::text::source_get_value2(&about, ";id=", "&\"");
			if (id.empty())
				id = utils::text::source_get_value2(&about, "?bid=", "&\"");
			std::string name = utils::text::source_get_value(&about, 3, "<strong", ">", "</strong");
			std::string surname;

			std::string::size_type pos;
			if ((pos = name.find(" ")) != std::string::npos) {
				surname = name.substr(pos + 1, name.length() - pos - 1);
				name = name.substr(0, pos);
			}

			// ignore self contact and empty ids
			if (!id.empty() && id != facy.self_.user_id){
				ptrW tid(mir_utf8decodeW(id.c_str()));
				ptrW tname(mir_utf8decodeW(name.c_str()));
				ptrW tsurname(mir_utf8decodeW(surname.c_str()));

				PROTOSEARCHRESULT psr = { 0 };
				psr.cbSize = sizeof(psr);
				psr.flags = PSR_UNICODE;
				psr.id.w = tid;
				psr.firstName.w = tname;
				psr.lastName.w = tsurname;
				ProtoBroadcastAck(NULL, ACKTYPE_SEARCH, ACKRESULT_DATA, targ, (LPARAM)&psr);
			}
		}
	}

	ProtoBroadcastAck(NULL, ACKTYPE_SEARCH, ACKRESULT_SUCCESS, targ, 0);

	facy.handle_success("searchIdAckThread");

	mir_free(targ);
}
