/*
Plugin of Miranda IM for communicating with users of the MSN Messenger protocol.

Copyright (c) 2012-2016 Miranda NG Team
Copyright (c) 2006-2012 Boris Krasnovskiy.
Copyright (c) 2003-2005 George Hazan.
Copyright (c) 2002-2003 Richard Hughes (original version).

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "stdafx.h"
#include "msn_proto.h"
#include "version.h"


// MirandaStatusToMSN - status helper functions
const char* CMsnProto::MirandaStatusToMSN(int status)
{
	switch (status) {
		case ID_STATUS_OFFLINE:		return "FLN";
		case ID_STATUS_ONTHEPHONE:	return "PHN";
		case ID_STATUS_OUTTOLUNCH:	return "LUN";
		case ID_STATUS_NA:
		case ID_STATUS_AWAY:		return "AWY";
		case ID_STATUS_DND:
		case ID_STATUS_OCCUPIED:	return "BSY";
		case ID_STATUS_INVISIBLE:	return "HDN";
		case ID_STATUS_IDLE:		return "IDL";
		default:					return "NLN";
	}
}

WORD CMsnProto::MSNStatusToMiranda(const char *status)
{
	switch ((*(PDWORD)status & 0x00FFFFFF) | 0x20000000) {
		case ' LDI': return ID_STATUS_IDLE;
		case ' NLN': return ID_STATUS_ONLINE;
		case ' NHP': return ID_STATUS_ONTHEPHONE;
		case ' NUL': return ID_STATUS_OUTTOLUNCH;
		case ' BRB':
		case ' YWA': return ID_STATUS_AWAY;
		case ' YSB': return ID_STATUS_OCCUPIED;
		case ' NDH': return ID_STATUS_INVISIBLE;
		default: return ID_STATUS_OFFLINE;
	}
}

char** CMsnProto::GetStatusMsgLoc(int status)
{
	static const int modes[MSN_NUM_MODES] =
	{
		ID_STATUS_ONLINE,
		ID_STATUS_AWAY,
		ID_STATUS_DND,
		ID_STATUS_NA,
		ID_STATUS_OCCUPIED,
		ID_STATUS_FREECHAT,
		ID_STATUS_INVISIBLE,
		ID_STATUS_ONTHEPHONE,
		ID_STATUS_OUTTOLUNCH,
	};

	for (int i = 0; i < MSN_NUM_MODES; i++)
		if (modes[i] == status) return &msnModeMsgs[i];

	return NULL;
}

// MSN_AddAuthRequest - adds the authorization event to the database
void CMsnProto::MSN_AddAuthRequest(const char *email, const char *nick, const char *reason)
{
	//blob is: UIN=0(DWORD), hContact(DWORD), nick(ASCIIZ), ""(ASCIIZ), ""(ASCIIZ), email(ASCIIZ), ""(ASCIIZ)

	MCONTACT hContact = MSN_HContactFromEmail(email, nick, true, true);

	int emaillen = (int)mir_strlen(email);

	if (nick == NULL) nick = "";
	int nicklen = (int)mir_strlen(nick);

	if (reason == NULL) reason = "";
	int reasonlen = (int)mir_strlen(reason);

	PROTORECVEVENT pre = { 0 };
	pre.timestamp = (DWORD)time(NULL);
	pre.lParam = sizeof(DWORD) + sizeof(HANDLE) + nicklen + emaillen + 5 + reasonlen;

	char* pCurBlob = (char*)alloca(pre.lParam);
	pre.szMessage = pCurBlob;

	*(PDWORD)pCurBlob = 0; pCurBlob += sizeof(DWORD);                // UID
	*(PDWORD)pCurBlob = (DWORD)hContact; pCurBlob += sizeof(DWORD);  // Contact Handle
	mir_strcpy(pCurBlob, nick); pCurBlob += nicklen + 1;                 // Nickname
	*pCurBlob = '\0'; pCurBlob++;                                    // First Name
	*pCurBlob = '\0'; pCurBlob++;	                                   // Last Name
	mir_strcpy(pCurBlob, email); pCurBlob += emaillen + 1;               // E-mail
	mir_strcpy(pCurBlob, reason);                                        // Reason

	ProtoChainRecv(hContact, PSR_AUTH, 0, (LPARAM)&pre);
}

/////////////////////////////////////////////////////////////////////////////////////////

void CMsnProto::InitCustomFolders(void)
{
	if (InitCstFldRan) return;

	wchar_t folder[MAX_PATH];
	mir_snwprintf(folder, L"%%miranda_avatarcache%%\\%S", m_szModuleName);
	hCustomSmileyFolder = FoldersRegisterCustomPathT(LPGEN("Custom Smileys"), m_szModuleName, folder, m_tszUserName);

	InitCstFldRan = true;
}


char* MSN_GetAvatarHash(char* szContext, char** pszUrl)
{
	if (pszUrl)
		*pszUrl = NULL;

	if (szContext == NULL)
		return NULL;

	char *res = NULL;

	ezxml_t xmli = ezxml_parse_str(NEWSTR_ALLOCA(szContext), mir_strlen(szContext));
	const char *szAvatarHash = ezxml_attr(xmli, "SHA1D");
	if (szAvatarHash != NULL) {
		unsigned hashLen;
		mir_ptr<BYTE> hash((BYTE*)mir_base64_decode(szAvatarHash, &hashLen));
		if (hash)
			res = arrayToHex(hash, hashLen);

		if (pszUrl) {
			const char *pszUrlAttr;
			for (int i = 0;; i++) {
				char szSetting[20];
				if (i == 0)
					mir_strcpy(szSetting, "Url");
				else
					mir_snprintf(szSetting, "Url%d", i);
				pszUrlAttr = ezxml_attr(xmli, szSetting);
				if (pszUrlAttr == NULL)
					break;

				if (pszUrlAttr[0] != 0) {
					*pszUrl = mir_strdup(pszUrlAttr);
					break;
				}
			}
		}
	}
	ezxml_free(xmli);

	return res;
}

// MSN_GetAvatarFileName - gets a file name for an contact's avatar
void CMsnProto::MSN_GetAvatarFileName(MCONTACT hContact, wchar_t* pszDest, size_t cbLen, const wchar_t *ext)
{
	size_t tPathLen = mir_snwprintf(pszDest, cbLen, L"%s\\%S", VARSW(L"%miranda_avatarcache%"), m_szModuleName);

	if (_waccess(pszDest, 0))
		CreateDirectoryTreeW(pszDest);

	size_t tPathLen2 = tPathLen;
	if (hContact != NULL) {
		DBVARIANT dbv;
		if (getString(hContact, "PictContext", &dbv) == 0) {
			char* szAvatarHash = MSN_GetAvatarHash(dbv.pszVal);
			if (szAvatarHash != NULL) {
				wchar_t *sztAvatarHash = mir_a2u(szAvatarHash);
				tPathLen += mir_snwprintf(pszDest + tPathLen, cbLen - tPathLen, L"\\%s.", sztAvatarHash);
				mir_free(sztAvatarHash);
				mir_free(szAvatarHash);
			}
			else {
				delSetting(hContact, "PictContext");
				if (cbLen) pszDest[0] = 0;
			}
			db_free(&dbv);
		}
		else if (cbLen)
			pszDest[0] = 0;
	}
	else {
		wchar_t *sztModuleName = mir_a2u(m_szModuleName);
		tPathLen += mir_snwprintf(pszDest + tPathLen, cbLen - tPathLen, L"\\%s avatar.", sztModuleName);
		mir_free(sztModuleName);
	}

	if (ext == NULL) {
		mir_snwprintf(pszDest + tPathLen, cbLen - tPathLen, L"*");

		bool found = false;
		_wfinddata_t c_file;
		long hFile = _wfindfirst(pszDest, &c_file);
		if (hFile > -1L) {
			do {
				if (wcsrchr(c_file.name, '.')) {
					mir_snwprintf(pszDest + tPathLen2, cbLen - tPathLen2, L"\\%s", c_file.name);
					found = true;
				}
			} while (_wfindnext(hFile, &c_file) == 0);
			_findclose(hFile);
		}

		if (!found) pszDest[0] = 0;
	}
	else {
		tPathLen--;
		mir_snwprintf(pszDest + tPathLen, cbLen - tPathLen, ext);
	}
}

int CMsnProto::MSN_SetMyAvatar(const wchar_t* sztFname, void* pData, size_t cbLen)
{
	mir_sha1_ctx sha1ctx;
	BYTE sha1c[MIR_SHA1_HASH_SIZE], sha1d[MIR_SHA1_HASH_SIZE];

	T2Utf szFname(sztFname);

	mir_sha1_init(&sha1ctx);
	mir_sha1_append(&sha1ctx, (BYTE*)pData, (int)cbLen);
	mir_sha1_finish(&sha1ctx, sha1d);

	ptrA szSha1d(mir_base64_encode((PBYTE)sha1d, sizeof(sha1d)));

	mir_sha1_init(&sha1ctx);
	ezxml_t xmlp = ezxml_new("msnobj");

	mir_sha1_append(&sha1ctx, (PBYTE)"Creator", 7);
	mir_sha1_append(&sha1ctx, (PBYTE)MyOptions.szEmail, (int)mir_strlen(MyOptions.szEmail));
	ezxml_set_attr(xmlp, "Creator", MyOptions.szEmail);

	char szFileSize[20];
	_ultoa((unsigned)cbLen, szFileSize, 10);
	mir_sha1_append(&sha1ctx, (PBYTE)"Size", 4);
	mir_sha1_append(&sha1ctx, (PBYTE)szFileSize, (int)mir_strlen(szFileSize));
	ezxml_set_attr(xmlp, "Size", szFileSize);

	mir_sha1_append(&sha1ctx, (PBYTE)"Type", 4);
	mir_sha1_append(&sha1ctx, (PBYTE)"3", 1);  // MSN_TYPEID_DISPLAYPICT
	ezxml_set_attr(xmlp, "Type", "3");

	mir_sha1_append(&sha1ctx, (PBYTE)"Location", 8);
	mir_sha1_append(&sha1ctx, (PBYTE)szFname, (int)mir_strlen(szFname));
	ezxml_set_attr(xmlp, "Location", szFname);

	mir_sha1_append(&sha1ctx, (PBYTE)"Friendly", 8);
	mir_sha1_append(&sha1ctx, (PBYTE)"AAA=", 4);
	ezxml_set_attr(xmlp, "Friendly", "AAA=");

	mir_sha1_append(&sha1ctx, (PBYTE)"SHA1D", 5);
	mir_sha1_append(&sha1ctx, (PBYTE)(char*)szSha1d, (int)mir_strlen(szSha1d));
	ezxml_set_attr(xmlp, "SHA1D", szSha1d);

	mir_sha1_finish(&sha1ctx, sha1c);

	ptrA szSha1c(mir_base64_encode((PBYTE)sha1c, sizeof(sha1c)));

	//	ezxml_set_attr(xmlp, "SHA1C", szSha1c);

	char* szBuffer = ezxml_toxml(xmlp, false);
	ezxml_free(xmlp);
	ptrA szEncodedBuffer(mir_urlEncode(szBuffer));
	free(szBuffer);

	const wchar_t *szExt;
	int fmt = ProtoGetBufferFormat(pData, &szExt);
	if (fmt == PA_FORMAT_UNKNOWN)
		return fmt;

	wchar_t szFileName[MAX_PATH];
	MSN_GetAvatarFileName(NULL, szFileName, _countof(szFileName), NULL);
	_wremove(szFileName);

	MSN_GetAvatarFileName(NULL, szFileName, _countof(szFileName), szExt);

	int fileId = _wopen(szFileName, _O_CREAT | _O_TRUNC | _O_WRONLY | O_BINARY, _S_IREAD | _S_IWRITE);
	if (fileId >= 0) {
		_write(fileId, pData, (unsigned)cbLen);
		_close(fileId);

		char szAvatarHashdOld[41] = "";
		if (!db_get_static(NULL, m_szModuleName, "AvatarHash", szAvatarHashdOld, sizeof(szAvatarHashdOld))) {
			char *szAvatarHash = arrayToHex(sha1d, sizeof(sha1d));
			if (mir_strcmp(szAvatarHashdOld, szAvatarHash)) {
				setString("PictObject", szEncodedBuffer);
				setString("AvatarHash", szAvatarHash);
			}
			mir_free(szAvatarHash);
		}
	}
	else MSN_ShowError("Cannot set avatar. File '%s' could not be created/overwritten", szFileName);

	return fmt;
}

// MSN_GetCustomSmileyFileName - gets a file name for an contact's custom smiley
void CMsnProto::MSN_GetCustomSmileyFileName(MCONTACT hContact, wchar_t* pszDest, size_t cbLen, const char* SmileyName, int type)
{
	size_t tPathLen;

	InitCustomFolders();

	wchar_t* path = (wchar_t*)alloca(cbLen * sizeof(wchar_t));
	if (hCustomSmileyFolder == NULL || FoldersGetCustomPathT(hCustomSmileyFolder, path, (int)cbLen, L"")) {
		wchar_t *tmpPath = Utils_ReplaceVarsT(L"%miranda_userdata%");
		wchar_t *tszModuleName = mir_a2u(m_szModuleName);
		tPathLen = mir_snwprintf(pszDest, cbLen, L"%s\\%s\\CustomSmiley", tmpPath, tszModuleName);
		mir_free(tszModuleName);
		mir_free(tmpPath);
	}
	else {
		mir_wstrcpy(pszDest, path);
		tPathLen = mir_wstrlen(pszDest);
	}

	if (hContact != NULL) {
		DBVARIANT dbv = { 0 };
		if (getWString(hContact, "e-mail", &dbv)) {
			dbv.type = DBVT_ASCIIZ;
			dbv.ptszVal = (wchar_t*)mir_alloc(11*sizeof(wchar_t));
			_ui64tow((UINT_PTR)hContact, dbv.ptszVal, 10);
		}

		tPathLen += mir_snwprintf(pszDest + tPathLen, cbLen - tPathLen, L"\\%s", dbv.ptszVal);
		db_free(&dbv);
	}
	else {
		wchar_t *tszModuleName = mir_a2u(m_szModuleName);
		tPathLen += mir_snwprintf(pszDest + tPathLen, cbLen - tPathLen, L"\\%s", tszModuleName);
		mir_free(tszModuleName);
	}

	bool exist = _waccess(pszDest, 0) == 0;

	if (type == 0) {
		if (!exist) pszDest[0] = 0;
		return;
	}

	if (!exist)
		CreateDirectoryTreeW(pszDest);

	wchar_t *sztSmileyName = mir_a2u(SmileyName);
	mir_snwprintf(pszDest + tPathLen, cbLen - tPathLen, L"\\%s.%s", sztSmileyName,
		type == MSN_APPID_CUSTOMSMILEY ? L"png" : L"gif");
	mir_free(sztSmileyName);
}

// MSN_GoOffline - performs several actions when a server goes offline
void CMsnProto::MSN_GoOffline(void)
{
	if (m_iStatus == ID_STATUS_OFFLINE) return;

	msnLoggedIn = false;

	if (mStatusMsgTS)
		ForkThread(&CMsnProto::msn_storeProfileThread, NULL);

	mir_free(msnPreviousUUX);
	msnPreviousUUX = NULL;
	msnSearchId = NULL;

	if (!g_bTerminated)
		MSN_EnableMenuItems(false);

	MSN_FreeGroups();
#ifdef OBSOLETE
	MsgQueue_Clear();
#endif
	clearCachedMsg();

	if (!g_bTerminated) {
		int msnOldStatus = m_iStatus; m_iStatus = m_iDesiredStatus = ID_STATUS_OFFLINE;
		ProtoBroadcastAck(NULL, ACKTYPE_STATUS, ACKRESULT_SUCCESS, (HANDLE)msnOldStatus, ID_STATUS_OFFLINE);
		isIdle = false;

		MCONTACT hContact = NULL;

		for (hContact = db_find_first(m_szModuleName); hContact; 
				hContact = db_find_next(hContact, m_szModuleName)) 
		{
			if (isChatRoom(hContact) != 0) {
				ptrW wszRoom(getWStringA(hContact, "ChatRoomID"));
				if (wszRoom != NULL)
					Chat_Control(m_szModuleName, wszRoom, SESSION_OFFLINE);
			}
			else {
				if (ID_STATUS_OFFLINE != getWord(hContact, "Status", ID_STATUS_OFFLINE)) {
					setWord(hContact, "Status", ID_STATUS_OFFLINE);
					setDword(hContact, "IdleTS", 0);
				}
			}
		}
	}
}
#ifdef OBSOLETE
int ThreadData::sendMessage(int msgType, const char* email, int netId, const char* parMsg, int parFlags)
#else
// MSN_SendMessage - formats and sends a MSG packet through the server
int ThreadData::sendMessage(int, const char *email, int netId, const char *parMsg, int parFlags)
#endif
{
	CMStringA buf;

	if ((parFlags & MSG_DISABLE_HDR) == 0) {
		char  tFontName[100], tFontStyle[3], *pszMsgType, *pszContType;
		DWORD tFontColor = 0;

		if (parFlags & MSG_CONTACT) {
			pszMsgType = "RichText/Contacts";
			pszContType = "application/user+xml\r\nSkype-Age: 18";
			*tFontName = 0;
		} else if (parFlags & MSG_NUDGE) {
			pszMsgType = "Nudge";
			pszContType = "text/x-msnmsgr-datacast";
			*tFontName = 0;
		} else {
			pszMsgType = "Text";
			pszContType = "Text/plain; charset=UTF-8";

			mir_strcpy(tFontName, "Arial");

			if (proto->getByte("SendFontInfo", 1)) {
				char* p;

				DBVARIANT dbv;
				if (!db_get_s(NULL, "SRMsg", "Font0", &dbv)) {
					for (p = dbv.pszVal; *p; p++)
						if (BYTE(*p) >= 128 || *p < 32)
							break;

					if (*p == 0) {
						strncpy_s(tFontName, sizeof(tFontName), ptrA(mir_urlEncode(dbv.pszVal)), _TRUNCATE);
						db_free(&dbv);
					}
				}

				int  tStyle = db_get_b(NULL, "SRMsg", "Font0Sty", 0);
				p = tFontStyle;
				if (tStyle & 1) *p++ = 'B';
				if (tStyle & 2) *p++ = 'I';
				*p = 0;

				tFontColor = db_get_dw(NULL, "SRMsg", "Font0Col", 0);
			}
			else tFontStyle[0] = 0;

#ifdef OBSOLETE
			if (parFlags & MSG_OFFLINE)
				off += mir_snprintf((buf + off), (_countof(buf) - off), "Dest-Agent: client\r\n"); 
#endif
		}

		char *pszNick=proto->MyOptions.szEmail;
		DBVARIANT dbv;
		time_t cur_time;

		/* FIXME: Use a real message ID and save it, not just this random UUID */
		unsigned __int64 msgid;
		time(&cur_time);
		msgid = ((unsigned __int64)cur_time<<32)|GetTickCount();

		if (!proto->getString("Nick", &dbv))
			pszNick = dbv.pszVal;

		CMStringA msgClean;

		for (BYTE *p = (BYTE*)parMsg; *p; p++) if (*p >= 0x20 || (*p>=0x09 && *p<=0x0D)) msgClean.AppendChar(*p);
		buf.AppendFormat(
			"Messaging: 2.0\r\n"
			"Client-Message-ID: %llu\r\n"
			"Message-Type: %s\r\n"
			"IM-Display-Name: %s\r\n"
			"Content-Type: %s\r\n"
			"Content-Length: %d\r\n",
			msgid, pszMsgType, pszNick, pszContType, msgClean.GetLength());

		if (*tFontName) buf.AppendFormat("X-MMS-IM-Format: FN=%s; EF=%s; CO=%x; CS=0; PF=31%s\r\n",
			tFontName, tFontStyle, tFontColor, (parFlags & MSG_RTL) ? ";RL=1" : "");
		buf.AppendFormat("\r\n%s", msgClean);

		if (pszNick!=proto->MyOptions.szEmail) db_free(&dbv);
		parMsg = buf;
	}

	// TODO: Handle msgType!

	int seq = sendPacketPayload("SDG", "MSGR", 
		"Routing: 1.0\r\n"
		"To: %d:%s\r\n"
		"From: %d:%s;epid=%s\r\n\r\n"
		"Reliability: 1.0\r\n\r\n%s",
		netId, email,
		netId == NETID_SKYPE?netId:proto->MyOptions.netId, proto->GetMyUsername(netId), proto->MyOptions.szMachineGuid,
		parMsg);

#ifdef OBSOLETE
	if (netId == NETID_YAHOO || netId == NETID_MOB || (parFlags & MSG_OFFLINE))
		seq = sendPacket("UUM", "%s %d %c %d\r\n%s%s", email, netId, msgType,
		mir_strlen(parMsg) + off, buf, parMsg);
	else
		seq = sendPacket("MSG", "%c %d\r\n%s%s", msgType,
		mir_strlen(parMsg) + off, buf, parMsg);
#endif

	return seq;
}

#ifdef OBSOLETE
void ThreadData::sendCaps(void)
{
	char mversion[100], capMsg[1000];
	CallService(MS_SYSTEM_GETVERSIONTEXT, sizeof(mversion), (LPARAM)mversion);

	mir_snprintf(capMsg,
		"Content-Type: text/x-clientcaps\r\n\r\n"
		"Client-Name: Miranda NG %s (MSN v.%s)\r\n",
		mversion, __VERSION_STRING_DOTS);

	sendMessage('U', NULL, 1, capMsg, MSG_DISABLE_HDR);
}
#endif

void ThreadData::sendTerminate(void)
{
	if (!termPending) {
		sendPacket("OUT", "CON 0");
		termPending = true;
	}
}

// MSN_SendRawPacket - sends a packet accordingly to the MSN protocol
int ThreadData::sendRawMessage(int msgType, const char* data, int datLen)
{
	if (data == NULL)
		data = "";

	if (datLen == -1)
		datLen = (int)mir_strlen(data);

	char* buf = (char*)alloca(datLen + 100);

	int thisTrid = InterlockedIncrement(&mTrid);
	int nBytes = mir_snprintf(buf, 100, "MSG %d %c %d\r\nMIME-Version: 1.0\r\n",
		thisTrid, msgType, datLen + 19);
	memcpy(buf + nBytes, data, datLen);

	send(buf, nBytes + datLen);

	return thisTrid;
}

// Typing notifications support
void CMsnProto::MSN_SendTyping(ThreadData* info, const char* email, int netId, bool bTyping)
{
	char tCommand[1024];
	mir_snprintf(tCommand,
		"Messaging: 2.0\r\n"
		"Message-Type: %s\r\n"
		"Content-Type: Application/Message\r\n"
		"Content-Length: 0\r\n",
		bTyping?"Control/Typing":"Control/ClearTyping");

	info->sendMessage(netId == NETID_MSN ? 'U' : '2', email, netId, tCommand, MSG_DISABLE_HDR);
}

void CMsnProto::MSN_StartStopTyping(GCThreadData* info, bool start)
{
	MSN_SendTyping(msnNsThread, info->szEmail, info->netId, start);
}

// Helper to process texts
static char * HtmlEncodeUTF8T(const wchar_t *src)
{
	if (src == NULL)
		return mir_strdup("");

	return HtmlEncode(UTF8(src));
}

#ifdef OBSOLETE
void CMsnProto::MSN_SendStatusMessage(const char* msg)
#else
// MSN_SendStatusMessage - notify a server about the status message change
void CMsnProto::MSN_SendStatusMessage(const char*)
#endif
{
	if (!msnLoggedIn)
		return;

	MSN_SetServerStatus(m_iDesiredStatus);
	/* FIXME: Currently not implemented, should be set on status change anyway  */

#ifdef OBSOLETE
	char* msgEnc = HtmlEncode(msg ? msg : "");

	size_t sz;
	char  szMsg[2048];
	if (msnCurrentMedia.cbSize == 0) {
		sz = mir_snprintf(szMsg, "<Data><PSM>%s</PSM><CurrentMedia></CurrentMedia><MachineGuid>%s</MachineGuid>"
			"<DDP></DDP><SignatureSound></SignatureSound><Scene></Scene><ColorScheme></ColorScheme></Data>",
			msgEnc, MyOptions.szMachineGuid);
	}
	else {
		char *szFormatEnc;
		if (ServiceExists(MS_LISTENINGTO_GETPARSEDTEXT)) {
			LISTENINGTOINFO lti = { 0 };
			lti.cbSize = sizeof(lti);
			if (msnCurrentMedia.ptszTitle != NULL) lti.ptszTitle = L"{0}";
			if (msnCurrentMedia.ptszArtist != NULL) lti.ptszArtist = L"{1}";
			if (msnCurrentMedia.ptszAlbum != NULL) lti.ptszAlbum = L"{2}";
			if (msnCurrentMedia.ptszTrack != NULL) lti.ptszTrack = L"{3}";
			if (msnCurrentMedia.ptszYear != NULL) lti.ptszYear = L"{4}";
			if (msnCurrentMedia.ptszGenre != NULL) lti.ptszGenre = L"{5}";
			if (msnCurrentMedia.ptszLength != NULL) lti.ptszLength = L"{6}";
			if (msnCurrentMedia.ptszPlayer != NULL) lti.ptszPlayer = L"{7}";
			if (msnCurrentMedia.ptszType != NULL) lti.ptszType = L"{8}";

			wchar_t *tmp = (wchar_t *)CallService(MS_LISTENINGTO_GETPARSEDTEXT, (WPARAM)L"%title% - %artist%", (LPARAM)&lti);
			szFormatEnc = HtmlEncodeUTF8T(tmp);
			mir_free(tmp);
		}
		else szFormatEnc = HtmlEncodeUTF8T(L"{0} - {1}");

		char *szArtist = HtmlEncodeUTF8T(msnCurrentMedia.ptszArtist),
			*szAlbum = HtmlEncodeUTF8T(msnCurrentMedia.ptszAlbum),
			*szTitle = HtmlEncodeUTF8T(msnCurrentMedia.ptszTitle),
			*szTrack = HtmlEncodeUTF8T(msnCurrentMedia.ptszTrack),
			*szYear = HtmlEncodeUTF8T(msnCurrentMedia.ptszYear),
			*szGenre = HtmlEncodeUTF8T(msnCurrentMedia.ptszGenre),
			*szLength = HtmlEncodeUTF8T(msnCurrentMedia.ptszLength),
			*szPlayer = HtmlEncodeUTF8T(msnCurrentMedia.ptszPlayer),
			*szType = HtmlEncodeUTF8T(msnCurrentMedia.ptszType);

		sz = mir_snprintf(szMsg, _countof(szMsg),
			"<Data>"
			"<PSM>%s</PSM>"
			"<CurrentMedia>%s\\0%s\\01\\0%s\\0%s\\0%s\\0%s\\0%s\\0%s\\0%s\\0%s\\0%s\\0%s\\0\\0</CurrentMedia>"
			"<MachineGuid>%s</MachineGuid><DDP></DDP><SignatureSound></SignatureSound><Scene></Scene><ColorScheme></ColorScheme>"
			"<DDP></DDP><SignatureSound></SignatureSound><Scene></Scene><ColorScheme></ColorScheme>"
			"</Data>",
			msgEnc, szPlayer, szType, szFormatEnc, szTitle, szArtist, szAlbum, szTrack, szYear, szGenre,
			szLength, szPlayer, szType, MyOptions.szMachineGuid);

		mir_free(szArtist);
		mir_free(szAlbum);
		mir_free(szTitle);
		mir_free(szTrack);
		mir_free(szYear);
		mir_free(szGenre);
		mir_free(szLength);
		mir_free(szPlayer);
		mir_free(szType);
		mir_free(szFormatEnc);
	}
	mir_free(msgEnc);

	if (msnPreviousUUX == NULL || mir_strcmp(msnPreviousUUX, szMsg)) {
		replaceStr(msnPreviousUUX, szMsg);
		msnNsThread->sendPacket("UUX", "%d\r\n%s", sz, szMsg);
		mStatusMsgTS = clock();
	}
#endif
}

// MSN_SendPacket - sends a packet accordingly to the MSN protocol
int ThreadData::sendPacket(const char* cmd, const char* fmt, ...)
{
	if (this == NULL) return 0;

	CMStringA str;
	int thisTrid = 0;

	if (fmt == NULL)
		str = cmd;
	else {
		thisTrid = InterlockedIncrement(&mTrid);
		if (fmt[0] == '\0')
			str.AppendFormat("%s %d", cmd, thisTrid);
		else {
			va_list vararg;
			va_start(vararg, fmt);
			str.AppendFormat("%s %d ", cmd, thisTrid);
			str.AppendFormatV(fmt, vararg);
			va_end(vararg);
		}
	}

	if (strchr(str, '\r') == NULL)
		str += "\r\n";

	int result = send(str, str.GetLength());
	return (result > 0) ? thisTrid : -1;
}

int ThreadData::sendPacketPayload(const char* cmd, const char *param, const char* fmt, ...)
{
	if (this == NULL) return 0;

	va_list vararg;
	va_start(vararg, fmt);

	bool bTopHdr = fmt[0] == '\b';
	int thisTrid = InterlockedIncrement(&mTrid);

	CMStringA str, payLoad;
	if (bTopHdr)
		fmt++;

	if (proto->msnRegistration)
		payLoad.AppendFormat("Registration: %s\r\n", proto->msnRegistration);
	if (!bTopHdr)
		payLoad.Append("\r\n");
	payLoad.AppendFormatV(fmt, vararg);
	va_end(vararg);

	str.AppendFormat("%s %d %s ", cmd, thisTrid, param);
	str.AppendFormat("%d\r\n", payLoad.GetLength());
	str += payLoad;

	int result = send(str, str.GetLength());
	return (result > 0) ? thisTrid : -1;
}

// MSN_SetServerStatus - changes plugins status at the server
void CMsnProto::MSN_SetServerStatus(int newStatus)
{
	debugLogA("Setting MSN server status %d, logged in = %d", newStatus, msnLoggedIn);

	if (!msnLoggedIn)
		return;

	if (isIdle)
		newStatus = ID_STATUS_IDLE;

	const char* szStatusName = MirandaStatusToMSN(newStatus);

	if (newStatus != ID_STATUS_OFFLINE) {
		DBVARIANT msnObject = { 0 };
		if (ServiceExists(MS_AV_SETMYAVATAR))
			getString("PictObject", &msnObject);

		// Capabilties: WLM 2009, Chunking, UUN Bootstrap
		myFlags = 0xA0000000 | cap_SupportsChunking | cap_SupportsP2PBootstrap | cap_SupportsSipInvite;
		if (getByte("MobileEnabled", 0) && getByte("MobileAllowed", 0))
			myFlags |= cap_MobileEnabled;

		unsigned myFlagsEx = capex_SupportsPeerToPeerV2 | capex_SupportsOfflineIM;
		unsigned myFlagsExEx = capexex_SupportsMissedConversations | capexex_SupportsShortCircuit;

		char szMsg[2048];
#ifdef OBSOLETE
		if (m_iStatus < ID_STATUS_ONLINE) {
			int sz = mir_snprintf(szMsg,
				"<EndpointData><Capabilities>%u:%u</Capabilities></EndpointData>", myFlags, myFlagsEx);
			msnNsThread->sendPacket("UUX", "%d\r\n%s", sz, szMsg);

			msnNsThread->sendPacket("BLP", msnOtherContactsBlocked ? "BL" : "AL");

			DBVARIANT dbv;
			if (!getStringUtf("Nick", &dbv)) {
				if (dbv.pszVal[0])
					MSN_SetNicknameUtf(dbv.pszVal);
				db_free(&dbv);
			}
		}
#endif

		char *szPlace;
		DBVARIANT dbv;
		if (!getStringUtf("Place", &dbv))
			szPlace = dbv.pszVal;
		else {
			wchar_t buf[128] = L"Miranda";
			DWORD buflen = _countof(buf);
			GetComputerName(buf, &buflen);
			szPlace = mir_utf8encodeW(buf);
		}

		char** msgptr = GetStatusMsgLoc(newStatus);
		int sz = mir_snprintf(szMsg,
			"<user>"
			"<sep n=\"PE\" epid=\"%s\"><VER>%s</VER><TYP>11</TYP><Capabilities>0:0</Capabilities></sep>"
			"<s n=\"IM\"><Status>%s</Status></s>"
			"<sep n=\"IM\" epid=\"%s\"><Capabilities>%u:%u:%u</Capabilities></sep>"
			"<sep n=\"PD\" epid=\"%s\"><EpName>%s</EpName><ClientType>11</ClientType></sep>"
			"<s n=\"SKP\"><Mood>%s</Mood><Skypename>%s</Skypename></s>"
			"<sep n=\"SKP\" epid=\"%s\"><NodeInfo></NodeInfo><Version>24</Version><Seamless>true</Seamless></sep>"
			"</user>",
			MyOptions.szMachineGuid, msnProductVer,
			szStatusName,
			MyOptions.szMachineGuid, myFlags, myFlagsEx, myFlagsExEx,
			MyOptions.szMachineGuid, szPlace,
			msgptr?ptrA(HtmlEncode(*msgptr)):"", GetMyUsername(NETID_SKYPE),
			MyOptions.szMachineGuid,
			MyOptions.szMachineGuid);
		msnNsThread->sendPacketPayload("PUT", "MSGR\\PRESENCE", 
			"Routing: 1.0\r\n"
			"To: %d:%s\r\n"
			"From: %d:%s;epid=%s\r\n\r\n"
			"Reliability: 1.0\r\n\r\n"
			"Publication: 1.0\r\n"
			"Uri: /user\r\n"
			"Content-Type: application/user+xml\r\n"
			"Content-Length: %d\r\n\r\n%s",
			MyOptions.netId, MyOptions.szEmail,
			MyOptions.netId, MyOptions.szEmail,
			MyOptions.szMachineGuid,
			sz, szMsg);


		// TODO: Send, MSN_SendStatusMessage anpassen.
#ifdef OBSOLETE
		int sz = mir_snprintf(szMsg,
			"<PrivateEndpointData>"
			"<EpName>%s</EpName>"
			"<Idle>%s</Idle>"
			"<ClientType>1</ClientType>"
			"<State>%s</State>"
			"</PrivateEndpointData>",
			szPlace, newStatus == ID_STATUS_IDLE ? "true" : "false", szStatusName);
		msnNsThread->sendPacket("UUX", "%d\r\n%s", sz, szMsg);
#endif
		mir_free(szPlace);

#ifdef OBSOLETE
		if (newStatus != ID_STATUS_IDLE) {
			char** msgptr = GetStatusMsgLoc(newStatus);
			if (msgptr != NULL)
				MSN_SendStatusMessage(*msgptr);
		}

		msnNsThread->sendPacket("CHG", "%s %u:%u %s", szStatusName, myFlags, myFlagsEx, msnObject.pszVal ? msnObject.pszVal : "0");
#endif
		db_free(&msnObject);
	}
#ifdef OBSOLETE
	else msnNsThread->sendPacket("CHG", szStatusName);
#endif
}

// MSN_FetchRecentMessages - fetches missed offline messages
void CMsnProto::MSN_FetchRecentMessages(time_t since)
{
	if (!since) {
		/* Assuming that you want all messages that were sent after the last
		 * user conversation according to DB 
		 */
		MCONTACT hContact;
		MEVENT hDbEvent;
		for (hContact = db_find_first(m_szModuleName); hContact; 
			 hContact = db_find_next(hContact, m_szModuleName)) 
		{
			if (!(hDbEvent = db_event_last(hContact)))
				continue;

			DBEVENTINFO dbei = { sizeof(dbei) };
			db_event_get(hDbEvent, &dbei);
			if (dbei.timestamp>since) since=dbei.timestamp;
		}
	}

	msnNsThread->sendPacketPayload("GET", "MSGR\\RECENTCONVERSATIONS", 
		"<recentconversations><start>%llu</start><pagesize>100</pagesize></recentconversations>",
		((unsigned __int64)since)*1000);
}

static const char postdataM[] = "ct=%u&bver=7&wa=wsignin1.0&ru=%s&pl=MBI";
static const char postdataS[] = "ct=%u&bver=7&id=73625&ru=%s&js=yes&pl=%%3Fid%%3D73625";

// Display Hotmail Inbox thread
void CMsnProto::MsnInvokeMyURL(bool ismail, const char* url)
{
	if (!url)
		url = ismail ? "http://mail.live.com?rru=inbox" : "http://profile.live.com";

	const char *postdata = ismail ? postdataM : postdataS;

	char passport[256];
	if (db_get_static(NULL, m_szModuleName, "MsnPassportHost", passport, 256))
		mir_strcpy(passport, "https://login.live.com/");

	char *p = strchr(passport, '/');
	if (p && p[1] == '/') p = strchr(p + 2, '/');
	if (p)
		*p = 0;

	CMStringA post = HotmailLogin(CMStringA().Format(postdata, (unsigned)time(NULL), ptrA(mir_urlEncode(url))));
	if (!post.IsEmpty()) {
		CMStringA hippy(passport);
		hippy.AppendFormat("/ppsecure/sha1auth.srf?lc=%d&token=%s", itoa(langpref, passport, 10), ptrA(mir_urlEncode(post)));

		debugLogA("Starting URL: '%s'", hippy);
		Utils_OpenUrl(hippy.GetString());
	}
}

// MSN_ShowError - shows an error
void CMsnProto::MSN_ShowError(const char* msgtext, ...)
{
	wchar_t   tBuffer[4096];
	va_list tArgs;

	wchar_t *buf = Langpack_PcharToTchar(msgtext);

	va_start(tArgs, msgtext);
	mir_vsnwprintf(tBuffer, _countof(tBuffer), buf, tArgs);
	va_end(tArgs);

	mir_free(buf);

	MSN_ShowPopup(m_tszUserName, tBuffer, MSN_ALLOW_MSGBOX | MSN_SHOW_ERROR, NULL);
}

void HandlePopupData(PopupData *tData)  {
	if (tData != NULL) {
		if (tData->flags & MSN_HOTMAIL_POPUP) {
			MCONTACT hContact = tData->proto->MSN_HContactFromEmail(tData->proto->MyOptions.szEmail, NULL);
			if (hContact)
				pcli->pfnRemoveEvent(hContact, 1);
			if (tData->flags & MSN_ALLOW_ENTER)
				tData->proto->MsnInvokeMyURL(true, tData->url);
		}
		else if (tData->url != NULL)
			Utils_OpenUrl(tData->url);
	}
}

void RemovePopupData(PopupData *tData) {
	if (tData != NULL && (tData->flags & MSN_HOTMAIL_POPUP)) {
		MCONTACT hContact = tData->proto->MSN_HContactFromEmail(tData->proto->MyOptions.szEmail, NULL);
		if (hContact)
			pcli->pfnRemoveEvent(hContact, 1);
	}
}

// Popup plugin window proc
LRESULT CALLBACK NullWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	PopupData *tData = (PopupData*)PUGetPluginData(hWnd);

	switch (msg) {
	case WM_COMMAND:
		HandlePopupData(tData);
		PUDeletePopup(hWnd);
		break;

	case WM_CONTEXTMENU:
		RemovePopupData(tData);
		PUDeletePopup(hWnd);
		break;

	case UM_FREEPLUGINDATA:
		if (tData != NULL && tData != (PopupData*)CALLSERVICE_NOTFOUND) {
			mir_free(tData->title);
			mir_free(tData->text);
			mir_free(tData->url);
			mir_free(tData);
		}
		break;
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}

// InitPopups - popup plugin support
void CMsnProto::InitPopups(void)
{
	wchar_t desc[256];
	char name[256];

	POPUPCLASS ppc = { sizeof(ppc) };
	ppc.flags = PCF_TCHAR;
	ppc.PluginWindowProc = NullWindowProc;
	ppc.hIcon = LoadIconEx("main");
	ppc.pwszDescription = desc;
	ppc.pszName = name;

	ppc.colorBack = RGB(173, 206, 247);
	ppc.colorText = GetSysColor(COLOR_WINDOWTEXT);
	ppc.iSeconds = 3;
	mir_snwprintf(desc, L"%s/%s", m_tszUserName, TranslateT("Hotmail"));
	mir_snprintf(name, "%s_%s", m_szModuleName, "Hotmail");
	hPopupHotmail = Popup_RegisterClass(&ppc);

	ppc.colorBack = RGB(173, 206, 247);
	ppc.colorText = GetSysColor(COLOR_WINDOWTEXT);
	ppc.iSeconds = 3;
	mir_snwprintf(desc, L"%s/%s", m_tszUserName, TranslateT("Notify"));
	mir_snprintf(name, "%s_%s", m_szModuleName, "Notify");
	hPopupNotify = Popup_RegisterClass(&ppc);

	ppc.hIcon = (HICON)LoadImage(NULL, IDI_WARNING, IMAGE_ICON, 0, 0, LR_SHARED);
	ppc.colorBack = RGB(191, 0, 0); //Red
	ppc.colorText = RGB(255, 245, 225); //Yellow
	ppc.iSeconds = 60;

	mir_snwprintf(desc, L"%s/%s", m_tszUserName, TranslateT("Error"));
	mir_snprintf(name, "%s_%s", m_szModuleName, "Error");
	hPopupError = Popup_RegisterClass(&ppc);
}

// MSN_ShowPopup - popup plugin support
void CALLBACK sttMainThreadCallback(void *param)
{
	PopupData* pud = (PopupData*)param;

	bool iserr = (pud->flags & MSN_SHOW_ERROR) != 0;
	if ((iserr && !pud->proto->MyOptions.ShowErrorsAsPopups) || !ServiceExists(MS_POPUP_ADDPOPUPCLASS)) {
		if (pud->flags & MSN_ALLOW_MSGBOX) {
			wchar_t szMsg[MAX_SECONDLINE + MAX_CONTACTNAME];
			mir_snwprintf(szMsg, L"%s:\n%s", pud->title, pud->text);
			int ret = MessageBox(NULL, szMsg, TranslateT("MSN Protocol"),
				MB_YESNO | (iserr ? MB_ICONERROR : MB_ICONINFORMATION));
			if (ret == IDYES)
				HandlePopupData(pud);
			else
				RemovePopupData(pud);
		}
		mir_free(pud->title);
		mir_free(pud->text);
		mir_free(pud->url);
		mir_free(pud);
		return;
	}

	char name[256];

	POPUPDATACLASS ppd = { sizeof(ppd) };
	ppd.pwszTitle = pud->title;
	ppd.pwszText = pud->text;
	ppd.PluginData = pud;
	ppd.pszClassName = name;

	if (pud->flags & MSN_SHOW_ERROR)
		mir_snprintf(name, "%s_%s", pud->proto->m_szModuleName, "Error");
	else if (pud->flags & (MSN_HOTMAIL_POPUP | MSN_ALERT_POPUP))
		mir_snprintf(name, "%s_%s", pud->proto->m_szModuleName, "Hotmail");
	else
		mir_snprintf(name, "%s_%s", pud->proto->m_szModuleName, "Notify");

	CallService(MS_POPUP_ADDPOPUPCLASS, 0, (LPARAM)&ppd);
}

void CMsnProto::MSN_ShowPopup(const wchar_t* nickname, const wchar_t* msg, int flags, const char* url)
{
	if (g_bTerminated) return;

	PopupData *pud = (PopupData*)mir_calloc(sizeof(PopupData));
	pud->flags = flags;
	pud->url = mir_strdup(url);
	pud->title = mir_wstrdup(nickname);
	pud->text = mir_wstrdup(msg);
	pud->proto = this;

	CallFunctionAsync(sttMainThreadCallback, pud);
}


void CMsnProto::MSN_ShowPopup(const MCONTACT hContact, const wchar_t* msg, int flags)
{
	MSN_ShowPopup(GetContactNameT(hContact), msg, flags, NULL);
}


/////////////////////////////////////////////////////////////////////////////////////////
// filetransfer class members
filetransfer::filetransfer(CMsnProto* prt)
{
	memset(this, 0, sizeof(filetransfer));
	fileId = -1;
	std.cbSize = sizeof(std);
	std.flags = PFTS_UNICODE;
	proto = prt;

	hLockHandle = CreateMutex(NULL, FALSE, NULL);
	hResumeEvt = CreateEvent(NULL, FALSE, FALSE, NULL);
}

filetransfer::~filetransfer(void)
{
	if (p2p_sessionid)
		proto->debugLogA("Destroying file transfer session %08X", p2p_sessionid);

	WaitForSingleObject(hLockHandle, 2000);
	CloseHandle(hLockHandle);
	CloseHandle(hResumeEvt);

	if (fileId != -1) {
		_close(fileId);
#ifdef OBSOLETE
		if (tType != SERVER_HTTP && p2p_appID != MSN_APPID_FILE && !(std.flags & PFTS_SENDING))
			proto->p2p_pictureTransferFailed(this);
#endif
	}

	if (!bCompleted && p2p_appID == MSN_APPID_FILE) {
		std.ptszFiles = NULL;
		std.totalFiles = 0;
		proto->ProtoBroadcastAck(std.hContact, ACKTYPE_FILE, ACKRESULT_FAILED, this, 0);
	}

	mir_free(p2p_branch);
	mir_free(p2p_callID);
	mir_free(p2p_dest);
	mir_free(p2p_object);

	mir_free(std.tszCurrentFile);
	mir_free(std.tszWorkingDir);
	if (std.ptszFiles != NULL) {
		for (int i = 0; std.ptszFiles[i]; i++)
			mir_free(std.ptszFiles[i]);
		mir_free(std.ptszFiles);
	}

	mir_free(szInvcookie);
}

void filetransfer::close(void)
{
	if (fileId != -1) _close(fileId);
	fileId = -1;
}

void filetransfer::complete(void)
{
	close();

	bCompleted = true;
	proto->ProtoBroadcastAck(std.hContact, ACKTYPE_FILE, ACKRESULT_SUCCESS, this, 0);
}

int filetransfer::create(void)
{
	int flags = _O_BINARY | _O_CREAT | _O_WRONLY | _O_APPEND;

	if (std.currentFileProgress == 0) flags |= _O_TRUNC;
	fileId = _wopen(std.tszCurrentFile, flags, _S_IREAD | _S_IWRITE);

	if (fileId == -1)
		proto->MSN_ShowError("Cannot create file '%s' during a file transfer", std.tszCurrentFile);
	//	else if (std.currentFileSize != 0)
	//		_chsize(fileId, std.currentFileSize);

	return fileId;
}

int filetransfer::openNext(void)
{
	if (fileId != -1) {
		close();
		++std.currentFileNumber;
		++cf;
	}

	while (std.ptszFiles && std.ptszFiles[cf]) {
		struct _stati64 statbuf;
		if (_wstat64(std.ptszFiles[cf], &statbuf) == 0 && (statbuf.st_mode & _S_IFDIR) == 0)
			break;

		++cf;
	}

	if (std.ptszFiles && std.ptszFiles[cf]) {
		bCompleted = false;
		replaceStrW(std.tszCurrentFile, std.ptszFiles[cf]);
		fileId = _wopen(std.tszCurrentFile, _O_BINARY | _O_RDONLY, _S_IREAD);
		if (fileId != -1) {
			std.currentFileSize = _filelengthi64(fileId);
			std.currentFileProgress = 0;

			p2p_sendmsgid = 0;
			p2p_byemsgid = 0;
			tType = SERVER_NOTIFICATION;
			bAccepted = false;

			mir_free(p2p_branch); p2p_branch = NULL;
			mir_free(p2p_callID); p2p_callID = NULL;
		}
		else
			proto->MSN_ShowError("Unable to open file '%s' for the file transfer, error %d", std.tszCurrentFile, errno);
	}

	return fileId;
}

#ifdef OBSOLETE
directconnection::directconnection(const char* CallID, const char* Wlid)
{
	memset(this, 0, sizeof(directconnection));

	wlid = mir_strdup(Wlid);
	callId = mir_strdup(CallID);
	mNonce = (UUID*)mir_alloc(sizeof(UUID));
	UuidCreate(mNonce);
	ts = time(NULL);
}

directconnection::~directconnection()
{
	mir_free(wlid);
	mir_free(callId);
	mir_free(mNonce);
	mir_free(xNonce);
}


char* directconnection::calcHashedNonce(UUID* nonce)
{
	mir_sha1_ctx sha1ctx;
	BYTE sha[MIR_SHA1_HASH_SIZE];

	mir_sha1_init(&sha1ctx);
	mir_sha1_append(&sha1ctx, (BYTE*)nonce, sizeof(UUID));
	mir_sha1_finish(&sha1ctx, sha);

	char* p;
	UuidToStringA((UUID*)&sha, (BYTE**)&p);
	size_t len = mir_strlen(p) + 3;
	char* result = (char*)mir_alloc(len);
	mir_snprintf(result, len, "{%s}", p);
	_strupr(result);
	RpcStringFreeA((BYTE**)&p);

	return result;
}

char* directconnection::mNonceToText(void)
{
	char* p;
	UuidToStringA(mNonce, (BYTE**)&p);
	size_t len = mir_strlen(p) + 3;
	char* result = (char*)mir_alloc(len);
	mir_snprintf(result, len, "{%s}", p);
	_strupr(result);
	RpcStringFreeA((BYTE**)&p);

	return result;
}


void directconnection::xNonceToBin(UUID* nonce)
{
	size_t len = mir_strlen(xNonce);
	char *p = (char*)alloca(len);
	mir_strcpy(p, xNonce + 1);
	p[len - 2] = 0;
	UuidFromStringA((BYTE*)p, nonce);
}
#endif

/////////////////////////////////////////////////////////////////////////////////////////
// TWinErrorCode class

TWinErrorCode::TWinErrorCode() : mErrorText(NULL)
{
	mErrorCode = ::GetLastError();
}

TWinErrorCode::~TWinErrorCode()
{
	mir_free(mErrorText);
}

char* TWinErrorCode::getText()
{
	if (mErrorText == NULL)
		return NULL;

	int tBytes = 0;
	mErrorText = (char*)mir_alloc(256);

	if (tBytes == 0)
		tBytes = FormatMessageA(
		FORMAT_MESSAGE_FROM_SYSTEM, NULL,
		mErrorCode, LANG_NEUTRAL, mErrorText, 256, NULL);

	if (tBytes == 0)
		tBytes = mir_snprintf(mErrorText, 256, "unknown Windows error code %d", mErrorCode);

	*mErrorText = (char)tolower(*mErrorText);

	if (mErrorText[tBytes - 1] == '\n')
		mErrorText[--tBytes] = 0;
	if (mErrorText[tBytes - 1] == '\r')
		mErrorText[--tBytes] = 0;
	if (mErrorText[tBytes - 1] == '.')
		mErrorText[tBytes - 1] = 0;

	return mErrorText;
}

bool CMsnProto::MSN_IsMyContact(MCONTACT hContact)
{
	const char* szProto = GetContactProto(hContact);
	return szProto != NULL && mir_strcmp(m_szModuleName, szProto) == 0;
}

bool CMsnProto::MSN_IsMeByContact(MCONTACT hContact, char* szEmail)
{
	char tEmail[MSN_MAX_EMAIL_LEN];
	char *emailPtr = szEmail ? szEmail : tEmail;

	*emailPtr = 0;
	if (db_get_static(hContact, m_szModuleName, "wlid", emailPtr, sizeof(tEmail)) &&
		db_get_static(hContact, m_szModuleName, "e-mail", emailPtr, sizeof(tEmail)))
		return false;

	return _stricmp(emailPtr, MyOptions.szEmail) == 0;
}

bool MSN_MsgWndExist(MCONTACT hContact)
{
	MessageWindowInputData msgWinInData =
	{ sizeof(MessageWindowInputData), hContact, MSG_WINDOW_UFLAG_MSG_BOTH };
	MessageWindowData msgWinData = { 0 };
	msgWinData.cbSize = sizeof(MessageWindowData);

	bool res = CallService(MS_MSG_GETWINDOWDATA, (WPARAM)&msgWinInData, (LPARAM)&msgWinData) != 0;
	res = res || msgWinData.hwndWindow;
	if (res) {
		msgWinInData.hContact = db_mc_getMeta(hContact);
		if (msgWinInData.hContact != NULL) {
			res = CallService(MS_MSG_GETWINDOWDATA, (WPARAM)&msgWinInData, (LPARAM)&msgWinData) != 0;
			res |= (msgWinData.hwndWindow == NULL);
		}
	}
	return res;
}

void MSN_MakeDigest(const char* chl, char* dgst)
{
	//Digest it
	DWORD md5hash[4], md5hashOr[4];
	mir_md5_state_t context;
	mir_md5_init(&context);
	mir_md5_append(&context, (BYTE*)chl, (int)mir_strlen(chl));
	mir_md5_append(&context, (BYTE*)msnProtChallenge, (int)mir_strlen(msnProtChallenge));
	mir_md5_finish(&context, (BYTE*)md5hash);

	memcpy(md5hashOr, md5hash, sizeof(md5hash));

	size_t i;
	for (i = 0; i < 4; i++)
		md5hash[i] &= 0x7FFFFFFF;

	char chlString[128];
	mir_snprintf(chlString, "%s%s00000000", chl, msnProductID);
	chlString[(mir_strlen(chl) + mir_strlen(msnProductID) + 7) & 0xF8] = 0;

	LONGLONG high = 0, low = 0;
	int* chlStringArray = (int*)chlString;
	for (i = 0; i < mir_strlen(chlString) / 4; i += 2) {
		LONGLONG temp = chlStringArray[i];

		temp = (0x0E79A9C1 * temp) % 0x7FFFFFFF;
		temp += high;
		temp = md5hash[0] * temp + md5hash[1];
		temp = temp % 0x7FFFFFFF;

		high = chlStringArray[i + 1];
		high = (high + temp) % 0x7FFFFFFF;
		high = md5hash[2] * high + md5hash[3];
		high = high % 0x7FFFFFFF;

		low = low + high + temp;
	}
	high = (high + md5hash[1]) % 0x7FFFFFFF;
	low = (low + md5hash[3]) % 0x7FFFFFFF;

	md5hashOr[0] ^= high;
	md5hashOr[1] ^= low;
	md5hashOr[2] ^= high;
	md5hashOr[3] ^= low;

	char* str = arrayToHex((PBYTE)md5hashOr, sizeof(md5hashOr));
	mir_strcpy(dgst, str);
	mir_free(str);
}

char* GetGlobalIp(void)
{
	NETLIBIPLIST* ihaddr = (NETLIBIPLIST*)CallService(MS_NETLIB_GETMYIP, 1, 0);
	for (unsigned i = 0; i < ihaddr->cbNum; ++i)
		if (strchr(ihaddr->szIp[i], ':'))
			return mir_strdup(ihaddr->szIp[i]);

	mir_free(ihaddr);
	return NULL;
}
