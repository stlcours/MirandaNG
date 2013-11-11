/*

Facebook plugin for Miranda Instant Messenger
_____________________________________________

Copyright � 2009-11 Michal Zelinka, 2011-13 Robert P�sel

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

#include "common.h"

int facebook_json_parser::parse_buddy_list(void* data, List::List< facebook_user >* buddy_list)
{
	facebook_user* current = NULL;
	std::string jsonData = static_cast< std::string* >(data)->substr(9);

	JSONNODE *root = json_parse(jsonData.c_str());
	if (root == NULL)
		return EXIT_FAILURE;

	JSONNODE *payload = json_get(root, "payload");
	if (payload == NULL) {
		json_delete(root);
		return EXIT_FAILURE;
	}

	JSONNODE *list = json_get(payload, "buddy_list");
	if (list == NULL) {
		json_delete(root);
		return EXIT_FAILURE;
	}

	// Set all contacts in map to offline
	for (List::Item< facebook_user >* i = buddy_list->begin(); i != NULL; i = i->next) {
		i->data->status_id = ID_STATUS_OFFLINE;
	}
	
	// Load last active times
	JSONNODE *lastActive = json_get(list, "last_active_times");
	if (lastActive != NULL) {
		for (unsigned int i = 0; i < json_size(lastActive); i++) {
			JSONNODE *it = json_at(lastActive, i);
			const char *id = json_name(it);
			
			current = buddy_list->find(id);
			if (current == NULL) {
				buddy_list->insert(std::make_pair(id, new facebook_user()));
				current = buddy_list->find(id);
				current->user_id = id;
			}

			current->last_active = json_as_int(it);
		}
	}
	
	// Find mobile friends
	JSONNODE *mobileFriends = json_get(list, "mobile_friends");
	if (mobileFriends != NULL) {
		for (unsigned int i = 0; i < json_size(mobileFriends); i++) {
			JSONNODE *it = json_at(mobileFriends, i);
			std::string id = json_as_pstring(it);
			
			current = buddy_list->find(id);
			if (current == NULL) {
				buddy_list->insert(std::make_pair(id, new facebook_user()));
				current = buddy_list->find(id);
				current->user_id = id;
			}

			current->status_id = ID_STATUS_ONTHEPHONE;
		}
	}

	// Find now available contacts
	JSONNODE *nowAvailable = json_get(list, "nowAvailableList");
	if (nowAvailable != NULL) {		
		for (unsigned int i = 0; i < json_size(nowAvailable); i++) {
			JSONNODE *it = json_at(nowAvailable, i);
			const char *id = json_name(it);
			
			current = buddy_list->find(id);
			if (current == NULL) {
				buddy_list->insert(std::make_pair(id, new facebook_user()));
				current = buddy_list->find(id);
				current->user_id = id;
			}

			current->status_id = ID_STATUS_ONLINE;
			
			// In new version of Facebook "i" means "offline"
			JSONNODE *idle = json_get(it, "i");
			if (idle != NULL && json_as_bool(idle))
				current->status_id = ID_STATUS_OFFLINE;
		}
	}

	// Get aditional informations about contacts (if available)
	JSONNODE *userInfos = json_get(list, "userInfos");
	if (userInfos != NULL) {		
		for (unsigned int i = 0; i < json_size(userInfos); i++) {
			JSONNODE *it = json_at(userInfos, i);
			const char *id = json_name(it);
			
			current = buddy_list->find(id);
			if (current == NULL)
				continue;

			JSONNODE *name = json_get(it, "name");
			JSONNODE *thumbSrc = json_get(it, "thumbSrc");

			if (name != NULL)
				current->real_name = utils::text::slashu_to_utf8(utils::text::special_expressions_decode(json_as_pstring(name)));
			if (thumbSrc != NULL)			
				current->image_url = utils::text::slashu_to_utf8(utils::text::special_expressions_decode(json_as_pstring(thumbSrc)));
		}
	}

	json_delete(root);
	return EXIT_SUCCESS;
}

int facebook_json_parser::parse_friends(void* data, std::map< std::string, facebook_user* >* friends)
{
	std::string jsonData = static_cast< std::string* >(data)->substr(9);
	
	JSONNODE *root = json_parse(jsonData.c_str());
	if (root == NULL)
		return EXIT_FAILURE;

	JSONNODE *payload = json_get(root, "payload");
	if (payload == NULL) {
		json_delete(root);
		return EXIT_FAILURE;
	}

	for (unsigned int i = 0; i < json_size(payload); i++) {
		JSONNODE *it = json_at(payload, i);
		const char *id = json_name(it);

		JSONNODE *name = json_get(it, "name");
		JSONNODE *thumbSrc = json_get(it, "thumbSrc");
		JSONNODE *gender = json_get(it, "gender");
		//JSONNODE *vanity = json_get(it, "vanity"); // username
		//JSONNODE *uri = json_get(it, "uri"); // profile url
		//JSONNODE *is_friend = json_get(it, "is_friend"); // e.g. "True"
		//JSONNODE *type = json_get(it, "type"); // e.g. "friend" (classic contact) or "user" (disabled/deleted account)

		facebook_user *fbu = new facebook_user();

		fbu->user_id = id;
		if (name)
			fbu->real_name = utils::text::slashu_to_utf8(utils::text::special_expressions_decode(json_as_pstring(name)));
		if (thumbSrc)
			fbu->image_url = utils::text::slashu_to_utf8(utils::text::special_expressions_decode(json_as_pstring(thumbSrc)));

		if (gender)
			switch (json_as_int(gender)) {
			case 1: // female
				fbu->gender = 70;
				break;
			case 2: // male
				fbu-> gender = 77;
				break;
			}

		friends->insert(std::make_pair(id, fbu));
	}

	json_delete(root);
	return EXIT_SUCCESS;
}


int facebook_json_parser::parse_notifications(void *data, std::vector< facebook_notification* > *notifications) 
{
	std::string jsonData = static_cast< std::string* >(data)->substr(9);
	
	JSONNODE *root = json_parse(jsonData.c_str());
	if (root == NULL)
		return EXIT_FAILURE;

	JSONNODE *payload = json_get(root, "payload");
	if (payload == NULL) {
		json_delete(root);
		return EXIT_FAILURE;
	}

	JSONNODE *list = json_get(payload, "notifications");
	if (list == NULL) {
		json_delete(root);
		return EXIT_FAILURE;
	}

	for (unsigned int i = 0; i < json_size(list); i++) {
		JSONNODE *it = json_at(list, i);
		const char *id = json_name(it);

		JSONNODE *markup = json_get(it, "markup");
		JSONNODE *unread = json_get(it, "unread");
		//JSONNODE *time = json_get(it, "time");

		// Ignore empty and old notifications
		if (markup == NULL || unread == NULL || json_as_int(unread) == 0)
			continue;

		std::string text = utils::text::slashu_to_utf8(utils::text::special_expressions_decode(json_as_pstring(markup)));

		facebook_notification* notification = new facebook_notification();

		notification->id = id;
		notification->link = utils::text::source_get_value(&text, 3, "<a ", "href=\"", "\"");
		notification->text = utils::text::remove_html(utils::text::source_get_value(&text, 1, "<abbr"));		

		notifications->push_back(notification);
	}

	json_delete(root);
	return EXIT_SUCCESS;
}

bool ignore_duplicits(FacebookProto *proto, std::string mid, std::string text)
{
	std::map<std::string, bool>::iterator it = proto->facy.messages_ignore.find(mid);
	if (it != proto->facy.messages_ignore.end()) {
		std::string msg = "????? Ignoring duplicit/sent message\n" + text;
		proto->debugLogA(msg.c_str());

		it->second = true; // mark to delete it at the end
		return true;
	}
					
	// remember this id to ignore duplicits
	proto->facy.messages_ignore.insert(std::make_pair(mid, true));
	return false;
}

void parseAttachments(FacebookProto *proto, std::string *message_text, JSONNODE *it)
{
	// Process attachements and stickers
	JSONNODE *has_attachment = json_get(it, "has_attachment");
	if (has_attachment != NULL && json_as_bool(has_attachment)) {
		// Append attachements
		std::string type = "";
		std::string attachments_text = "";
		JSONNODE *attachments = json_get(it, "attachments");
		for (unsigned int j = 0; j < json_size(attachments); j++) {
			JSONNODE *itAttachment = json_at(attachments, j);
			JSONNODE *attach_type = json_get(itAttachment, "attach_type"); // "sticker", "photo", "file"
			if (attach_type != NULL) {
				// Get attachment type - "file" has priority over other types
				if (type.empty() || type != "file")
					type = json_as_pstring(attach_type);
			}
			JSONNODE *name = json_get(itAttachment, "name");
			JSONNODE *url = json_get(itAttachment, "url");
			if (url != NULL) {
				std::string link = json_as_pstring(url);
							
				if (link.find("/ajax/mercury/attachments/photo/view/") != std::string::npos)
					// fix photo url
					link = utils::url::decode(utils::text::source_get_value(&link, 2, "?uri=", "&"));
				else if (link.find("/") == 0) {
					// make absolute url
					bool useHttps = proto->getByte(FACEBOOK_KEY_FORCE_HTTPS, 1) > 0;
					link = (useHttps ? HTTP_PROTO_SECURE : HTTP_PROTO_REGULAR) + std::string(FACEBOOK_SERVER_REGULAR) + link;
				}
								
				if (!link.empty()) {
					std::string filename;
					if (name != NULL)
						filename = json_as_pstring(name);
					if (filename == "null")
						filename.clear();

					attachments_text += "\n" + (!filename.empty() ? "<" + filename + "> " : "") + link + "\n";
				}
			}
		}

		if (!attachments_text.empty()) {
			// TODO: have this as extra event, not replace or append message content
			if (!message_text->empty())
				*message_text += "\n\n";
			
			// we can't use this as offline messages doesn't have it
			/* JSONNODE *admin_snippet = json_get(it, "admin_snippet");
			if (admin_snippet != NULL) {
				*message_text += json_as_pstring(admin_snippet);
			} */

			std::tstring newText;
			if (type == "sticker")
				newText = TranslateT("a sticker");
			else if (type == "file")
				newText = (json_size(attachments) > 1) ? TranslateT("files") : TranslateT("a file");
			else if (type == "photo")
				newText = (json_size(attachments) > 1) ? TranslateT("photos") : TranslateT("a photo");
			else
				newText = _A2T(type.c_str());

			TCHAR title[200];
			mir_sntprintf(title, SIZEOF(title), TranslateT("User sent you %s:"), newText.c_str()); 

			*message_text += ptrA(mir_utf8encodeT(title));
			*message_text += attachments_text;
		}
	}
}

int facebook_json_parser::parse_messages(void* data, std::vector< facebook_message* >* messages, std::vector< facebook_notification* >* notifications)
{
	std::string jsonData = static_cast< std::string* >(data)->substr(9);

	JSONNODE *root = json_parse(jsonData.c_str());
	if (root == NULL)
		return EXIT_FAILURE;

	JSONNODE *ms = json_get(root, "ms");
	if (ms == NULL) {
		json_delete(root);
		return EXIT_FAILURE;
	}

	for (unsigned int i = 0; i < json_size(ms); i++) {
		JSONNODE *it = json_at(ms, i);
		
		JSONNODE *type = json_get(it, "type");
		if (type == NULL)
			continue;

		std::string t = json_as_pstring(type);
		if (t == "messaging") {
			// we use this only for incomming messages (and getting seen info)

			JSONNODE *type = json_get(it, "event");
			if (type == NULL)
				continue;

			std::string t = json_as_pstring(type);

			if (t == "read_receipt") {
				// user read message
				JSONNODE *reader = json_get(it, "reader");
				JSONNODE *time = json_get(it, "time");

				if (reader == NULL || time == NULL)
					continue;

				JSONNODE *threadid = json_get(it, "tid");
				if (threadid != NULL) { // multi user chat
					std::string tid = json_as_pstring(threadid);
					std::string reader_id = json_as_pstring(reader);

					std::map<std::string, facebook_chatroom>::iterator chatroom = proto->facy.chat_rooms.find(tid);
					if (chatroom != proto->facy.chat_rooms.end()) {
						std::map<std::string, std::string>::const_iterator participant = chatroom->second.participants.find(reader_id);
						if (participant == chatroom->second.participants.end()) {
							std::string search = utils::url::encode(tid) + "?";
							http::response resp = proto->facy.flap(REQUEST_USER_INFO, NULL, &search);

							if (resp.code == HTTP_CODE_FOUND && resp.headers.find("Location") != resp.headers.end()) {
								search = utils::text::source_get_value(&resp.headers["Location"], 2, FACEBOOK_SERVER_MOBILE"/", "_rdr", true);
								resp = proto->facy.flap(REQUEST_USER_INFO, NULL, &search);
							}

							if (resp.code == HTTP_CODE_OK) {
								std::string about = utils::text::source_get_value(&resp.data, 2, "<div class=\"timeline", "<div id=\"footer");

								std::string name = utils::text::source_get_value(&about, 3, "class=\"profileName", ">", "</");
								std::string surname;

								std::string::size_type pos;
								if ((pos = name.find(" ")) != std::string::npos) {
									surname = name.substr(pos + 1, name.length() - pos - 1);
									name = name.substr(0, pos);
								}

								proto->AddChatContact(tid.c_str(), reader_id.c_str(), name.c_str());
							}
						}

						participant = chatroom->second.participants.find(reader_id);
						if (participant != chatroom->second.participants.end()) {
							HANDLE hChatContact = proto->ChatIDToHContact(tid);
							const char *participant_name = participant->second.c_str();

							if (!chatroom->second.message_readers.empty())
								chatroom->second.message_readers += ", ";
							chatroom->second.message_readers += participant_name;

							TCHAR ttime[64], tstr[100];
							_tcsftime(ttime, SIZEOF(ttime), _T("%X"), utils::conversion::fbtime_to_timeinfo(json_as_float(time)));
							mir_sntprintf(tstr, SIZEOF(tstr), TranslateT("Message read: %s by %s"), ttime, ptrT(mir_utf8decodeT(chatroom->second.message_readers.c_str())));
							CallService(MS_MSG_SETSTATUSTEXT, (WPARAM)hChatContact, (LPARAM)tstr);
						}
					}
 				} else { // classic contact
					HANDLE hContact = proto->ContactIDToHContact(json_as_pstring(reader));
					if (hContact) {
						TCHAR ttime[64], tstr[100];
						_tcsftime(ttime, SIZEOF(ttime), _T("%X"), utils::conversion::fbtime_to_timeinfo(json_as_float(time)));
						mir_sntprintf(tstr, SIZEOF(tstr), TranslateT("Message read: %s"), ttime);

						CallService(MS_MSG_SETSTATUSTEXT, (WPARAM)hContact, (LPARAM)tstr);
					}
				}
			} else if (t == "deliver") {
				// inbox message (multiuser or direct)

				JSONNODE *msg = json_get(it, "message");

				JSONNODE *sender_fbid = json_get(msg, "sender_fbid");
				JSONNODE *sender_name = json_get(msg, "sender_name");
				JSONNODE *body = json_get(msg, "body");
				JSONNODE *tid = json_get(msg, "tid");
				JSONNODE *mid = json_get(msg, "mid");
				JSONNODE *timestamp = json_get(msg, "timestamp");

				if (sender_fbid == NULL || sender_name == NULL || body == NULL || mid == NULL || timestamp == NULL)
					continue;

				std::string id = json_as_pstring(sender_fbid);
				std::string message_id = json_as_pstring(mid);
				std::string message_text = json_as_pstring(body);

				// Process attachements and stickers
				parseAttachments(proto, &message_text, msg);

				// Ignore messages from myself, as there is no id of recipient
				if (id == proto->facy.self_.user_id)
					continue;

				// Ignore duplicits or messages sent from miranda
				if (body == NULL || ignore_duplicits(proto, message_id, message_text))
					continue;

				message_text = utils::text::trim(utils::text::special_expressions_decode(utils::text::slashu_to_utf8(message_text)), true);
				if (message_text.empty())
					continue;

				// Multi-user
				JSONNODE *gthreadinfo = json_get(msg, "group_thread_info");
				if (gthreadinfo != NULL) {
					std::string thread_id = json_as_pstring(tid);

					// This is a recent 5 person listing of participants.
					JSONNODE *participants_ids = json_get(gthreadinfo, "participant_ids");
					JSONNODE *participants_names = json_get(gthreadinfo, "participant_names");

					JSONNODE *thread_name_ = json_get(gthreadinfo, "name");
					std::string name = json_as_pstring(thread_name_);

					// if there is no name for this room, set own name
					if (name.empty()) {
						unsigned int namesCount = 3; // how many names should be in room name; max. 5

						for (unsigned int n = 0; n < json_size(participants_names) && n < namesCount; n++) {
							JSONNODE *nameItr = json_at(participants_names, n);
							
							if (!name.empty())
								name += ", ";

							name += json_as_pstring(nameItr);
						}
						JSONNODE *count_ = json_get(gthreadinfo, "participant_total_count");
						unsigned int count = json_as_int(count_);

						if (count > namesCount) {
							char more[200];
							mir_snprintf(more, SIZEOF(more), Translate("%s and more (%d)"), name.c_str(), count - namesCount);
							name = more;
						}
					}

					HANDLE hChatContact;

					// RM TODO: better use check if chatroom exists/is in db/is online... no?
					/// e.g. HANDLE hChatContact = proto->ChatIDToHContact(thread_id); ?
					if (proto->GetChatUsers(thread_id.c_str()) == NULL) {
						// Set thread id (TID) for later.
						proto->AddChat(thread_id.c_str(), name.c_str());
						hChatContact = proto->ChatIDToHContact(thread_id);
						proto->setString(hChatContact, FACEBOOK_KEY_TID, thread_id.c_str());
					}

					if (!hChatContact)
						hChatContact = proto->ChatIDToHContact(thread_id);

					for (unsigned int n = 0; n < json_size(participants_ids); n++) {
						JSONNODE *idItr = json_at(participants_ids, n);
						std::string pId = json_as_pstring(idItr);

						JSONNODE *nameItr = json_at(participants_names, n);
						std::string pName = json_as_pstring(nameItr);

						if (!proto->IsChatContact(thread_id.c_str(), pId.c_str())) {
							proto->AddChatContact(thread_id.c_str(), pId.c_str(), pName.c_str());
						}
					}

					std::string senderName = json_as_pstring(sender_name);
					/*std::string::size_type pos;
					if ((pos = senderName.find(" ")) != std::string::npos) {
						senderName = senderName.substr(0, pos);							
					}*/

					// Last fall-back for adding this sender (Incase was not in the participants) - Is this even needed?
					if (!proto->IsChatContact(thread_id.c_str(), id.c_str())) {
						proto->AddChatContact(thread_id.c_str(), id.c_str(), senderName.c_str());
					}

					// Update chat with message.
					proto->UpdateChat(thread_id.c_str(), id.c_str(), senderName.c_str(), message_text.c_str(), utils::time::fix_timestamp(json_as_float(timestamp)));
					proto->setString(hChatContact, FACEBOOK_KEY_MESSAGE_ID, message_id.c_str());
					proto->ForkThread(&FacebookProto::ReadMessageWorker, hChatContact);
				} else {
					facebook_message* message = new facebook_message();
					message->message_text = message_text;
					message->sender_name = utils::text::special_expressions_decode(utils::text::slashu_to_utf8(json_as_pstring(sender_name)));
					message->time = utils::time::fix_timestamp(json_as_float(timestamp));
					message->user_id = id; // TODO: Check if we have contact with this ID in friendlist and otherwise do something different?
					message->message_id = message_id;

					messages->push_back(message);
				}
			}
		} else if (t == "notification_json") {
			// event notification

			if (!proto->getByte(FACEBOOK_KEY_EVENT_NOTIFICATIONS_ENABLE, DEFAULT_EVENT_NOTIFICATIONS_ENABLE))
				continue;

			JSONNODE *nodes = json_get(it, "nodes");
			for (unsigned int j = 0; j < json_size(nodes); j++) {
				JSONNODE *itNodes = json_at(nodes, j);

				//JSONNODE *text = json_get(itNodes, "title/text"); // use this when popups will be ready to allow changes of their content
				JSONNODE *text_ = json_get(itNodes, "unaggregatedTitle"); // notifications one by one, not grouped
				if (text_ == NULL)
					continue;
				JSONNODE *text = json_get(text_, "text");
				JSONNODE *url = json_get(itNodes, "url");
				JSONNODE *alert_id = json_get(itNodes, "alert_id");
				
				JSONNODE *time_ = json_get(itNodes, "timestamp");
				if (time_ == NULL)
					continue;
				JSONNODE *time = json_get(time_, "time");
				if (time == NULL || text == NULL || url == NULL || alert_id == NULL || time == NULL)
					continue;

				unsigned __int64 timestamp = json_as_float(time);
				if (timestamp > proto->facy.last_notification_time_) {
					// Only new notifications
					proto->facy.last_notification_time_ = timestamp;

					facebook_notification* notification = new facebook_notification();
					notification->text = utils::text::slashu_to_utf8(json_as_pstring(text));
  					notification->link = utils::text::special_expressions_decode(json_as_pstring(url));					
					notification->id = json_as_pstring(alert_id);

					std::string::size_type pos = notification->id.find(":");
					if (pos != std::string::npos)
						notification->id = notification->id.substr(pos+1);

					notifications->push_back(notification);
				}
			}
		} else if (t == "typ") {
			// chat typing notification

			JSONNODE *from = json_get(it, "from");
			if (from == NULL)
				continue;

			facebook_user fbu;
			fbu.user_id = json_as_pstring(from);

			HANDLE hContact = proto->AddToContactList(&fbu, CONTACT_FRIEND);
				
			if (proto->isOnline() && proto->getWord(hContact, "Status", 0) == ID_STATUS_OFFLINE)
				proto->setWord(hContact, "Status", ID_STATUS_ONLINE);

			JSONNODE *st = json_get(it, "st");
			if (json_as_int(st) == 1)
				CallService(MS_PROTO_CONTACTISTYPING, (WPARAM)hContact, (LPARAM)60);
			else
				CallService(MS_PROTO_CONTACTISTYPING, (WPARAM)hContact, (LPARAM)PROTOTYPE_CONTACTTYPING_OFF);
		} else if (t == "privacy_changed") {
			// settings changed

			JSONNODE *event_type = json_get(it, "event");
			JSONNODE *event_data = json_get(it, "data");

			if (event_type == NULL || event_data == NULL)
				continue;

			std::string t = json_as_pstring(event_type);
			if (t == "visibility_update") {
				// change of chat status
				JSONNODE *visibility = json_get(event_data, "visibility");
				
				bool isVisible = (visibility != NULL) && json_as_bool(visibility);
				proto->debugLogA("      Requested chat switch to %s", isVisible ? "Online" : "Offline");
				proto->SetStatus(isVisible ? ID_STATUS_ONLINE : ID_STATUS_INVISIBLE);
			}
		} else if (t == "buddylist_overlay") {
			// we opened/closed chat window - pretty useless info for us
			continue;
		} else if (t == "ticker_update:home") {
			JSONNODE *actor_ = json_get(it, "actor");
			JSONNODE *time_ = json_get(it, "actor");
			JSONNODE *story_ = json_get(it, "story_xhp");

			std::string id = json_as_pstring(actor_);
			time_t time = json_as_float(time_);
			std::string text = json_as_pstring(story_);
			text = utils::text::slashu_to_utf8(text);

			text = utils::text::trim(
					utils::text::special_expressions_decode(
						utils::text::source_get_value(&text, 3, "<h5", ">", "</h5>")));
			
			// TODO: notify ticker updates
			/* if (!text.empty()) {
				proto->NotifyEvent()
			}*/
		} else if (t == "inbox") {
			// count of unread/unseen messages - pretty useless info for us
			/* JSONNODE *unread_ = json_get(it, "unread");
			JSONNODE *unseen_ = json_get(it, "unseen");
			JSONNODE *other_unread_ = json_get(it, "other_unread");
			JSONNODE *other_unseen_ = json_get(it, "other_unseen");
			JSONNODE *seen_timestamp_ = json_get(it, "seen_timestamp"); */
			continue;
		} else if (t == "mercury") {
			// rename multi user chat, ...

			JSONNODE *actions_ = json_get(it, "actions");
			if (actions_ == NULL)
				continue;

			for (unsigned int i = 0; i < json_size(actions_); i++) {
				JSONNODE *action_ = json_at(actions_, i);
		
				JSONNODE *thread_id_ = json_get(action_, "thread_id");
				JSONNODE *log_body_ = json_get(action_, "log_message_body");
				JSONNODE *log_data_ = json_get(action_, "log_message_data");
				if (!log_data_ || !thread_id_)
					continue;

				JSONNODE *name_ = json_get(log_data_, "name");
				std::string name = json_as_pstring(name_);
				std::string thread_id = json_as_pstring(thread_id_);
				std::string message = json_as_pstring(log_body_);

				// proto->RenameChat(thread_id.c_str(), name.c_str()); // this don't work, why?
				proto->setStringUtf(proto->ChatIDToHContact(thread_id), FACEBOOK_KEY_NICK, name.c_str());

				proto->UpdateChat(thread_id.c_str(), NULL, NULL, message.c_str());
			}
		} else if (t == "notifications_read") {
			// TODO: close popups with these IDs
			JSONNODE *alert_ids = json_get(it, "alert_ids");
			for (unsigned int n = 0; n < json_size(alert_ids); n++) {
				JSONNODE *idItr = json_at(alert_ids, n);
				std::string alert_id = json_as_pstring(idItr);

				// PUDeletePopup(hWndPopup);
			}
		} else
			continue;
	}

	// remove received messages from map		
	for (std::map<std::string, bool>::iterator it = proto->facy.messages_ignore.begin(); it != proto->facy.messages_ignore.end(); ) {
		if (it->second)
			it = proto->facy.messages_ignore.erase(it);
		else
			++it;
	}

	json_delete(root);
	return EXIT_SUCCESS;
}

int facebook_json_parser::parse_unread_threads(void* data, std::vector< std::string >* threads)
{
	std::string jsonData = static_cast< std::string* >(data)->substr(9);
	
	JSONNODE *root = json_parse(jsonData.c_str());
	if (root == NULL)
		return EXIT_FAILURE;

	JSONNODE *payload = json_get(root, "payload");
	if (payload == NULL) {
		json_delete(root);
		return EXIT_FAILURE;
	}

	JSONNODE *unread_threads = json_get(payload, "unread_thread_ids");
	if (unread_threads == NULL) {
		json_delete(root);
		return EXIT_FAILURE;
	}

	for (unsigned int i = 0; i < json_size(unread_threads); i++) {
		JSONNODE *it = json_at(unread_threads, i);

		JSONNODE *folder = json_get(it, "folder");
		JSONNODE *thread_ids = json_get(it, "thread_ids");

		for (unsigned int j = 0; j < json_size(thread_ids); j++) {
			JSONNODE *id = json_at(thread_ids, j);
			threads->push_back(json_as_pstring(id));
		}
	}

	json_delete(root);
	return EXIT_SUCCESS;
}

int facebook_json_parser::parse_thread_messages(void* data, std::vector< facebook_message* >* messages, std::map< std::string, facebook_chatroom* >* chatrooms, bool unreadOnly, int limit)
{
	std::string jsonData = static_cast< std::string* >(data)->substr(9);

	JSONNODE *root = json_parse(jsonData.c_str());
	if (root == NULL)
		return EXIT_FAILURE;

	JSONNODE *payload = json_get(root, "payload");
	if (payload == NULL) {
		json_delete(root);
		return EXIT_FAILURE;
	}

	JSONNODE *actions = json_get(payload, "actions");
	JSONNODE *threads = json_get(payload, "threads");
	if (actions == NULL || threads == NULL) {
		json_delete(root);
		return EXIT_FAILURE;
	}

	JSONNODE *roger = json_get(payload, "roger");
	if (roger != NULL) {
		for (unsigned int i = 0; i < json_size(roger); i++) {
			JSONNODE *it = json_at(roger, i);
			
			facebook_chatroom *room = new facebook_chatroom();
			room->thread_id = json_name(it);

			for (unsigned int j = 0; j < json_size(it); j++) {
				JSONNODE *jt = json_at(it, j);
				std::string user_id = json_name(jt);
				room->participants.insert(std::make_pair(user_id, user_id)); // TODO: get name somehow
			}

			chatrooms->insert(std::make_pair(room->thread_id, room));
		}
	}

	std::map<std::string, std::string> thread_ids;
	for (unsigned int i = 0; i < json_size(threads); i++) {
		JSONNODE *it = json_at(threads, i);
		JSONNODE *canonical = json_get(it, "canonical_fbid");
		JSONNODE *thread_id = json_get(it, "thread_id");
		JSONNODE *name = json_get(it, "name");
		JSONNODE *unread_count = json_get(it, "unread_count"); // TODO: use it to check against number of loaded messages... but how?
		
		std::map<std::string, facebook_chatroom*>::iterator iter = chatrooms->find(json_as_pstring(thread_id));
		if (iter != chatrooms->end()) {
			iter->second->chat_name = json_as_pstring(name); // TODO: create name from users if there is no name...
		}

		if (canonical == NULL || thread_id == NULL)
			continue;

		std::string id = json_as_pstring(canonical);
		if (id == "null")
			continue;
		
		std::string tid = json_as_pstring(thread_id);
		thread_ids.insert(std::make_pair(tid, id));
	}

	for (unsigned int i = 0; i < json_size(actions); i++) {
		JSONNODE *it = json_at(actions, i);

		JSONNODE *author = json_get(it, "author");
		JSONNODE *author_email = json_get(it, "author_email");
		JSONNODE *body = json_get(it, "body");
		JSONNODE *tid = json_get(it, "thread_id");
		JSONNODE *mid = json_get(it, "message_id");
		JSONNODE *timestamp = json_get(it, "timestamp");

		if (author == NULL || body == NULL || mid == NULL || tid == NULL || timestamp == NULL)
			continue;		

		std::string thread_id = json_as_pstring(tid);
		std::string message_id = json_as_pstring(mid);		
		std::string message_text = json_as_pstring(body);
		std::string author_id = json_as_pstring(author);
		std::string::size_type pos = author_id.find(":");		
		if (pos != std::string::npos)
			author_id = author_id.substr(pos+1);

		// Process attachements and stickers
		parseAttachments(proto, &message_text, it);

		message_text = utils::text::trim(utils::text::special_expressions_decode(utils::text::slashu_to_utf8(message_text)), true);
		if (message_text.empty())
			continue;

		facebook_message* message = new facebook_message();	
		message->message_text = message_text;
		if (author_email != NULL)
			message->sender_name = json_as_pstring(author_email);
		message->time = utils::time::fix_timestamp(json_as_float(timestamp));
		message->message_id = message_id;
		message->isIncoming = (author_id != proto->facy.self_.user_id);

		std::map<std::string, facebook_chatroom*>::iterator iter = chatrooms->find(thread_id);
		if (iter != chatrooms->end()) {
			// this is chatroom message
			message->isChat = true;
			message->user_id = author_id;
			message->thread_id = thread_id;
		} else {
			// this is standard message

			// Ignore read messages if we want only unread messages
			JSONNODE *is_unread = json_get(it, "is_unread");
			if (unreadOnly && (is_unread == NULL || !json_as_bool(is_unread)))
				continue;

			message->isChat = false;
			std::map<std::string, std::string>::iterator iter = thread_ids.find(thread_id);
			if (iter != thread_ids.end()) {
				message->user_id = iter->second; // TODO: Check if we have contact with this ID in friendlist and otherwise do something different?
			} else {
				continue;
			}
		}		

		messages->push_back(message);
	}

	json_delete(root);
	return EXIT_SUCCESS;
}
