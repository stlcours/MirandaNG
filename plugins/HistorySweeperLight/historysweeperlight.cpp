/*
Miranda IM History Sweeper Light plugin
Copyright (C) 2002-2003  Sergey V. Gershovich
Copyright (C) 2006-2009  Boris Krasnovskiy
Copyright (C) 2010, 2011 tico-tico

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

#include "historysweeperlight.h"

typedef struct
{
	time_t time;
	int keep;
} CriteriaStruct;

// build criteria
time_t BuildCriteria(int dwti)
{
	time_t tim =  time(NULL);

	switch (dwti)
	{
		case 0:
			tim -= 1 * 24 * 3600;
			break;
		case 1:
			tim -= 3 * 24 * 3600;
			break;
		case 2:
			tim -= 7 * 24 * 3600;
			break;
		case 3:
			tim -= 14 * 24 * 3600;
			break;
		case 4:
			tim -= 30 * 24 * 3600;
			break;
		case 5:
			tim -= 90 * 24 * 3600;
			break;
		case 6:
			tim -= 180 * 24 * 3600;
			break;
		default:
			tim -= 365 * 24 * 3600;
	}
	return tim;
}

// keep criteria
int KeepCriteria(int dwti)
{
	switch (dwti)
	{
		case 0:
			return 1;
		case 1:
			return 2;
		case 2:
			return 5;
		case 3:
			return 10;
		case 4:
			return 20;
		default:
			return 50;
	}
}

typedef struct
{
	HANDLE hDBEvent;
	DWORD CRC32;
	DWORD Timestamp;
} BEventData;

static int CompareBookmarks( const void* p1, const void* p2 )
{
	return (BYTE*)((BEventData*)p1)->hDBEvent - (BYTE*)((BEventData*)p2)->hDBEvent;
}

static void GetBookmarks(HANDLE hContact, BEventData** books, size_t* bookcnt )
{
	DBVARIANT dbv;
	DBCONTACTGETSETTING cgs;

	*books = NULL;
	*bookcnt = 0;

	cgs.szModule = "HistoryPlusPlus";
	cgs.szSetting = "Bookmarks";
	cgs.pValue = &dbv;
	if (CallService(MS_DB_CONTACT_GETSETTING, (WPARAM)hContact, (LPARAM)&cgs) == 0)
	{
		if (dbv.cpbVal > 2 && *(WORD*)dbv.pbVal >= sizeof(BEventData))
		{
			size_t recSize = *(WORD*)dbv.pbVal;
			size_t numb = (dbv.cpbVal - sizeof(WORD)) / recSize;

			BEventData* tbooks = (BEventData*)mir_alloc(sizeof(BEventData)*numb);
			size_t tbookcnt = 0;

			size_t i;
			BYTE* evs = dbv.pbVal + sizeof(WORD);

			for (i=0; i<numb; ++i)
			{
				tbooks[tbookcnt++] = *(BEventData*)evs;
				evs += recSize;
			}
			if (tbookcnt != 0)
			{
				qsort(tbooks, tbookcnt, sizeof(BEventData), CompareBookmarks);
				*bookcnt = tbookcnt;
				*books = tbooks;
			}
			else
				mir_free(tbooks);
		}
	
		DBFreeVariant(&dbv);
	}
}

//Sweep history from specified contact
void SweepHistoryFromContact(HANDLE hContact, CriteriaStruct Criteria, BOOL keepUnread)
{
	int lPolicy, eventsCnt;
	
	if (hContact == NULL)			// for system history
		lPolicy = DBGetContactSettingByte(NULL, ModuleName, "SweepSHistory", 0);
	else							// for contact history (or "SweepHistory" - default action)
		lPolicy = DBGetContactSettingByte(hContact, ModuleName, "SweepHistory", DBGetContactSettingByte(NULL, ModuleName, "SweepHistory", 0));
	
	if (lPolicy == 0) return;		// nothing to do
	
	eventsCnt = CallService(MS_DB_EVENT_GETCOUNT, (WPARAM)hContact, 0);
	
	if (eventsCnt != 0) 
	{ 
		BOOL doDelete, unsafe = DBGetContactSettingByte(NULL, ModuleName, "UnsafeMode", 0);
		HANDLE hDBEvent, hDBEventNext; BEventData *books, *item, ev = { 0 };
		DBEVENTINFO dbei; size_t bookcnt, btshift;
		
		SetCursor(LoadCursor(0, IDC_WAIT));

		// switch off SAFETYMODE if necessary
		if (unsafe)	CallService(MS_DB_SETSAFETYMODE, 0, 0); 

		GetBookmarks(hContact, &books, &bookcnt);

		//Get first event
		hDBEvent = (HANDLE)CallService(MS_DB_EVENT_FINDFIRST, (WPARAM)hContact, 0);
		
		while (hDBEvent != NULL) 
		{
			ZeroMemory(&dbei, sizeof(dbei));
			dbei.cbSize = sizeof(dbei);

			CallService(MS_DB_EVENT_GET, (WPARAM)hDBEvent, (LPARAM)&dbei);

			// should we stop processing?
			// lPolicy == 1 - for time criterion, lPolicy == 2 - keep N last events, lPolicy == 3 - delete all events
			if ( (lPolicy == 1 && (unsigned)Criteria.time < dbei.timestamp) || (lPolicy == 2 && Criteria.keep > --eventsCnt)) break;
			
			doDelete = TRUE;

			if (!(dbei.flags & (DBEF_SENT | DBEF_READ)) && keepUnread) doDelete = FALSE;	// keep unread events

			if (bookcnt != 0)	// keep bookmarks
			{
				ev.hDBEvent = hDBEvent;
				item = (BEventData*)bsearch( &ev, books, bookcnt, sizeof(BEventData), CompareBookmarks);				
				if (item != NULL)
					if (item->Timestamp == dbei.timestamp) 
					{
						doDelete = FALSE;
						btshift = (--bookcnt - (item - books))*sizeof(BEventData);
						if (btshift) memmove(item, item+1, btshift);
					}
			}

			// find next event
			hDBEventNext = (HANDLE)CallService(MS_DB_EVENT_FINDNEXT, (WPARAM)hDBEvent, 0);

			if (doDelete) CallService(MS_DB_EVENT_DELETE, (WPARAM)hContact, (LPARAM)hDBEvent);
			
			hDBEvent = hDBEventNext;
		} // while (hDBEvent != NULL)

		mir_free(books);

		// switch ON safety mode as fast as we can to avoid  DB corruption
		if (unsafe)	CallService(MS_DB_SETSAFETYMODE, 1, 0);

		SetCursor(LoadCursor(0, IDC_ARROW));
	} // if (eventsCnt != 0)
}

// shutdown action
void ShutdownAction(void)
{ 
	CriteriaStruct Criteria;
	HANDLE hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);

	Criteria.keep = KeepCriteria(DBGetContactSettingByte(NULL, ModuleName, "StartupShutdownKeep", 0));
	Criteria.time = BuildCriteria(DBGetContactSettingByte(NULL, ModuleName, "StartupShutdownOlder", 0));

	SweepHistoryFromContact(NULL, Criteria, FALSE);				// sweep system history, keepunread==0
	
	while ( hContact != NULL )
	{
		SweepHistoryFromContact(hContact, Criteria, TRUE);		// sweep contact history, keepunread==1

		hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, 0);	// go to next contact
	}
}

int OnWindowEvent(WPARAM wParam, LPARAM lParam)
{
	MessageWindowEventData* msgEvData  = (MessageWindowEventData*)lParam;

	if (msgEvData->uType == MSG_WINDOW_EVT_CLOSE && DBGetContactSettingByte(NULL, ModuleName, "SweepOnClose", 0))
	{
		CriteriaStruct Criteria;

		Criteria.keep = KeepCriteria(DBGetContactSettingByte(NULL, ModuleName, "StartupShutdownKeep", 0));
		Criteria.time = BuildCriteria(DBGetContactSettingByte(NULL, ModuleName, "StartupShutdownOlder", 0));

		SweepHistoryFromContact(msgEvData->hContact, Criteria, TRUE);
	}

	return 0;
}