/*

Miranda NG: the free IM client for Microsoft* Windows*

Copyright 2012-14 Miranda NG project,
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

int CDb3Mmap::CheckProto(DBCachedContact *cc, const char *proto)
{
	if (cc->szProto == NULL) {
		char protobuf[MAX_PATH] = { 0 };
		DBVARIANT dbv;
		dbv.type = DBVT_ASCIIZ;
		dbv.pszVal = protobuf;
		dbv.cchVal = sizeof(protobuf);
		if (GetContactSettingStatic(cc->contactID, "Protocol", "p", &dbv) != 0 || (dbv.type != DBVT_ASCIIZ))
			return 0;

		cc->szProto = m_cache->GetCachedSetting(NULL, protobuf, 0, (int)strlen(protobuf));
	}

	return !strcmp(cc->szProto, proto);
}

STDMETHODIMP_(LONG) CDb3Mmap::GetContactCount(void)
{
	mir_cslock lck(m_csDbAccess);
	return m_dbHeader.contactCount;
}

STDMETHODIMP_(MCONTACT) CDb3Mmap::FindFirstContact(const char *szProto)
{
	mir_cslock lck(m_csDbAccess);
	DBCachedContact *cc = m_cache->GetFirstContact();
	if (cc == NULL)
		return NULL;

	if (!szProto || CheckProto(cc, szProto))
		return cc->contactID;

	return FindNextContact(cc->contactID, szProto);
}

STDMETHODIMP_(MCONTACT) CDb3Mmap::FindNextContact(MCONTACT contactID, const char *szProto)
{
	mir_cslock lck(m_csDbAccess);
	while (contactID) {
		DBCachedContact *cc = m_cache->GetNextContact(contactID);
		if (cc == NULL)
			break;

		if (!szProto || CheckProto(cc, szProto))
			return cc->contactID;

		contactID = cc->contactID;
	}

	return NULL;
}

STDMETHODIMP_(LONG) CDb3Mmap::DeleteContact(MCONTACT contactID)
{
	if (contactID == NULL)
		return 1;

	mir_cslockfull lck(m_csDbAccess);
	DBCachedContact *cc = m_cache->GetCachedContact(contactID);
	if (cc == NULL)
		return 1;

	DBContact *dbc = (DBContact*)DBRead(cc->dwDriverData, sizeof(DBContact), NULL);
	if (dbc->signature != DBCONTACT_SIGNATURE)
		return 1;

	if (cc->dwDriverData == m_dbHeader.ofsUser) {
		log0("FATAL: del of user chain attempted.");
		return 1;
	}

	lck.unlock();
	log0("del contact");

	// call notifier while outside mutex
	NotifyEventHooks(hContactDeletedEvent, contactID, 0);

	// get back in
	lck.lock();

	// delete settings chain
	DWORD ofsThis = dbc->ofsFirstSettings;
	DWORD ofsFirstEvent = dbc->ofsFirstEvent;
	while (ofsThis) {
		DBContactSettings *dbcs = (DBContactSettings*)DBRead(ofsThis, sizeof(DBContactSettings), NULL);
		DWORD ofsNext = dbcs->ofsNext;
		DeleteSpace(ofsThis, offsetof(DBContactSettings, blob) + dbcs->cbBlob);
		ofsThis = ofsNext;
	}

	// delete event chain
	ofsThis = ofsFirstEvent;
	while (ofsThis) {
		DBEvent *dbe = (DBEvent*)DBRead(ofsThis, sizeof(DBEvent), NULL);
		DWORD ofsNext = dbe->ofsNext;
		DeleteSpace(ofsThis, offsetof(DBEvent, blob) + dbe->cbBlob);
		ofsThis = ofsNext;
	}

	// find previous contact in chain and change ofsNext
	if (m_dbHeader.ofsFirstContact == cc->dwDriverData) {
		m_dbHeader.ofsFirstContact = dbc->ofsNext;
		DBWrite(0, &m_dbHeader, sizeof(m_dbHeader));
	}
	else {
		DWORD ofsNext = dbc->ofsNext;
		ofsThis = m_dbHeader.ofsFirstContact;
		DBContact *dbcPrev = (DBContact*)DBRead(ofsThis, sizeof(DBContact), NULL);
		while (dbcPrev->ofsNext != cc->dwDriverData) {
			if (dbcPrev->ofsNext == 0) DatabaseCorruption(NULL);
			ofsThis = dbcPrev->ofsNext;
			dbcPrev = (DBContact*)DBRead(ofsThis, sizeof(DBContact), NULL);
		}
		dbcPrev->ofsNext = ofsNext;
		DBWrite(ofsThis, dbcPrev, sizeof(DBContact));
	}

	// delete contact
	DeleteSpace(cc->dwDriverData, sizeof(DBContact));

	// decrement contact count
	m_dbHeader.contactCount--;
	DBWrite(0, &m_dbHeader, sizeof(m_dbHeader));
	DBFlush(0);

	// free cache item
	m_cache->FreeCachedContact(contactID);
	if (contactID == m_hLastCachedContact)
		m_hLastCachedContact = NULL;
	return 0;
}

STDMETHODIMP_(HANDLE) CDb3Mmap::AddContact()
{
	DWORD ofsNew;
	log0("add contact");

	DBContact dbc = { 0 };
	dbc.signature = DBCONTACT_SIGNATURE;
	{
		mir_cslock lck(m_csDbAccess);
		ofsNew = CreateNewSpace(sizeof(DBContact));

		dbc.ofsNext = m_dbHeader.ofsFirstContact;
		dbc.dwContactID = m_dwMaxContactId++;
		m_dbHeader.ofsFirstContact = ofsNew;
		m_dbHeader.contactCount++;
		DBWrite(ofsNew, &dbc, sizeof(DBContact));
		DBWrite(0, &m_dbHeader, sizeof(m_dbHeader));
		DBFlush(0);
	}

	DBCachedContact *cc = m_cache->AddContactToCache(dbc.dwContactID);
	cc->dwDriverData = ofsNew;

	NotifyEventHooks(hContactAddedEvent, dbc.dwContactID, 0);
	return (HANDLE)dbc.dwContactID;
}

STDMETHODIMP_(BOOL) CDb3Mmap::IsDbContact(MCONTACT contactID)
{
	DBCachedContact *cc = m_cache->GetCachedContact(contactID);
	if (cc == NULL)
		return FALSE;

	mir_cslock lck(m_csDbAccess);
	DBContact *dbc = (DBContact*)DBRead(cc->dwDriverData, sizeof(DBContact), NULL);
	if (dbc->signature == DBCONTACT_SIGNATURE) {
		m_cache->AddContactToCache(contactID);
		return TRUE;
	}

	return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////////////
// contacts convertor

#define OLD_CONTACT_SIZE offsetof(DBContact, dwContactID)

void CDb3Mmap::ConvertContacts()
{
	DBContact *pPrev = NULL;

	m_dbHeader.ofsUser = ReallocSpace(m_dbHeader.ofsUser, OLD_CONTACT_SIZE, sizeof(DBContact));

	for (DWORD dwOffset = m_dbHeader.ofsFirstContact; dwOffset != 0;) {
		DBContact *pOld = (DBContact*)DBRead(dwOffset, sizeof(DBContact), NULL);
		DWORD dwNew = ReallocSpace(dwOffset, OLD_CONTACT_SIZE, sizeof(DBContact));
		DBContact *pNew = (DBContact*)DBRead(dwNew, sizeof(DBContact), NULL);
		pNew->dwContactID = m_dwMaxContactId++;

		if (pPrev == NULL)
			m_dbHeader.ofsFirstContact = dwNew;
		else
			pPrev->ofsNext = dwNew;
		pPrev = pNew;
		dwOffset = pNew->ofsNext;
	}

	FlushViewOfFile(m_pDbCache, 0);
}

void CDb3Mmap::FillContacts()
{
	for (DWORD dwOffset = m_dbHeader.ofsFirstContact; dwOffset != 0;) {
		DBContact *p = (DBContact*)DBRead(dwOffset, sizeof(DBContact), NULL);
		if (p->signature != DBCONTACT_SIGNATURE)
			break;

		if (p->dwContactID > m_dwMaxContactId)
			m_dwMaxContactId = p->dwContactID + 1;

		DBCachedContact *cc = m_cache->AddContactToCache(p->dwContactID);
		cc->dwDriverData = dwOffset;
		CheckProto(cc, "");

		dwOffset = p->ofsNext;
	}
}

DWORD CDb3Mmap::GetContactOffset(MCONTACT contactID)
{
	if (contactID == 0)
		return m_dbHeader.ofsUser;

	DBCachedContact *cc = m_cache->GetCachedContact(contactID);
	return (cc == NULL) ? 0 : cc->dwDriverData;
}
