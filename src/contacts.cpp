#include "common.h"

bool WhatsAppProto::IsMyContact(HANDLE hContact, bool include_chat)
{
	const char *proto = GetContactProto(hContact);
	if( proto && strcmp(m_szModuleName,proto) == 0 )
	{
		if( include_chat )
			return true;
		else
			return db_get_b(hContact,m_szModuleName,"ChatRoom",0) == 0;
	} else {
		return false;
	}
}

HANDLE WhatsAppProto::AddToContactList(const std::string& jid, BYTE type, bool dont_check, const char *new_name,
   bool isChatRoom, bool isHidden)
{
	HANDLE hContact;

	if (!dont_check) {
		// First, check if this contact exists
		hContact = ContactIDToHContact(jid);

		if( hContact )
      {
         if (new_name != NULL)
         {
            DBVARIANT dbv;
            string oldName;
            if (db_get_s(hContact, m_szModuleName, WHATSAPP_KEY_PUSH_NAME, &dbv, DBVT_UTF8))
            {
               oldName = jid.c_str();
            }
            else
            {
               oldName = dbv.pszVal;
               db_free(&dbv);
            }
            db_set_utf(hContact, m_szModuleName, WHATSAPP_KEY_PUSH_NAME, new_name);

            if (oldName.compare(string(new_name)) != 0)
            {
               this->NotifyEvent(oldName.c_str(), this->TranslateStr("is now known as '%s'", new_name),
                  hContact, WHATSAPP_EVENT_OTHER);
            }
         }
         if (db_get_b(hContact, "CList", "Hidden", 0) > 0)
         {
            db_unset(hContact, "CList", "Hidden");
         }
			return hContact;
      }
	}

	// If not, make a new contact!
	hContact = (HANDLE)CallService(MS_DB_CONTACT_ADD, 0, 0);
	if (hContact)
	{
		if (CallService(MS_PROTO_ADDTOCONTACT, (WPARAM)hContact, (LPARAM)m_szModuleName) == 0)
		{
			db_set_s(hContact ,m_szModuleName, "ID", jid.c_str());
         LOG("Added contact %s", jid.c_str());
			db_set_s(hContact, m_szModuleName, "MirVer", "WhatsApp");
			db_unset(hContact, "CList", "MyHandle");
         db_set_b(hContact, "CList", "NotOnList", 1);

         /*
         std::string newNameStr;
         if (hasNickName)
         {
            newNameStr = new_name;
         }

         DBEVENTINFO dbei = {0};
         dbei.cbSize = sizeof(dbei);
         dbei.szModule = m_szModuleName;
         dbei.timestamp = time(NULL);
         dbei.flags = DBEF_UTF;
         dbei.eventType = EVENTTYPE_ADDED;
         dbei.cbBlob = sizeof(DWORD) * 2 + newNameStr.length() + 5;

			PBYTE pCurBlob = dbei.pBlob = ( PBYTE ) mir_alloc( dbei.cbBlob );					
			*(PDWORD)pCurBlob = 0; pCurBlob += sizeof(DWORD);                    // UID
			*(PDWORD)pCurBlob = (DWORD)hContact; pCurBlob += sizeof(DWORD);      // Contact Handle
			strcpy((char*)pCurBlob, newNameStr.data()); pCurBlob += newNameStr.length()+1;	// Nickname
			*pCurBlob = '\0'; pCurBlob++;                                        // First Name
			*pCurBlob = '\0'; pCurBlob++;                                        // Last Name
			*pCurBlob = '\0'; pCurBlob++;                                        // E-mail
			*pCurBlob = '\0';                                                    // Reason

         CallService(MS_DB_EVENT_ADD, 0, (LPARAM) &dbei);
         */

			DBVARIANT dbv;
			if( !db_get_s(NULL, m_szModuleName, WHATSAPP_KEY_DEF_GROUP, &dbv, DBVT_WCHAR))
			{
				db_set_ws(hContact, "CList", "Group", dbv.ptszVal);
				db_free(&dbv);
			}

			if (new_name != NULL)
			{
				db_set_utf(hContact, m_szModuleName, WHATSAPP_KEY_PUSH_NAME, new_name);
			}

         if (isChatRoom)
         {
            db_set_b(hContact, m_szModuleName, "SimpleChatRoom", 1);
            //ForkThread(&WhatsAppProto::SendGetGroupInfoWorker, this, (void*) &jid);
         }

			return hContact;
		} else {
			CallService(MS_DB_CONTACT_DELETE, (WPARAM)hContact, 0);
		}
	}

   return INVALID_HANDLE_VALUE;
}

HANDLE WhatsAppProto::ContactIDToHContact(const std::string& phoneNumber)
{
   // Cache
   std::map<string, HANDLE>::iterator it = this->hContactByJid.find(phoneNumber);
   if (it != this->hContactByJid.end())
   {
      return it->second;
   }

   const char* id;
   const char* idForContact = "ID";
   const char* idForChat = "ChatRoomID";

	for(HANDLE hContact = db_find_first();
	    hContact;
	    hContact = db_find_next(hContact))
	{
		if(!IsMyContact(hContact, true))
			continue;

      id = db_get_b(hContact, m_szModuleName, "ChatRoom", 0) > 0 ? idForChat : idForContact;

		DBVARIANT dbv;
		if( !db_get_s(hContact,m_szModuleName, id,&dbv, DBVT_ASCIIZ))
		{
			if( strcmp(phoneNumber.c_str(),dbv.pszVal) == 0 )
			{
				db_free(&dbv);
            this->hContactByJid[phoneNumber] = hContact;
				return hContact;
			}
         else
         {
				db_free(&dbv);
			}
		}
	}

	return 0;
}

void WhatsAppProto::SetAllContactStatuses(int status, bool reset_client)
{
	for (HANDLE hContact = db_find_first();
	    hContact;
	    hContact = db_find_next(hContact))
	{
		if (!IsMyContact(hContact))
			continue;
		
		if (reset_client) {
			DBVARIANT dbv;
			if (!db_get_s(hContact,m_szModuleName,"MirVer",&dbv,DBVT_WCHAR)) {
				if (_tcscmp(dbv.ptszVal, _T("WhatsApp")))
					db_set_ws(hContact,m_szModuleName,"MirVer", _T("WhatsApp"));
				db_free(&dbv);
			}

         db_set_ws(hContact, "CList", "StatusMsg", _T(""));
		}

		if (db_get_w(hContact,m_szModuleName,"Status",ID_STATUS_OFFLINE) != status)
			db_set_w(hContact,m_szModuleName,"Status",status);
	}
}

void WhatsAppProto::ProcessBuddyList(void*)
{
   std::vector<std::string> jids;
   DBVARIANT dbv;
   for (HANDLE hContact = db_find_first(); hContact; hContact = db_find_next(hContact)) {
      if (!IsMyContact(hContact))
         continue;

      if (!db_get_s(hContact, m_szModuleName, WHATSAPP_KEY_ID, &dbv, DBVT_ASCIIZ)) {
         std::string id(dbv.pszVal);
         db_free(&dbv);

         CODE_BLOCK_TRY
            if (!db_get_b(hContact, "CList", "Hidden", 0))
            {
               // Do not request picture for inactive groups - this would make the group visible again
               jids.push_back(id);
            }
            if (db_get_b(hContact, m_szModuleName, "SimpleChatRoom", 0) == 0)
            {
               this->connection->sendQueryLastOnline(id);
               this->connection->sendPresenceSubscriptionRequest(id);
            }
         CODE_BLOCK_CATCH_ALL
      }
   }
   if (jids.size() > 0) {
      CODE_BLOCK_TRY
         this->connection->sendGetPictureIds(jids);
      CODE_BLOCK_CATCH_ALL
   }
   CODE_BLOCK_TRY
      this->connection->sendGetGroups();
      this->connection->sendGetOwningGroups();
   CODE_BLOCK_CATCH_ALL
}

void WhatsAppProto::SearchAckThread(void *targ)
{
	char *id = mir_utf8encodeT((TCHAR*)targ);
   std::string jid(id);
   jid.append("@s.whatsapp.net");

   this->connection->sendQueryLastOnline(jid);
   this->connection->sendPresenceSubscriptionRequest(jid);

   mir_free(targ);
   mir_free(id);
}

void WhatsAppProto::onAvailable(const std::string& paramString, bool paramBoolean)
{
   HANDLE hContact = this->AddToContactList(paramString, 0, false);
   if (hContact != NULL)
   {
      if (paramBoolean)
      {
         /*
         this->connection->sendGetPicture(paramString, "image", "old", "new");
         std::vector<std::string> ids;
         ids.push_back(paramString);
         this->connection->sendGetPictureIds(ids);
         */
         db_set_w(hContact, m_szModuleName, "Status", ID_STATUS_ONLINE);
      }
      else
      {
         db_set_w(hContact, m_szModuleName, "Status", ID_STATUS_OFFLINE);
         this->UpdateStatusMsg(hContact);
      }
   }
   
   db_set_dw(hContact, this->m_szModuleName, WHATSAPP_KEY_LAST_SEEN, 0);
   this->UpdateStatusMsg(hContact);
}

void WhatsAppProto::onLastSeen(const std::string& paramString1, int paramInt, std::string* paramString2)
{
   /*
   HANDLE hContact = this->ContactIDToHContact(paramString1);
   if (hContact == NULL)
   {
      // This contact was searched
		PROTOSEARCHRESULT isr = {0};
		isr.cbSize = sizeof(isr);
		isr.flags = PSR_TCHAR;
		isr.id  = mir_a2t_cp(id.c_str(), CP_UTF8);
		isr.nick  = "";
		isr.firstName = "";
		isr.lastName = "";
		isr.email = "";
      ProtoBroadcastAck(m_szModuleName, NULL, ACKTYPE_SEARCH, ACKRESULT_DATA, targ, (LPARAM)&isr);
      // #TODO
   }
   */
   HANDLE hContact = this->AddToContactList(paramString1, 0, false);
   db_set_dw(hContact, this->m_szModuleName, WHATSAPP_KEY_LAST_SEEN, paramInt);

   this->UpdateStatusMsg(hContact);
}

void WhatsAppProto::UpdateStatusMsg(HANDLE hContact)
{
   std::wstringstream ss;

   int lastSeen = db_get_dw(hContact, m_szModuleName, WHATSAPP_KEY_LAST_SEEN, -1);
   if (lastSeen != -1)
   {
      time_t timestamp = time(NULL) - lastSeen;
   
      tm* t = localtime(&timestamp);
      ss << _T("Last seen on ") << std::setfill(_T('0')) << std::setw(2) << (t->tm_mon + 1) << 
         _T("/") << std::setw(2) << t->tm_mday << _T("/") << (t->tm_year + 1900) << _T(" ")
         << std::setw(2) << t->tm_hour << _T(":") << std::setw(2) << t->tm_min;
   }

   int state = db_get_dw(hContact, m_szModuleName, WHATSAPP_KEY_LAST_MSG_STATE, 2);
   if (state < 2 && lastSeen != -1)
      ss << _T(" - ");
   for (; state < 2; ++state)
      ss << _T("\u2713");

   db_set_ws(hContact, "CList", "StatusMsg", ss.str().c_str());
}

void WhatsAppProto::onPictureChanged(const std::string& from, const std::string& author, bool set)
{
   if (this->isOnline())
   {
      vector<string> ids;
      ids.push_back(from);
      this->connection->sendGetPictureIds(ids);
   }
}

void WhatsAppProto::onSendGetPicture(const std::string& jid, const std::vector<unsigned char>& data, const std::string& oldId, const std::string& newId)
{
   HANDLE hContact = this->ContactIDToHContact(jid);
   if (hContact)
   {
      LOG("Updating avatar for jid %s", jid.c_str());

      // Save avatar
      std::tstring filename = this->GetAvatarFolder() ;
      if (_taccess(filename.c_str(), 0))
         CallService(MS_UTILS_CREATEDIRTREET, 0, (LPARAM)filename.c_str());
      filename = filename + _T("\\") + (TCHAR*) _A2T(jid.c_str()) + _T("-") + (TCHAR*) _A2T(newId.c_str()) +_T(".jpg");
      FILE *f = _tfopen(filename.c_str(), _T("wb"));
      int r = fwrite(std::string(data.begin(), data.end()).c_str(), 1, data.size(), f);
      fclose(f);

      PROTO_AVATAR_INFORMATIONT ai = {sizeof(ai)};
      ai.hContact = hContact;
      ai.format = PA_FORMAT_JPEG;
      _tcsncpy(ai.filename, filename.c_str(), SIZEOF(ai.filename));
      ai.filename[SIZEOF(ai.filename)-1] = 0;

      int ackResult;
      if (r > 0)
      {
         db_set_s(hContact, m_szModuleName, WHATSAPP_KEY_AVATAR_ID, newId.c_str());
         ackResult = ACKRESULT_SUCCESS;
      }
      else
      {
         ackResult = ACKRESULT_FAILED;
      }
      ProtoBroadcastAck(m_szModuleName, ai.hContact, ACKTYPE_AVATAR, ackResult, (HANDLE)&ai, 0);
   }
}

void WhatsAppProto::onSendGetPictureIds(std::map<string,string>* ids)
{
   for (std::map<string,string>::iterator it = ids->begin(); it != ids->end(); ++it)
   {
      HANDLE hContact = this->AddToContactList(it->first);
      if (hContact != NULL)
      {
         DBVARIANT dbv;
         std::string oldId;
         if (db_get_s(hContact, m_szModuleName, WHATSAPP_KEY_AVATAR_ID, &dbv, DBVT_ASCIIZ))
         {
            oldId = "";
         }
         else
         {
            oldId = dbv.pszVal;
            db_free(&dbv);
         }

         if (it->second.size() > 0 && it->second.compare(oldId) != 0)
         {
            CODE_BLOCK_TRY
               this->connection->sendGetPicture(it->first, "image", oldId, it->second);
            CODE_BLOCK_CATCH_ALL
         }
      }
   }
}

string WhatsAppProto::GetContactDisplayName(HANDLE hContact)
{
   return string((CHAR*) CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM) hContact, 0));
}

string WhatsAppProto::GetContactDisplayName(const string& jid)
{
   HANDLE hContact = this->ContactIDToHContact(jid);
   return hContact ? this->GetContactDisplayName(hContact) : (string("+")+ Utilities::removeWaDomainFromJid(jid));
}

// Group contacts --------------------------

void WhatsAppProto::SendGetGroupInfoWorker(void* data)
{
   if (this->isOnline())
   {
      this->connection->sendGetGroupInfo(*((std::string*) data));
   }
}

void WhatsAppProto::onGroupInfo(const std::string& gjid, const std::string& ownerJid, const std::string& subject, const std::string& createrJid, int paramInt1, int paramInt2)
{
   LOG("'%s', '%s', '%s', '%s'", gjid.c_str(), ownerJid.c_str(), subject.c_str(), createrJid.c_str());
   HANDLE hContact = ContactIDToHContact(gjid);
   if (!hContact)
   {
      LOG("Group info requested for non existing contact '%s'", gjid.c_str());
      return;
   }
   db_set_b(hContact, m_szModuleName, "SimpleChatRoom", ownerJid.compare(this->jid) == 0 ? 2 : 1);
   if (this->isOnline())
   {
      this->connection->sendGetParticipants(gjid);
   }
}

void WhatsAppProto::onGroupInfoFromList(const std::string& paramString1, const std::string& paramString2, const std::string& paramString3, const std::string& paramString4, int paramInt1, int paramInt2)
{
   // Called before onOwningGroups() or onParticipatingGroups() is called!
   LOG("");
}

void WhatsAppProto::onGroupNewSubject(const std::string& from, const std::string& author, const std::string& newSubject, int paramInt)
{
   LOG("'%s', '%s', '%s'", from.c_str(), author.c_str(), newSubject.c_str());
   HANDLE hContact = this->AddToContactList(from, 0, false, newSubject.c_str(), true);
}

void WhatsAppProto::onGroupAddUser(const std::string& paramString1, const std::string& paramString2)
{
   LOG("%s - user: %s", paramString1.c_str(), paramString2.c_str());
   HANDLE hContact = this->AddToContactList(paramString1);
   std::string groupName(this->GetContactDisplayName(hContact));

   if (paramString2.compare(this->jid) == 0)
   {
      this->NotifyEvent(groupName, this->TranslateStr("You have been added to the group"), hContact, WHATSAPP_EVENT_OTHER);
      db_set_b(hContact, m_szModuleName, "IsGroupMember", 1);
   }
   else
   {
      this->NotifyEvent(groupName, this->TranslateStr("User '%s' has been added to the group",
         this->GetContactDisplayName(paramString2).c_str()), hContact, WHATSAPP_EVENT_OTHER);
   }

   if(this->isOnline())
   {
      this->connection->sendGetGroupInfo(paramString1);
   }
}

void WhatsAppProto::onGroupRemoveUser(const std::string& paramString1, const std::string& paramString2)
{
   LOG("%s - user: %s", paramString1.c_str(), paramString2.c_str());
   HANDLE hContact = this->ContactIDToHContact(paramString1);
   if (!hContact)
      return;

   string groupName(this->GetContactDisplayName(hContact));

   if (paramString2.compare(this->jid) == 0)
   {
      //db_set_b(hContact, "CList", "Hidden", 1);
      db_set_b(hContact, m_szModuleName, "IsGroupMember", 0);
         
      this->NotifyEvent(groupName, this->TranslateStr("You have been removed from the group"),
         hContact, WHATSAPP_EVENT_OTHER);
   }
   else if(this->isOnline())
   {
      this->NotifyEvent(groupName, this->TranslateStr("User '%s' has been removed from the group", 
         this->GetContactDisplayName(paramString2).c_str()), hContact, WHATSAPP_EVENT_OTHER);
      this->connection->sendGetGroupInfo(paramString1);
      //this->connection->sendGetParticipants(paramString1);
   }
}

void WhatsAppProto::onLeaveGroup(const std::string& paramString)
{
   // Won't be called for unknown reasons!
   LOG("%s", this->GetContactDisplayName(paramString).c_str());
   HANDLE hContact = this->ContactIDToHContact(paramString);
   if (hContact)
   {
      //db_set_b(hContact, "CList", "Hidden", 1);
      db_set_b(hContact, m_szModuleName, "IsGroupMember", 0);
   }
}

void WhatsAppProto::onGetParticipants(const std::string& gjid, const std::vector<string>& participants)
{
   LOG("%s", this->GetContactDisplayName(gjid).c_str());

   HANDLE hUserContact;
   HANDLE hContact = this->ContactIDToHContact(gjid);
   if (!hContact)
      return;

   if (db_get_b(hContact, "CList", "Hidden", 0) == 1)
      return;

   bool isHidden = true;
   bool isOwningGroup = db_get_b(hContact, m_szModuleName, "SimpleChatRoom", 0) == 2;

   if (isOwningGroup)
   {
      this->isMemberByGroupContact[hContact].clear();
   }

   for (std::vector<string>::const_iterator it = participants.begin(); it != participants.end(); ++it)
   {
      // Hide, if we are not member of the group
      // Sometimes the group is shown shortly after hiding it again, due to other threads which stored the contact
      //    in a cache before it has been removed (E.g. picture-id list in processBuddyList)
      if (isHidden && this->jid.compare(*it) == 0)
      {
         isHidden = false;
         if (!isOwningGroup)
         {
            // Break, as we don't need to collect group-members
            break;
         }
      }

      // #TODO Slow for big count of participants
      // #TODO If a group is hidden it has been deleted from the local contact list
      //          => don't allow to add users anymore
      if (isOwningGroup)
      {
         hUserContact = this->ContactIDToHContact(*it);
         if (hUserContact)
         {
            this->isMemberByGroupContact[hContact][hUserContact] = true;
         }
      }
   }
   if (isHidden)
   {
      //db_set_b(hContact, "CList", "Hidden", 1);
      // #TODO Check if it's possible to reach this point at all
      db_set_b(hContact, m_szModuleName, "IsGroupMember", 0);
   }
}

// Menu handler
INT_PTR __cdecl WhatsAppProto::OnAddContactToGroup(WPARAM wParam, LPARAM, LPARAM lParam)
{
   string a = GetContactDisplayName((HANDLE) wParam);
   string b = GetContactDisplayName((HANDLE) lParam);
   LOG("Request add user %s to group %s", a.c_str(), b.c_str());

   if (!this->isOnline())
      return NULL;

   DBVARIANT dbv;

   if (db_get_s((HANDLE) wParam, m_szModuleName, "ID", &dbv, DBVT_ASCIIZ))
      return NULL; 

   std::vector<string> participants;
   participants.push_back(string(dbv.pszVal));
   db_free(&dbv);

   if (db_get_s((HANDLE) lParam, m_szModuleName, "ID", &dbv, DBVT_ASCIIZ))
      return NULL;

   this->connection->sendAddParticipants(string(dbv.pszVal), participants);

   db_free(&dbv);
   return NULL;
}

// Menu handler
INT_PTR __cdecl WhatsAppProto::OnRemoveContactFromGroup(WPARAM wParam, LPARAM, LPARAM lParam)
{
   string a = GetContactDisplayName((HANDLE) wParam);
   string b = GetContactDisplayName((HANDLE) lParam);
   LOG("Request remove user %s from group %s", a.c_str(), b.c_str());

   if (!this->isOnline())
      return NULL;

   DBVARIANT dbv;

   if (db_get_s((HANDLE) lParam, m_szModuleName, "ID", &dbv, DBVT_ASCIIZ))
      return NULL;

   std::vector<string> participants;
   participants.push_back(string(dbv.pszVal));
   db_free(&dbv);

   if (db_get_s((HANDLE) wParam, m_szModuleName, "ID", &dbv, DBVT_ASCIIZ))
      return NULL;

   this->connection->sendRemoveParticipants(string(dbv.pszVal), participants);

   db_free(&dbv);
   return NULL;
}

void WhatsAppProto::onOwningGroups(const std::vector<string>& paramVector)
{
   LOG("");
   this->HandleReceiveGroups(paramVector, true);
}

void WhatsAppProto::onParticipatingGroups(const std::vector<string>& paramVector)
{
   LOG("");
   this->HandleReceiveGroups(paramVector, false);
}

void WhatsAppProto::HandleReceiveGroups(const std::vector<string>& groups, bool isOwned)
{
   HANDLE hContact;
   map<HANDLE, bool> isMember; // at the moment, only members of owning groups are stored

   // This could take long time if there are many new groups which aren't
   //    yet stored to the database. But that should be a rare case
   for (std::vector<string>::const_iterator it = groups.begin(); it != groups.end(); ++it)
   {
      hContact = this->AddToContactList(*it, 0, false, NULL, true);
      db_set_b(hContact, m_szModuleName, "IsGroupMember", 1);
      if (isOwned)
      {
         this->isMemberByGroupContact[hContact]; // []-operator creates entry, if it doesn't exist
         db_set_b(hContact, m_szModuleName, "SimpleChatRoom", 2);
         this->connection->sendGetParticipants(*it);
      }
      else
      {
         isMember[hContact] = true;
      }
   }

   // Mark as non-meber if group only exists locally
   if (!isOwned)
   {
      for (hContact = db_find_first(); hContact; hContact = db_find_next(hContact))
      {
         if (IsMyContact(hContact) && db_get_b(hContact, m_szModuleName, "SimpleChatRoom", 0) > 0)
         {
            //LOG("Set IsGroupMember to 0 for '%s'", this->GetContactDisplayName(hContact).c_str());
            db_set_b(hContact, m_szModuleName, "IsGroupMember", 
               isMember.find(hContact) == isMember.end() ? 0 : 1);
         }
      }
   }
}

void WhatsAppProto::onGroupCreated(const std::string& paramString1, const std::string& paramString2)
{
   // Must be received after onOwningGroups() :/
   LOG("%s / %s", paramString1.c_str(), paramString2.c_str());
   string jid = paramString2 +string("@")+ paramString1;
   HANDLE hContact = this->AddToContactList(jid, 0, false, NULL, true);
   db_set_b(hContact, m_szModuleName, "SimpleChatRoom", 2);
}

// Menu-handler
int __cdecl WhatsAppProto::OnCreateGroup(WPARAM wParam, LPARAM lParam)
{
   LOG("");
   input_box* ib = new input_box;
   ib->defaultValue = _T("");
   ib->limit = WHATSAPP_GROUP_NAME_LIMIT;
   ib->proto = this;
   ib->text = _T("Enter group subject");
   ib->title = _T("WhatsApp - Create Group");
   ib->thread = &WhatsAppProto::SendCreateGroupWorker;
   HWND hDlg = CreateDialogParam(g_hInstance, MAKEINTRESOURCE(IDD_INPUTBOX), 0, WhatsAppInputBoxProc,
      reinterpret_cast<LPARAM>(ib));
   ShowWindow(hDlg, SW_SHOW);
   return FALSE;
}

void __cdecl WhatsAppProto::SendSetGroupNameWorker(void* data)
{
   input_box_ret* ibr(static_cast<input_box_ret*>(data));
   string groupName(ibr->value);
   mir_free(ibr->value);
   DBVARIANT dbv;
   if (!db_get_s(*((HANDLE*) ibr->userData), m_szModuleName, WHATSAPP_KEY_ID, &dbv, DBVT_ASCIIZ)
      && this->isOnline())
   {
      this->connection->sendSetNewSubject(dbv.pszVal, groupName);
      db_free(&dbv);
   }
   delete ibr->userData;
   delete ibr;
}

void __cdecl WhatsAppProto::SendCreateGroupWorker(void* data)
{
   input_box_ret* ibr(static_cast<input_box_ret*>(data));
   string groupName(ibr->value);
   mir_free(ibr->value);
   if (this->isOnline())
   {
      this->connection->sendCreateGroupChat(groupName);
   }
}

int __cdecl WhatsAppProto::OnChangeGroupSubject(WPARAM wParam, LPARAM lParam)
{
   DBVARIANT dbv;
   HANDLE hContact = reinterpret_cast<HANDLE>(wParam);
   input_box* ib = new input_box;

   if (db_get_s(hContact, m_szModuleName, WHATSAPP_KEY_PUSH_NAME, &dbv, DBVT_WCHAR))
   {
      ib->defaultValue = _T("");
   }
   else
   {
      ib->defaultValue = dbv.ptszVal;
      db_free(&dbv);
   }
   ib->limit = WHATSAPP_GROUP_NAME_LIMIT;
   ib->text = _T("Enter new group subject");
   ib->title = _T("WhatsApp - Change Group Subject");
   
   ib->thread = &WhatsAppProto::SendSetGroupNameWorker;
   ib->proto = this;
   HANDLE* hContactPtr = new HANDLE(hContact);
   ib->userData = (void*) hContactPtr;
   
   HWND hDlg = CreateDialogParam(g_hInstance, MAKEINTRESOURCE(IDD_INPUTBOX), 0, WhatsAppInputBoxProc,
      reinterpret_cast<LPARAM>(ib));
   ShowWindow(hDlg, SW_SHOW);
   return 0;
}

int __cdecl WhatsAppProto::OnLeaveGroup(WPARAM wParam, LPARAM)
{
   DBVARIANT dbv;
   HANDLE hContact = reinterpret_cast<HANDLE>(wParam);
   if (this->isOnline() && !db_get_s(hContact, m_szModuleName, WHATSAPP_KEY_ID, &dbv, DBVT_ASCIIZ))
   {
      db_set_b(hContact, m_szModuleName, "IsGroupMember", 0);
      this->connection->sendLeaveGroup(dbv.pszVal);
      db_free(&dbv);
   }
   return 0;
}