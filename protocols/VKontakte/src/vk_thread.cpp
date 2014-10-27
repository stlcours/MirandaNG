/*
Copyright (c) 2013-14 Miranda NG project (http://miranda-ng.org)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation version 2
of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include "stdafx.h"

UINT_PTR CVkProto::m_timer;

char szBlankUrl[] = "http://api.vk.com/blank.html";

void CVkProto::ShutdownSession()
{
	debugLogA("CVkProto::ShutdownSession");
	if (m_hWorkerThread) {
		m_bTerminated = true;
		SetEvent(m_evRequestsQueue);
	}

	OnLoggedOut();
}

void CVkProto::ConnectionFailed(int iReason)
{
	delSetting("AccessToken");

	ProtoBroadcastAck(NULL, ACKTYPE_LOGIN, ACKRESULT_FAILED, NULL, iReason);
	debugLogA("CVkProto::ConnectionFailed ShutdownSession");
	ShutdownSession();
}

static VOID CALLBACK TimerProc(HWND, UINT, UINT_PTR, DWORD)
{
	for (int i = 0; i < vk_Instances.getCount(); i++){
		vk_Instances[i]->SetServerStatus(vk_Instances[i]->m_iDesiredStatus);
		vk_Instances[i]->RetrieveUsersInfo(true);
	}
}

static void CALLBACK VKSetTimer(void *pObject)
{
	CVkProto::m_timer = SetTimer(NULL, 0, 60000, TimerProc);
}

static void CALLBACK VKUnsetTimer(void *pObject)
{
	CVkProto *ppro = (CVkProto*)pObject;
	if (CVkProto::m_timer)
		KillTimer(NULL, CVkProto::m_timer);
	CVkProto::m_timer = 0;
}

void CVkProto::OnLoggedIn()
{
	debugLogA("CVkProto::OnLoggedIn");
	m_bOnline = true;
	SetServerStatus(m_iDesiredStatus);

	// initialize online timer
	if (!m_timer)
		CallFunctionAsync(VKSetTimer, this);
}

void CVkProto::OnLoggedOut()
{
	debugLogA("CVkProto::OnLoggedOut");
	m_bOnline = false;

	if (m_pollingConn)
		CallService(MS_NETLIB_SHUTDOWN, (WPARAM)m_pollingConn, 0);

	ProtoBroadcastAck(NULL, ACKTYPE_STATUS, ACKRESULT_SUCCESS, (HANDLE)m_iStatus, ID_STATUS_OFFLINE);
	m_iStatus = m_iDesiredStatus = ID_STATUS_OFFLINE;

	bool bOnline = false;
	for (int i=0; i < vk_Instances.getCount(); i++)
		bOnline = bOnline&&vk_Instances[i]->IsOnline();
	if(!bOnline)
		CallFunctionAsync(VKUnsetTimer, this);
	SetAllContactStatuses(ID_STATUS_OFFLINE);
	m_chats.destroy();
}

void CVkProto::SetServerStatus(int iNewStatus)
{
	debugLogA("CVkProto::SetServerStatus %d %d", iNewStatus, m_iStatus);
	if (!IsOnline() || iNewStatus < ID_STATUS_OFFLINE)
		return;

	int iOldStatus = m_iStatus;
	CMString oldStatusMsg = db_get_tsa(0, m_szModuleName, "OldStatusMsg");
	CMString ListeningToMsg = db_get_tsa(0, m_szModuleName, "ListeningTo");

	if (iNewStatus == ID_STATUS_OFFLINE) {
		m_iStatus = ID_STATUS_OFFLINE;
		if (!ListeningToMsg.IsEmpty()) 
			RetrieveStatusMsg(oldStatusMsg);
		Push(new AsyncHttpRequest(this, REQUEST_GET, "/method/account.setOffline.json", true, &CVkProto::OnReceiveSmth)
			<< VER_API);
	}
	else if (iNewStatus != ID_STATUS_INVISIBLE) {
		m_iStatus = ID_STATUS_ONLINE; 
		Push(new AsyncHttpRequest(this, REQUEST_GET, "/method/account.setOnline.json", true, &CVkProto::OnReceiveSmth)
			<< VER_API);
	}
	else {
		m_iStatus = ID_STATUS_INVISIBLE;
		if (!ListeningToMsg.IsEmpty()) 
			RetrieveStatusMsg(oldStatusMsg);
	}

	ProtoBroadcastAck(NULL, ACKTYPE_STATUS, ACKRESULT_SUCCESS, (HANDLE)iOldStatus, m_iStatus);
}

/////////////////////////////////////////////////////////////////////////////////////////

static char VK_TOKEN_BEG[] = "access_token=";

void CVkProto::OnOAuthAuthorize(NETLIBHTTPREQUEST *reply, AsyncHttpRequest *pReq)
{
	debugLogA("CVkProto::OnOAuthAuthorize %d", reply->resultCode);
	GrabCookies(reply);

	if (reply->resultCode == 302) { // manual redirect
		LPCSTR pszLocation = findHeader(reply, "Location");
		if (pszLocation) {
			if (!_strnicmp(pszLocation, szBlankUrl, sizeof(szBlankUrl)-1)) {
				m_szAccessToken = NULL;
				LPCSTR p = strstr(pszLocation, VK_TOKEN_BEG);
				if (p) {
					p += sizeof(VK_TOKEN_BEG)-1;
					for (LPCSTR q = p+1; *q; q++) {
						if (*q == '&' || *q == '=' || *q == '\"') {
							m_szAccessToken = mir_strndup(p, q-p);
							break;
						}
					}
					if (m_szAccessToken == NULL)
						m_szAccessToken = mir_strdup(p);
					setString("AccessToken", m_szAccessToken);
					RetrieveMyInfo();
				}
				else {
					delSetting("AccessToken");
					ConnectionFailed(LOGINERR_NOSERVER);
				}
			}
			else {
				AsyncHttpRequest *pReq = new AsyncHttpRequest();
				pReq->requestType = REQUEST_GET;
				pReq->flags = NLHRF_DUMPASTEXT | NLHRF_HTTP11;
				pReq->m_pFunc = &CVkProto::OnOAuthAuthorize;
				pReq->AddHeader("Referer", m_prevUrl);
				pReq->Redirect(reply);
				if (pReq->m_szUrl) {
					ApplyCookies(pReq);
					m_prevUrl = pReq->m_szUrl;
				}
				Push(pReq);
			}
		}
		else ConnectionFailed(LOGINERR_NOSERVER);
		return;
	}

	if (reply->resultCode != 200) { // something went wrong
LBL_NoForm:
		ConnectionFailed(LOGINERR_NOSERVER);
		return;
	}

	if (strstr(reply->pData, "service_msg_warning")) {
		ConnectionFailed(LOGINERR_WRONGPASSWORD);
		return;
	}

	// Application requests access to user's account
	if (!strstr(reply->pData, "form method=\"post\""))
		goto LBL_NoForm;

	CMStringA szAction, szBody;
	bool bSuccess = AutoFillForm(reply->pData, szAction, szBody);
	if (!bSuccess || szAction.IsEmpty() || szBody.IsEmpty()) {
		if (m_prevError)
			goto LBL_NoForm;
		m_prevError = true;
	}

	pReq = new AsyncHttpRequest();
	pReq->requestType = REQUEST_POST;
	pReq->flags = NLHRF_DUMPASTEXT | NLHRF_HTTP11;
	pReq->m_szParam = szBody;
	pReq->m_szUrl = szAction; m_prevUrl = pReq->m_szUrl;
	pReq->m_pFunc = &CVkProto::OnOAuthAuthorize;
	pReq->AddHeader("Content-Type", "application/x-www-form-urlencoded");
	pReq->Redirect(reply);
	ApplyCookies(pReq);
	Push(pReq);
}

/////////////////////////////////////////////////////////////////////////////////////////

void CVkProto::RetrieveMyInfo()
{
	debugLogA("CVkProto::RetrieveMyInfo");
	Push(new AsyncHttpRequest(this, REQUEST_GET, "/method/users.get.json", true, &CVkProto::OnReceiveMyInfo)
		<< VER_API);
}

void CVkProto::OnReceiveMyInfo(NETLIBHTTPREQUEST *reply, AsyncHttpRequest *pReq)
{
	debugLogA("CVkProto::OnReceiveMyInfo %d", reply->resultCode);
	if (reply->resultCode != 200) {
		ConnectionFailed(LOGINERR_WRONGPASSWORD);
		return;
	}

	JSONROOT pRoot;
	JSONNODE *pResponse = CheckJsonResponse(pReq, reply, pRoot);
	if (pResponse == NULL)
		return;
	
	m_myUserId = json_as_int(json_get(json_at(pResponse, 0), "id"));
	setDword("ID", m_myUserId);
		
	OnLoggedIn();
	RetrieveUserInfo(m_myUserId);
	RetrieveUnreadMessages();
	RetrieveFriends();
	RetrievePollingInfo();
}

/////////////////////////////////////////////////////////////////////////////////////////

static char fieldsName[] = "id, first_name, last_name, photo_100, bdate, sex, timezone, contacts, online, status, about, domain";

MCONTACT CVkProto::SetContactInfo(JSONNODE* pItem, bool flag, bool self)
{
	debugLogA("CVkProto::SetContactInfo");
	if (pItem == NULL)
		return -1;

	LONG userid = json_as_int(json_get(pItem, "id"));
	if (userid == 0)
		return -1;

	MCONTACT hContact = FindUser(userid, flag);
	
	if (userid == m_myUserId){
		if (hContact != NULL)
			if (self)
				hContact = NULL;
			else
				SetContactInfo(pItem, flag, true);
	}
	else if (hContact == NULL)
		return -1;
	
	CMString tszNick, tszValue;
	int iValue;

	tszValue = json_as_string(json_get(pItem, "first_name"));
	if (!tszValue.IsEmpty()) {
		setTString(hContact, "FirstName", tszValue.GetBuffer());
		tszNick.Append(tszValue);
		tszNick.AppendChar(' ');
	}

	tszValue = json_as_string(json_get(pItem, "last_name"));
	if (!tszValue.IsEmpty()) {
		setTString(hContact, "LastName", tszValue.GetBuffer());
		tszNick.Append(tszValue);
	}

	if (!tszNick.IsEmpty())
		setTString(hContact, "Nick", tszNick);

	int sex = json_as_int(json_get(pItem, "sex"));
	if (sex)
		setByte(hContact, "Gender", sex == 2 ? 'M' : 'F');

	tszValue = json_as_string(json_get(pItem, "bdate"));
	if (!tszValue.IsEmpty()) {
		int d, m, y, iReadCount;
		iReadCount = _stscanf(tszValue.GetBuffer(), L"%d.%d.%d", &d, &m, &y);
		if (iReadCount> 1) {
			setByte(hContact, "BirthDay", d);
			setByte(hContact, "BirthMonth", m);
			if (iReadCount == 3)
				setWord(hContact, "BirthYear", y);
		}
	}

	tszValue = json_as_string(json_get(pItem, "photo_100"));
	if (!tszValue.IsEmpty()){
		SetAvatarUrl(hContact, tszValue.GetBuffer());
		ReloadAvatarInfo(hContact);
	}

	int iNewStatus = (json_as_int(json_get(pItem, "online")) == 0) ? ID_STATUS_OFFLINE : ID_STATUS_ONLINE;
	if (getWord(hContact, "Status", 0) != iNewStatus)
		setWord(hContact, "Status", iNewStatus);

	if (iNewStatus == ID_STATUS_ONLINE){
		int online_app = _ttoi(json_as_string(json_get(pItem, "online_app")));
		int online_mobile = json_as_int(json_get(pItem, "online_mobile"));
		
		if (online_app == 0 && online_mobile == 0)
			SetMirVer(hContact, 7); // vk.com
		else if (online_app != 0)
			SetMirVer(hContact, online_app); // App
		else 
			SetMirVer(hContact, 1); // m.vk.com
	}
	else
		SetMirVer(hContact, -1); // unset MinVer
		
	if ((iValue = json_as_int(json_get(pItem, "timezone"))) != 0)
		setByte(hContact, "Timezone", iValue * -2);

	tszValue = json_as_string(json_get(pItem, "mobile_phone"));
	if (!tszValue.IsEmpty())
		setTString(hContact, "Cellular", tszValue.GetBuffer());
	tszValue = json_as_string(json_get(pItem, "home_phone"));
	if (!tszValue.IsEmpty())
		setTString(hContact, "Phone", tszValue.GetBuffer());

	tszValue = json_as_string(json_get(pItem, "status"));
	CMString tszOldStatus(db_get_tsa(hContact, hContact ? "CList" : m_szModuleName, "StatusMsg"));
	if (tszValue != tszOldStatus){
		db_set_ts(hContact, hContact ? "CList" : m_szModuleName, "StatusMsg", tszValue.GetBuffer());
		db_unset(hContact, m_szModuleName, "AudioUrl");
		JSONNODE* pAudio = json_get(pItem, "status_audio");
		if (pAudio != NULL) {
			db_set_ts(hContact, m_szModuleName, "ListeningTo", tszValue.GetBuffer());
			tszValue = json_as_string(json_get(pAudio, "url"));
			db_set_ts(hContact, m_szModuleName, "AudioUrl", tszValue.GetBuffer());
		}
		else if ((tszValue.GetBuffer()[0] == TCHAR(9835)) && (tszValue.GetLength() > 2))
			db_set_ts(hContact, m_szModuleName, "ListeningTo", &(tszValue.GetBuffer())[2]);
		else
			db_unset(hContact, m_szModuleName, "ListeningTo");
	}

	tszValue = json_as_string(json_get(pItem, "about"));
	
	if (!tszValue.IsEmpty())
		setTString(hContact, "About", tszValue.GetBuffer());

	tszValue = json_as_string(json_get(pItem, "domain"));
	if (!tszValue.IsEmpty())
		setTString(hContact, "domain", tszValue.GetBuffer());

	return hContact;
}

void CVkProto::RetrieveUserInfo(LONG userID)
{
	debugLogA("CVkProto::RetrieveUserInfo (%d)", userID);
	if (!IsOnline())
		return;
	CMString userIDs, code;
	userIDs.AppendFormat(L"%i", userID);
	CMString codeformat("var userIDs=\"%s\";"
		"return{\"users\":API.users.get({\"user_ids\":userIDs,\"fields\":\"%s\",\"name_case\":\"nom\"})};");
		
	code.AppendFormat(codeformat, userIDs.GetBuffer(), CMString(fieldsName).GetBuffer());
	Push(new AsyncHttpRequest(this, REQUEST_POST, "/method/execute.json", true, &CVkProto::OnReceiveUserInfo)
		<< TCHAR_PARAM("code", code)
		<< VER_API);
}

void CVkProto::RetrieveUsersInfo(bool flag)
{
	debugLogA("CVkProto::RetrieveUsersInfo");
	if (!IsOnline())
		return;
	CMString userIDs, code;
	for (MCONTACT hContact = db_find_first(m_szModuleName); hContact; hContact = db_find_next(hContact, m_szModuleName)){
		LONG userID = getDword(hContact, "ID", -1);
		if (userID == -1)
			continue;
		if (!userIDs.IsEmpty())
			userIDs.AppendChar(',');
		userIDs.AppendFormat(L"%i", userID);
	}

	CMString codeformat("var userIDs=\"%s\";"
		"return{\"users\":API.users.get({\"user_ids\":userIDs,\"fields\":\"%s\",\"name_case\":\"nom\"})"); 
	if (flag)
		codeformat += CMString(",\"requests\":API.friends.getRequests({\"extended\":0,\"need_mutual\":0,\"out\":0})};");
	else
		codeformat += CMString("};");
	code.AppendFormat(codeformat, userIDs.GetBuffer(), CMString(flag ? "online,status" : fieldsName).GetBuffer());

	Push(new AsyncHttpRequest(this, REQUEST_POST, "/method/execute.json", true, &CVkProto::OnReceiveUserInfo)
		<< TCHAR_PARAM("code", code)
		<< VER_API);
}

void CVkProto::OnReceiveUserInfo(NETLIBHTTPREQUEST *reply, AsyncHttpRequest *pReq)
{
	debugLogA("CVkProto::OnReceiveUserInfo %d", reply->resultCode);

	if ((reply->resultCode != 200) || !IsOnline())
		return;
	
	JSONROOT pRoot;
	JSONNODE *pResponse = CheckJsonResponse(pReq, reply, pRoot);
	if (pResponse == NULL)
		return;

	JSONNODE *pUsers = json_get(pResponse, "users");
	if (pUsers == NULL)
		return;
	
	for (size_t i = 0; SetContactInfo(json_at(pUsers, i)) != -1; i++);

	JSONNODE *pRequests = json_get(pResponse, "requests");
	if (pRequests == NULL)
		return;
	
	int iCount = json_as_int(json_get(pRequests, "count"));
	JSONNODE *pItems = json_get(pRequests, "items"), *pInfo;
	if (!iCount||(pItems == NULL))
		return;

	debugLogA("CVkProto::OnReceiveUserInfo AuthRequests");
	for (int i = 0; (pInfo = json_at(pItems, i)) != NULL; i++) {
		LONG userid = json_as_int(pInfo);
		if (userid == 0)
			break;
		MCONTACT hContact = FindUser(userid, true);
		if (!getBool(hContact, "ReqAuth", false)){
			RetrieveUserInfo(userid);
			setByte(hContact, "ReqAuth", 1);
			Sleep(1000);
			DBAddAuthRequest(hContact);
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////

void CVkProto::RetrieveFriends()
{
	debugLogA("CVkProto::RetrieveFriends");
	if (!IsOnline())
		return;
	Push(new AsyncHttpRequest(this, REQUEST_GET, "/method/friends.get.json", true, &CVkProto::OnReceiveFriends)
		<< INT_PARAM("count", 1000) 
		<< CHAR_PARAM("fields", fieldsName)
		<<VER_API);
}

void CVkProto::OnReceiveFriends(NETLIBHTTPREQUEST *reply, AsyncHttpRequest *pReq)
{
	debugLogA("CVkProto::OnReceiveFriends %d", reply->resultCode);
	if ((reply->resultCode != 200) || !IsOnline())
		return;

	JSONROOT pRoot;
	JSONNODE *pResponse = CheckJsonResponse(pReq, reply, pRoot);
	if (pResponse == NULL)
		return;

	bool bCleanContacts = getBool("AutoClean", false);
	LIST<void> arContacts(10, PtrKeySortT);
		
	for (MCONTACT hContact = db_find_first(m_szModuleName); hContact; hContact = db_find_next(hContact, m_szModuleName)){
		if (!isChatRoom(hContact))
			setByte(hContact, "Auth", 1);
		db_unset(hContact, m_szModuleName, "ReqAuth");
		SetMirVer(hContact, -1);
		if (bCleanContacts&&!isChatRoom(hContact))
			arContacts.insert((HANDLE)hContact);
	}

	int iCount = json_as_int(json_get(pResponse, "count"));
	JSONNODE *pItems = json_get(pResponse, "items");

	if (pItems)
		for (int i = 0; i<iCount; i++) {
			MCONTACT hContact = SetContactInfo(json_at(pItems, i), true);

			if ((hContact == NULL) || hContact == -1)
				continue;

			arContacts.remove((HANDLE)hContact);
			setByte(hContact, "Auth", 0);
		}

	if (bCleanContacts)
		for (int i = 0; i < arContacts.getCount(); i++)
			CallService(MS_DB_CONTACT_DELETE, (WPARAM)arContacts[i], 0);
}

/////////////////////////////////////////////////////////////////////////////////////////
int  CVkProto::OnDbEventRead(WPARAM hContact, LPARAM)
{
	debugLogA("CVkProto::OnDbEventRead");
	if (m_iMarkMessageReadOn == markOnRead)
		MarkMessagesRead(hContact);
	return 0;
}

void CVkProto::MarkMessagesRead(const CMStringA &mids)
{
	debugLogA("CVkProto::MarkMessagesRead (mids)");
	if (!IsOnline())
		return;
	if (mids.IsEmpty())
		return;

	Push(new AsyncHttpRequest(this, REQUEST_GET, "/method/messages.markAsRead.json", true, &CVkProto::OnReceiveSmth)
		<< CHAR_PARAM("message_ids", mids) 
		<< VER_API);
}

void CVkProto::MarkMessagesRead(const MCONTACT hContact)
{
	debugLogA("CVkProto::MarkMessagesRead (hContact)");
	if (!IsOnline())
		return;
	LONG userID = getDword(hContact, "ID", -1);
	if (userID == -1)
		return;

	Push(new AsyncHttpRequest(this, REQUEST_GET, "/method/messages.markAsRead.json", true, &CVkProto::OnReceiveSmth)
		<< INT_PARAM("peer_id", userID) 
		<< VER_API);
}

void CVkProto::RetrieveMessagesByIds(const CMStringA &mids)
{
	debugLogA("CVkProto::RetrieveMessagesByIds");
	if (!IsOnline())
		return;
	if (mids.IsEmpty())
		return;

	Push(new AsyncHttpRequest(this, REQUEST_GET, "/method/messages.getById.json", true, &CVkProto::OnReceiveMessages)
		<< CHAR_PARAM("message_ids", mids)
		<< VER_API);
}

void CVkProto::RetrieveUnreadMessages()
{
	debugLogA("CVkProto::RetrieveUnreadMessages");
	if (!IsOnline())
		return;
	Push(new AsyncHttpRequest(this, REQUEST_GET, "/method/messages.getDialogs.json", true, &CVkProto::OnReceiveDlgs)
		<< VER_API);
}

static char* szImageTypes[] = { "photo_2560", "photo_1280", "photo_807", "photo_604", "photo_256", "photo_130", "photo_128", "photo_75", "photo_64" };

void CVkProto::OnReceiveMessages(NETLIBHTTPREQUEST *reply, AsyncHttpRequest *pReq)
{
	debugLogA("CVkProto::OnReceiveMessages %d", reply->resultCode);
	if (reply->resultCode != 200)
		return;

	JSONROOT pRoot;
	JSONNODE *pResponse = CheckJsonResponse(pReq, reply, pRoot);
	if (pResponse == NULL)
		return;

	CMStringA mids;
	int numMessages = json_as_int(json_get(pResponse, "count"));
	JSONNODE *pMsgs = json_get(pResponse, "items");

	for (int i = 0; i < numMessages; i++) {
		JSONNODE *pMsg = json_at(pMsgs, i);
		if (pMsg == NULL)
			break;

		UINT mid = json_as_int(json_get(pMsg, "id"));
		ptrT ptszBody(json_as_string(json_get(pMsg, "body")));
		int datetime = json_as_int(json_get(pMsg, "date"));
		int isOut = json_as_int(json_get(pMsg, "out"));
		int isRead = json_as_int(json_get(pMsg, "read_state"));
		int uid = json_as_int(json_get(pMsg, "user_id"));
		JSONNODE *pAttachments = json_get(pMsg, "attachments");

		if (pAttachments != NULL)
			ptszBody = mir_tstrdup(CMString(ptszBody) + GetAttachmentDescr(pAttachments));

		MCONTACT hContact = FindUser(uid, true);
		int chat_id = json_as_int(json_get(pMsg, "chat_id"));

		char szMid[40];
		_itoa(mid, szMid, 10);
		if ((m_iMarkMessageReadOn == markOnReceive) || (chat_id != 0)){
			if (!mids.IsEmpty())
				mids.AppendChar(',');
			mids.Append(szMid);
		}

		if (chat_id != 0) {
			CMString action_chat = json_as_string(json_get(pMsg, "action"));
			int action_mid = _ttoi(json_as_string(json_get(pMsg, "action_mid")));
			if ((action_chat == "chat_kick_user") && (action_mid == m_myUserId))
				KickFromChat(chat_id, uid, pMsg);
			else
				AppendChatMessage(chat_id, pMsg, false);
			continue;
		}

		PROTORECVEVENT recv = { 0 };
			recv.flags = PREF_TCHAR;
		if (isRead&&!m_bMesAsUnread)
			recv.flags |= PREF_CREATEREAD;
		if (isOut)
			recv.flags |= PREF_SENT;
		else if (m_bUserForceOnlineOnActivity)
			setWord(hContact, "Status", ID_STATUS_ONLINE);

		recv.timestamp = m_bUseLocalTime?time(NULL):datetime;
		recv.tszMessage = ptszBody;
		recv.lParam = isOut;
		recv.pCustomData = szMid;
		recv.cbCustomDataSize = (int)strlen(szMid);
		Sleep(100);
		if (!CheckMid(mid)){
			ProtoChainRecvMsg(hContact, &recv);
			if (mid>getDword(hContact, "lastmsgid", -1))
				setDword(hContact, "lastmsgid", mid);
		}
	}

	if (!mids.IsEmpty())
		MarkMessagesRead(mids);
}

void CVkProto::OnReceiveDlgs(NETLIBHTTPREQUEST *reply, AsyncHttpRequest *pReq)
{
	debugLogA("CVkProto::OnReceiveDlgs %d", reply->resultCode);
	if (reply->resultCode != 200)
		return;

	JSONROOT pRoot;
	JSONNODE *pResponse = CheckJsonResponse(pReq, reply, pRoot);
	if (pResponse == NULL)
		return;

	int numDlgs = json_as_int(json_get(pResponse, "count"));
	JSONNODE *pDlgs = json_get(pResponse, "items");
	
	if (pDlgs == NULL)
		return;
	if (m_bAutoSyncHistory&&m_bPopUpSyncHistory)
		MsgPopup(NULL, TranslateT("Start sync history"), TranslateT("Sync history"));
	for (int i = 0; i < numDlgs; i++) {
		JSONNODE *pDlg = json_at(pDlgs, i);
		if (pDlg == NULL)
			break; 
		
		int numUnread = json_as_int(json_get(pDlg, "unread"));
		
		pDlg = json_get(pDlg, "message");
		if (pDlg == NULL)
			break; 

		int chatid = json_as_int(json_get(pDlg, "chat_id"));
		if (chatid != 0) {
			debugLogA("CVkProto::OnReceiveDlgs chatid = %d", chatid);
			if (m_chats.find((CVkChatInfo*)&chatid) == NULL)
				AppendChat(chatid, pDlg);
		}
		else if (m_bAutoSyncHistory) {
			int mid = json_as_int(json_get(pDlg, "id"));
			int uid = json_as_int(json_get(pDlg, "user_id"));
			MCONTACT hContact = FindUser(uid, true);
			
			if ((getDword(hContact, "lastmsgid", -1) == -1) && numUnread){
				setDword(hContact, "new_lastmsgid", mid);
				GetHistoryDlgMessages(hContact, 0, numUnread, -1);
			}
			else
				GetHistoryDlg(hContact, mid);
			
			if (m_iMarkMessageReadOn==markOnReceive&&numUnread)
				MarkMessagesRead(hContact);
		}
		else if (numUnread) {
			int mid = json_as_int(json_get(pDlg, "id"));
			int uid = json_as_int(json_get(pDlg, "user_id"));
			MCONTACT hContact = FindUser(uid, true);
			setDword(hContact, "new_lastmsgid", mid);
			GetHistoryDlgMessages(hContact, 0, numUnread, -1);
			
			if (m_iMarkMessageReadOn == markOnReceive)
				MarkMessagesRead(hContact);
		}
	}
	if (m_bAutoSyncHistory&&m_bPopUpSyncHistory)
		MsgPopup(NULL, TranslateT("Sync history complete"), TranslateT("Sync history"));
	RetrieveUsersInfo();
}

/////////////////////////////////////////////////////////////////////////////////////////

void CVkProto::GetHistoryDlg(MCONTACT hContact, int iLastMsg)
{
	debugLogA("CVkProto::GetHistoryDlg %d", iLastMsg);
	int lastmsgid = getDword(hContact, "lastmsgid", -1);
	if ((lastmsgid == -1)||(!IsOnline())) {
		setDword(hContact, "lastmsgid", iLastMsg);
		return;
	}
	int maxOffset = iLastMsg - lastmsgid;
	setDword(hContact, "new_lastmsgid", iLastMsg);
	GetHistoryDlgMessages(hContact, 0, maxOffset, -1);
}

void CVkProto::GetHistoryDlgMessages(MCONTACT hContact, int iOffset, int iMaxCount, int lastcount)
{
	debugLogA("CVkProto::GetHistoryDlgMessages");
	LONG userID = getDword(hContact, "ID", -1);
	if (-1 == userID)
		return;

	if (lastcount == 0 || iMaxCount < 1 || !IsOnline()) {
		setDword(hContact, "lastmsgid", getDword(hContact, "new_lastmsgid", -1));
		db_unset(hContact, m_szModuleName, "new_lastmsgid");
		if (getBool(hContact, "ImportHistory", false))
			MsgPopup(hContact, TranslateT("History import is complete"), TranslateT("History import"));
		db_unset(hContact, m_szModuleName, "ImportHistory");
		return;
	}

	int iReqCount = iMaxCount > MAXHISTORYMIDSPERONE ? MAXHISTORYMIDSPERONE : iMaxCount;

	debugLogA("CVkProto::GetHistoryDlgMessages %d %d %d", userID, iOffset, iReqCount);
	AsyncHttpRequest *pReq = new AsyncHttpRequest(this, REQUEST_GET, "/method/messages.getHistory.json", true, &CVkProto::OnReceiveHistoryMessages)
		<< INT_PARAM("offset", iOffset) 
		<< INT_PARAM("count", iReqCount) 
		<< INT_PARAM("user_id", userID) 
		<< VER_API;

	pReq->pUserInfo = new CVkSendMsgParam(hContact, iOffset, iMaxCount);
	Push(pReq);
}

void CVkProto::OnReceiveHistoryMessages(NETLIBHTTPREQUEST *reply, AsyncHttpRequest *pReq)
{
	debugLogA("CVkProto::OnReceiveHistoryMessages %d", reply->resultCode);
	if (reply->resultCode != 200)
		return;

	JSONROOT pRoot;
	JSONNODE *pResponse = CheckJsonResponse(pReq, reply, pRoot);
	if (pResponse == NULL)
		return;

	CMStringA mids;
	CVkSendMsgParam *param = (CVkSendMsgParam*)pReq->pUserInfo;
	int numMessages = json_as_int(json_get(pResponse, "count")),
		i = 0,
		lastmsgid = getDword(param->hContact, "lastmsgid", -1),
		mid = -1;

	JSONNODE *pMsgs = json_get(pResponse, "items");
	
	int new_lastmsgid = getDword(param->hContact, "new_lastmsgid", -1);
	int his = getByte(param->hContact, "ImportHistory", 0);

	for (i = 0; i < numMessages; i++) {
		JSONNODE *pMsg = json_at(pMsgs, i);
		if (pMsg == NULL)
			break;

		mid = json_as_int(json_get(pMsg, "id"));
		if (his && new_lastmsgid == -1) {
			new_lastmsgid = mid;
			setDword(param->hContact, "new_lastmsgid", mid);
		}
		if (mid <= lastmsgid)
			break;

		char szMid[40];
		_itoa(mid, szMid, 10);

		ptrT ptszBody(json_as_string(json_get(pMsg, "body")));
		int datetime = json_as_int(json_get(pMsg, "date"));
		int isOut = json_as_int(json_get(pMsg, "out"));
		int isRead = json_as_int(json_get(pMsg, "read_state"));
		int uid = json_as_int(json_get(pMsg, "user_id"));

		JSONNODE *pAttachments = json_get(pMsg, "attachments");
		if (pAttachments != NULL)
			ptszBody = mir_tstrdup(CMString(ptszBody) + GetAttachmentDescr(pAttachments));

		MCONTACT hContact = FindUser(uid, true);
		PROTORECVEVENT recv = { 0 };
		recv.flags = PREF_TCHAR;
		if (isRead)
			recv.flags |= PREF_CREATEREAD;
		if (isOut)
			recv.flags |= PREF_SENT;
		recv.timestamp = datetime;
		recv.tszMessage = ptszBody;
		recv.lParam = isOut;
		recv.pCustomData = szMid;
		recv.cbCustomDataSize = (int)strlen(szMid);
		ProtoChainRecvMsg(hContact, &recv);
	}
	GetHistoryDlgMessages(param->hContact, param->iMsgID + i, param->iCount - i, i);
	delete param;
}

void CVkProto::RetrievePollingInfo()
{
	debugLogA("CVkProto::RetrievePollingInfo");
	if (!IsOnline())
		return;
	Push(new AsyncHttpRequest(this, REQUEST_GET, "/method/messages.getLongPollServer.json", true, &CVkProto::OnReceivePollingInfo)
		<< VER_API);
}

void CVkProto::OnReceivePollingInfo(NETLIBHTTPREQUEST *reply, AsyncHttpRequest *pReq)
{
	debugLogA("CVkProto::OnReceivePollingInfo %d", reply->resultCode);
	if (reply->resultCode != 200)
		return;

	JSONROOT pRoot;
	JSONNODE *pResponse = CheckJsonResponse(pReq, reply, pRoot);
	if (pResponse == NULL)
		return;

	m_pollingTs = mir_t2a(ptrT(json_as_string(json_get(pResponse, "ts"))));
	m_pollingKey = mir_t2a(ptrT(json_as_string(json_get(pResponse, "key"))));
	m_pollingServer = mir_t2a(ptrT(json_as_string(json_get(pResponse, "server"))));
	if (!m_hPollingThread && m_pollingTs != NULL && m_pollingKey != NULL && m_pollingServer != NULL)
		m_hPollingThread = ForkThreadEx(&CVkProto::PollingThread, NULL, NULL);
}

/////////////////////////////////////////////////////////////////////////////////////////
void CVkProto::OnReceiveStatus(NETLIBHTTPREQUEST *reply, AsyncHttpRequest *pReq)
{
	debugLogA("CVkProto::OnReceiveStatus %d", reply->resultCode);
	if (reply->resultCode == 200){
		JSONROOT pRoot;
		JSONNODE *pResponse = CheckJsonResponse(pReq, reply, pRoot);
		if (pResponse != NULL){
			JSONNODE *pAudio = json_get(pResponse, "audio");
			if (pAudio == NULL){
				CMString StatusText = json_as_string(json_get(pResponse, "text"));
				if (StatusText.GetBuffer()[0] != TCHAR(9835))
					setTString("OldStatusMsg", StatusText.GetBuffer());
			}
		}
	}
}

void CVkProto::RetrieveStatusMsg(const CMString &StatusMsg)
{
	debugLogA("CVkProto::RetrieveStatusMsg");
	if (!IsOnline())
		return;
	Push(new AsyncHttpRequest(this, REQUEST_GET, "/method/status.set.json", true, &CVkProto::OnReceiveSmth)
		<< TCHAR_PARAM("text", StatusMsg)
		<< VER_API);
}

void CVkProto::RetrieveStatusMusic(const CMString &StatusMsg)
{
	debugLogA("CVkProto::RetrieveStatusMusic");
	if (!IsOnline() || (m_iStatus == ID_STATUS_INVISIBLE) || (m_iMusicSendMetod == sendNone))
		return;
	
	CMString code;
	CMString oldStatusMsg = db_get_tsa(0, m_szModuleName, "OldStatusMsg");
	if (StatusMsg.IsEmpty()){
		if (m_iMusicSendMetod == sendBroadcastOnly)
		    code = "API.audio.setBroadcast();return null;";
		else{
		    CMString codeformat("API.status.set({text:\"%s\"});return null;");
		    code.AppendFormat(codeformat, oldStatusMsg);
		}
	}
	else {
		if (m_iMusicSendMetod == sendBroadcastOnly){
			CMString codeformat("var StatusMsg=\"%s\";var CntLmt=100;var OldMsg=API.status.get();"
				"var Tracks = API.audio.search({\"q\":StatusMsg,\"count\":CntLmt, \"search_own\":1});"
				"var Cnt=Tracks.count;if (Cnt>CntLmt){Cnt=CntLmt;}"
				"if (Cnt == 0){API.audio.setBroadcast();}"
				"else{var i = 0;var j = 0;var Track=\" \";"
				"while (i<Cnt){Track=Tracks.items[i].artist+\" - \"+Tracks.items[i].title;if(Track == StatusMsg){j = i;}i=i+1;}"
				"Track=Tracks.items[j].owner_id + \"_\" + Tracks.items[j].id;API.audio.setBroadcast({\"audio\":Track});"
				"};return OldMsg;");
			code.AppendFormat(codeformat, StatusMsg);
		}
		else if (m_iMusicSendMetod == sendStatusOnly){
			CMString codeformat("var StatusMsg=\"&#9835; %s\";var OldMsg=API.status.get();"
				"API.status.set({\"text\":StatusMsg});"
				"return OldMsg;");
		    code.AppendFormat(codeformat, StatusMsg);
		}
		else if (m_iMusicSendMetod == sendBroadcastAndStatus){
			CMString codeformat("var StatusMsg=\"%s\";var CntLmt=100;var Track=\" \";var OldMsg=API.status.get();"
				"var Tracks = API.audio.search({\"q\":StatusMsg,\"count\":CntLmt, \"search_own\":1});"
				"var Cnt=Tracks.count;if (Cnt>CntLmt){Cnt=CntLmt;}"
				"if (Cnt == 0){Track = \"&#9835; \"+StatusMsg; API.status.set({\"text\":Track});}"
				"else{var i = 0;var j = -1;"
				"while (i<Cnt){Track=Tracks.items[i].artist+\" - \"+Tracks.items[i].title;if(Track == StatusMsg){j = i;}i=i+1;}"
				"if(j==-1) {Track = \"&#9835; \"+StatusMsg; API.status.set({\"text\":Track});} else {"
				"Track=Tracks.items[j].owner_id + \"_\" + Tracks.items[j].id;};API.audio.setBroadcast({\"audio\":Track});"
				"}; return OldMsg;");
			code.AppendFormat(codeformat, StatusMsg);
		}
	}
	Push(new AsyncHttpRequest(this, REQUEST_GET, "/method/execute.json", true, &CVkProto::OnReceiveStatus)
		<< TCHAR_PARAM("code", code)
		<< VER_API);
}

INT_PTR __cdecl CVkProto::SvcSetListeningTo(WPARAM wParam, LPARAM lParam)
{
	debugLogA("CVkProto::SvcSetListeningTo");
	if (m_iMusicSendMetod == sendNone)
		return 1;
	LISTENINGTOINFO *pliInfo = (LISTENINGTOINFO*)lParam;
	CMStringW wszListeningTo;
	if (pliInfo == NULL || pliInfo->cbSize != sizeof(LISTENINGTOINFO)) 
		db_unset(NULL, m_szModuleName, "ListeningTo");
	else if (pliInfo->dwFlags & LTI_UNICODE) {
		if (ServiceExists(MS_LISTENINGTO_GETPARSEDTEXT))
			wszListeningTo = ptrT((LPWSTR)CallService(MS_LISTENINGTO_GETPARSEDTEXT, (WPARAM)L"%artist% - %title%", (LPARAM)pliInfo));
		else
			wszListeningTo.Format(L"%s - %s",
			pliInfo->ptszArtist ? pliInfo->ptszArtist : _T(""),
			pliInfo->ptszTitle ? pliInfo->ptszTitle : _T(""));
		setTString("ListeningTo", wszListeningTo);
	}
	RetrieveStatusMusic(wszListeningTo);
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
INT_PTR __cdecl CVkProto::SvcAddAsFriend(WPARAM hContact, LPARAM)
{
	debugLogA("CVkProto::SvcAddAsFriend");
	if (!IsOnline())
		return 1;
	CallContactService(hContact, PSS_AUTHREQUESTW, 0, (LPARAM)TranslateT("Please authorize me to add you to my friend list."));
	return 0;
}

INT_PTR __cdecl CVkProto::SvcDeleteFriend(WPARAM hContact, LPARAM flag)
{
	debugLogA("CVkProto::SvcDeleteFriend");
	LONG userID = getDword(hContact, "ID", -1);
	if (!IsOnline() ||(userID == -1))
		return 1;
	
	CMString formatstr = TranslateT("Are you sure to delete %s from your friend list?"),
		tszNick = db_get_tsa(hContact, m_szModuleName, "Nick"), 
		ptszMsg;
	if (flag==0){
		ptszMsg.AppendFormat(formatstr, tszNick.IsEmpty() ? TranslateT("(Unknown contact)") : tszNick);
		if (IDNO == MessageBox(NULL, ptszMsg.GetBuffer(), TranslateT("Attention!"), MB_ICONWARNING | MB_YESNO))
			return 1;
	}
	Push(new AsyncHttpRequest(this, REQUEST_GET, "/method/friends.delete.json", true, &CVkProto::OnReceiveDeleteFriend)
		<< INT_PARAM("user_id", userID)
		<< VER_API)->pUserInfo = new CVkSendMsgParam(hContact);

	return 0;
}

void CVkProto::OnReceiveDeleteFriend(NETLIBHTTPREQUEST* reply, AsyncHttpRequest* pReq)
{
	debugLogA("CVkProto::OnReceiveDeleteFriend %d", reply->resultCode);
	CVkSendMsgParam *param = (CVkSendMsgParam*)pReq->pUserInfo;
	if (reply->resultCode == 200){
		JSONROOT pRoot;
		JSONNODE *pResponse = CheckJsonResponse(pReq, reply, pRoot);
		if (pResponse != NULL) {
			CMString tszNick = db_get_tsa(param->hContact, m_szModuleName, "Nick");
			if (tszNick.IsEmpty())
				tszNick = TranslateT("(Unknown contact)");
			CMString msgformat, msg;
			int iRet = json_as_int(pResponse);
			switch (iRet){
			case 1:
				msgformat = TranslateT("User %s was deleted from your friend list");
				break;
			case 2:
				msgformat = TranslateT("Friend request from the user %s declined");
				break;
			case 3:
				msgformat = TranslateT("Friend request suggestion for the user %s deleted");
				break;
			}

			msg.AppendFormat(msgformat, tszNick);
			MsgPopup(param->hContact, msg.GetBuffer(), tszNick.GetBuffer());
			setByte(param->hContact, "Auth", 1);
		}
	}
	delete param;
}

INT_PTR __cdecl CVkProto::SvcBanUser(WPARAM hContact, LPARAM)
{
	debugLogA("CVkProto::SvcBanUser");
	LONG userID = getDword(hContact, "ID", -1);
	if (!IsOnline() || (userID == -1))
		return 1;

	CMStringA code;
	code.AppendFormat("var userID=\"%d\";API.account.banUser({\"user_id\":userID});", userID);
	CMString tszVarWarning;

	if (m_bReportAbuse){
		debugLogA("CVkProto::SvcBanUser m_bReportAbuse = true");
		code += "API.users.report({\"user_id\":userID,type:\"spam\"});";
		tszVarWarning = TranslateT(" report abuse on him/her");
	}
	if (m_bClearServerHistory){
		debugLogA("CVkProto::SvcBanUser m_bClearServerHistory = true");
		code += "API.messages.deleteDialog({\"user_id\":userID,count:10000});";
		if (!tszVarWarning.IsEmpty())
			tszVarWarning.AppendChar(L',');
		tszVarWarning += TranslateT(" clear server history with him/her");
	}
	if (m_bRemoveFromFrendlist){
		debugLogA("CVkProto::SvcBanUser m_bRemoveFromFrendlist = true");
		code += "API.friends.delete({\"user_id\":userID});";
		if (!tszVarWarning.IsEmpty())
			tszVarWarning.AppendChar(L',');
		tszVarWarning += TranslateT(" remove him/her from your friend list");
	}
	if (m_bRemoveFromClist){
		debugLogA("CVkProto::SvcBanUser m_bRemoveFromClist = true");
		if (!tszVarWarning.IsEmpty())
			tszVarWarning.AppendChar(L',');
		tszVarWarning += TranslateT(" remove him/her from your contact list");
	}

	if (!tszVarWarning.IsEmpty())
		tszVarWarning += ".\n";
	code += "return 1;";

	CMString formatstr = TranslateT("Are you sure to ban %s? %s%sContinue?"),
		tszNick = db_get_tsa(hContact, m_szModuleName, "Nick"),
		ptszMsg;

	ptszMsg.AppendFormat(formatstr, 
		tszNick.IsEmpty() ? TranslateT("(Unknown contact)") : tszNick.GetBuffer(), 
		tszVarWarning.IsEmpty() ? L" " : TranslateT("\nIt will also"),
		tszVarWarning.IsEmpty() ? L"\n" : tszVarWarning.GetBuffer());

	if (IDNO == MessageBox(NULL, ptszMsg.GetBuffer(), TranslateT("Attention!"), MB_ICONWARNING | MB_YESNO))
		return 1;
	
	Push(new AsyncHttpRequest(this, REQUEST_GET, "/method/execute.json", true, &CVkProto::OnReceiveSmth)
		<< CHAR_PARAM("code", code)
		<< VER_API);

	if (m_bRemoveFromClist)
		CallService(MS_DB_CONTACT_DELETE, (WPARAM)hContact, 0);

	return 0;
}

INT_PTR __cdecl CVkProto::SvcReportAbuse(WPARAM hContact, LPARAM)
{
	debugLogA("CVkProto::SvcReportAbuse");
	LONG userID = getDword(hContact, "ID", -1);
	if (!IsOnline() || (userID == -1))
		return 1;

	CMString formatstr = TranslateT("Are you sure to report abuse on %s?"),
		tszNick = db_get_tsa(hContact, m_szModuleName, "Nick"),
		ptszMsg;
	ptszMsg.AppendFormat(formatstr, tszNick.IsEmpty() ? TranslateT("(Unknown contact)") : tszNick);
	if (IDNO == MessageBox(NULL, ptszMsg.GetBuffer(), TranslateT("Attention!"), MB_ICONWARNING | MB_YESNO))
		return 1;

	Push(new AsyncHttpRequest(this, REQUEST_GET, "/method/users.report.json", true, &CVkProto::OnReceiveSmth)
		<< INT_PARAM("user_id", userID)
		<< CHAR_PARAM("type", "spam")
		<< VER_API);

	return 0;
}

INT_PTR __cdecl CVkProto::SvcOpenBroadcast(WPARAM hContact, LPARAM)
{
	debugLogA("CVkProto::SvcOpenBroadcast");
	CMString tszAudio(db_get_tsa(hContact, m_szModuleName, "AudioUrl"));
	
	if (!tszAudio.IsEmpty())
		CallService(MS_UTILS_OPENURL, (WPARAM)OUF_TCHAR, (LPARAM)tszAudio.GetBuffer());
	
	return 0;
}

INT_PTR __cdecl CVkProto::SvcVisitProfile(WPARAM hContact, LPARAM)
{
	debugLogA("CVkProto::SvcVisitProfile");
	LONG userID = getDword(hContact, "ID", -1);
	ptrT tszDomain(db_get_tsa(hContact, m_szModuleName, "domain"));
	CMString tszUrl("https://vk.com/");

	if (tszDomain)
		tszUrl.Append(tszDomain);
	else
		tszUrl.AppendFormat(_T("id%i"), userID);
	
	CallService(MS_UTILS_OPENURL, (WPARAM)OUF_TCHAR, (LPARAM)tszUrl.GetBuffer());
	
	return 0;
}

INT_PTR __cdecl CVkProto::SvcGetAllServerHistory(WPARAM hContact, LPARAM)
{
	debugLogA("CVkProto::SvcGetAllServerHistory");
	if (!IsOnline())
		return 0;
	LPCTSTR str = TranslateT("Are you sure to reload all messages from vk.com?\nLocal contact history will be deleted and reloaded from the server.\nIt may take a long time.\nDo you want to continue?");
	if (IDNO == MessageBox(NULL, str, TranslateT("Attention!"), MB_ICONWARNING | MB_YESNO))
		return 0;

	LONG userID = getDword(hContact, "ID", -1);
	if (userID == -1)
		return 0;

	setByte(hContact, "ImportHistory", 1);
	setDword(hContact, "lastmsgid", 0);

	HANDLE hDBEvent = db_event_first(hContact);
	while (hDBEvent) {
		HANDLE hDBEventNext = db_event_next(hContact, hDBEvent);
		db_event_delete(hContact, hDBEvent);
		hDBEvent = hDBEventNext;
	}

	debugLogA("CVkProto::SvcGetAllServerHistory");
	GetHistoryDlgMessages(hContact, 0, INT_MAX, -1);
	return 1;
}

/////////////////////////////////////////////////////////////////////////////////////////

void CVkProto::PollUpdates(JSONNODE *pUpdates)
{
	debugLogA("CVkProto::PollUpdates");
	CMStringA mids;
	int msgid, uid, flags, platform;
	MCONTACT hContact;

	JSONNODE *pChild;
	for (int i = 0; (pChild = json_at(pUpdates, i)) != NULL; i++) {
		switch (json_as_int(json_at(pChild, 0))) {
		case VKPOLL_MSG_DELFLAGS:
			flags = json_as_int(json_at(pChild, 2));
			uid = json_as_int(json_at(pChild, 3));
			hContact = FindUser(uid);
			if (hContact != NULL) {
				if (flags&VKFLAG_MSGUNREAD){
					setDword(hContact, "LastMsgReadTime", time(NULL));
					SetSrmmReadStatus(hContact);
				}
				if (m_bUserForceOnlineOnActivity)
					setWord(hContact, "Status", ID_STATUS_ONLINE);
			}
			break;

		case VKPOLL_MSG_ADDED: // new message
			msgid = json_as_int(json_at(pChild, 1));

			// skip outgoing messages sent from a client
			flags = json_as_int(json_at(pChild, 2));
			if ((flags & VKFLAG_MSGOUTBOX) && !(flags & VKFLAG_MSGCHAT))
				if (CheckMid(msgid))
					break;

			if (!mids.IsEmpty())
				mids.AppendChar(',');
			mids.AppendFormat("%d", msgid);
			break;

		case VKPOLL_READ_ALL_OUT:
			uid = json_as_int(json_at(pChild, 1));
			hContact = FindUser(uid);
			if (hContact != NULL){
				setDword(hContact, "LastMsgReadTime", time(NULL));
				SetSrmmReadStatus(hContact);
				if (m_bUserForceOnlineOnActivity)
					setWord(hContact, "Status", ID_STATUS_ONLINE);
			}
			break;

		case VKPOLL_USR_ONLINE:
			uid = -json_as_int(json_at(pChild, 1));
			if ((hContact = FindUser(uid)) != NULL)
				setWord(hContact, "Status", ID_STATUS_ONLINE);
			platform = json_as_int(json_at(pChild, 2));
			SetMirVer(hContact, platform);
			break;

		case VKPOLL_USR_OFFLINE:
			uid = -json_as_int(json_at(pChild, 1));
			if ((hContact = FindUser(uid)) != NULL)
				setWord(hContact, "Status", ID_STATUS_OFFLINE);
			break;

		case VKPOLL_USR_UTN:
			uid = json_as_int(json_at(pChild, 1));
			hContact = FindUser(uid);
			if (hContact != NULL){
				ForkThread(&CVkProto::ContactTypingThread, (void *)hContact);
				if (m_bUserForceOnlineOnActivity)
					setWord(hContact, "Status", ID_STATUS_ONLINE);
			}
			break;

		case VKPOLL_CHAT_CHANGED:
			int chat_id = json_as_int(json_at(pChild, 1));
			CVkChatInfo *cc = m_chats.find((CVkChatInfo*)&chat_id);
			if (cc)
				RetrieveChatInfo(cc);
			break;
		}
	}

	RetrieveMessagesByIds(mids);
}

int CVkProto::PollServer()
{
	debugLogA("CVkProto::PollServer");
	if (!IsOnline())
		return 0;

	NETLIBHTTPREQUEST req = { sizeof(req) };
	req.requestType = REQUEST_GET;
	req.szUrl = NEWSTR_ALLOCA(CMStringA().Format("http://%s?act=a_check&key=%s&ts=%s&wait=25&access_token=%s&mode=%d", m_pollingServer, m_pollingKey, m_pollingTs, m_szAccessToken, 106));
	// see mode parametr description on https://vk.com/dev/using_longpoll (Russian version)
	req.flags = VK_NODUMPHEADERS | NLHRF_PERSISTENT;
	req.timeout = 30000;
	req.nlc = m_pollingConn;

	NETLIBHTTPREQUEST *reply = (NETLIBHTTPREQUEST*)CallService(MS_NETLIB_HTTPTRANSACTION, (WPARAM)m_hNetlibUser, (LPARAM)&req);
	if (reply == NULL) {
		debugLogA("CVkProto::PollServer is dead");	
		m_pollingConn = NULL;
		CallService(MS_NETLIB_FREEHTTPREQUESTSTRUCT, 0, (LPARAM)reply);
		ShutdownSession();
		return 0;
	}

	int retVal = 0;
	if (reply->resultCode == 200) {
		JSONROOT pRoot(reply->pData);
		JSONNODE *pFailed = json_get(pRoot, "failed");
		if (pFailed != NULL && json_as_int(pFailed) == 2) {
			RetrievePollingInfo();
			retVal = -1;
			debugLogA("Polling key expired, restarting polling thread");
		}
		else if (CheckJsonResult(NULL, reply, pRoot)) {
			m_pollingTs = mir_t2a(ptrT(json_as_string(json_get(pRoot, "ts"))));
			JSONNODE *pUpdates = json_get(pRoot, "updates");
			if (pUpdates != NULL)
				PollUpdates(pUpdates);
			retVal = 1;
		}
	} 
	else if (((reply->resultCode >= 400) && (reply->resultCode <= 417))
		|| ((reply->resultCode >= 500) && (reply->resultCode <= 509))) {
		debugLogA("CVkProto::PollServer is dead. Error code - %d", reply->resultCode);
		m_pollingConn = NULL;
		CallService(MS_NETLIB_FREEHTTPREQUESTSTRUCT, 0, (LPARAM)reply);
		ShutdownSession();
		return 0;
	}

	m_pollingConn = reply->nlc;

	CallService(MS_NETLIB_FREEHTTPREQUESTSTRUCT, 0, (LPARAM)reply);
	return retVal;
}

void CVkProto::PollingThread(void*)
{
	debugLogA("CVkProto::PollingThread: entering");

	while (!m_bTerminated)
		if (PollServer() == -1)
			break;

	m_hPollingThread = NULL;
	m_pollingConn = NULL;
	debugLogA("CVkProto::PollingThread: leaving");
}

CMString CVkProto::GetAttachmentDescr(JSONNODE *pAttachments)
{
	debugLogA("CVkProto::GetAttachmentDescr");
	CMString res;
	res.AppendChar('\n');
	res += TranslateT("Attachments:");
	res.AppendChar('\n');
	JSONNODE *pAttach;
	for (int k = 0; (pAttach = json_at(pAttachments, k)) != NULL; k++) {
		res.AppendChar('\t');
		ptrT ptszType(json_as_string(json_get(pAttach, "type")));
		if (!lstrcmp(ptszType, _T("photo"))) {
			JSONNODE *pPhoto = json_get(pAttach, "photo");
			if (pPhoto == NULL) continue;

			ptrT ptszLink;
			for (int i = 0; i < SIZEOF(szImageTypes); i++) {
				JSONNODE *n = json_get(pPhoto, szImageTypes[i]);
				if (n != NULL) {
					ptszLink = json_as_string(n);
					break;
				}
			}

			int iWidth = json_as_int(json_get(pPhoto, "width"));
			int iHeight = json_as_int(json_get(pPhoto, "height"));
			res.AppendFormat(_T("%s: %s (%dx%d)"), TranslateT("Photo"), ptszLink, iWidth, iHeight);
			if (m_bAddImgBbc)
				res.AppendFormat(L"\n\t[img]%s[/img]", ptszLink);
		}
		else if (!lstrcmp(ptszType, _T("audio"))) {
			JSONNODE *pAudio = json_get(pAttach, "audio");
			if (pAudio == NULL) continue;
	
			ptrT ptszArtist(json_as_string(json_get(pAudio, "artist")));
			ptrT ptszTitle(json_as_string(json_get(pAudio, "title")));
			ptrT ptszUrl(json_as_string(json_get(pAudio, "url")));
			res.AppendFormat(_T("%s: (%s - %s) - %s"),
				TranslateT("Audio"), ptszArtist, ptszTitle, ptszUrl);
		}
		else if (!lstrcmp(ptszType, _T("video"))) {
			JSONNODE *pVideo = json_get(pAttach, "video");
			if (pVideo == NULL) continue;

			ptrT ptszTitle(json_as_string(json_get(pVideo, "title")));
			int  vid = json_as_int(json_get(pVideo, "id"));
			int  ownerID = json_as_int(json_get(pVideo, "owner_id"));
			res.AppendFormat(_T("%s: %s - http://vk.com/video%d_%d"),
				TranslateT("Video"), ptszTitle, ownerID, vid);
		}
		else if (!lstrcmp(ptszType, _T("doc"))) {
			JSONNODE *pDoc = json_get(pAttach, "doc");
			if (pDoc == NULL) continue;

			ptrT ptszTitle(json_as_string(json_get(pDoc, "title")));
			ptrT ptszUrl(json_as_string(json_get(pDoc, "url")));
			res.AppendFormat(_T("%s: (%s) - %s"),
				TranslateT("Document"), ptszTitle, ptszUrl);
		}
		else if (!lstrcmp(ptszType, _T("wall"))) {
			JSONNODE *pWall = json_get(pAttach, "wall");
			if (pWall == NULL) continue;
			
			ptrT ptszText(json_as_string(json_get(pWall, "text")));
			int  id = json_as_int(json_get(pWall, "id"));
			int  fromID = json_as_int(json_get(pWall, "from_id"));
			res.AppendFormat(_T("%s: %s - http://vk.com/wall%d_%d"), TranslateT("Wall post"), ptszText ? ptszText : L" ", fromID, id);
		}
		else if (!lstrcmp(ptszType, _T("sticker"))) {
			JSONNODE *pSticker = json_get(pAttach, "sticker");
			if (pSticker == NULL) continue;
			res.Empty(); // sticker is not really an attachment, so we don't want all that heading info
			
			if (m_bStikersAsSmyles){
				int  id = json_as_int(json_get(pSticker, "id"));
				res.AppendFormat(L"[sticker:%d]", id);
			}
			else {
				ptrT ptszLink;
				for (int i = 0; i < SIZEOF(szImageTypes); i++) {
					JSONNODE *n = json_get(pSticker, szImageTypes[i]);
					if (n != NULL) {
						ptszLink = json_as_string(n);
						break;
					}
				}
				res.AppendFormat(L"%s", ptszLink);

				if (m_bAddImgBbc)
					res.AppendFormat(L"[img]%s[/img]", ptszLink);
			}
		}
		else res.AppendFormat(TranslateT("Unsupported or unknown attachment type: %s"), ptszType);

		res.AppendChar('\n');
	}

	return res;
}
