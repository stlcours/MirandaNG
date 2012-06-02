//***********************************************************
//	Copyright  2003-2008 Alexander S. Kiselev, Valentin Pavlyuchenko
//
//	This file is part of Boltun.
//
//    Boltun is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 2 of the License, or
//    (at your option) any later version.
//
//    Boltun is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//	  along with Boltun. If not, see <http://www.gnu.org/licenses/>.
//
//***********************************************************

#include "actionQueue.h"
#include "config.h"
#include "boltun.h"
#include "Engine/tstring.h"
#include "Engine/TalkEngine.h"

#include <list>
#include <set>
#include <time.h>
#include "Engine/CriticalSection.h"
#ifdef _DEBUG
#include <assert.h>
#endif

#define MIRANDA_VER 0x0A00
#include "newpluginapi.h"
#include "m_database.h"
#include "m_system.h"
#include "m_protosvc.h"

using namespace std;

extern TalkBot* bot;

typedef void (*ActionHandler)(HANDLE hContact, const TalkBot::MessageInfo *inf);

typedef struct _QueueElement {
	HANDLE hContact;
	const TalkBot::MessageInfo *inf;
	ActionHandler Handler;
	bool Sticky;
	int TimeOffset;
	_QueueElement(HANDLE contact, ActionHandler handler, int timeOffset, const TalkBot::MessageInfo *info = NULL, bool sticky = false)
		:hContact(contact), Handler(handler), TimeOffset(timeOffset), inf(info), Sticky(sticky)
	{
	}
} QueueElement;

static list<QueueElement> actionQueue;
static set<HANDLE> typingContacts;
UINT_PTR timerID = 0;

CriticalSection cs;
CriticalSection typingContactsLock;

void UpdateTimer();

VOID CALLBACK TimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	cs.Enter();
	QueueElement q = actionQueue.front();
	actionQueue.pop_front();
	UpdateTimer();
	cs.Leave();
	q.Handler(q.hContact, q.inf);
}

void UpdateTimer()
{
	if (timerID)
		KillTimer(NULL, timerID);
	if (actionQueue.size())
		timerID = SetTimer(NULL, 0, actionQueue.front().TimeOffset, TimerProc);	
	else
		timerID = 0;
}

static bool NotifyTyping(HANDLE hContact)
{
	int res = DBGetContactSettingByte(hContact, "SRMsg", "SupportTyping", 2);
	if (res == 2)
		res = DBGetContactSettingByte(NULL, "SRMsg", "DefaultTyping", 1);
	return res != 0;
}

static char *MsgServiceName(HANDLE hContact)
{
#ifdef _UNICODE
    char szServiceName[100];
    char *szProto = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) hContact, 0);
    if (szProto == NULL)
        return PSS_MESSAGE;

    mir_snprintf(szServiceName, sizeof(szServiceName), "%s%sW", szProto, PSS_MESSAGE);
    if (ServiceExists(szServiceName))
        return PSS_MESSAGE "W";
#endif
    return PSS_MESSAGE;
}

static void TimerAnswer(HANDLE hContact, const TalkBot::MessageInfo* info)
{
	DBEVENTINFO ldbei;
	int size = (int)info->Answer.length() + 1;
	int bufsize = size;
	char* msg;
#ifdef UNICODE
	bufsize *= sizeof(TCHAR) + 1;
	msg = new char[bufsize];
	//msg[size - 1] = '\0';
	
	if (!WideCharToMultiByte(CP_ACP, 0, info->Answer.c_str(), -1, msg, size, 
		NULL, NULL))
		FillMemory(msg, size - 1, '-'); //In case of fault return "----" in ANSI part
	CopyMemory(msg + size, info->Answer.c_str(), size * 2);
#else
	msg = respItem->szMes;
#endif

	CallContactService(hContact, MsgServiceName(hContact), PREF_TCHAR, (LPARAM)msg);

	ZeroMemory(&ldbei, sizeof(ldbei));
	ldbei.cbSize    = sizeof(ldbei);
	//FIXME: Error may happen
	ldbei.cbBlob    = bufsize;
	ldbei.pBlob     = (PBYTE)(void*)msg;
	ldbei.eventType = EVENTTYPE_MESSAGE;
	ldbei.flags     = DBEF_SENT;
	ldbei.szModule  = BOLTUN_NAME;
	ldbei.timestamp = (DWORD)time(NULL);

	CallService(MS_DB_EVENT_ADD, (WPARAM)hContact, (LPARAM)&ldbei);
	bot->AnswerGiven(hContact, *info);
	delete info;
#ifdef UNICODE
	delete msg;
#endif
	typingContactsLock.Enter();
	typingContacts.erase(hContact);
	typingContactsLock.Leave();
}

static void StartTyping(HANDLE hContact, const TalkBot::MessageInfo*)
{
	CallService(MS_PROTO_SELFISTYPING, (WPARAM)hContact, 
		(LPARAM)PROTOTYPE_SELFTYPING_ON);
	typingContactsLock.Enter();
	typingContacts.insert(hContact);
	typingContactsLock.Leave();
}

void DoAnswer(HANDLE hContact, const TalkBot::MessageInfo *info, bool sticky = false)
{
	if (info->Answer[0] == _T('\0'))
		return;
	int waitTime, thinkTime = 0;
	int defWaitTime = Config.AnswerPauseTime * 1000;
	if (Config.PauseDepends)
		waitTime = defWaitTime * (int)info->Answer.length() / 25;
	else
		waitTime = defWaitTime;
	if (Config.PauseRandom)
	{
		//Let it be up to 4 times longer.
		waitTime =  waitTime * (rand() % 300) / 100 + waitTime;
	}
	if (waitTime == 0)
		waitTime = 50; //it's essential, because otherwise message will be added later 
					   //then its response, that will cause incorrect ordering of 
					   //messages in the opened history (reopening will
					   //help, but anyway it's no good)
	if (NotifyTyping(hContact) && Config.AnswerThinkTime)
	{
		thinkTime = Config.AnswerThinkTime * 1000;
		if (Config.PauseRandom)
		{
			//Let it be up to 4 times longer.
			thinkTime = thinkTime * (rand() % 300) / 100 + thinkTime;
		}				
	}
	cs.Enter();
	//Check if this contact's timer handler is now waiting for a cs.
	bool needTimerRearrange = false;
	if (!actionQueue.empty() && actionQueue.front().hContact == hContact)
	{
		needTimerRearrange = true;
		KillTimer(NULL, timerID);
		cs.Leave();
		cs.Enter();
	}
	if (!actionQueue.empty())
	{
		list<QueueElement>::iterator it = actionQueue.end();
		it--;
		while (true)
		{
			if ((*it).hContact == hContact)
			{
				if ((*it).Sticky)
					break;
				list<QueueElement>::iterator tmp = it;
				if (tmp != actionQueue.begin())
					tmp--;
				actionQueue.erase(it);
				it = tmp;
				if (actionQueue.empty())
					break;
			}
			if (it == actionQueue.begin())
				break;
			it--;
		}
	}
	typingContactsLock.Enter();
	if (typingContacts.find(hContact) != typingContacts.end())
	{
		CallService(MS_PROTO_SELFISTYPING, (WPARAM)hContact, (LPARAM)PROTOTYPE_SELFTYPING_OFF);
		typingContacts.erase(hContact);
	}
	typingContactsLock.Leave();
	if (actionQueue.empty())
		needTimerRearrange = true;
	if (thinkTime)
		actionQueue.push_back(QueueElement(hContact, StartTyping, thinkTime, NULL, sticky));
	actionQueue.push_back(QueueElement(hContact, TimerAnswer, waitTime, info, sticky));
	if (needTimerRearrange)
		UpdateTimer();
	cs.Leave();
}

void AnswerToContact(HANDLE hContact, const TCHAR* messageToAnswer)
{
	if (Config.TalkWarnContacts && DBGetContactSettingByte(hContact, BOLTUN_KEY, 
		DB_CONTACT_WARNED, FALSE) == FALSE)
	{
		DoAnswer(hContact, new TalkBot::MessageInfo((const TCHAR*)Config.WarnText), true);
		DBWriteContactSettingByte(hContact, BOLTUN_KEY, DB_CONTACT_WARNED, TRUE);
	}
	else
		DoAnswer(hContact, bot->Reply(hContact, messageToAnswer, false));
}

void StartChatting(HANDLE hContact)
{
	DoAnswer(hContact, new TalkBot::MessageInfo(bot->GetInitMessage(hContact)), true);
}
