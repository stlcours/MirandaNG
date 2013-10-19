/* 
Copyright (C) 2006 Ricardo Pescuma Domenecci, Nightwish

This is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with this file; see the file license.txt.  If
not, write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  
*/

#include "commonheaders.h"

/*
It has 1 queue:
A queue to request items. One request is done at a time, REQUEST_WAIT_TIME miliseconts after it has beeing fired
   ACKRESULT_STATUS. This thread only requests the avatar (and maybe add it to the cache queue)
*/

#define REQUEST_WAIT_TIME 3000

// Time to wait before re-requesting an avatar that failed
#define REQUEST_FAIL_WAIT_TIME (3 * 60 * 60 * 1000)

// Time to wait before re-requesting an avatar that received an wait for
#define REQUEST_WAITFOR_WAIT_TIME (30 * 60 * 1000)

// Number of mileseconds the threads wait until take a look if it is time to request another item
#define POOL_DELAY 1000

// Number of mileseconds the threads wait after a GAIR_WAITFOR is returned
#define REQUEST_DELAY 18000


// Prototypes ///////////////////////////////////////////////////////////////////////////

static void RequestThread(void *vParam);

extern HANDLE hShutdownEvent;
extern int DeleteAvatar(HANDLE hContact);
extern void MakePathRelative(HANDLE hContact, TCHAR *path);
int Proto_GetDelayAfterFail(const char *proto);
BOOL Proto_IsFetchingAlwaysAllowed(const char *proto);

#ifdef _DEBUG
int _DebugTrace(const char *fmt, ...);
int _DebugTrace(HANDLE hContact, const char *fmt, ...);
#endif

// Functions ////////////////////////////////////////////////////////////////////////////

// Items with higher priority at end
static int QueueSortItems(const QueueItem *p1, const QueueItem *p2)
{
	return p2->check_time - p1->check_time; // sort backwards
}

static OBJLIST<QueueItem> queue(20, QueueSortItems);
static CRITICAL_SECTION cs;
static int waitTime;

void InitPolls() 
{
	waitTime = REQUEST_WAIT_TIME;
	InitializeCriticalSection(&cs);

	// Init request queue
	mir_forkthread(RequestThread, NULL);
}

void UninitPolls() 
{
	DeleteCriticalSection(&cs);
	queue.destroy();
}

// Return true if this protocol can have avatar requested
static BOOL PollProtocolCanHaveAvatar(const char *szProto)
{
	int pCaps = CallProtoService(szProto, PS_GETCAPS, PFLAGNUM_4, 0);
	int status = CallProtoService(szProto, PS_GETSTATUS, 0, 0);
	return (pCaps & PF4_AVATARS)
		&& (g_szMetaName == NULL || strcmp(g_szMetaName, szProto))
		&& ((status > ID_STATUS_OFFLINE && status != ID_STATUS_INVISIBLE) || Proto_IsFetchingAlwaysAllowed(szProto));
}

// Return true if this protocol has to be checked
static BOOL PollCheckProtocol(const char *szProto)
{
	return db_get_b(NULL, AVS_MODULE, szProto, 1);
}

// Return true if this contact can have avatar requested
static BOOL PollContactCanHaveAvatar(HANDLE hContact, const char *szProto)
{
	int status = db_get_w(hContact, szProto, "Status", ID_STATUS_OFFLINE);
	return (Proto_IsFetchingAlwaysAllowed(szProto) || status != ID_STATUS_OFFLINE)
		&& !db_get_b(hContact, "CList", "NotOnList", 0) && db_get_b(hContact, "CList", "ApparentMode", 0) != ID_STATUS_OFFLINE;
}

// Return true if this contact has to be checked
static BOOL PollCheckContact(HANDLE hContact, const char *szProto)
{
	return !db_get_b(hContact, "ContactPhoto", "Locked", 0) && FindAvatarInCache(hContact, FALSE, TRUE) != NULL;
}

static void QueueRemove(HANDLE hContact)
{
	mir_cslock lck(cs);

	for (int i = queue.getCount()-1 ; i >= 0 ; i-- ) {
		QueueItem& item = queue[i];
		if (item.hContact == hContact)
			queue.remove(i);
	}
}

static void QueueAdd(HANDLE hContact, int waitTime)
{
	if (fei == NULL || g_shutDown)
		return;

	mir_cslock lck(cs);

	// Only add if not exists yet
	for (int i = queue.getCount()-1; i >= 0; i--)
		if (queue[i].hContact == hContact)
			return;

	QueueItem *item = new QueueItem;
	item->hContact = hContact;
	item->check_time = GetTickCount() + waitTime;
	queue.insert(item);
}

// Add an contact to a queue
void QueueAdd(HANDLE hContact)
{
	QueueAdd(hContact, waitTime);
}

void ProcessAvatarInfo(HANDLE hContact, int type, PROTO_AVATAR_INFORMATIONT *pai, const char *szProto)
{
	QueueRemove(hContact);

	if (type == GAIR_SUCCESS) {
		if (pai == NULL)
			return;

		// Fix settings in DB
		db_unset(hContact, "ContactPhoto", "NeedUpdate");
		db_unset(hContact, "ContactPhoto", "RFile");
		if (!db_get_b(hContact, "ContactPhoto", "Locked", 0))
			db_unset(hContact, "ContactPhoto", "Backup");
		db_set_ts(hContact, "ContactPhoto", "File", pai->filename);
		db_set_w(hContact, "ContactPhoto", "Format", pai->format);

		if (pai->format == PA_FORMAT_PNG || pai->format == PA_FORMAT_JPEG 
			|| pai->format == PA_FORMAT_ICON  || pai->format == PA_FORMAT_BMP
			|| pai->format == PA_FORMAT_GIF) {
			// We can load it!
			MakePathRelative(hContact, pai->filename);
			ChangeAvatar(hContact, TRUE, TRUE, pai->format);
		}
		else {
			// As we can't load it, notify but don't load
			ChangeAvatar(hContact, FALSE, TRUE, pai->format);
		}
	}
	else if (type == GAIR_NOAVATAR) {
		db_unset(hContact, "ContactPhoto", "NeedUpdate");

		if (db_get_b(NULL, AVS_MODULE, "RemoveAvatarWhenContactRemoves", 1)) {
			// Delete settings
			db_unset(hContact, "ContactPhoto", "RFile");
			if (!db_get_b(hContact, "ContactPhoto", "Locked", 0))
				db_unset(hContact, "ContactPhoto", "Backup");
			db_unset(hContact, "ContactPhoto", "File");
			db_unset(hContact, "ContactPhoto", "Format");

			// Fix cache
			ChangeAvatar(hContact, FALSE, TRUE, 0);
		}
	}
	else if (type == GAIR_FAILED) {
		int wait = Proto_GetDelayAfterFail(szProto);
		if (wait > 0) {
			// Reschedule to request after needed time (and avoid requests before that)
			mir_cslock lock(cs);
			QueueRemove(hContact);
			QueueAdd(hContact, wait);
		}
	}
}

int FetchAvatarFor(HANDLE hContact, char *szProto)
{
	int result = GAIR_NOAVATAR;

	if (szProto == NULL)
		szProto = GetContactProto(hContact);

	if (szProto != NULL && PollProtocolCanHaveAvatar(szProto) && PollContactCanHaveAvatar(hContact, szProto)) {
		// Can have avatar, but must request it?
		if ((g_AvatarHistoryAvail && CallService(MS_AVATARHISTORY_ENABLED, (WPARAM) hContact, 0))
			 || (PollCheckProtocol(szProto) && PollCheckContact(hContact, szProto))) 
		{
			// Request it
			PROTO_AVATAR_INFORMATIONT pai_s = {0};
			pai_s.cbSize = sizeof(pai_s);
			pai_s.hContact = hContact;
			INT_PTR res = CallProtoService(szProto, PS_GETAVATARINFOT, GAIF_FORCE, (LPARAM)&pai_s);
			if (res == CALLSERVICE_NOTFOUND) {
				PROTO_AVATAR_INFORMATION pai = {0};
				pai.cbSize = sizeof(pai);
				pai.hContact = hContact;
				res = CallProtoService(szProto, PS_GETAVATARINFO, GAIF_FORCE, (LPARAM)&pai);
				MultiByteToWideChar( CP_ACP, 0, pai.filename, -1, pai_s.filename, SIZEOF(pai_s.filename));
				pai_s.format = pai.format;
			}

			if (res != CALLSERVICE_NOTFOUND) result = res;
			ProcessAvatarInfo(pai_s.hContact, result, &pai_s, szProto);
		}
	}

	return result;
}

static void RequestThread(void *vParam)
{
	while (!g_shutDown) {
		EnterCriticalSection(&cs);

		if ( queue.getCount() == 0 ) {
			// No items, so suspend thread
			LeaveCriticalSection(&cs);
			mir_sleep(POOL_DELAY);
			continue;
		}

		// Take a look at first item
		QueueItem& qi = queue[ queue.getCount()-1 ];
		if (qi.check_time > GetTickCount()) {
			// Not time to request yet, wait...
			LeaveCriticalSection(&cs);
			mir_sleep(POOL_DELAY);
			continue;
		}

		// Will request this item
		HANDLE hContact = qi.hContact;
		queue.remove( queue.getCount()-1 );
		QueueRemove(hContact);
		LeaveCriticalSection(&cs);

		if (FetchAvatarFor(hContact) == GAIR_WAITFOR) {
			// Mark to not request this contact avatar for more 30 min
			{
				mir_cslock lock(cs);
				QueueRemove(hContact);
				QueueAdd(hContact, REQUEST_WAITFOR_WAIT_TIME);
			}

			// Wait a little until requesting again
			mir_sleep(REQUEST_DELAY);
		}
	}
}
