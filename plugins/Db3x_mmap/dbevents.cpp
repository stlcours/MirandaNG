/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2003 Miranda ICQ/IM project,
all portions of this codebase are copyrighted to the people
listed in contributors.txt.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "commonheaders.h"

extern BOOL safetyMode;

DWORD GetModuleNameOfs(const char *szName);
char *GetModuleNameByOfs(DWORD ofs);

static HANDLE hEventDeletedEvent,hEventAddedEvent,hEventFilterAddedEvent;

STDMETHODIMP_(LONG) CDdxMmap::GetEventCount(HANDLE hContact)
{
	LONG ret;

	EnterCriticalSection(&csDbAccess);
	if (hContact == 0)
		hContact = (HANDLE)dbHeader.ofsUser;
	DBContact *dbc = (DBContact*)DBRead(hContact,sizeof(DBContact),NULL);
	if (dbc->signature != DBCONTACT_SIGNATURE) ret = -1;
	else ret = dbc->eventCount;
	LeaveCriticalSection(&csDbAccess);
	return ret;
}

STDMETHODIMP_(HANDLE) CDdxMmap::AddEvent(HANDLE hContact, DBEVENTINFO *dbei)
{
	DBContact dbc;
	DBEvent dbe,*dbeTest;
	DWORD ofsNew,ofsModuleName,ofsContact,ofsThis;
	BOOL neednotify;

	if (dbei == NULL||dbei->cbSize!=sizeof(DBEVENTINFO)) return 0;
	if (dbei->timestamp == 0) return 0;
	if (NotifyEventHooks(hEventFilterAddedEvent, (WPARAM)hContact, (LPARAM)dbei)) {
		return 0;
	}
	EnterCriticalSection(&csDbAccess);
	if (hContact == 0) ofsContact = dbHeader.ofsUser;
	else ofsContact = (DWORD)hContact;
	dbc = *(DBContact*)DBRead(ofsContact,sizeof(DBContact),NULL);
	if (dbc.signature!=DBCONTACT_SIGNATURE) {
		LeaveCriticalSection(&csDbAccess);
	  	return 0;
	}
	ofsNew = CreateNewSpace(offsetof(DBEvent,blob)+dbei->cbBlob);
	ofsModuleName = GetModuleNameOfs(dbei->szModule);

	dbe.signature = DBEVENT_SIGNATURE;
	dbe.ofsModuleName = ofsModuleName;
	dbe.timestamp = dbei->timestamp;
	dbe.flags = dbei->flags;
	dbe.eventType = dbei->eventType;
	dbe.cbBlob = dbei->cbBlob;
	//find where to put it - sort by timestamp
	if (dbc.eventCount == 0) {
		dbe.ofsPrev = (DWORD)hContact;
		dbe.ofsNext = 0;
		dbe.flags |= DBEF_FIRST;
		dbc.ofsFirstEvent = dbc.ofsLastEvent = ofsNew;
	}
	else {
		dbeTest = (DBEvent*)DBRead(dbc.ofsFirstEvent,sizeof(DBEvent),NULL);
		// Should new event be placed before first event in chain?
		if (dbei->timestamp < dbeTest->timestamp) {
			dbe.ofsPrev = (DWORD)hContact;
			dbe.ofsNext = dbc.ofsFirstEvent;
			dbe.flags |= DBEF_FIRST;
			dbc.ofsFirstEvent = ofsNew;
			dbeTest = (DBEvent*)DBRead(dbe.ofsNext,sizeof(DBEvent),NULL);
			dbeTest->flags &= ~DBEF_FIRST;
			dbeTest->ofsPrev = ofsNew;
			DBWrite(dbe.ofsNext,dbeTest,sizeof(DBEvent));
		}
		else {
			// Loop through the chain, starting at the end
			ofsThis = dbc.ofsLastEvent;
			dbeTest = (DBEvent*)DBRead(ofsThis, sizeof(DBEvent), NULL);
			for (;;) {
				// If the new event's timesstamp is equal to or greater than the
				// current dbevent, it will be inserted after. If not, continue
				// with the previous dbevent in chain.
				if (dbe.timestamp >= dbeTest->timestamp) {
					dbe.ofsPrev = ofsThis;
					dbe.ofsNext = dbeTest->ofsNext;
					dbeTest->ofsNext = ofsNew;
					DBWrite(ofsThis, dbeTest, sizeof(DBEvent));
					if (dbe.ofsNext == 0)
						dbc.ofsLastEvent = ofsNew;
					else {
						dbeTest = (DBEvent*)DBRead(dbe.ofsNext, sizeof(DBEvent), NULL);
						dbeTest->ofsPrev = ofsNew;
						DBWrite(dbe.ofsNext, dbeTest, sizeof(DBEvent));
					}
					break;
				}
				ofsThis = dbeTest->ofsPrev;
				dbeTest = (DBEvent*)DBRead(ofsThis, sizeof(DBEvent), NULL);
			}
		}
	}
	dbc.eventCount++;
	if (!(dbe.flags&(DBEF_READ|DBEF_SENT))) {
		if (dbe.timestamp<dbc.timestampFirstUnread || dbc.timestampFirstUnread == 0) {
			dbc.timestampFirstUnread = dbe.timestamp;
			dbc.ofsFirstUnreadEvent = ofsNew;
		}
		neednotify = TRUE;
	}
	else neednotify = safetyMode;

	DBWrite(ofsContact,&dbc,sizeof(DBContact));
	DBWrite(ofsNew,&dbe,offsetof(DBEvent,blob));
	DBWrite(ofsNew+offsetof(DBEvent,blob),dbei->pBlob,dbei->cbBlob);
	DBFlush(0);

	LeaveCriticalSection(&csDbAccess);
	log1("add event @ %08x",ofsNew);

	// Notify only in safe mode or on really new events
	if (neednotify)
		NotifyEventHooks(hEventAddedEvent, (WPARAM)hContact, (LPARAM)ofsNew);

	return (HANDLE)ofsNew;
}

STDMETHODIMP_(BOOL) CDdxMmap::DeleteEvent(HANDLE hContact, HANDLE hDbEvent)
{
	DBContact dbc;
	DWORD ofsContact,ofsThis;
	DBEvent dbe,*dbeNext,*dbePrev;

	EnterCriticalSection(&csDbAccess);
	if (hContact == 0) ofsContact = dbHeader.ofsUser;
	else ofsContact = (DWORD)hContact;
	dbc = *(DBContact*)DBRead(ofsContact,sizeof(DBContact),NULL);
	dbe = *(DBEvent*)DBRead(hContact,sizeof(DBEvent),NULL);
	if (dbc.signature!=DBCONTACT_SIGNATURE || dbe.signature!=DBEVENT_SIGNATURE) {
		LeaveCriticalSection(&csDbAccess);
		return 1;
	}
	log1("delete event @ %08x",wParam);
	LeaveCriticalSection(&csDbAccess);
	//call notifier while outside mutex
	NotifyEventHooks(hEventDeletedEvent,(WPARAM)hContact, (LPARAM)hDbEvent);
	//get back in
	EnterCriticalSection(&csDbAccess);
	dbc = *(DBContact*)DBRead(ofsContact,sizeof(DBContact),NULL);
	dbe = *(DBEvent*)DBRead(hDbEvent,sizeof(DBEvent),NULL);
	//check if this was the first unread, if so, recalc the first unread
	if (dbc.ofsFirstUnreadEvent == (DWORD)hDbEvent) {
		dbeNext = &dbe;
		for (;;) {
			if (dbeNext->ofsNext == 0) {
				dbc.ofsFirstUnreadEvent = 0;
				dbc.timestampFirstUnread = 0;
				break;
			}
			ofsThis = dbeNext->ofsNext;
			dbeNext = (DBEvent*)DBRead(ofsThis,sizeof(DBEvent),NULL);
			if (!(dbeNext->flags&(DBEF_READ|DBEF_SENT))) {
				dbc.ofsFirstUnreadEvent = ofsThis;
				dbc.timestampFirstUnread = dbeNext->timestamp;
				break;
			}
		}
	}
	//get previous and next events in chain and change offsets
	if (dbe.flags&DBEF_FIRST) {
		if (dbe.ofsNext == 0) {
			dbc.ofsFirstEvent = dbc.ofsLastEvent = 0;
		}
		else {
			dbeNext = (DBEvent*)DBRead(dbe.ofsNext,sizeof(DBEvent),NULL);
			dbeNext->flags |= DBEF_FIRST;
			dbeNext->ofsPrev = dbe.ofsPrev;
			DBWrite(dbe.ofsNext,dbeNext,sizeof(DBEvent));
			dbc.ofsFirstEvent = dbe.ofsNext;
		}
	}
	else {
		if (dbe.ofsNext == 0) {
			dbePrev = (DBEvent*)DBRead(dbe.ofsPrev,sizeof(DBEvent),NULL);
			dbePrev->ofsNext = 0;
			DBWrite(dbe.ofsPrev,dbePrev,sizeof(DBEvent));
			dbc.ofsLastEvent = dbe.ofsPrev;
		}
		else {
			dbePrev = (DBEvent*)DBRead(dbe.ofsPrev,sizeof(DBEvent),NULL);
			dbePrev->ofsNext = dbe.ofsNext;
			DBWrite(dbe.ofsPrev,dbePrev,sizeof(DBEvent));
			dbeNext = (DBEvent*)DBRead(dbe.ofsNext,sizeof(DBEvent),NULL);
			dbeNext->ofsPrev = dbe.ofsPrev;
			DBWrite(dbe.ofsNext,dbeNext,sizeof(DBEvent));
		}
	}
	//delete event
	DeleteSpace((DWORD)hDbEvent, offsetof(DBEvent,blob)+dbe.cbBlob);
	//decrement event count
	dbc.eventCount--;
	DBWrite(ofsContact,&dbc,sizeof(DBContact));
	DBFlush(0);
	//quit
	LeaveCriticalSection(&csDbAccess);
	return 0;
}

STDMETHODIMP_(LONG) CDdxMmap::GetBlobSize(HANDLE hDbEvent)
{
	INT_PTR ret;
	DBEvent *dbe;

	EnterCriticalSection(&csDbAccess);
	dbe = (DBEvent*)DBRead(hDbEvent, sizeof(DBEvent), NULL);
	if (dbe->signature!=DBEVENT_SIGNATURE) ret = -1;
	else ret = dbe->cbBlob;
	LeaveCriticalSection(&csDbAccess);
	return ret;
}

STDMETHODIMP_(BOOL) CDdxMmap::GetEvent(HANDLE hDbEvent, DBEVENTINFO *dbei)
{
	DBEvent *dbe;
	int bytesToCopy,i;

	if (dbei == NULL||dbei->cbSize!=sizeof(DBEVENTINFO)) return 1;
	if (dbei->cbBlob>0 && dbei->pBlob == NULL) {
		dbei->cbBlob = 0;
		return 1;
	}
	EnterCriticalSection(&csDbAccess);
	dbe = (DBEvent*)DBRead(hDbEvent,sizeof(DBEvent),NULL);
	if (dbe->signature!=DBEVENT_SIGNATURE) {
		LeaveCriticalSection(&csDbAccess);
	  	return 1;
	}
	dbei->szModule = GetModuleNameByOfs(dbe->ofsModuleName);
	dbei->timestamp = dbe->timestamp;
	dbei->flags = dbe->flags;
	dbei->eventType = dbe->eventType;
	if (dbei->cbBlob<dbe->cbBlob) bytesToCopy = dbei->cbBlob;
	else bytesToCopy = dbe->cbBlob;
	dbei->cbBlob = dbe->cbBlob;
	if (bytesToCopy && dbei->pBlob)
	{
		for(i = 0;;i += MAXCACHEDREADSIZE) {
			if (bytesToCopy-i <= MAXCACHEDREADSIZE) {
				CopyMemory(dbei->pBlob+i,DBRead(DWORD(hDbEvent)+offsetof(DBEvent,blob)+i,bytesToCopy-i,NULL),bytesToCopy-i);
				break;
			}
			CopyMemory(dbei->pBlob+i,DBRead(DWORD(hDbEvent)+offsetof(DBEvent,blob)+i,MAXCACHEDREADSIZE,NULL),MAXCACHEDREADSIZE);
		}
	}
	LeaveCriticalSection(&csDbAccess);
	return 0;
}

STDMETHODIMP_(BOOL) CDdxMmap::MarkEventRead(HANDLE hContact, HANDLE hDbEvent)
{
	INT_PTR ret;
	DBEvent *dbe;
	DBContact dbc;
	DWORD ofsThis;

	EnterCriticalSection(&csDbAccess);
	if (hContact == 0)
		hContact = (HANDLE)dbHeader.ofsUser;
	dbc = *(DBContact*)DBRead(hContact,sizeof(DBContact),NULL);
	dbe = (DBEvent*)DBRead(hDbEvent,sizeof(DBEvent),NULL);
	if (dbe->signature!=DBEVENT_SIGNATURE || dbc.signature!=DBCONTACT_SIGNATURE) {
		LeaveCriticalSection(&csDbAccess);
	  	return -1;
	}
	if (dbe->flags&DBEF_READ || dbe->flags&DBEF_SENT) {
		ret = (INT_PTR)dbe->flags;
		LeaveCriticalSection(&csDbAccess);
		return ret;
	}
	log1("mark read @ %08x",wParam);
	dbe->flags |= DBEF_READ;
	DBWrite((DWORD)hDbEvent,dbe,sizeof(DBEvent));
	ret = (int)dbe->flags;
	if (dbc.ofsFirstUnreadEvent == (DWORD)hDbEvent) {
		for (;;) {
			if (dbe->ofsNext == 0) {
				dbc.ofsFirstUnreadEvent = 0;
				dbc.timestampFirstUnread = 0;
				break;
			}
			ofsThis = dbe->ofsNext;
			dbe = (DBEvent*)DBRead(ofsThis,sizeof(DBEvent),NULL);
			if (!(dbe->flags&(DBEF_READ|DBEF_SENT))) {
				dbc.ofsFirstUnreadEvent = ofsThis;
				dbc.timestampFirstUnread = dbe->timestamp;
				break;
			}
		}
	}
	DBWrite((DWORD)hContact,&dbc,sizeof(DBContact));
	DBFlush(0);
	LeaveCriticalSection(&csDbAccess);
	return ret;
}

STDMETHODIMP_(HANDLE) CDdxMmap::GetEventContact(HANDLE hDbEvent)
{
	EnterCriticalSection(&csDbAccess);
	DBEvent *dbe = (DBEvent*)DBRead(hDbEvent,sizeof(DBEvent),NULL);
	if (dbe->signature != DBEVENT_SIGNATURE) {
		LeaveCriticalSection(&csDbAccess);
	  	return (HANDLE)-1;
	}
	while(!(dbe->flags & DBEF_FIRST))
		dbe = (DBEvent*)DBRead(dbe->ofsPrev,sizeof(DBEvent),NULL);
	
	HANDLE ret = (HANDLE)dbe->ofsPrev;
	LeaveCriticalSection(&csDbAccess);
	return ret;
}

STDMETHODIMP_(HANDLE) CDdxMmap::FindFirstEvent(HANDLE hContact)
{
	HANDLE ret;

	EnterCriticalSection(&csDbAccess);
	if (hContact == 0)
		hContact = (HANDLE)dbHeader.ofsUser;
	
	DBContact *dbc = (DBContact*)DBRead(hContact, sizeof(DBContact), NULL);
	if (dbc->signature != DBCONTACT_SIGNATURE)
		ret = 0;
	else
		ret = (HANDLE)dbc->ofsFirstEvent;
	LeaveCriticalSection(&csDbAccess);
	return ret;
}

STDMETHODIMP_(HANDLE) CDdxMmap::FindFirstUnreadEvent(HANDLE hContact)
{
	HANDLE ret;

	EnterCriticalSection(&csDbAccess);
	if (hContact == 0)
		hContact = (HANDLE)dbHeader.ofsUser;
	DBContact *dbc = (DBContact*)DBRead(hContact,sizeof(DBContact),NULL);
	if (dbc->signature!=DBCONTACT_SIGNATURE) ret = 0;
	else ret = (HANDLE)dbc->ofsFirstUnreadEvent;
	LeaveCriticalSection(&csDbAccess);
	return ret;
}

STDMETHODIMP_(HANDLE) CDdxMmap::FindLastEvent(HANDLE hContact)
{
	HANDLE ret;

	EnterCriticalSection(&csDbAccess);
	if (hContact == 0)
		hContact = (HANDLE)dbHeader.ofsUser;
	DBContact *dbc = (DBContact*)DBRead(hContact,sizeof(DBContact),NULL);
	if (dbc->signature!=DBCONTACT_SIGNATURE) ret = 0;
	else ret = (HANDLE)dbc->ofsLastEvent;
	LeaveCriticalSection(&csDbAccess);
	return ret;
}

STDMETHODIMP_(HANDLE) CDdxMmap::FindNextEvent(HANDLE hDbEvent)
{
	HANDLE ret;

	EnterCriticalSection(&csDbAccess);
	DBEvent *dbe = (DBEvent*)DBRead(hDbEvent,sizeof(DBEvent),NULL);
	if (dbe->signature!=DBEVENT_SIGNATURE) ret = 0;
	else ret = (HANDLE)dbe->ofsNext;
	LeaveCriticalSection(&csDbAccess);
	return ret;
}

STDMETHODIMP_(HANDLE) CDdxMmap::FindPrevEvent(HANDLE hDbEvent)
{
	HANDLE ret;

	EnterCriticalSection(&csDbAccess);
	DBEvent *dbe = (DBEvent*)DBRead(hDbEvent,sizeof(DBEvent),NULL);
	if (dbe->signature!=DBEVENT_SIGNATURE) ret = 0;
	else if (dbe->flags&DBEF_FIRST) ret = 0;
	else ret = (HANDLE)dbe->ofsPrev;
	LeaveCriticalSection(&csDbAccess);
	return ret;
}

///////////////////////////////////////////////////////////////////////////////

int InitEvents(void)
{
	hEventAddedEvent = CreateHookableEvent(ME_DB_EVENT_ADDED);
	hEventDeletedEvent = CreateHookableEvent(ME_DB_EVENT_DELETED);
	hEventFilterAddedEvent = CreateHookableEvent(ME_DB_EVENT_FILTER_ADD);
	return 0;
}
