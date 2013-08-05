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

void FacebookProto::ChangeStatus(void*)
{
	ScopedLock s(signon_lock_);
	ScopedLock b(facy.buddies_lock_);
	
	int new_status = m_iDesiredStatus;
	int old_status = m_iStatus;

	if (new_status == ID_STATUS_OFFLINE)
	{ // Logout	
		LOG("##### Beginning SignOff process");

		m_iStatus = facy.self_.status_id = ID_STATUS_OFFLINE;
		SetEvent(update_loop_lock_);
		Netlib_Shutdown(facy.hMsgCon);

		OnLeaveChat(NULL, NULL);
		SetAllContactStatuses(ID_STATUS_OFFLINE, true);
		ToggleStatusMenuItems(false);
		delSetting("LogonTS");

		ProtoBroadcastAck(0, ACKTYPE_STATUS, ACKRESULT_SUCCESS, (HANDLE)old_status, m_iStatus);

		if (getByte(FACEBOOK_KEY_DISCONNECT_CHAT, DEFAULT_DISCONNECT_CHAT))
			facy.chat_state(false);

		facy.logout();

		facy.clear_cookies();
		facy.buddies.clear();
		facy.messages_ignore.clear();
		facy.pages.clear();

		if (facy.hMsgCon)
			Netlib_CloseHandle(facy.hMsgCon);
		facy.hMsgCon = NULL;

		LOG("##### SignOff complete");

		return;
	}
	else if (old_status == ID_STATUS_OFFLINE)
	{ // Login
		SYSTEMTIME t;
		GetLocalTime(&t);
		Log("[%d.%d.%d] Using Facebook Protocol RM %s", t.wDay, t.wMonth, t.wYear, __VERSION_STRING);
		
		LOG("***** Beginning SignOn process");

		m_iStatus = facy.self_.status_id = ID_STATUS_CONNECTING;
		ProtoBroadcastAck(0, ACKTYPE_STATUS, ACKRESULT_SUCCESS, (HANDLE)old_status, m_iStatus);

		ResetEvent(update_loop_lock_);

		if (NegotiateConnection() && facy.home())
		{		
			facy.reconnect();
			facy.load_friends();
			facy.load_pages();

			// Process Friends requests
			ForkThread(&FacebookProto::ProcessFriendRequests, NULL);

			// Get unread messages
			ForkThread(&FacebookProto::ProcessUnreadMessages, NULL);

			// Get notifications
			ForkThread(&FacebookProto::ProcessNotifications, NULL);

			setDword("LogonTS", (DWORD)time(NULL));
			ForkThread(&FacebookProto::UpdateLoop,  NULL);
			ForkThread(&FacebookProto::MessageLoop, NULL);

			if (getByte(FACEBOOK_KEY_SET_MIRANDA_STATUS, DEFAULT_SET_MIRANDA_STATUS))
				ForkThread(&FacebookProto::SetAwayMsgWorker, NULL);
		}
		else {
			ProtoBroadcastAck(0, ACKTYPE_STATUS, ACKRESULT_FAILED, (HANDLE)old_status, m_iStatus);

			if (facy.hFcbCon)
				Netlib_CloseHandle(facy.hFcbCon);
			facy.hFcbCon = NULL;

			facy.clear_cookies();

			// Set to offline
			m_iStatus = m_iDesiredStatus = facy.self_.status_id = ID_STATUS_OFFLINE;
			ProtoBroadcastAck(0, ACKTYPE_STATUS, ACKRESULT_SUCCESS, (HANDLE)old_status, m_iStatus);

			LOG("***** SignOn failed");

			return;
		}

		ToggleStatusMenuItems(true);
		LOG("***** SignOn complete");
	}
	else if (new_status == ID_STATUS_INVISIBLE)
	{
		facy.buddies.clear();
		this->SetAllContactStatuses(ID_STATUS_OFFLINE, true);
	}

	facy.chat_state(m_iDesiredStatus != ID_STATUS_INVISIBLE);	
	facy.buddy_list();

	m_iStatus = facy.self_.status_id = m_iDesiredStatus;
	ProtoBroadcastAck(0, ACKTYPE_STATUS, ACKRESULT_SUCCESS, (HANDLE)old_status, m_iStatus);

	LOG("***** ChangeStatus complete");
}

/** Return true on success, false on error. */
bool FacebookProto::NegotiateConnection()
{
	LOG("***** Negotiating connection with Facebook");

	bool error;
	std::string user, pass;
	DBVARIANT dbv = {0};

	error = true;
	if (!getString(FACEBOOK_KEY_LOGIN, &dbv))
	{
		user = dbv.pszVal;
		db_free(&dbv);
		error = user.empty();
	}
	if (error)
	{
		NotifyEvent(m_tszUserName,TranslateT("Please enter a username."),NULL,FACEBOOK_EVENT_CLIENT);
		return false;
	}

	error = true;
	if (!getString(FACEBOOK_KEY_PASS, &dbv))
	{
		CallService(MS_DB_CRYPT_DECODESTRING,strlen(dbv.pszVal)+1,
			reinterpret_cast<LPARAM>(dbv.pszVal));
		pass = dbv.pszVal;
		db_free(&dbv);
		error = pass.empty();
	}
	if (error)
	{
		NotifyEvent(m_tszUserName,TranslateT("Please enter a password."),NULL,FACEBOOK_EVENT_CLIENT);
		return false;
	}

	// Load machine name
	if (!getString(FACEBOOK_KEY_DEVICE_ID, &dbv))
	{
		facy.cookies["datr"] = dbv.pszVal;
		db_free(&dbv);
	}

	// Refresh last time of feeds update
	facy.last_feeds_update_ = ::time(NULL);

	// Get info about secured connection
	facy.https_ = getByte(FACEBOOK_KEY_FORCE_HTTPS, DEFAULT_FORCE_HTTPS) != 0;

	// Create default group for new contacts
	ptrT groupName( getTStringA(FACEBOOK_KEY_DEF_GROUP));
	if (groupName != NULL)
		CallService(MS_CLIST_GROUPCREATE, 0, (LPARAM)groupName);
	
	return facy.login(user, pass);
}

void FacebookProto::UpdateLoop(void *)
{
	time_t tim = ::time(NULL);
	LOG(">>>>> Entering Facebook::UpdateLoop[%d]", tim);

	for (int i = -1; !isOffline(); i = ++i % 50)
	{
		if (i != -1) {
			if (!facy.invisible_)
				if (!facy.buddy_list())
    				break;
		}
		if (i == 2 && getByte(FACEBOOK_KEY_EVENT_FEEDS_ENABLE, DEFAULT_EVENT_FEEDS_ENABLE))
			if (!facy.feeds())
				break;

		if (i == 49)
			ForkThread(&FacebookProto::ProcessFriendRequests, NULL);

		LOG("***** FacebookProto::UpdateLoop[%d] going to sleep...", tim);
		if (WaitForSingleObjectEx(update_loop_lock_, GetPollRate() * 1000, true) != WAIT_TIMEOUT)
			break;
		LOG("***** FacebookProto::UpdateLoop[%d] waking up...", tim);
	}

	ResetEvent(update_loop_lock_);
	LOG("<<<<< Exiting FacebookProto::UpdateLoop[%d]", tim);
}

void FacebookProto::MessageLoop(void *)
{
	time_t tim = ::time(NULL);
	LOG(">>>>> Entering Facebook::MessageLoop[%d]", tim);

	while (facy.channel())
	{
		if (isOffline())
			break;
		LOG("***** FacebookProto::MessageLoop[%d] refreshing...", tim);
	}

	LOG("<<<<< Exiting FacebookProto::MessageLoop[%d]", tim);
}

BYTE FacebookProto::GetPollRate()
{
	BYTE poll_rate = getByte(FACEBOOK_KEY_POLL_RATE, FACEBOOK_DEFAULT_POLL_RATE);

	return (
	    (poll_rate >= FACEBOOK_MINIMAL_POLL_RATE &&
	      poll_rate <= FACEBOOK_MAXIMAL_POLL_RATE)
	    ? poll_rate : FACEBOOK_DEFAULT_POLL_RATE);
}
