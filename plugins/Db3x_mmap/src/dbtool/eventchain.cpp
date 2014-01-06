/*

Miranda Database Tool
Miranda NG: the free IM client for Microsoft* Windows*

Copyright (c) 2012-14 Miranda NG project (http://miranda-ng.org),
Copyright 2000-2011 Miranda ICQ/IM project,
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
#include "..\commonheaders.h"

#define DBEF_ALL (DBEF_FIRST | DBEF_READ | DBEF_SENT | DBEF_RTL | DBEF_UTF | DBEF_ENCRYPTED)

static BOOL backLookup;
static DWORD ofsThisEvent, ofsPrevEvent;
static DWORD ofsDestPrevEvent;
static DWORD eventCount;
static DWORD lastTimestamp;
static DWORD ofsFirstUnread, timestampFirstUnread;
static DWORD memsize = 0;
static DBEvent* memblock = NULL;
static DBEvent* dbePrevEvent = NULL;

void CDb3Base::ConvertOldEvent(DBEvent*& dbei)
{
	int msglen = (int)strlen((char*)dbei->blob) + 1, msglenW = 0;
	if (msglen != (int)dbei->cbBlob) {
		int i, count = ((dbei->cbBlob - msglen) / sizeof(WCHAR));
		WCHAR* p = (WCHAR*)&dbei->blob[msglen];
		for (i = 0; i < count; i++) {
			if (p[i] == 0) {
				msglenW = i;
				break;
			}
		}
	}
	else {
		if (!Utf8CheckString((char*)dbei->blob))
			dbei->flags &= ~DBEF_UTF;
	}

	if (msglenW > 0 && msglenW <= msglen) {
		char* utf8str = Utf8EncodeW((WCHAR*)&dbei->blob[msglen]);
		if (utf8str == NULL)
			return;

		dbei->cbBlob = (DWORD)strlen(utf8str) + 1;
		dbei->flags |= DBEF_UTF;
		if (offsetof(DBEvent, blob) + dbei->cbBlob > memsize) {
			memsize = offsetof(DBEvent, blob) + dbei->cbBlob;
			memblock = (DBEvent*)realloc(memblock, memsize);
			dbei = memblock;
		}
		memcpy(&dbei->blob, utf8str, dbei->cbBlob);
		mir_free(utf8str);
	}
}

void CDb3Base::WriteOfsNextToPrevious(DWORD ofsPrev, DBContact *dbc, DWORD ofsNext)
{
	if (ofsPrev)
		WriteSegment(ofsPrev + offsetof(DBEvent, ofsNext), &ofsNext, sizeof(DWORD));
	else
		dbc->ofsFirstEvent = ofsNext;
}

void CDb3Base::FinishUp(DWORD ofsLast, DBContact *dbc)
{
	WriteOfsNextToPrevious(ofsLast, dbc, 0);
	if (eventCount != dbc->eventCount)
		cb->pfnAddLogMessage(STATUS_WARNING, TranslateT("Event count marked wrongly: correcting"));
	dbc->eventCount = eventCount;
	dbc->ofsLastEvent = ofsLast;
	if (cb->bMarkRead) {
		dbc->ofsFirstUnreadEvent = 0;
		dbc->timestampFirstUnread = 0;
	}
	else {
		dbc->ofsFirstUnreadEvent = ofsFirstUnread;
		dbc->timestampFirstUnread = timestampFirstUnread;
	}
	if (memsize && memblock) {
		free(memblock);
		memsize = 0;
		memblock = NULL;
	}
}

DWORD CDb3Base::WriteEvent(DBEvent *dbe)
{
	DWORD ofs = WriteSegment(WSOFS_END, dbe, offsetof(DBEvent, blob) + dbe->cbBlob);
	if (ofs == WS_ERROR) {
		free(memblock);
		memblock = NULL;
		memsize = 0;
		return 0;
	}
	return ofs;
}

int CDb3Base::WorkEventChain(DWORD ofsContact, DBContact *dbc, int firstTime)
{
	DBEvent *dbeNew, dbeOld;
	DBEvent *dbePrev = NULL;
	DWORD ofsDestThis;
	int isUnread = 0;

	if (firstTime) {
		dbePrevEvent = NULL;
		ofsPrevEvent = 0;
		ofsDestPrevEvent = 0;
		ofsThisEvent = dbc->ofsFirstEvent;
		eventCount = 0;
		backLookup = 0;
		lastTimestamp = 0;
		ofsFirstUnread = timestampFirstUnread = 0;
		if (cb->bEraseHistory) {
			dbc->eventCount = 0;
			dbc->ofsFirstEvent = 0;
			dbc->ofsLastEvent = 0;
			dbc->ofsFirstUnreadEvent = 0;
			dbc->timestampFirstUnread = 0;
			return ERROR_NO_MORE_ITEMS;
		}
	}

	if (ofsThisEvent == 0) {
		FinishUp(ofsDestPrevEvent, dbc);
		return ERROR_NO_MORE_ITEMS;
	}

	if (!SignatureValid(ofsThisEvent, DBEVENT_SIGNATURE)) {
		DWORD ofsNew = 0;
		DWORD ofsTmp = dbc->ofsLastEvent;

		if (!backLookup && ofsTmp) {
			backLookup = 1;
			while (SignatureValid(ofsTmp, DBEVENT_SIGNATURE)) {
				if (PeekSegment(ofsTmp, &dbeOld, sizeof(dbeOld)) != ERROR_SUCCESS)
					break;
				ofsNew = ofsTmp;
				ofsTmp = dbeOld.ofsPrev;
			}
		}
		if (ofsNew) {
			cb->pfnAddLogMessage(STATUS_WARNING, TranslateT("Event chain corrupted, trying to recover..."));
			ofsThisEvent = ofsNew;
		}
		else {
			cb->pfnAddLogMessage(STATUS_ERROR, TranslateT("Event chain corrupted, further entries ignored"));
			FinishUp(ofsDestPrevEvent, dbc);
			return ERROR_NO_MORE_ITEMS;
		}
	}

	if (PeekSegment(ofsThisEvent, &dbeOld, sizeof(dbeOld)) != ERROR_SUCCESS) {
		FinishUp(ofsDestPrevEvent, dbc);
		return ERROR_NO_MORE_ITEMS;
	}

	if (firstTime) {
		if (!(dbeOld.flags & DBEF_FIRST)) {
			cb->pfnAddLogMessage(STATUS_WARNING, TranslateT("First event not marked as such: correcting"));
			dbeOld.flags |= DBEF_FIRST;
		}
		dbeOld.ofsPrev = ofsContact;
		lastTimestamp = dbeOld.timestamp;
	}
	else if (dbeOld.flags & DBEF_FIRST) {
		cb->pfnAddLogMessage(STATUS_WARNING, TranslateT("Event marked as first which is not: correcting"));
		dbeOld.flags &= ~DBEF_FIRST;
	}

	if (dbeOld.flags & ~DBEF_ALL) {
		cb->pfnAddLogMessage(STATUS_WARNING, TranslateT("Extra flags found in event: removing"));
		dbeOld.flags &= DBEF_ALL;
	}

	if (!(dbeOld.flags & (DBEF_READ | DBEF_SENT))) {
		if (cb->bMarkRead) dbeOld.flags |= DBEF_READ;
		else if (ofsFirstUnread == 0) {
			if (dbc->ofsFirstUnreadEvent != ofsThisEvent || dbc->timestampFirstUnread != dbeOld.timestamp)
				cb->pfnAddLogMessage(STATUS_WARNING, TranslateT("First unread event marked wrong: fixing"));
			isUnread = 1;
		}
	}

	if (dbeOld.cbBlob > 1024 * 1024 || dbeOld.cbBlob == 0) {
		cb->pfnAddLogMessage(STATUS_ERROR, TranslateT("Infeasibly large event blob: skipping"));
		ofsThisEvent = dbeOld.ofsNext;
		return ERROR_SUCCESS;
	}

	if (dbePrevEvent && dbeOld.timestamp == lastTimestamp) {
		int len = offsetof(DBEvent, blob) + dbePrevEvent->cbBlob;
		dbePrev = (DBEvent*)malloc(len);
		memcpy(dbePrev, dbePrevEvent, len);
	}

	if (offsetof(DBEvent, blob) + dbeOld.cbBlob > memsize) {
		memsize = offsetof(DBEvent, blob) + dbeOld.cbBlob;
		memblock = (DBEvent*)realloc(memblock, memsize);
	}
	dbeNew = memblock;

	if (ReadSegment(ofsThisEvent, dbeNew, offsetof(DBEvent, blob) + dbeOld.cbBlob) != ERROR_SUCCESS) {
		FinishUp(ofsDestPrevEvent, dbc);
		return ERROR_NO_MORE_ITEMS;
	}

	if ((dbeNew->ofsModuleName = ConvertModuleNameOfs(dbeOld.ofsModuleName)) == 0) {
		ofsThisEvent = dbeOld.ofsNext;
		return ERROR_SUCCESS;
	}

	if (!firstTime && dbeOld.ofsPrev != ofsPrevEvent)
		cb->pfnAddLogMessage(STATUS_WARNING, TranslateT("Event not backlinked correctly: fixing"));

	dbeNew->flags = dbeOld.flags;
	dbeNew->ofsPrev = ofsDestPrevEvent;
	dbeNew->ofsNext = 0;

	if (dbeOld.eventType == EVENTTYPE_MESSAGE && cb->bConvertUtf && !(dbeOld.flags & DBEF_ENCRYPTED)) {
		DWORD oldSize = dbeNew->cbBlob;
		BYTE* pOldMemo = (BYTE*)_alloca(dbeNew->cbBlob);
		memcpy(pOldMemo, dbeNew->blob, dbeNew->cbBlob);
		DecodeCopyMemory(dbeNew->blob, pOldMemo, dbeNew->cbBlob); // decode
		ConvertOldEvent(dbeNew);
		if (dbeNew->cbBlob > oldSize)
			pOldMemo = (BYTE*)_alloca(dbeNew->cbBlob);
		memcpy(pOldMemo, dbeNew->blob, dbeNew->cbBlob);
		EncodeCopyMemory(dbeNew->blob, pOldMemo, dbeNew->cbBlob);   // encode
	}

	if (dbePrev) {
		if (dbePrev->cbBlob == dbeNew->cbBlob &&
			dbePrev->ofsModuleName == dbeNew->ofsModuleName &&
			dbePrev->eventType == dbeNew->eventType &&
			(dbePrev->flags & DBEF_SENT) == (dbeNew->flags & DBEF_SENT) &&
			!memcmp(dbePrev->blob, dbeNew->blob, dbeNew->cbBlob))
		{
			cb->pfnAddLogMessage(STATUS_WARNING, TranslateT("Duplicate event was found: skipping"));
			if (dbc->eventCount)
				dbc->eventCount--;
			free(dbePrev);
			// ofsDestPrevEvent is still the same!
			ofsPrevEvent = ofsThisEvent;
			ofsThisEvent = dbeOld.ofsNext;
			return ERROR_SUCCESS;
		}
		free(dbePrev);
	}
	else if (!firstTime && dbeNew->timestamp < lastTimestamp) {
		DWORD found = 0;
		DBEvent dbeTmp;
		DWORD ofsTmp;

		if (cb->bCheckOnly) {
			if (!cb->bAggressive) {
				ofsTmp = dbeOld.ofsPrev;
				while (PeekSegment(ofsTmp, &dbeTmp, sizeof(dbeTmp)) == ERROR_SUCCESS) {
					if (dbeTmp.ofsPrev == ofsContact) {
						found = 1;
						break;
					}
					if (dbeTmp.timestamp < dbeNew->timestamp) {
						found = 2;
						break;
					}
					ofsTmp = dbeTmp.ofsPrev;
				}
			}
			cb->pfnAddLogMessage(STATUS_WARNING, TranslateT("Event position in chain is not correct"));
		}
		else {
			ofsTmp = ofsDestPrevEvent;
			while (ReadWrittenSegment(ofsTmp, &dbeTmp, sizeof(dbeTmp)) == ERROR_SUCCESS) {
				if (dbeTmp.ofsPrev == ofsContact) {
					found = 1;
					break;
				}
				if (dbeTmp.timestamp < dbeNew->timestamp) {
					found = 2;
					break;
				}
				ofsTmp = dbeTmp.ofsPrev;
			}
			if (found)
				cb->pfnAddLogMessage(STATUS_WARNING, TranslateT("Event position in chain is not correct: fixing"));
			else
				cb->pfnAddLogMessage(STATUS_WARNING, TranslateT("Event position in chain is not correct: unable to fix"));
		}

		// insert before FIRST
		if (found == 1 && !cb->bCheckOnly) {
			dbeNew->flags |= DBEF_FIRST;
			dbeNew->ofsPrev = ofsContact;
			dbeNew->ofsNext = dbc->ofsFirstEvent;

			ofsDestThis = WriteEvent(dbeNew);
			if (!ofsDestThis)
				return ERROR_HANDLE_DISK_FULL;

			if (isUnread && timestampFirstUnread >= dbeNew->timestamp) {
				ofsFirstUnread = ofsDestThis;
				timestampFirstUnread = dbeNew->timestamp;
			}
			// fix first event
			WriteOfsNextToPrevious(0, dbc, ofsDestThis);
			// fix next event
			WriteSegment(dbeNew->ofsNext + offsetof(DBEvent, ofsPrev), &ofsDestThis, sizeof(DWORD));
			dbeTmp.flags &= ~DBEF_FIRST;
			WriteSegment(dbeNew->ofsNext + offsetof(DBEvent, flags), &dbeTmp.flags, sizeof(DWORD));
		}
		else if (found == 2 && !cb->bCheckOnly) {
			dbeNew->ofsPrev = ofsTmp;
			dbeNew->ofsNext = dbeTmp.ofsNext;

			ofsDestThis = WriteEvent(dbeNew);
			if (!ofsDestThis)
				return ERROR_HANDLE_DISK_FULL;

			if (isUnread && timestampFirstUnread >= dbeNew->timestamp) {
				ofsFirstUnread = ofsDestThis;
				timestampFirstUnread = dbeNew->timestamp;
			}
			// fix previous event
			WriteOfsNextToPrevious(dbeNew->ofsPrev, dbc, ofsDestThis);
			// fix next event
			WriteSegment(dbeNew->ofsNext + offsetof(DBEvent, ofsPrev), &ofsDestThis, sizeof(DWORD));
		}

		if (found) {
			eventCount++;
			// ofsDestPrevEvent is still the same!
			ofsPrevEvent = ofsThisEvent;
			ofsThisEvent = dbeOld.ofsNext;
			return ERROR_SUCCESS;
		}
	}

	lastTimestamp = dbeNew->timestamp;
	dbePrevEvent = dbeNew;

	ofsDestThis = WriteEvent(dbeNew);
	if (!ofsDestThis)
		return ERROR_HANDLE_DISK_FULL;

	if (isUnread) {
		ofsFirstUnread = ofsDestThis;
		timestampFirstUnread = dbeOld.timestamp;
	}

	eventCount++;
	WriteOfsNextToPrevious(ofsDestPrevEvent, dbc, ofsDestThis);

	ofsDestPrevEvent = ofsDestThis;
	ofsPrevEvent = ofsThisEvent;
	ofsThisEvent = dbeOld.ofsNext;
	return ERROR_SUCCESS;
}
