/*
Copyright (c) 2013-16 Miranda NG project (http://miranda-ng.org)

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

static const char *szImageTypes[] = { "photo_2560", "photo_1280", "photo_807", "photo_604", "photo_256", "photo_130", "photo_128", "photo_75", "photo_64" , "preview"};

static const char  *szGiftTypes[] = { "thumb_256", "thumb_96", "thumb_48" };

static const char *szVKUrls[] = { "http://vk.com/", "https://vk.com/", "http://new.vk.com/", "https://new.vk.com/", "http://m.vk.com/", "https://m.vk.com/" };
static const char *szAttachmentMasks[] = { "wall%d_%d",  "video%d_%d",  "photo%d_%d", "audio%d_%d", "doc%d_%d", "market%d_%d" };
static const char *szVKLinkParam[] = { "?z=", "?w=", "&z=", "&w=" };

JSONNode nullNode(JSON_NULL);

bool IsEmpty(LPCWSTR str)
{
	return (str == NULL || str[0] == 0);
}

bool IsEmpty(LPCSTR str)
{
	return (str == NULL || str[0] == 0);
}

LPCSTR findHeader(NETLIBHTTPREQUEST *pReq, LPCSTR szField)
{
	for (int i = 0; i < pReq->headersCount; i++)
		if (!_stricmp(pReq->headers[i].szName, szField))
			return pReq->headers[i].szValue;

	return NULL;
}

bool wlstrstr(wchar_t *_s1, wchar_t *_s2)
{
	wchar_t s1[1024], s2[1024];

	wcsncpy_s(s1, _s1, _TRUNCATE);
	CharLowerBuff(s1, _countof(s1));
	wcsncpy_s(s2, _s2, _TRUNCATE);
	CharLowerBuff(s2, _countof(s2));

	return wcsstr(s1, s2) != NULL;
}

/////////////////////////////////////////////////////////////////////////////////////////

static IconItem iconList[] =
{
	{ LPGEN("Captcha form icon"), "key", IDI_KEYS },
	{ LPGEN("Notification icon"), "notification", IDI_NOTIFICATION },
	{ LPGEN("Read message icon"), "read", IDI_READMSG },
	{ LPGEN("Visit profile icon"), "profile", IDI_VISITPROFILE },
	{ LPGEN("Load server history icon"), "history", IDI_HISTORY },
	{ LPGEN("Add to friend list icon"), "addfriend", IDI_FRIENDADD },
	{ LPGEN("Delete from friend list icon"), "delfriend", IDI_FRIENDDEL },
	{ LPGEN("Report abuse icon"), "abuse", IDI_ABUSE },
	{ LPGEN("Ban user icon"), "ban", IDI_BAN },
	{ LPGEN("Broadcast icon"), "broadcast", IDI_BROADCAST },
	{ LPGEN("Status icon"), "status", IDI_STATUS },
	{ LPGEN("Wall message icon"), "wall", IDI_WALL },
	{ LPGEN("Mark messages as read icon"), "markread", IDI_MARKMESSAGESASREAD }
};

void InitIcons()
{
	Icon_Register(hInst, LPGEN("Protocols") "/" LPGEN("VKontakte"), iconList, _countof(iconList), "VKontakte");
}

HANDLE GetIconHandle(int iCommand)
{
	for (int i = 0; i < _countof(iconList); i++)
		if (iconList[i].defIconID == iCommand)
			return iconList[i].hIcolib;

	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////

char* ExpUrlEncode(const char *szUrl, bool strict)
{
	const char szHexDigits[] = "0123456789ABCDEF";

	if (szUrl == NULL)
		return NULL;

	const BYTE *s;
	int outputLen;
	for (outputLen = 0, s = (const BYTE*)szUrl; *s; s++) 
		if ((*s & 0x80 && !strict) || // UTF-8 multibyte
			('0' <= *s && *s <= '9') || //0-9
			('A' <= *s && *s <= 'Z') || //ABC...XYZ
			('a' <= *s && *s <= 'z') || //abc...xyz
			*s == '~' || *s == '-' || *s == '_' 	|| *s == '.' || *s == ' ') 
			outputLen++;
		else 
			outputLen += 3;

	char *szOutput = (char*)mir_alloc(outputLen + 1);
	if (szOutput == NULL)
		return NULL;

	char *d = szOutput;
	for (s = (const BYTE*)szUrl; *s; s++)
		if ((*s & 0x80 && !strict) || // UTF-8 multibyte
			('0' <= *s && *s <= '9') || //0-9
			('A' <= *s && *s <= 'Z') || //ABC...XYZ
			('a' <= *s && *s <= 'z') || //abc...xyz
			*s == '~' || *s == '-' || *s == '_' || *s == '.') 
			*d++ = *s;
		else if (*s == ' ') 
			*d++ = '+';
		else {
			*d++ = '%';
			*d++ = szHexDigits[*s >> 4];
			*d++ = szHexDigits[*s & 0xF];
		}

	*d = '\0';
	return szOutput;
}


/////////////////////////////////////////////////////////////////////////////////////////

void CVkProto::ClearAccessToken()
{
	debugLogA("CVkProto::ClearAccessToken");
	setDword("LastAccessTokenTime", (DWORD)time(NULL));
	m_szAccessToken = NULL;
	delSetting("AccessToken");
	ShutdownSession();
}

wchar_t* CVkProto::GetUserStoredPassword()
{
	debugLogA("CVkProto::GetUserStoredPassword");
	ptrA szRawPass(getStringA("Password"));
	return (szRawPass != NULL) ? mir_utf8decodeW(szRawPass) : NULL;
}

void CVkProto::SetAllContactStatuses(int iStatus)
{
	debugLogA("CVkProto::SetAllContactStatuses (%d)", iStatus);
	for (MCONTACT hContact = db_find_first(m_szModuleName); hContact; hContact = db_find_next(hContact, m_szModuleName)) {
		if (isChatRoom(hContact))
			SetChatStatus(hContact, iStatus);
		else if (getWord(hContact, "Status") != iStatus)
			setWord(hContact, "Status", iStatus);

		if (iStatus == ID_STATUS_OFFLINE) {
			SetMirVer(hContact, -1);
			db_unset(hContact, m_szModuleName, "ListeningTo");
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////

MCONTACT CVkProto::FindUser(LONG dwUserid, bool bCreate)
{
	if (!dwUserid)
		return NULL;

	for (MCONTACT hContact = db_find_first(m_szModuleName); hContact; hContact = db_find_next(hContact, m_szModuleName)) {
		LONG dbUserid = getDword(hContact, "ID", VK_INVALID_USER);
		if (dbUserid == VK_INVALID_USER)
			continue;

		if (dbUserid == dwUserid)
			return hContact;
	}

	if (!bCreate)
		return NULL;

	MCONTACT hNewContact = (MCONTACT)CallService(MS_DB_CONTACT_ADD);
	Proto_AddToContact(hNewContact, m_szModuleName);
	setDword(hNewContact, "ID", dwUserid);
	db_set_ws(hNewContact, "CList", "Group", m_vkOptions.pwszDefaultGroup);
	if (dwUserid < 0)
		setByte(hNewContact, "IsGroup", 1);
	return hNewContact;
}

MCONTACT CVkProto::FindChat(LONG dwUserid)
{
	if (!dwUserid)
		return NULL;

	for (MCONTACT hContact = db_find_first(m_szModuleName); hContact; hContact = db_find_next(hContact, m_szModuleName)) {
		LONG dbUserid = getDword(hContact, "vk_chat_id", VK_INVALID_USER);
		if (dbUserid == VK_INVALID_USER)
			continue;

		if (dbUserid == dwUserid)
			return hContact;
	}

	return NULL;
}

bool CVkProto::IsGroupUser(MCONTACT hContact) 
{
	if (getBool(hContact, "IsGroup", false))
		return true;

	LONG userID = getDword(hContact, "ID", VK_INVALID_USER);
	return (userID < 0);
}

/////////////////////////////////////////////////////////////////////////////////////////

bool CVkProto::CheckMid(LIST<void> &lList, int guid)
{
	for (int i = lList.getCount() - 1; i >= 0; i--)
		if ((INT_PTR)lList[i] == guid) {
			lList.remove(i);
			return true;
		}

	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////

JSONNode& CVkProto::CheckJsonResponse(AsyncHttpRequest *pReq, NETLIBHTTPREQUEST *reply, JSONNode &root)
{
	debugLogA("CVkProto::CheckJsonResponse");

	if (!reply || !reply->pData)
		return nullNode;

	root = JSONNode::parse(reply->pData);

	if (!CheckJsonResult(pReq, root))
		return nullNode;

	return root["response"];
}

bool CVkProto::CheckJsonResult(AsyncHttpRequest *pReq, const JSONNode &jnNode)
{
	debugLogA("CVkProto::CheckJsonResult");
	if (!jnNode) {
		pReq->m_iErrorCode = VKERR_NO_JSONNODE;
		return false;
	}

	const JSONNode &jnError = jnNode["error"];
	const JSONNode &jnErrorCode = jnError["error_code"];
	const JSONNode &jnRedirectUri = jnError["redirect_uri"];

	if (!jnError || !jnErrorCode)
		return true;

	pReq->m_iErrorCode = jnErrorCode.as_int();
	debugLogA("CVkProto::CheckJsonResult %d", pReq->m_iErrorCode);
	switch (pReq->m_iErrorCode) {
	case VKERR_AUTHORIZATION_FAILED:
		ConnectionFailed(LOGINERR_WRONGPASSWORD);
		break;
	case VKERR_ACCESS_DENIED:
		if (time(NULL) - getDword("LastAccessTokenTime", 0) > 60 * 60 * 24) {
			debugLogA("CVkProto::CheckJsonResult VKERR_ACCESS_DENIED (AccessToken fail?)");
			ClearAccessToken();
			return false;
		}
		debugLogA("CVkProto::CheckJsonResult VKERR_ACCESS_DENIED");
		MsgPopup(NULL, TranslateT("Access denied! Data will not be sent or received."), TranslateT("Error"), true);
		break;
	case VKERR_CAPTCHA_NEEDED:
		ApplyCaptcha(pReq, jnError);
		break;
	case VKERR_VALIDATION_REQUIRED:	// Validation Required
		MsgPopup(NULL, TranslateT("You must validate your account before use VK in Miranda NG"), TranslateT("Error"), true);
		if (jnRedirectUri) {
			T2Utf szRedirectUri(jnRedirectUri.as_mstring());
			AsyncHttpRequest *pRedirectReq = new AsyncHttpRequest(this, REQUEST_GET, szRedirectUri, false, &CVkProto::OnOAuthAuthorize);
			pRedirectReq->m_bApiReq = false;
			pRedirectReq->bIsMainConn = true;
			Push(pRedirectReq);
		}
		break;
	case VKERR_FLOOD_CONTROL:
		pReq->m_iRetry = 0;
		// fall through
	case VKERR_UNKNOWN:
	case VKERR_TOO_MANY_REQ_PER_SEC:
	case VKERR_INTERNAL_SERVER_ERR:
		if (pReq->m_iRetry > 0) {
			pReq->bNeedsRestart = true;
			Sleep(500); //Pause for fix err 
			debugLogA("CVkProto::CheckJsonResult Retry = %d", pReq->m_iRetry);
			pReq->m_iRetry--;
		}
		else {
			CMStringW msg(FORMAT, TranslateT("Error %d. Data will not be sent or received."), pReq->m_iErrorCode);
			MsgPopup(NULL, msg, TranslateT("Error"), true);
			debugLogA("CVkProto::CheckJsonResult SendError");
		}
		break;

	case VKERR_INVALID_PARAMETERS:
		MsgPopup(NULL, TranslateT("One of the parameters specified was missing or invalid"), TranslateT("Error"), true);
		break;
	case VKERR_ACC_WALL_POST_DENIED:
		MsgPopup(NULL, TranslateT("Access to adding post denied"), TranslateT("Error"), true);
		break;
	case VKERR_COULD_NOT_SAVE_FILE:
	case VKERR_INVALID_ALBUM_ID:
	case VKERR_INVALID_SERVER:
	case VKERR_INVALID_HASH:
	case VKERR_INVALID_AUDIO:
	case VKERR_AUDIO_DEL_COPYRIGHT:
	case VKERR_INVALID_FILENAME:
	case VKERR_INVALID_FILESIZE:
	case VKERR_HIMSELF_AS_FRIEND:
	case VKERR_YOU_ON_BLACKLIST:
	case VKERR_USER_ON_BLACKLIST:
		// See CVkProto::SendFileFiled 
		break;
	}

	return pReq->m_iErrorCode == 0;
}

void CVkProto::OnReceiveSmth(NETLIBHTTPREQUEST *reply, AsyncHttpRequest *pReq)
{
	JSONNode jnRoot;
	const JSONNode &jnResponse = CheckJsonResponse(pReq, reply, jnRoot);
	debugLogA("CVkProto::OnReceiveSmth %d", jnResponse.as_int());
}

/////////////////////////////////////////////////////////////////////////////////////////
// Quick & dirty form parser

static CMStringA getAttr(char *szSrc, LPCSTR szAttrName)
{
	char *pEnd = strchr(szSrc, '>');
	if (pEnd == NULL)
		return "";

	*pEnd = 0;

	char *p1 = strstr(szSrc, szAttrName);
	if (p1 == NULL) {
		*pEnd = '>';
		return "";
	}

	p1 += mir_strlen(szAttrName);
	if (p1[0] != '=' || p1[1] != '\"') {
		*pEnd = '>';
		return "";
	}

	p1 += 2;
	char *p2 = strchr(p1, '\"');
	*pEnd = '>';
	if (p2 == NULL) 
		return "";

	return CMStringA(p1, (int)(p2-p1));
}

bool CVkProto::AutoFillForm(char *pBody, CMStringA &szAction, CMStringA& szResult)
{
	debugLogA("CVkProto::AutoFillForm");
	szResult.Empty();

	char *pFormBeg = strstr(pBody, "<form ");
	if (pFormBeg == NULL) 
		return false;

	char *pFormEnd = strstr(pFormBeg, "</form>");
	if (pFormEnd == NULL) 
		return false;

	*pFormEnd = 0;

	szAction = getAttr(pFormBeg, "action");

	CMStringA result;
	char *pFieldBeg = pFormBeg;
	while (true) {
		if ((pFieldBeg = strstr(pFieldBeg+1, "<input ")) == NULL)
			break;

		CMStringA type = getAttr(pFieldBeg, "type");
		if (type != "submit") {
			CMStringA name = getAttr(pFieldBeg, "name");
			CMStringA value = getAttr(pFieldBeg, "value");
			if (name == "email")
				value = (char*)T2Utf(ptrW(getWStringA("Login")));
			else if (name == "pass")
				value = (char*)T2Utf(ptrW(GetUserStoredPassword()));
			else if (name == "captcha_key") {
				char *pCaptchaBeg = strstr(pFormBeg, "<img id=\"captcha\"");
				if (pCaptchaBeg != NULL)
					if (!RunCaptchaForm(getAttr(pCaptchaBeg, "src"), value))
						return false;
			}
			else if (name == "code") 
				value = RunConfirmationCode();

			if (!result.IsEmpty())
				result.AppendChar('&');
			result += name + "=";
			result += ptrA(mir_urlEncode(value));
		}
	}

	szResult = result;
	debugLogA("CVkProto::AutoFillForm result = \"%s\"", szResult);
	return true;
}

CMStringW CVkProto::RunConfirmationCode()
{
	ENTER_STRING pForm = { sizeof(pForm) };
	pForm.type = ESF_PASSWORD;
	pForm.caption = TranslateT("Enter confirmation code");
	pForm.ptszInitVal = NULL;
	pForm.szModuleName = m_szModuleName;
	pForm.szDataPrefix = "confirmcode_";
	return (!EnterString(&pForm)) ? CMStringW() : CMStringW(ptrW(pForm.ptszResult));
}

CMStringW CVkProto::RunRenameNick(LPCWSTR pwszOldName)
{
	ENTER_STRING pForm = { sizeof(pForm) };
	pForm.type = ESF_COMBO;
	pForm.recentCount = 0;
	pForm.caption = TranslateT("Enter new nickname");
	pForm.ptszInitVal = pwszOldName;
	pForm.szModuleName = m_szModuleName;
	pForm.szDataPrefix = "renamenick_";
	return (!EnterString(&pForm)) ? CMStringW() : CMStringW(ptrW(pForm.ptszResult));
}

/////////////////////////////////////////////////////////////////////////////////////////

void CVkProto::GrabCookies(NETLIBHTTPREQUEST *nhr)
{
	debugLogA("CVkProto::GrabCookies");
	for (int i = 0; i < nhr->headersCount; i++) {
		if (_stricmp(nhr->headers[i].szName, "Set-cookie"))
			continue;

		CMStringA szValue = nhr->headers[i].szValue, szCookieName, szCookieVal, szDomain;
		int iStart = 0;
		while (true) {
			bool bFirstToken = (iStart == 0);
			CMStringA szToken = szValue.Tokenize(";", iStart).Trim();
			if (iStart == -1)
				break;

			if (bFirstToken) {
				int iStart2 = 0;
				szCookieName = szToken.Tokenize("=", iStart2);
				szCookieVal = szToken.Tokenize("=", iStart2);
			}
			else if (!strncmp(szToken, "domain=", 7))
				szDomain = szToken.Mid(7);
		}

		if (!szCookieName.IsEmpty() && !szDomain.IsEmpty()) {
			int k;
			for (k=0; k < m_cookies.getCount(); k++) {
				if (m_cookies[k].m_name == szCookieName) {
					m_cookies[k].m_value = szCookieVal;
					break;
				}
			}
			if (k == m_cookies.getCount())
				m_cookies.insert(new CVkCookie(szCookieName, szCookieVal, szDomain));
		}
	}
}

void CVkProto::ApplyCookies(AsyncHttpRequest *pReq)
{
	debugLogA("CVkProto::ApplyCookies");
	CMStringA szCookie;

	for (int i=0; i < m_cookies.getCount(); i++) {
		if (!strstr(pReq->m_szUrl, m_cookies[i].m_domain))
			continue;

		if (!szCookie.IsEmpty())
			szCookie.Append("; ");
		szCookie.Append(m_cookies[i].m_name);
		szCookie.AppendChar('=');
		szCookie.Append(m_cookies[i].m_value);
	}

	if (!szCookie.IsEmpty())
		pReq->AddHeader("Cookie", szCookie);
}

/////////////////////////////////////////////////////////////////////////////////////////

void __cdecl CVkProto::DBAddAuthRequestThread(void *p)
{
	CVkDBAddAuthRequestThreadParam *param = (CVkDBAddAuthRequestThreadParam *)p;
	if (param->hContact == NULL || param->hContact == INVALID_CONTACT_ID || !IsOnline())
		return;

	for (int i = 0; i < MAX_RETRIES && IsEmpty(ptrW(db_get_wsa(param->hContact, m_szModuleName, "Nick"))); i++) {
		Sleep(1500);
		
		if (!IsOnline())
			return;
	}

	DBAddAuthRequest(param->hContact, param->bAdded);
	delete param;
}

void CVkProto::DBAddAuthRequest(const MCONTACT hContact, bool added)
{
	debugLogA("CVkProto::DBAddAuthRequest");

	T2Utf szNick(ptrW(db_get_wsa(hContact, m_szModuleName, "Nick")));
	T2Utf szFirstName(ptrW(db_get_wsa(hContact, m_szModuleName, "FirstName")));
	T2Utf szLastName(ptrW(db_get_wsa(hContact, m_szModuleName, "LastName")));

	//blob is: uin(DWORD), hContact(DWORD), nick(ASCIIZ), first(ASCIIZ), last(ASCIIZ), email(ASCIIZ), reason(ASCIIZ)
	//blob is: 0(DWORD), hContact(DWORD), nick(ASCIIZ), first(ASCIIZ), last(ASCIIZ), ""(ASCIIZ), ""(ASCIIZ)
	DBEVENTINFO dbei = { sizeof(DBEVENTINFO) };
	dbei.szModule = m_szModuleName;
	dbei.timestamp = (DWORD)time(NULL);
	dbei.flags = DBEF_UTF;
	dbei.eventType = added ? EVENTTYPE_ADDED : EVENTTYPE_AUTHREQUEST;
	dbei.cbBlob = (DWORD)(sizeof(DWORD) * 2 + mir_strlen(szNick) + mir_strlen(szFirstName) + mir_strlen(szLastName) + 5);

	PBYTE pCurBlob = dbei.pBlob = (PBYTE)mir_alloc(dbei.cbBlob);

	*((PDWORD)pCurBlob) = 0; 
	pCurBlob += sizeof(DWORD); // uin(DWORD) = 0 (DWORD)
	
	*((PDWORD)pCurBlob) = (DWORD)hContact;
	pCurBlob += sizeof(DWORD); // hContact(DWORD)

	mir_strcpy((char*)pCurBlob, szNick); 
	pCurBlob += mir_strlen(szNick) + 1;

	mir_strcpy((char*)pCurBlob, szFirstName);
	pCurBlob += mir_strlen(szFirstName) + 1;

	mir_strcpy((char*)pCurBlob, szLastName);
	pCurBlob += mir_strlen(szLastName) + 1;
	
	*pCurBlob = '\0';	//email
	pCurBlob++;
	*pCurBlob = '\0';	//reason

	db_event_add(NULL, &dbei);
	debugLogA("CVkProto::DBAddAuthRequest %s", szNick ? szNick : "<null>");
}

MCONTACT CVkProto::MContactFromDbEvent(MEVENT hDbEvent)
{
	debugLogA("CVkProto::MContactFromDbEvent");
	if (!hDbEvent || !IsOnline())
		return INVALID_CONTACT_ID;

	DWORD body[2];
	DBEVENTINFO dbei = { sizeof(dbei) };
	dbei.cbBlob = sizeof(DWORD) * 2;
	dbei.pBlob = (PBYTE)&body;

	if (db_event_get(hDbEvent, &dbei))
		return INVALID_CONTACT_ID;
	if (dbei.eventType != EVENTTYPE_AUTHREQUEST || mir_strcmp(dbei.szModule, m_szModuleName))
		return INVALID_CONTACT_ID;

	MCONTACT hContact = DbGetAuthEventContact(&dbei);
	db_unset(hContact, m_szModuleName, "ReqAuth");
	return hContact;
}

/////////////////////////////////////////////////////////////////////////////////////////

void CVkProto::SetMirVer(MCONTACT hContact, int platform)
{
	if (hContact == NULL || hContact == INVALID_CONTACT_ID)
		return;
	if (platform == -1) {
		db_unset(hContact, m_szModuleName, "MirVer");
		return;
	}

	CMStringW MirVer, OldMirVer(ptrW(db_get_wsa(hContact, m_szModuleName, "MirVer")));
	bool bSetFlag = true;

	switch (platform) {
	case VK_APP_ID:
		MirVer = L"Miranda NG VKontakte";
		break;
	case 2386311:
		MirVer = L"QIP 2012 VKontakte";
		break;
	case 1:
		MirVer = L"VKontakte (Mobile)";
		break;
	case 3087106: // iPhone
	case 3140623:
	case 2:
		MirVer = L"VKontakte (iPhone)";
		break;
	case 3682744: // iPad
	case 3:
		MirVer = L"VKontakte (iPad)";
		break;
	case 2685278: // Android - Kate
		MirVer = L"Kate Mobile (Android)";
		break;
	case 2890984: // Android
	case 2274003:
	case 4:
		MirVer = L"VKontakte (Android)";
		break;
	case 3059453: // Windows Phone
	case 2424737:
	case 3502561:
	case 5:
		MirVer = L"VKontakte (WPhone)";
		break;
	case 3584591: // Windows 8.x
	case 6:
		MirVer = L"VKontakte (Windows)";
		break; 
	case 7:
		MirVer = L"VKontakte (Website)";
		break;
	default:
		MirVer = L"VKontakte (Other)";
		bSetFlag = OldMirVer.IsEmpty();
	}

	if (OldMirVer == MirVer)
		return;

	if (bSetFlag)
		setWString(hContact, "MirVer", MirVer);
}

/////////////////////////////////////////////////////////////////////////////////////////

void CVkProto::ContactTypingThread(void *p)
{
	debugLogA("CVkProto::ContactTypingThread");
	MCONTACT hContact = (UINT_PTR)p;
	CallService(MS_PROTO_CONTACTISTYPING, hContact, 5);
	Sleep(4500);
	CallService(MS_PROTO_CONTACTISTYPING, hContact);

	if (!ServiceExists(MS_MESSAGESTATE_UPDATE)) {
		Sleep(1500);
		SetSrmmReadStatus(hContact);
	}
}

int CVkProto::OnProcessSrmmEvent(WPARAM, LPARAM lParam)
{
	debugLogA("CVkProto::OnProcessSrmmEvent");
	MessageWindowEventData *event = (MessageWindowEventData *)lParam;

	if (event->uType == MSG_WINDOW_EVT_OPENING && !ServiceExists(MS_MESSAGESTATE_UPDATE))
		SetSrmmReadStatus(event->hContact);

	return 0;
}

void CVkProto::SetSrmmReadStatus(MCONTACT hContact)
{
	time_t time = getDword(hContact, "LastMsgReadTime");
	if (!time)
		return;

	wchar_t ttime[64];
	_locale_t locale = _create_locale(LC_ALL, "");
	_wcsftime_l(ttime, _countof(ttime), L"%X - %x", localtime(&time), locale);
	_free_locale(locale);

	StatusTextData st = { 0 };
	st.cbSize = sizeof(st);
	st.hIcon = IcoLib_GetIconByHandle(GetIconHandle(IDI_READMSG));
	mir_snwprintf(st.tszText, TranslateT("Message read: %s"), ttime);
	CallService(MS_MSG_SETSTATUSTEXT, (WPARAM)hContact, (LPARAM)&st);
}

void CVkProto::MarkDialogAsRead(MCONTACT hContact) 
{
	debugLogA("CVkProto::MarkDialogAsRead");
	if (!IsOnline())
		return;

	LONG userID = getDword(hContact, "ID", VK_INVALID_USER);
	if (userID == VK_INVALID_USER || userID == VK_FEED_USER)
		return;

	MEVENT hDBEvent = NULL;
	MCONTACT hMContact = db_mc_tryMeta(hContact);
	while ((hDBEvent = db_event_firstUnread(hContact)) != NULL) 
	{
		DBEVENTINFO dbei = { sizeof(dbei) };
		if (!db_event_get(hDBEvent, &dbei) && !mir_strcmp(m_szModuleName, dbei.szModule))
		{
			db_event_markRead(hContact, hDBEvent);
			pcli->pfnRemoveEvent(hMContact, hDBEvent);
			if (hContact != hMContact)
				pcli->pfnRemoveEvent(hContact, hDBEvent);
		}
	}
}

char* CVkProto::GetStickerId(const char *Msg, int &stickerid)
{
	stickerid = 0;
	char *retMsg = NULL;

	int iRes = 0;
	char HeadMsg[32] = { 0 };
	const char *tmpMsg = strstr(Msg, "[sticker:");
	if (tmpMsg)
		iRes = sscanf(tmpMsg, "[sticker:%d]", &stickerid);
	if (iRes == 1) {
		mir_snprintf(HeadMsg, "[sticker:%d]", stickerid);
		size_t retLen = mir_strlen(HeadMsg);
		if (retLen < mir_strlen(Msg)) {
			CMStringA szMsg(Msg, int(mir_strlen(Msg) - mir_strlen(tmpMsg)));
			szMsg.Append(&tmpMsg[retLen]);
			retMsg = mir_strdup(szMsg.Trim());
		}	
	}

	return retMsg;
}

const char* FindVKUrls(const char *Msg)
{
	if (IsEmpty(Msg))
		return NULL;

	const char *pos = NULL;
	for (int i = 0; i < _countof(szVKUrls) && !pos; i++) {
		pos = strstr(Msg, szVKUrls[i]);
		if (pos)
			pos += mir_strlen(szVKUrls[i]);
	}

	if (pos >= (Msg + mir_strlen(Msg)))
		pos = NULL;

	return pos;
}


CMStringA CVkProto::GetAttachmentsFromMessage(const char *Msg)
{
	if (IsEmpty(Msg))
		return CMStringA();

	const char *pos = FindVKUrls(Msg);
	if (!pos)
		return CMStringA();

	const char *nextpos = FindVKUrls(pos);
	const char *pos2 = NULL;

	for (int i = 0; i < _countof(szVKLinkParam) && !pos2; i++) {
		pos2 = strstr(pos, szVKLinkParam[i]);
		if (pos2 && (!nextpos || pos2 < nextpos)) 
			pos = pos2 + mir_strlen(szVKLinkParam[i]);
	}

	if (pos >= (Msg + mir_strlen(Msg)))
		return CMStringA();

	int iRes = 0,
		iOwner = 0,
		iId = 0;

	for (int i = 0; i < _countof(szAttachmentMasks); i++) {
		iRes = sscanf(pos, szAttachmentMasks[i], &iOwner, &iId);
		if (iRes == 2) {
			CMStringA szAttachment(FORMAT, szAttachmentMasks[i], iOwner, iId);
			CMStringA szAttachment2;
			
			if (nextpos)
				szAttachment2 = GetAttachmentsFromMessage(pos + szAttachment.GetLength());
			
			if (!szAttachment2.IsEmpty())
				szAttachment += "," + szAttachment2;
			
			return szAttachment;
		}
		else if (iRes == 1)
			break;
	}

	return GetAttachmentsFromMessage(pos);
}

int CVkProto::OnDbSettingChanged(WPARAM hContact, LPARAM lParam)
{
	DBCONTACTWRITESETTING *cws = (DBCONTACTWRITESETTING*)lParam;
	if (hContact != NULL)
		return 0;

	if (strcmp(cws->szModule, "ListeningTo"))
		return 0;

	CMStringA szListeningTo(m_szModuleName);
	szListeningTo += "Enabled";
	if (!strcmp(cws->szSetting, szListeningTo)) {
		MusicSendMetod iOldMusicSendMetod = (MusicSendMetod)getByte("OldMusicSendMetod", sendBroadcastAndStatus);
		
		if (cws->value.bVal == 0)
			setByte("OldMusicSendMetod", m_vkOptions.iMusicSendMetod);
		else
			db_unset(0, m_szModuleName, "OldMusicSendMetod");
		
		m_vkOptions.iMusicSendMetod = cws->value.bVal == 0 ? sendNone : iOldMusicSendMetod;
		setByte("MusicSendMetod", m_vkOptions.iMusicSendMetod);
	}

	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////

CMStringW CVkProto::SpanVKNotificationType(CMStringW& wszType, VKObjType& vkFeedback, VKObjType& vkParent)
{
	CVKNotification vkNotification[] = {
		// type, parent, feedback, string for translate
		{ L"group", vkInvite, vkNull, TranslateT("has invited you to a group") },
		{ L"page", vkInvite, vkNull, TranslateT("has invited you to subscribe to a page") },
		{ L"event", vkInvite, vkNull, TranslateT("invites you to event") },

		{ L"follow", vkNull, vkUsers, L"" },
		{ L"friend_accepted", vkNull, vkUsers, L"" },
		{ L"mention", vkNull, vkPost, L"" },
		{ L"wall", vkNull, vkPost, L"" },
		{ L"wall_publish", vkNull, vkPost, L"" },

		{ L"comment_post", vkPost, vkComment, TranslateT("commented on your post") },
		{ L"comment_photo", vkPhoto, vkComment, TranslateT("commented on your photo") },
		{ L"comment_video", vkVideo, vkComment, TranslateT("commented on your video") },
		{ L"reply_comment", vkComment, vkComment, TranslateT("replied to your comment") },
		{ L"reply_comment_photo", vkComment, vkComment, TranslateT("replied to your comment to photo") },
		{ L"reply_comment_video", vkComment, vkComment, TranslateT("replied to your comment to video") },
		{ L"reply_topic", vkTopic, vkComment, TranslateT("replied to your topic") },
		{ L"like_post", vkPost, vkUsers, TranslateT("liked your post") },
		{ L"like_comment", vkComment, vkUsers, TranslateT("liked your comment") },
		{ L"like_photo", vkPhoto, vkUsers, TranslateT("liked your photo") },
		{ L"like_video", vkVideo, vkUsers, TranslateT("liked your video") },
		{ L"like_comment_photo", vkComment, vkUsers, TranslateT("liked your comment to photo") },
		{ L"like_comment_video", vkComment, vkUsers, TranslateT("liked your comment to video" ) },
		{ L"like_comment_topic", vkComment, vkUsers, TranslateT("liked your comment to topic") },
		{ L"copy_post", vkPost, vkCopy, TranslateT("shared your post") },
		{ L"copy_photo", vkPhoto, vkCopy, TranslateT("shared your photo") },
		{ L"copy_video", vkVideo, vkCopy, TranslateT("shared your video") },
		{ L"mention_comments", vkPost, vkComment, L"mentioned you in comment" },
		{ L"mention_comment_photo", vkPhoto, vkComment, L"mentioned you in comment to photo" }, 
		{ L"mention_comment_video", vkVideo, vkComment, L"mentioned you in comment to video" }
	};

	CMStringW wszRes;
	vkFeedback = vkParent = vkNull;
	for (int i = 0; i < _countof(vkNotification); i++)
		if (wszType == vkNotification[i].pwszType) {
			vkFeedback = vkNotification[i].vkFeedback;
			vkParent = vkNotification[i].vkParent;
			wszRes = vkNotification[i].pwszTranslate;
			break;
		}
	return wszRes;
}

CMStringW CVkProto::GetVkPhotoItem(const JSONNode &jnPhoto, BBCSupport iBBC)
{
	CMStringW wszRes;

	if (!jnPhoto)
		return wszRes;

	CMStringW wszLink, wszPreviewLink;
	for (int i = 0; i < _countof(szImageTypes); i++) {
		const JSONNode &n = jnPhoto[szImageTypes[i]];
		if (n) {
			wszLink = n.as_mstring();
			break;
		}
	}

	switch (m_vkOptions.iIMGBBCSupport) {
	case imgNo:
		wszPreviewLink = L"";
		break;
	case imgFullSize:
		wszPreviewLink = wszLink;
		break;
	case imgPreview130:
	case imgPreview604:
		wszPreviewLink = jnPhoto[m_vkOptions.iIMGBBCSupport == imgPreview130 ? "photo_130" : "photo_604"].as_mstring();
		break;
	}

	int iWidth = jnPhoto["width"].as_int();
	int iHeight = jnPhoto["height"].as_int();

	wszRes.AppendFormat(L"%s (%dx%d)", SetBBCString(TranslateT("Photo"), iBBC, vkbbcUrl, wszLink), iWidth, iHeight);
	if (m_vkOptions.iIMGBBCSupport && iBBC != bbcNo)
		wszRes.AppendFormat(L"\n\t%s", SetBBCString(!wszPreviewLink.IsEmpty() ? wszPreviewLink : (!wszLink.IsEmpty() ? wszLink : L""), bbcBasic, vkbbcImg));
	CMStringW wszText(jnPhoto["text"].as_mstring());
	if (!wszText.IsEmpty())
		wszRes += L"\n" + wszText;

	return wszRes;
}

CMStringW CVkProto::SetBBCString(LPCWSTR pwszString, BBCSupport iBBC, VKBBCType bbcType, LPCWSTR wszAddString)
{
	CVKBBCItem bbcItem[] = {
		{ vkbbcB, bbcNo, L"%s" },
		{ vkbbcB, bbcBasic, L"[b]%s[/b]" },
		{ vkbbcB, bbcAdvanced, L"[b]%s[/b]" },
		{ vkbbcI, bbcNo, L"%s" },
		{ vkbbcI, bbcBasic, L"[i]%s[/i]" },
		{ vkbbcI, bbcAdvanced, L"[i]%s[/i]" },
		{ vkbbcS, bbcNo, L"%s" },
		{ vkbbcS, bbcBasic, L"[s]%s[/s]" },
		{ vkbbcS, bbcAdvanced, L"[s]%s[/s]" },
		{ vkbbcU, bbcNo, L"%s" },
		{ vkbbcU, bbcBasic, L"[u]%s[/u]" },
		{ vkbbcU, bbcAdvanced, L"[u]%s[/u]" },
		{ vkbbcCode, bbcNo, L"%s" },
		{ vkbbcCode, bbcBasic, L"%s" },
		{ vkbbcCode, bbcAdvanced, L"[code]%s[/code]" },
		{ vkbbcImg, bbcNo, L"%s" },
		{ vkbbcImg, bbcBasic, L"[img]%s[/img]" },
		{ vkbbcImg, bbcAdvanced, L"[img]%s[/img]" },
		{ vkbbcUrl, bbcNo, L"%s (%s)" },
		{ vkbbcUrl, bbcBasic, L"[i]%s[/i] (%s)" },
		{ vkbbcUrl, bbcAdvanced, L"[url=%s]%s[/url]" },
		{ vkbbcSize, bbcNo, L"%s" },
		{ vkbbcSize, bbcBasic, L"%s" },
		{ vkbbcSize, bbcAdvanced, L"[size=%s]%s[/size]" },
		{ vkbbcColor, bbcNo, L"%s" },
		{ vkbbcColor, bbcBasic, L"%s" },
		{ vkbbcColor, bbcAdvanced, L"[color=%s]%s[/color]" },
	};

	if (IsEmpty(pwszString))
		return CMStringW();

	wchar_t *pwszFormat = NULL;
	for (int i = 0; i < _countof(bbcItem); i++)
		if (bbcItem[i].vkBBCType == bbcType && bbcItem[i].vkBBCSettings == iBBC) {
			pwszFormat = bbcItem[i].pwszTempate;
			break;
		}

	CMStringW res;
	if (pwszFormat == NULL)
		return CMStringW(pwszString);

	if (bbcType == vkbbcUrl && iBBC != bbcAdvanced)
		res.AppendFormat(pwszFormat, pwszString, wszAddString ? wszAddString : L"");
	else if (iBBC == bbcAdvanced && bbcType >= vkbbcUrl)
		res.AppendFormat(pwszFormat, wszAddString ? wszAddString : L"", pwszString);
	else
		res.AppendFormat(pwszFormat, pwszString);

	return res;
}

CMStringW& CVkProto::ClearFormatNick(CMStringW& wszText)
{
	int iNameEnd = wszText.Find(L"],"), iNameBeg = wszText.Find(L"|");
	if (iNameEnd != -1 && iNameBeg != -1 && iNameBeg < iNameEnd) {
		CMStringW wszName = wszText.Mid(iNameBeg + 1, iNameEnd - iNameBeg - 1);
		CMStringW wszBody = wszText.Mid(iNameEnd + 2);
		if (!wszName.IsEmpty() && !wszBody.IsEmpty())
			wszText = wszName + L"," + wszBody;
	}

	return wszText;
}

/////////////////////////////////////////////////////////////////////////////////////////

CMStringW CVkProto::GetAttachmentDescr(const JSONNode &jnAttachments, BBCSupport iBBC)
{
	debugLogA("CVkProto::GetAttachmentDescr");
	CMStringW res;
	if (!jnAttachments) {
		debugLogA("CVkProto::GetAttachmentDescr pAttachments == NULL");
		return res;
	}

	res += SetBBCString(TranslateT("Attachments:"), iBBC, vkbbcB);
	res.AppendChar('\n');
	
	for (auto it = jnAttachments.begin(); it != jnAttachments.end(); ++it) {
		const JSONNode &jnAttach = (*it);

		res.AppendChar('\t');
		CMStringW wszType(jnAttach["type"].as_mstring());
		if (wszType == L"photo") {
			const JSONNode &jnPhoto = jnAttach["photo"];
			if (!jnPhoto)
				continue;

			res += GetVkPhotoItem(jnPhoto, iBBC);
		}
		else if (wszType ==L"audio") {
			const JSONNode &jnAudio = jnAttach["audio"];
			if (!jnAudio)
				continue;

			CMStringW wszArtist(jnAudio["artist"].as_mstring());
			CMStringW wszTitle(jnAudio["title"].as_mstring());
			CMStringW wszUrl(jnAudio["url"].as_mstring());
			CMStringW wszAudio(FORMAT, L"%s - %s", wszArtist, wszTitle);

			int iParamPos = wszUrl.Find(L"?");
			if (m_vkOptions.bShortenLinksForAudio &&  iParamPos != -1)
				wszUrl = wszUrl.Left(iParamPos);

			res.AppendFormat(L"%s: %s",
				SetBBCString(TranslateT("Audio"), iBBC, vkbbcB),
				SetBBCString(wszAudio, iBBC, vkbbcUrl, wszUrl));
		}
		else if (wszType ==L"video") {
			const JSONNode &jnVideo = jnAttach["video"];
			if (!jnVideo)
				continue;

			CMStringW wszTitle(jnVideo["title"].as_mstring());
			int vid = jnVideo["id"].as_int();
			int ownerID = jnVideo["owner_id"].as_int();
			CMStringW wszUrl(FORMAT, L"https://vk.com/video%d_%d", ownerID, vid);
			res.AppendFormat(L"%s: %s",
				SetBBCString(TranslateT("Video"), iBBC, vkbbcB),
				SetBBCString(wszTitle, iBBC, vkbbcUrl, wszUrl));
		}
		else if (wszType == L"doc") {
			const JSONNode &jnDoc = jnAttach["doc"];
			if (!jnDoc)
				continue;

			CMStringW wszTitle(jnDoc["title"].as_mstring());
			CMStringW wszUrl(jnDoc["url"].as_mstring());
			res.AppendFormat(L"%s: %s",
				SetBBCString(TranslateT("Document"), iBBC, vkbbcB),
				SetBBCString(wszTitle, iBBC, vkbbcUrl, wszUrl));
		}
		else if (wszType == L"wall") {
			const JSONNode &jnWall = jnAttach["wall"];
			if (!jnWall)
				continue;

			CMStringW wszText(jnWall["text"].as_mstring());
			int id = jnWall["id"].as_int();
			int fromID = jnWall["from_id"].as_int();
			CMStringW wszUrl(FORMAT, L"https://vk.com/wall%d_%d", fromID, id);
			res.AppendFormat(L"%s: %s",
				SetBBCString(TranslateT("Wall post"), iBBC, vkbbcUrl, wszUrl),
				wszText.IsEmpty() ? L" " : wszText);
			
			const JSONNode &jnCopyHystory = jnWall["copy_history"];
			for (auto aCHit = jnCopyHystory.begin(); aCHit != jnCopyHystory.end(); ++aCHit) {
				const JSONNode &jnCopyHystoryItem = (*aCHit);

				CMStringW wszCHText(jnCopyHystoryItem["text"].as_mstring());
				int iCHid = jnCopyHystoryItem["id"].as_int();
				int iCHfromID = jnCopyHystoryItem["from_id"].as_int();
				CMStringW wszCHUrl(FORMAT, L"https://vk.com/wall%d_%d", iCHfromID, iCHid);
				wszCHText.Replace(L"\n", L"\n\t\t");
				res.AppendFormat(L"\n\t\t%s: %s",
					SetBBCString(TranslateT("Wall post"), iBBC, vkbbcUrl, wszCHUrl),
					wszCHText.IsEmpty() ? L" " : wszCHText);

				const JSONNode &jnSubAttachments = jnCopyHystoryItem["attachments"];
				if (jnSubAttachments) {
					debugLogA("CVkProto::GetAttachmentDescr SubAttachments");
					CMStringW wszAttachmentDescr = GetAttachmentDescr(jnSubAttachments, iBBC);
					wszAttachmentDescr.Replace(L"\n", L"\n\t\t");
					res += L"\n\t\t" + wszAttachmentDescr;
				}
			}

			const JSONNode &jnSubAttachments = jnWall["attachments"];
			if (jnSubAttachments) {
				debugLogA("CVkProto::GetAttachmentDescr SubAttachments");
				CMStringW wszAttachmentDescr = GetAttachmentDescr(jnSubAttachments, iBBC);
				wszAttachmentDescr.Replace(L"\n", L"\n\t");
				res += L"\n\t" + wszAttachmentDescr;
			}
		}
		else if (wszType == L"sticker") {
			const JSONNode &jnSticker = jnAttach["sticker"];
			if (!jnSticker)
				continue;
			res.Empty(); // sticker is not really an attachment, so we don't want all that heading info

			if (m_vkOptions.bStikersAsSmyles) {
				int id = jnSticker["id"].as_int();
				res.AppendFormat(L"[sticker:%d]", id);
			}
			else {
				CMStringW wszLink;
				for (int i = 0; i < _countof(szImageTypes); i++) {
					const JSONNode &n = jnSticker[szImageTypes[i]];
					if (n) {
						wszLink = n.as_mstring();
						break;
					}
				}
				res.AppendFormat(L"%s", wszLink);

				if (m_vkOptions.iIMGBBCSupport && iBBC != bbcNo)
					res += SetBBCString(wszLink, iBBC, vkbbcImg);
			}
		}
		else if (wszType == L"link") {
			const JSONNode &jnLink = jnAttach["link"];
			if (!jnLink)
				continue;

			CMStringW wszUrl(jnLink["url"].as_mstring());
			CMStringW wszTitle(jnLink["title"].as_mstring());
			CMStringW wszCaption(jnLink["caption"].as_mstring());
			CMStringW wszDescription(jnLink["description"].as_mstring());

			res.AppendFormat(L"%s: %s",
				SetBBCString(TranslateT("Link"), iBBC, vkbbcB),
				SetBBCString(wszTitle, iBBC, vkbbcUrl, wszUrl));
			
			if (!wszCaption.IsEmpty())
				res.AppendFormat(L"\n\t%s", SetBBCString(wszCaption, iBBC, vkbbcI));
			
			if (jnLink["photo"])
				res.AppendFormat(L"\n\t%s", GetVkPhotoItem(jnLink["photo"], iBBC));

			if (!wszDescription.IsEmpty())
				res.AppendFormat(L"\n\t%s", wszDescription);
		}
		else if (wszType == L"market") {
			const JSONNode &jnMarket = jnAttach["market"];

			int id = jnMarket["id"].as_int();
			int ownerID = jnMarket["owner_id"].as_int();
			CMStringW wszTitle(jnMarket["title"].as_mstring());
			CMStringW wszDescription(jnMarket["description"].as_mstring());
			CMStringW wszPhoto(jnMarket["thumb_photo"].as_mstring());
			CMStringW wszUrl(FORMAT, L"https://vk.com/%s%d?w=product%d_%d",
				ownerID > 0 ? L"id" : L"club",
				ownerID > 0 ? ownerID : (-1)*ownerID,
				ownerID,
				id);

			res.AppendFormat(L"%s: %s",
				SetBBCString(TranslateT("Product"), iBBC, vkbbcB),
				SetBBCString(wszTitle, iBBC, vkbbcUrl, wszUrl));

			if (!wszPhoto.IsEmpty())
				res.AppendFormat(L"\n\t%s: %s", 
					SetBBCString(TranslateT("Photo"), iBBC, vkbbcB), 
					SetBBCString(wszPhoto, iBBC, vkbbcImg));			
			
			if (jnMarket["price"] && jnMarket["price"]["text"])
				res.AppendFormat(L"\n\t%s: %s", 
					SetBBCString(TranslateT("Price"), iBBC, vkbbcB), 
					jnMarket["price"]["text"].as_mstring());

			if (!wszDescription.IsEmpty())
				res.AppendFormat(L"\n\t%s", wszDescription);
		}
		else if (wszType == L"gift") {
			const JSONNode &jnGift = jnAttach["gift"];
			if (!jnGift)
				continue;

			CMStringW wszLink;
			for (int i = 0; i < _countof(szGiftTypes); i++) {
				const JSONNode &n = jnGift[szGiftTypes[i]];
				if (n) {
					wszLink = n.as_mstring();
					break;
				}
			}
			if (wszLink.IsEmpty())
				continue;
			res += SetBBCString(TranslateT("Gift"), iBBC, vkbbcUrl, wszLink);

			if (m_vkOptions.iIMGBBCSupport && iBBC != bbcNo)
				res.AppendFormat(L"\n\t%s", SetBBCString(wszLink, iBBC, vkbbcImg));
		}
		else
			res.AppendFormat(TranslateT("Unsupported or unknown attachment type: %s"), SetBBCString(wszType, iBBC, vkbbcB));

		res.AppendChar('\n');
	}

	return res;
}

CMStringW CVkProto::GetFwdMessages(const JSONNode &jnMessages, const JSONNode &jnFUsers, BBCSupport iBBC)
{
	CMStringW res;
	debugLogA("CVkProto::GetFwdMessages");
	if (!jnMessages) {
		debugLogA("CVkProto::GetFwdMessages pMessages == NULL");
		return res;
	}

	OBJLIST<CVkUserInfo> vkUsers(2, NumericKeySortT);

	for (auto it = jnFUsers.begin(); it != jnFUsers.end(); ++it) {
		const JSONNode &jnUser = (*it);

		int iUserId = jnUser["id"].as_int();
		CMStringW wszNick(FORMAT, L"%s %s", jnUser["first_name"].as_mstring(), jnUser["last_name"].as_mstring());
		CMStringW wszLink(FORMAT, L"https://vk.com/id%d", iUserId);
		
		CVkUserInfo *vkUser = new CVkUserInfo(jnUser["id"].as_int(), false, wszNick, wszLink, FindUser(iUserId));
		vkUsers.insert(vkUser);
	}


	for (auto it = jnMessages.begin(); it != jnMessages.end(); ++it) {
		const JSONNode &jnMsg = (*it);

		UINT uid = jnMsg["user_id"].as_int();
		CVkUserInfo *vkUser = vkUsers.find((CVkUserInfo *)&uid);
		CMStringW wszNick, wszUrl;

		if (vkUser) {
			wszNick = vkUser->m_wszUserNick;
			wszUrl = vkUser->m_wszLink;
		} 
		else {
			MCONTACT hContact = FindUser(uid);
			if (hContact || uid == m_msgId)
				wszNick = ptrW(db_get_wsa(hContact, m_szModuleName, "Nick"));
			else 
				wszNick = TranslateT("(Unknown contact)");		
			wszUrl.AppendFormat(L"https://vk.com/id%d", uid);
		}

		time_t datetime = (time_t)jnMsg["date"].as_int();
		wchar_t ttime[64];
		_locale_t locale = _create_locale(LC_ALL, "");
		_wcsftime_l(ttime, _countof(ttime), L"%x %X", localtime(&datetime), locale);
		_free_locale(locale);

		CMStringW wszBody(jnMsg["body"].as_mstring());

		const JSONNode &jnFwdMessages = jnMsg["fwd_messages"];
		if (jnFwdMessages) {
			CMStringW wszFwdMessages = GetFwdMessages(jnFwdMessages, jnFUsers, iBBC == bbcNo ? iBBC : m_vkOptions.BBCForAttachments());
			if (!wszBody.IsEmpty())
				wszFwdMessages = L"\n" + wszFwdMessages;
			wszBody += wszFwdMessages;
		}

		const JSONNode &jnAttachments = jnMsg["attachments"];
		if (jnAttachments) {
			CMStringW wszAttachmentDescr = GetAttachmentDescr(jnAttachments, iBBC == bbcNo ? iBBC : m_vkOptions.BBCForAttachments());
			if (!wszBody.IsEmpty())
				wszAttachmentDescr = L"\n" + wszAttachmentDescr;
			wszBody += wszAttachmentDescr;
		}

		wszBody.Replace(L"\n", L"\n\t");
		wchar_t tcSplit = m_vkOptions.bSplitFormatFwdMsg ? '\n' : ' ';
		CMStringW wszMes(FORMAT, L"%s %s%c%s %s:\n\n%s\n",
			SetBBCString(TranslateT("Message from"), iBBC, vkbbcB),
			SetBBCString(wszNick, iBBC, vkbbcUrl, wszUrl),
			tcSplit,
			SetBBCString(TranslateT("at"), iBBC, vkbbcB),
			ttime,
			SetBBCString(wszBody, iBBC, vkbbcCode));

		if (!res.IsEmpty())
			res.AppendChar('\n');
		res += wszMes;
	}

	vkUsers.destroy();
	return res;
}

/////////////////////////////////////////////////////////////////////////////////////////

void CVkProto::SetInvisible(MCONTACT hContact)
{
	debugLogA("CVkProto::SetInvisible %d", getDword(hContact, "ID", VK_INVALID_USER));
	if (getWord(hContact, "Status", ID_STATUS_OFFLINE) == ID_STATUS_OFFLINE) {
		setWord(hContact, "Status", ID_STATUS_INVISIBLE);
		SetMirVer(hContact, 1);
		db_set_dw(hContact, "BuddyExpectator", "LastStatus", ID_STATUS_INVISIBLE);
		debugLogA("CVkProto::SetInvisible %d set ID_STATUS_INVISIBLE", getDword(hContact, "ID", VK_INVALID_USER));
	}
	time_t now = time(NULL);
	db_set_dw(hContact, "BuddyExpectator", "LastSeen", (DWORD)now);
	setDword(hContact, "InvisibleTS", (DWORD)now);
}

CMStringW CVkProto::RemoveBBC(CMStringW& wszSrc) 
{
	static const wchar_t *wszSimpleBBCodes[][2] = {
		{ L"[b]", L"[/b]" },
		{ L"[u]", L"[/u]" },
		{ L"[i]", L"[/i]" },
		{ L"[s]", L"[/s]" },
	};

	static const wchar_t *wszParamBBCodes[][2] = {
		{ L"[url=", L"[/url]" },
		{ L"[img=", L"[/img]" },
		{ L"[size=", L"[/size]" },
		{ L"[color=", L"[/color]" },
	};

	CMStringW wszRes(wszSrc); 
	CMStringW wszLow(wszSrc); 
	wszLow.MakeLower();

	for (int i = 0; i < _countof(wszSimpleBBCodes); i++) {
		CMStringW wszOpenTag(wszSimpleBBCodes[i][0]);
		CMStringW wszCloseTag(wszSimpleBBCodes[i][1]);
		
		int lenOpen = wszOpenTag.GetLength();
		int lenClose = wszCloseTag.GetLength();

		int posOpen = 0;
		int posClose = 0;

		while (true) {
			if ((posOpen = wszLow.Find(wszOpenTag, posOpen)) < 0)
				break;

			if ((posClose = wszLow.Find(wszCloseTag, posOpen + lenOpen)) < 0)
				break;

			wszLow.Delete(posOpen, lenOpen);
			wszLow.Delete(posClose - lenOpen, lenClose);

			wszRes.Delete(posOpen, lenOpen);
			wszRes.Delete(posClose - lenOpen, lenClose);

		}
	}

	for (int i = 0; i < _countof(wszParamBBCodes); i++) {
		CMStringW wszOpenTag(wszParamBBCodes[i][0]);
		CMStringW wszCloseTag(wszParamBBCodes[i][1]);

		int lenOpen = wszOpenTag.GetLength();
		int lenClose = wszCloseTag.GetLength();

		int posOpen = 0;
		int posOpen2 = 0;
		int posClose = 0;

		while (true) {
			if ((posOpen = wszLow.Find(wszOpenTag, posOpen)) < 0)
				break;
			
			if ((posOpen2 = wszLow.Find(L"]", posOpen + lenOpen)) < 0)
				break;

			if ((posClose = wszLow.Find(wszCloseTag, posOpen2 + 1)) < 0)
				break;

			wszLow.Delete(posOpen, posOpen2 - posOpen + 1);
			wszLow.Delete(posClose - posOpen2 + posOpen - 1, lenClose);

			wszRes.Delete(posOpen, posOpen2 - posOpen + 1);
			wszRes.Delete(posClose - posOpen2 + posOpen - 1, lenClose);

		}

	}

	return wszRes;
}

void CVkProto::ShowCaptchaInBrowser(HBITMAP hBitmap)
{
	wchar_t wszTempDir[MAX_PATH];
	if (!GetEnvironmentVariable(L"TEMP", wszTempDir, MAX_PATH))
		return;

	CMStringW wszHTMLPath(FORMAT, L"%s\\miranda_captcha.html", wszTempDir);
		
	FILE *pFile = _wfopen(wszHTMLPath, L"w");
	if (pFile == NULL)
		return;

	FIBITMAP *dib = fii->FI_CreateDIBFromHBITMAP(hBitmap);
	FIMEMORY *hMem = fii->FI_OpenMemory(nullptr, 0);
	fii->FI_SaveToMemory(FIF_PNG, dib, hMem, 0);

	BYTE *buf = NULL;
	DWORD bufLen;
	fii->FI_AcquireMemory(hMem, &buf, &bufLen);
	ptrA base64(mir_base64_encode(buf, bufLen));
	fii->FI_CloseMemory(hMem);
	fii->FI_Unload(dib);

	CMStringA szHTML(FORMAT, "<html><body><img src=\"data:image/png;base64,%s\" /></body></html>", base64);
	fwrite(szHTML, 1, szHTML.GetLength(), pFile);
	fclose(pFile);

	wszHTMLPath = L"file://" + wszHTMLPath;
	Utils_OpenUrlW(wszHTMLPath);
}