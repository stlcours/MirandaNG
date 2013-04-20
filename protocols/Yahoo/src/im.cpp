/*
 * $Id: im.cpp 12307 2010-08-11 21:49:46Z Michael.Kunz@s2005.TU-Chemnitz.de $
 *
 * myYahoo Miranda Plugin 
 *
 * Authors: Gennady Feldman (aka Gena01) 
 *          Laurent Marechal (aka Peorth)
 *
 * This code is under GPL and is based on AIM, MSN and Miranda source code.
 * I want to thank Robert Rainwater and George Hazan for their code and support
 * and for answering some of my questions during development of this plugin.
 */

#include "yahoo.h"
#include <m_langpack.h>
#include <m_protosvc.h>

#include "avatar.h"
#include "im.h"
#include "ignore.h"

void CYahooProto::send_msg(const char *id, int protocol, const char *msg, int utf8)
{
	LOG(("[send_msg] Who: %s: protocol: %d Msg: '%s', utf: %d", id, protocol, msg, utf8));
	
	int buddy_icon = (GetDword("AvatarHash", 0) != 0) ? 2: 0;
	yahoo_send_im(m_id, NULL, id, protocol, msg, utf8, buddy_icon);
}

void CYahooProto::ext_got_im(const char *me, const char *who, int protocol, const char *msg, 
								long tm, int stat, int utf8, int buddy_icon, 
								const char *seqn, int sendn)
{
	char 		*umsg;
	const char	*c = msg;
	int 		oidx = 0;

	LOG(("YAHOO_GOT_IM id:%s %s: %s (len: %d) tm:%lu stat:%i utf8:%i buddy_icon: %i", me, who, msg, lstrlenA(msg), tm, stat, utf8, buddy_icon));

	if (stat == 2) {
		char z[1024];

		snprintf(z, sizeof z, "Error sending message to %s", who);
		LOG((z));
		ShowError( TranslateT("Yahoo Error"), _A2T(z));
		return;
	}

	if (!msg) {
		LOG(("Empty Incoming Message, exiting."));
		return;
	}

	if (GetByte("IgnoreUnknown", 0)) {

		/*
		* Check our buddy list to see if we have it there. And if it's not on the list then we don't accept any IMs.
		*/
		if (getbuddyH(who) == NULL) {
			LOG(("Ignoring unknown user messages. User '%s'. Dropping Message.", who));
			return;
		}
	}

	if ( BuddyIgnored( who )) {
		LOG(("User '%s' on our Ignore List. Dropping Message.", who));
		return;
	}

	// make a bigger buffer for \n -> \r\n conversion (x2)
	umsg = (char *) alloca(lstrlenA(msg) * 2 + 1); 

	while ( *c != '\0') {
		// Strip the font tag
		if (!_strnicmp(c,"<font ",6) || !_strnicmp(c,"</font>",6) ||
			// strip the fade tag
			!_strnicmp(c, "<FADE ",6) || !_strnicmp(c,"</FADE>",7) ||
			// strip the alternate colors tag
			!_strnicmp(c, "<ALT ",5) || !_strnicmp(c, "</ALT>",6)) { 
				while ((*c++ != '>') && (*c != '\0')); 
		} else
			// strip ANSI color combination
			if ((*c == 0x1b) && (*(c+1) == '[')) { 
				while ((*c++ != 'm') && (*c != '\0')); 
			} else

				if (*c != '\0') {
					umsg[oidx++] = *c;

					/* Adding \r to \r\n conversion */
					if (*c == '\r' && *(c + 1) != '\n') 
						umsg[oidx++] = '\n';

					c++;
				}
	}

	umsg[oidx++]= '\0';

	/* Need to strip off formatting stuff first. Then do all decoding/converting */
	LOG(("%s: %s", who, umsg));

	HANDLE hContact = add_buddy(who, who, protocol, PALF_TEMPORARY);
	//SetWord(hContact, "yprotoid", protocol);
	Set_Protocol(hContact, protocol);

	PROTORECVEVENT pre;
	pre.flags = (utf8) ? PREF_UTF : 0;

	if (tm) {
		HANDLE hEvent = db_event_last(hContact);

		if (hEvent) { // contact has events
			DWORD dummy;
			DBEVENTINFO dbei = { sizeof (dbei) };
			dbei.pBlob = (BYTE*)&dummy;
			dbei.cbBlob = 2;
			if (!db_event_get(hEvent, &dbei)) 
				// got that event, if newer than ts then reset to current time
				if ((DWORD)tm < dbei.timestamp) tm = (long)time(NULL);
		}

		pre.timestamp = (DWORD)time(NULL);
		
		if ((DWORD)tm < pre.timestamp)
			pre.timestamp = tm;
		
	}
	else pre.timestamp = (DWORD)time(NULL);

	pre.szMessage = umsg;
	pre.lParam = 0;

	// Turn off typing
	CallService(MS_PROTO_CONTACTISTYPING, (WPARAM) hContact, PROTOTYPE_CONTACTTYPING_OFF);
	ProtoChainRecvMsg(hContact, &pre);

	// ack the message we just got
	if (seqn)
		yahoo_send_im_ack(m_id, me, who, seqn, sendn);

	if (buddy_icon < 0) return;

	//?? Don't generate floods!!
	db_set_b(hContact, m_szModuleName, "AvatarType", (BYTE)buddy_icon);
	if (buddy_icon != 2) {
		reset_avatar(hContact);
	} else if (db_get_dw(hContact, m_szModuleName,"PictCK", 0) == 0) {
		/* request the buddy image */
		request_avatar(who); 
	} 
}

////////////////////////////////////////////////////////////////////////////////////////
// SendMessage - sends a message

void __cdecl CYahooProto::im_sendacksuccess(HANDLE hContact)
{
	ProtoBroadcastAck(m_szModuleName, hContact, ACKTYPE_MESSAGE, ACKRESULT_SUCCESS, (HANDLE) 1, 0);
}

void __cdecl CYahooProto::im_sendackfail(HANDLE hContact)
{
	SleepEx(1000, TRUE);
	ProtoBroadcastAck(m_szModuleName, hContact, ACKTYPE_MESSAGE, ACKRESULT_FAILED, (HANDLE) 1, 
						(LPARAM) Translate("The message send timed out."));
}

void __cdecl CYahooProto::im_sendackfail_longmsg(HANDLE hContact)
{
	SleepEx(1000, TRUE);
	ProtoBroadcastAck(m_szModuleName, hContact, ACKTYPE_MESSAGE, ACKRESULT_FAILED, (HANDLE) 1, 
						(LPARAM)Translate("Message is too long: Yahoo messages are limited by 800 UTF8 chars"));
}

int __cdecl CYahooProto::SendMsg( HANDLE hContact, int flags, const char* pszSrc )
{
	if (!m_bLoggedIn) {/* don't send message if we not connected! */
		YForkThread( &CYahooProto::im_sendackfail, hContact );
		return 1;
	}

	mir_ptr<char> msg;
	if (flags & PREF_UNICODE) /* convert to utf8 */
		msg = mir_utf8encodeW(( wchar_t* )&pszSrc[ strlen(pszSrc)+1 ] );
	else if ( flags & PREF_UTF )
		msg = mir_strdup(pszSrc);
	else
		msg = mir_utf8encode(pszSrc);

	if (lstrlenA(msg) > 800) {
		YForkThread( &CYahooProto::im_sendackfail_longmsg, hContact );
		return 1;
	}

	DBVARIANT dbv;
	if (!GetString( hContact, YAHOO_LOGINID, &dbv)) {
		send_msg(dbv.pszVal, GetWord( hContact, "yprotoid", 0), msg, 1);

		YForkThread( &CYahooProto::im_sendacksuccess, hContact );

		db_free(&dbv);
		return 1;
	}

	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////
// RecvMsg

int __cdecl CYahooProto::RecvMsg( HANDLE hContact, PROTORECVEVENT* pre )
{
	db_unset(hContact, "CList", "Hidden");

	// NUDGES
	if ( !lstrcmpA(pre->szMessage, "<ding>")  && ServiceExists("NUDGE/Send")) {
		DebugLog("[YahooRecvMessage] Doing Nudge Service!");
		NotifyEventHooks(hYahooNudge, (WPARAM)hContact, pre->timestamp);
		return 0;
	} 

	return Proto_RecvMessage(hContact, pre);
}

//=======================================================
//Send a nudge
//=======================================================

INT_PTR __cdecl CYahooProto::SendNudge(WPARAM wParam, LPARAM lParam)
{
	HANDLE hContact = (HANDLE) wParam;

	DebugLog("[YAHOO_SENDNUDGE]");

	if (!m_bLoggedIn) {/* don't send nudge if we not connected! */
		YForkThread( &CYahooProto::im_sendackfail, hContact );
		return 1;
	}

	DBVARIANT dbv;
	if (!GetString(hContact, YAHOO_LOGINID, &dbv)) {
		send_msg(dbv.pszVal, GetWord(hContact, "yprotoid", 0), "<ding>", 0);
		db_free(&dbv);

		YForkThread( &CYahooProto::im_sendacksuccess, hContact );
		return 1;
	}

	return 0;
}



