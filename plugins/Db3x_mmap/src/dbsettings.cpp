/*

Miranda NG: the free IM client for Microsoft* Windows*

Copyright 2012 Miranda NG project,
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

DWORD GetModuleNameOfs(const char *szName);
DBCachedContact* AddToCachedContactList(HANDLE hContact, int index);

int DBPreset_QuerySetting(const char *szModule, const char *szSetting, DBVARIANT *dbv, BOOL isStatic);

DWORD __forceinline GetSettingValueLength(PBYTE pSetting)
{
	if (pSetting[0] & DBVTF_VARIABLELENGTH)
		return 2+*(PWORD)(pSetting+1);
	return pSetting[0];
}

#define NeedBytes(n)   if (bytesRemaining<(n)) pBlob = (PBYTE)DBRead(ofsBlobPtr,(n),&bytesRemaining)
#define MoveAlong(n)   {int x = n; pBlob += (x); ofsBlobPtr += (x); bytesRemaining -= (x);}
#define VLT(n) ((n == DBVT_UTF8)?DBVT_ASCIIZ:n)

int CDb3Base::GetContactSettingWorker(HANDLE hContact,DBCONTACTGETSETTING *dbcgs,int isStatic)
{
	DWORD ofsModuleName,ofsContact,ofsSettingsGroup,ofsBlobPtr;
	int settingNameLen,moduleNameLen;
	int bytesRemaining;
	PBYTE pBlob;
	char* szCachedSettingName;

	if ((!dbcgs->szSetting) || (!dbcgs->szModule))
		return 1;
	// the db format can't tolerate more than 255 bytes of space (incl. null) for settings+module name
	settingNameLen = (int)strlen(dbcgs->szSetting);
	moduleNameLen = (int)strlen(dbcgs->szModule);
	if ( settingNameLen > 0xFE )
	{
		#ifdef _DEBUG
			OutputDebugStringA("GetContactSettingWorker() got a > 255 setting name length. \n");
		#endif
		return 1;
	}
	if ( moduleNameLen > 0xFE )
	{
		#ifdef _DEBUG
			OutputDebugStringA("GetContactSettingWorker() got a > 255 module name length. \n");
		#endif
		return 1;
	}

	mir_cslock lck(m_csDbAccess);

	szCachedSettingName = m_cache->GetCachedSetting(dbcgs->szModule,dbcgs->szSetting,moduleNameLen,settingNameLen);
	log3("get [%08p] %s (%p)", hContact, szCachedSettingName, szCachedSettingName);
	{
		DBVARIANT* pCachedValue = m_cache->GetCachedValuePtr(hContact, szCachedSettingName, 0);
		if ( pCachedValue != NULL ) {
			if ( pCachedValue->type == DBVT_ASCIIZ || pCachedValue->type == DBVT_UTF8 ) {
				int   cbOrigLen = dbcgs->pValue->cchVal;
				char* cbOrigPtr = dbcgs->pValue->pszVal;
				memcpy( dbcgs->pValue, pCachedValue, sizeof( DBVARIANT ));
				if ( isStatic ) {
					int cbLen = 0;
					if ( pCachedValue->pszVal != NULL )
						cbLen = (int)strlen( pCachedValue->pszVal );

					cbOrigLen--;
					dbcgs->pValue->pszVal = cbOrigPtr;
					if (cbLen<cbOrigLen) cbOrigLen = cbLen;
					CopyMemory(dbcgs->pValue->pszVal,pCachedValue->pszVal,cbOrigLen);
					dbcgs->pValue->pszVal[cbOrigLen] = 0;
					dbcgs->pValue->cchVal = cbLen;
				}
				else {
					dbcgs->pValue->pszVal = (char*)mir_alloc(strlen(pCachedValue->pszVal)+1);
					strcpy(dbcgs->pValue->pszVal,pCachedValue->pszVal);
				}
			}
			else
				memcpy( dbcgs->pValue, pCachedValue, sizeof( DBVARIANT ));

			log2("get cached %s (%p)", printVariant(dbcgs->pValue), pCachedValue);
			return ( pCachedValue->type == DBVT_DELETED ) ? 1 : 0;
	}	}

	ofsModuleName = GetModuleNameOfs(dbcgs->szModule);
	if (hContact == NULL) ofsContact = m_dbHeader.ofsUser;
	else ofsContact = (DWORD)hContact;
	
	DBContact dbc = *(DBContact*)DBRead(ofsContact,sizeof(DBContact),NULL);
	if (dbc.signature != DBCONTACT_SIGNATURE)
		return 1;

	ofsSettingsGroup = GetSettingsGroupOfsByModuleNameOfs(&dbc,ofsContact,ofsModuleName);
	if (ofsSettingsGroup) {
		ofsBlobPtr = ofsSettingsGroup+offsetof(DBContactSettings,blob);
		pBlob = DBRead(ofsBlobPtr,sizeof(DBContactSettings),&bytesRemaining);
		while (pBlob[0]) {
			NeedBytes(1+settingNameLen);
			if (pBlob[0] == settingNameLen && !memcmp(pBlob+1,dbcgs->szSetting,settingNameLen)) {
				MoveAlong(1+settingNameLen);
				NeedBytes(5);
				if (isStatic && pBlob[0]&DBVTF_VARIABLELENGTH && VLT(dbcgs->pValue->type) != VLT(pBlob[0]))
					return 1;

				dbcgs->pValue->type = pBlob[0];
				switch(pBlob[0]) {
					case DBVT_DELETED: /* this setting is deleted */
						dbcgs->pValue->type = DBVT_DELETED;
						return 2;

					case DBVT_BYTE: dbcgs->pValue->bVal = pBlob[1]; break;
					case DBVT_WORD: DecodeCopyMemory(&(dbcgs->pValue->wVal), (PWORD)(pBlob+1), 2); break;
					case DBVT_DWORD: DecodeCopyMemory(&(dbcgs->pValue->dVal), (PDWORD)(pBlob+1), 4); break;
					case DBVT_UTF8:
					case DBVT_ASCIIZ:
						NeedBytes(3+*(PWORD)(pBlob+1));
						if (isStatic) {
							dbcgs->pValue->cchVal--;
							if (*(PWORD)(pBlob+1)<dbcgs->pValue->cchVal) dbcgs->pValue->cchVal = *(PWORD)(pBlob+1);
							DecodeCopyMemory(dbcgs->pValue->pszVal,pBlob+3,dbcgs->pValue->cchVal);
							dbcgs->pValue->pszVal[dbcgs->pValue->cchVal] = 0;
							dbcgs->pValue->cchVal = *(PWORD)(pBlob+1);
						}
						else {
							dbcgs->pValue->pszVal = (char*)mir_alloc(1+*(PWORD)(pBlob+1));
							DecodeCopyMemory(dbcgs->pValue->pszVal,pBlob+3,*(PWORD)(pBlob+1));
							dbcgs->pValue->pszVal[*(PWORD)(pBlob+1)] = 0;
						}
						break;
					case DBVT_BLOB:
						NeedBytes(3+*(PWORD)(pBlob+1));
						if (isStatic) {
							if (*(PWORD)(pBlob+1)<dbcgs->pValue->cpbVal) dbcgs->pValue->cpbVal = *(PWORD)(pBlob+1);
							DecodeCopyMemory(dbcgs->pValue->pbVal,pBlob+3,dbcgs->pValue->cpbVal);
						}
						else {
							dbcgs->pValue->pbVal = (BYTE *)mir_alloc(*(PWORD)(pBlob+1));
							DecodeCopyMemory(dbcgs->pValue->pbVal,pBlob+3,*(PWORD)(pBlob+1));
						}
						dbcgs->pValue->cpbVal = *(PWORD)(pBlob+1);
						break;
				}

				/**** add to cache **********************/
				if ( dbcgs->pValue->type != DBVT_BLOB ) {
					DBVARIANT* pCachedValue = m_cache->GetCachedValuePtr( hContact, szCachedSettingName, 1 );
					if ( pCachedValue != NULL )
						m_cache->SetCachedVariant(dbcgs->pValue, pCachedValue);
				}

				logg();
				return 0;
			}
			NeedBytes(1);
			MoveAlong(pBlob[0]+1);
			NeedBytes(3);
			MoveAlong(1+GetSettingValueLength(pBlob));
			NeedBytes(1);
	}	}

	#ifndef DB3X_EXPORTS
		/**** nullbie: query info from preset **********************/
		if (!hContact && DBPreset_QuerySetting(dbcgs->szModule, dbcgs->szSetting, dbcgs->pValue, isStatic)) {
			/**** add to cache **********************/
			if ( dbcgs->pValue->type != DBVT_BLOB ) {
				DBVARIANT* pCachedValue = m_cache->GetCachedValuePtr( hContact, szCachedSettingName, 1 );
				if ( pCachedValue != NULL )
					m_cache->SetCachedVariant(dbcgs->pValue,pCachedValue);
			}
			return 0;
		}
	#endif

	/**** add missing setting to cache **********************/
	if ( dbcgs->pValue->type != DBVT_BLOB )
	{
		DBVARIANT* pCachedValue = m_cache->GetCachedValuePtr( hContact, szCachedSettingName, 1 );
		if ( pCachedValue != NULL )
			pCachedValue->type = DBVT_DELETED;
	}

	logg();
	return 1;
}

STDMETHODIMP_(BOOL) CDb3Base::GetContactSetting(HANDLE hContact, DBCONTACTGETSETTING *dgs)
{
	dgs->pValue->type = 0;
	if ( GetContactSettingWorker(hContact, dgs, 0 ))
		return 1;

	if ( dgs->pValue->type == DBVT_UTF8 ) {
		WCHAR* tmp = NULL;
		char*  p = NEWSTR_ALLOCA(dgs->pValue->pszVal);
		if ( mir_utf8decode( p, &tmp ) != NULL ) {
			BOOL bUsed = FALSE;
			int  result = WideCharToMultiByte( m_codePage, WC_NO_BEST_FIT_CHARS, tmp, -1, NULL, 0, NULL, &bUsed );

			mir_free( dgs->pValue->pszVal );

			if ( bUsed || result == 0 ) {
				dgs->pValue->type = DBVT_WCHAR;
				dgs->pValue->pwszVal = tmp;
			}
			else {
				dgs->pValue->type = DBVT_ASCIIZ;
				dgs->pValue->pszVal = (char *)mir_alloc(result);
				WideCharToMultiByte( m_codePage, WC_NO_BEST_FIT_CHARS, tmp, -1, dgs->pValue->pszVal, result, NULL, NULL );
				mir_free( tmp );
			}
		}
		else {
			dgs->pValue->type = DBVT_ASCIIZ;
			mir_free( tmp );
	}	}

	return 0;
}

STDMETHODIMP_(BOOL) CDb3Base::GetContactSettingStr(HANDLE hContact, DBCONTACTGETSETTING *dgs)
{
	int iSaveType = dgs->pValue->type;

	if ( GetContactSettingWorker(hContact, dgs, 0 ))
		return 1;

	if ( iSaveType == 0 || iSaveType == dgs->pValue->type )
		return 0;

	if ( dgs->pValue->type != DBVT_ASCIIZ && dgs->pValue->type != DBVT_UTF8 )
		return 1;

	if ( iSaveType == DBVT_WCHAR ) {
		if ( dgs->pValue->type != DBVT_UTF8 ) {
			int len = MultiByteToWideChar( CP_ACP, 0, dgs->pValue->pszVal, -1, NULL, 0 );
			wchar_t* wszResult = ( wchar_t* )mir_alloc(( len+1 )*sizeof( wchar_t ));
			if ( wszResult == NULL )
				return 1;

			MultiByteToWideChar( CP_ACP, 0, dgs->pValue->pszVal, -1, wszResult, len );
			wszResult[ len ] = 0;
			mir_free( dgs->pValue->pszVal );
			dgs->pValue->pwszVal = wszResult;
		}
		else {
			char* savePtr = NEWSTR_ALLOCA(dgs->pValue->pszVal);
			mir_free( dgs->pValue->pszVal );
			if ( !mir_utf8decode( savePtr, &dgs->pValue->pwszVal ))
				return 1;
		}
	}
	else if ( iSaveType == DBVT_UTF8 ) {
		char* tmpBuf = mir_utf8encode( dgs->pValue->pszVal );
		if ( tmpBuf == NULL )
			return 1;

		mir_free( dgs->pValue->pszVal );
		dgs->pValue->pszVal = tmpBuf;
	}
	else if ( iSaveType == DBVT_ASCIIZ )
		mir_utf8decode( dgs->pValue->pszVal, NULL );

	dgs->pValue->type = iSaveType;
	return 0;
}

STDMETHODIMP_(BOOL) CDb3Base::GetContactSettingStatic(HANDLE hContact, DBCONTACTGETSETTING *dgs)
{
	if ( GetContactSettingWorker(hContact, dgs, 1 ))
		return 1;

	if ( dgs->pValue->type == DBVT_UTF8 ) {
		mir_utf8decode( dgs->pValue->pszVal, NULL );
		dgs->pValue->type = DBVT_ASCIIZ;
	}

	return 0;
}

STDMETHODIMP_(BOOL) CDb3Base::FreeVariant(DBVARIANT *dbv)
{
	if ( dbv == 0 ) return 1;
	switch ( dbv->type ) {
		case DBVT_ASCIIZ:
		case DBVT_UTF8:
		case DBVT_WCHAR:
		{
			if ( dbv->pszVal ) mir_free(dbv->pszVal);
			dbv->pszVal = 0;
			break;
		}
		case DBVT_BLOB:
		{
			if ( dbv->pbVal ) mir_free(dbv->pbVal);
			dbv->pbVal = 0;
			break;
		}
	}
	dbv->type = 0;
	return 0;
}

STDMETHODIMP_(BOOL) CDb3Base::SetSettingResident(BOOL bIsResident, const char *pszSettingName)
{
	char *szSetting = m_cache->GetCachedSetting(NULL, pszSettingName, 0, strlen(pszSettingName));
	szSetting[-1] = (char)bIsResident;

	mir_cslock lck(m_csDbAccess);
	int idx = m_lResidentSettings.getIndex(szSetting);
	if (idx == -1) {
		if (bIsResident)
			m_lResidentSettings.insert(szSetting);
	}
	else if (!bIsResident)
		m_lResidentSettings.remove(idx);

	return 0;
}

STDMETHODIMP_(BOOL) CDb3Base::WriteContactSetting(HANDLE hContact, DBCONTACTWRITESETTING *dbcws)
{
	DBCONTACTWRITESETTING tmp;
	DWORD ofsModuleName;
	DBContactSettings dbcs;
	PBYTE pBlob;
	int settingNameLen = 0;
	int moduleNameLen = 0;
	int settingDataLen = 0;
	int bytesRequired,bytesRemaining;
	DWORD ofsContact,ofsSettingsGroup,ofsBlobPtr;

	if (dbcws == NULL || dbcws->szSetting == NULL || dbcws->szModule == NULL )
		return 1;

	// the db format can't tolerate more than 255 bytes of space (incl. null) for settings+module name
	settingNameLen = (int)strlen(dbcws->szSetting);
	moduleNameLen = (int)strlen(dbcws->szModule);
	if ( settingNameLen > 0xFE )
	{
		#ifdef _DEBUG
			OutputDebugStringA("WriteContactSetting() got a > 255 setting name length. \n");
		#endif
		return 1;
	}
	if ( moduleNameLen > 0xFE )
	{
		#ifdef _DEBUG
			OutputDebugStringA("WriteContactSetting() got a > 255 module name length. \n");
		#endif
		return 1;
	}

	tmp = *dbcws;

	if (tmp.value.type == DBVT_WCHAR) {
		if (tmp.value.pszVal != NULL) {
			char* val = mir_utf8encodeW(tmp.value.pwszVal);
			if ( val == NULL )
				return 1;

			tmp.value.pszVal = ( char* )alloca( strlen( val )+1 );
			strcpy( tmp.value.pszVal, val );
			mir_free(val);
			tmp.value.type = DBVT_UTF8;
		}
		else return 1;
	}

	if (tmp.value.type != DBVT_BYTE && tmp.value.type != DBVT_WORD && tmp.value.type != DBVT_DWORD && tmp.value.type != DBVT_ASCIIZ && tmp.value.type != DBVT_UTF8 && tmp.value.type != DBVT_BLOB)
		return 1;
	if ((!tmp.szModule) || (!tmp.szSetting) || ((tmp.value.type == DBVT_ASCIIZ || tmp.value.type == DBVT_UTF8 )&& tmp.value.pszVal == NULL) || (tmp.value.type == DBVT_BLOB && tmp.value.pbVal == NULL))
		return 1;

	// the db can not tolerate strings/blobs longer than 0xFFFF since the format writes 2 lengths
	switch( tmp.value.type ) {
	case DBVT_ASCIIZ:		case DBVT_BLOB:	case DBVT_UTF8:
		{	size_t len = ( tmp.value.type != DBVT_BLOB ) ? strlen(tmp.value.pszVal) : tmp.value.cpbVal;
			if ( len >= 0xFFFF ) {
				#ifdef _DEBUG
					OutputDebugStringA("WriteContactSetting() writing huge string/blob, rejecting ( >= 0xFFFF ) \n");
				#endif
				return 1;
			}
		}
	}

	mir_cslockfull lck(m_csDbAccess);

	char* szCachedSettingName = m_cache->GetCachedSetting(tmp.szModule, tmp.szSetting, moduleNameLen, settingNameLen);
	log3("set [%08p] %s (%p)", hContact, szCachedSettingName, szCachedSettingName);

	if ( tmp.value.type != DBVT_BLOB ) {
		DBVARIANT *pCachedValue = m_cache->GetCachedValuePtr(hContact, szCachedSettingName, 1);
		if ( pCachedValue != NULL ) {
			bool bIsIdentical = false;
			if ( pCachedValue->type == tmp.value.type ) {
				switch(tmp.value.type) {
					case DBVT_BYTE:   bIsIdentical = pCachedValue->bVal == tmp.value.bVal;  break;
					case DBVT_WORD:   bIsIdentical = pCachedValue->wVal == tmp.value.wVal;  break;
					case DBVT_DWORD:  bIsIdentical = pCachedValue->dVal == tmp.value.dVal;  break;
					case DBVT_UTF8:
					case DBVT_ASCIIZ: bIsIdentical = strcmp( pCachedValue->pszVal, tmp.value.pszVal ) == 0; break;
				}
				if ( bIsIdentical )
					return 0;
			}
			m_cache->SetCachedVariant(&tmp.value, pCachedValue);
		}
		if ( szCachedSettingName[-1] != 0 ) {
			lck.unlock();
			log2(" set resident as %s (%p)", printVariant(&tmp.value), pCachedValue);
			NotifyEventHooks(hSettingChangeEvent, (WPARAM)hContact, (LPARAM)&tmp);
			return 0;
		}
	}
	else m_cache->GetCachedValuePtr(hContact, szCachedSettingName, -1);

	log1(" write database as %s", printVariant(&tmp.value));

	ofsModuleName = GetModuleNameOfs(tmp.szModule);
 	if (hContact == 0) ofsContact = m_dbHeader.ofsUser;
	else ofsContact = (DWORD)hContact;

	DBContact dbc = *(DBContact*)DBRead(ofsContact,sizeof(DBContact),NULL);
	if (dbc.signature != DBCONTACT_SIGNATURE)
		return 1;

	//make sure the module group exists
	ofsSettingsGroup = GetSettingsGroupOfsByModuleNameOfs(&dbc,ofsContact,ofsModuleName);
	if (ofsSettingsGroup == 0) {  //module group didn't exist - make it
		if (tmp.value.type & DBVTF_VARIABLELENGTH) {
		  if (tmp.value.type == DBVT_ASCIIZ || tmp.value.type == DBVT_UTF8) bytesRequired = (int)strlen(tmp.value.pszVal)+2;
		  else if (tmp.value.type == DBVT_BLOB) bytesRequired = tmp.value.cpbVal+2;
		}
		else bytesRequired = tmp.value.type;
		bytesRequired += 2+settingNameLen;
		bytesRequired += (DB_SETTINGS_RESIZE_GRANULARITY-(bytesRequired%DB_SETTINGS_RESIZE_GRANULARITY))%DB_SETTINGS_RESIZE_GRANULARITY;
		ofsSettingsGroup = CreateNewSpace(bytesRequired+offsetof(DBContactSettings,blob));
		dbcs.signature = DBCONTACTSETTINGS_SIGNATURE;
		dbcs.ofsNext = dbc.ofsFirstSettings;
		dbcs.ofsModuleName = ofsModuleName;
		dbcs.cbBlob = bytesRequired;
		dbcs.blob[0] = 0;
		dbc.ofsFirstSettings = ofsSettingsGroup;
		DBWrite(ofsContact,&dbc,sizeof(DBContact));
		DBWrite(ofsSettingsGroup,&dbcs,sizeof(DBContactSettings));
		ofsBlobPtr = ofsSettingsGroup+offsetof(DBContactSettings,blob);
		pBlob = (PBYTE)DBRead(ofsBlobPtr,1,&bytesRemaining);
	}
	else {
		dbcs = *(DBContactSettings*)DBRead(ofsSettingsGroup,sizeof(DBContactSettings),&bytesRemaining);
		//find if the setting exists
		ofsBlobPtr = ofsSettingsGroup+offsetof(DBContactSettings,blob);
		pBlob = (PBYTE)DBRead(ofsBlobPtr,1,&bytesRemaining);
		while (pBlob[0]) {
			NeedBytes(settingNameLen+1);
			if (pBlob[0] == settingNameLen && !memcmp(pBlob+1,tmp.szSetting,settingNameLen))
				break;
			NeedBytes(1);
			MoveAlong(pBlob[0]+1);
			NeedBytes(3);
			MoveAlong(1+GetSettingValueLength(pBlob));
			NeedBytes(1);
		}
		if (pBlob[0]) {	 //setting already existed, and up to end of name is in cache
			MoveAlong(1+settingNameLen);
			//if different type or variable length and length is different
			NeedBytes(3);
			if (pBlob[0] != tmp.value.type || ((pBlob[0] == DBVT_ASCIIZ || pBlob[0] == DBVT_UTF8) && *(PWORD)(pBlob+1) != strlen(tmp.value.pszVal)) || (pBlob[0] == DBVT_BLOB && *(PWORD)(pBlob+1) != tmp.value.cpbVal)) {
				//bin it
				int nameLen,valLen;
				DWORD ofsSettingToCut;
				NeedBytes(3);
				nameLen = 1+settingNameLen;
				valLen = 1+GetSettingValueLength(pBlob);
				ofsSettingToCut = ofsBlobPtr-nameLen;
				MoveAlong(valLen);
				NeedBytes(1);
				while (pBlob[0]) {
					MoveAlong(pBlob[0]+1);
					NeedBytes(3);
					MoveAlong(1+GetSettingValueLength(pBlob));
					NeedBytes(1);
				}
				DBMoveChunk(ofsSettingToCut,ofsSettingToCut+nameLen+valLen,ofsBlobPtr+1-ofsSettingToCut);
				ofsBlobPtr -= nameLen+valLen;
				pBlob = (PBYTE)DBRead(ofsBlobPtr,1,&bytesRemaining);
			}
			else {
				//replace existing setting at pBlob
				MoveAlong(1);	//skip data type
				switch(tmp.value.type) {
					case DBVT_BYTE: DBWrite(ofsBlobPtr,&tmp.value.bVal,1); break;
					case DBVT_WORD: EncodeDBWrite(ofsBlobPtr,&tmp.value.wVal,2); break;
					case DBVT_DWORD: EncodeDBWrite(ofsBlobPtr,&tmp.value.dVal,4); break;
					case DBVT_UTF8:
					case DBVT_ASCIIZ: EncodeDBWrite(ofsBlobPtr+2,tmp.value.pszVal,(int)strlen(tmp.value.pszVal)); break;
					case DBVT_BLOB: EncodeDBWrite(ofsBlobPtr+2,tmp.value.pbVal,tmp.value.cpbVal); break;
				}
				//quit
				DBFlush(1);
				lck.unlock();
				//notify
				NotifyEventHooks(hSettingChangeEvent, (WPARAM)hContact, (LPARAM)&tmp);
				return 0;
			}
		}
	}
	//cannot do a simple replace, add setting to end of list
	//pBlob already points to end of list
	//see if it fits
	if (tmp.value.type&DBVTF_VARIABLELENGTH) {
	  if (tmp.value.type == DBVT_ASCIIZ || tmp.value.type == DBVT_UTF8) bytesRequired = (int)strlen(tmp.value.pszVal)+2;
	  else if (tmp.value.type == DBVT_BLOB) bytesRequired = tmp.value.cpbVal+2;
	}
	else bytesRequired = tmp.value.type;
	bytesRequired += 2+settingNameLen;
	bytesRequired += ofsBlobPtr+1-(ofsSettingsGroup+offsetof(DBContactSettings,blob));
	if ((DWORD)bytesRequired > dbcs.cbBlob) {
		//doesn't fit: move entire group
		DBContactSettings *dbcsPrev;
		DWORD ofsDbcsPrev,ofsNew;

		InvalidateSettingsGroupOfsCacheEntry(ofsSettingsGroup);
		bytesRequired += (DB_SETTINGS_RESIZE_GRANULARITY-(bytesRequired%DB_SETTINGS_RESIZE_GRANULARITY))%DB_SETTINGS_RESIZE_GRANULARITY;
		//find previous group to change its offset
		ofsDbcsPrev = dbc.ofsFirstSettings;
		if (ofsDbcsPrev == ofsSettingsGroup) ofsDbcsPrev = 0;
		else {
			dbcsPrev = (DBContactSettings*)DBRead(ofsDbcsPrev,sizeof(DBContactSettings),NULL);
			while (dbcsPrev->ofsNext != ofsSettingsGroup) {
				if (dbcsPrev->ofsNext == 0) DatabaseCorruption(NULL);
				ofsDbcsPrev = dbcsPrev->ofsNext;
				dbcsPrev = (DBContactSettings*)DBRead(ofsDbcsPrev,sizeof(DBContactSettings),NULL);
			}
		}

		//create the new one
		ofsNew = ReallocSpace(ofsSettingsGroup, dbcs.cbBlob+offsetof(DBContactSettings,blob), bytesRequired+offsetof(DBContactSettings,blob));

		dbcs.cbBlob = bytesRequired;

		DBWrite(ofsNew,&dbcs,offsetof(DBContactSettings,blob));
		if (ofsDbcsPrev == 0) {
			dbc.ofsFirstSettings = ofsNew;
			DBWrite(ofsContact,&dbc,sizeof(DBContact));
		}
		else {
			dbcsPrev = (DBContactSettings*)DBRead(ofsDbcsPrev,sizeof(DBContactSettings),NULL);
			dbcsPrev->ofsNext = ofsNew;
			DBWrite(ofsDbcsPrev,dbcsPrev,offsetof(DBContactSettings,blob));
		}
		ofsBlobPtr += ofsNew-ofsSettingsGroup;
		ofsSettingsGroup = ofsNew;
		pBlob = (PBYTE)DBRead(ofsBlobPtr,1,&bytesRemaining);
	}
	//we now have a place to put it and enough space: make it
	DBWrite(ofsBlobPtr,&settingNameLen,1);
	DBWrite(ofsBlobPtr+1,(PVOID)tmp.szSetting,settingNameLen);
	MoveAlong(1+settingNameLen);
	DBWrite(ofsBlobPtr,&tmp.value.type,1);
	MoveAlong(1);
	switch(tmp.value.type) {
		case DBVT_BYTE: DBWrite(ofsBlobPtr,&tmp.value.bVal,1); MoveAlong(1); break;
		case DBVT_WORD: EncodeDBWrite(ofsBlobPtr,&tmp.value.wVal,2); MoveAlong(2); break;
		case DBVT_DWORD: EncodeDBWrite(ofsBlobPtr,&tmp.value.dVal,4); MoveAlong(4); break;
		case DBVT_UTF8:
		case DBVT_ASCIIZ:
			{	int len = (int)strlen(tmp.value.pszVal);
				DBWrite(ofsBlobPtr,&len,2);
				EncodeDBWrite(ofsBlobPtr+2,tmp.value.pszVal,len);
				MoveAlong(2+len);
			}
			break;
		case DBVT_BLOB:
			DBWrite(ofsBlobPtr,&tmp.value.cpbVal,2)	;
			EncodeDBWrite(ofsBlobPtr+2,tmp.value.pbVal,tmp.value.cpbVal);
			MoveAlong(2+tmp.value.cpbVal);
			break;
	}
	
	BYTE zero = 0;
	DBWrite(ofsBlobPtr,&zero,1);
	
	//quit
	DBFlush(1);
	lck.unlock();

	//notify
	NotifyEventHooks(hSettingChangeEvent, (WPARAM)hContact, (LPARAM)&tmp );
	return 0;
}

STDMETHODIMP_(BOOL) CDb3Base::DeleteContactSetting(HANDLE hContact, DBCONTACTGETSETTING *dbcgs)
{
	DBContact *dbc;
	DWORD ofsModuleName,ofsSettingsGroup,ofsBlobPtr;
	PBYTE pBlob;
	int settingNameLen,moduleNameLen,bytesRemaining;
	char* szCachedSettingName;
	WPARAM saveWparam = (WPARAM)hContact;

	if ( !dbcgs->szModule || !dbcgs->szSetting)
		return 1;

	// the db format can't tolerate more than 255 bytes of space (incl. null) for settings+module name
	settingNameLen = (int)strlen(dbcgs->szSetting);
	moduleNameLen = (int)strlen(dbcgs->szModule);
	if ( settingNameLen > 0xFE ) {
		#ifdef _DEBUG
			OutputDebugStringA("DeleteContactSetting() got a > 255 setting name length. \n");
		#endif
		return 1;
	}
	if ( moduleNameLen > 0xFE ) {
		#ifdef _DEBUG
			OutputDebugStringA("DeleteContactSetting() got a > 255 module name length. \n");
		#endif
		return 1;
	}

	mir_cslockfull lck(m_csDbAccess);
	ofsModuleName = GetModuleNameOfs(dbcgs->szModule);
 	if (hContact == 0)
		hContact = (HANDLE)m_dbHeader.ofsUser;

	dbc = (DBContact*)DBRead(hContact,sizeof(DBContact),NULL);
	if (dbc->signature != DBCONTACT_SIGNATURE)
		return 1;

	//make sure the module group exists
	ofsSettingsGroup = GetSettingsGroupOfsByModuleNameOfs(dbc,(DWORD)hContact,ofsModuleName);
	if (ofsSettingsGroup == 0)
		return 1;

	//find if the setting exists
	ofsBlobPtr = ofsSettingsGroup+offsetof(DBContactSettings,blob);
	pBlob = (PBYTE)DBRead(ofsBlobPtr,1,&bytesRemaining);
	while (pBlob[0]) {
		NeedBytes(settingNameLen+1);
		if (pBlob[0] == settingNameLen && !memcmp(pBlob+1,dbcgs->szSetting,settingNameLen))
			break;
		NeedBytes(1);
		MoveAlong(pBlob[0]+1);
		NeedBytes(3);
		MoveAlong(1+GetSettingValueLength(pBlob));
		NeedBytes(1);
	}
	if (!pBlob[0]) //setting didn't exist
		return 1;

	//bin it
	int nameLen,valLen;
	DWORD ofsSettingToCut;
	MoveAlong(1+settingNameLen);
	NeedBytes(3);
	nameLen = 1+settingNameLen;
	valLen = 1+GetSettingValueLength(pBlob);
	ofsSettingToCut = ofsBlobPtr-nameLen;
	MoveAlong(valLen);
	NeedBytes(1);
	while (pBlob[0]) {
		MoveAlong(pBlob[0]+1);
		NeedBytes(3);
		MoveAlong(1+GetSettingValueLength(pBlob));
		NeedBytes(1);
	}
	DBMoveChunk(ofsSettingToCut,ofsSettingToCut+nameLen+valLen,ofsBlobPtr+1-ofsSettingToCut);

	szCachedSettingName = m_cache->GetCachedSetting(dbcgs->szModule,dbcgs->szSetting,moduleNameLen,settingNameLen);
	m_cache->GetCachedValuePtr((HANDLE)saveWparam, szCachedSettingName, -1 );

	//quit
	DBFlush(1);
	lck.unlock();
	
	//notify
	DBCONTACTWRITESETTING dbcws = {0};
	dbcws.szModule = dbcgs->szModule;
	dbcws.szSetting = dbcgs->szSetting;
	dbcws.value.type = DBVT_DELETED;
	NotifyEventHooks(hSettingChangeEvent,saveWparam,(LPARAM)&dbcws);
	return 0;
}

STDMETHODIMP_(BOOL) CDb3Base::EnumContactSettings(HANDLE hContact, DBCONTACTENUMSETTINGS* dbces)
{
	DBContact *dbc;
	DWORD ofsModuleName,ofsContact,ofsBlobPtr;
	int bytesRemaining, result;
	PBYTE pBlob;
	char szSetting[256];

	if (!dbces->szModule)
		return -1;

	mir_cslock lck(m_csDbAccess);

	ofsModuleName = GetModuleNameOfs(dbces->szModule);
	if (hContact == 0) ofsContact = m_dbHeader.ofsUser;
	else ofsContact = (DWORD)hContact;
	dbc = (DBContact*)DBRead(ofsContact,sizeof(DBContact),NULL);
	if (dbc->signature != DBCONTACT_SIGNATURE)
		return -1;

	dbces->ofsSettings = GetSettingsGroupOfsByModuleNameOfs(dbc,ofsContact,ofsModuleName);
	if ( !dbces->ofsSettings)
		return -1;

	ofsBlobPtr = dbces->ofsSettings+offsetof(DBContactSettings,blob);
	pBlob = (PBYTE)DBRead(ofsBlobPtr,1,&bytesRemaining);
	if (pBlob[0] == 0)
		return -1;

	result = 0;
	while (pBlob[0]) {
		NeedBytes(1);
		NeedBytes(1+pBlob[0]);
		CopyMemory(szSetting,pBlob+1,pBlob[0]); szSetting[pBlob[0]] = 0;
		result = (dbces->pfnEnumProc)(szSetting,dbces->lParam);
		MoveAlong(1+pBlob[0]);
		NeedBytes(3);
		MoveAlong(1+GetSettingValueLength(pBlob));
		NeedBytes(1);
	}
	return result;
}

STDMETHODIMP_(BOOL) CDb3Base::EnumResidentSettings(DBMODULEENUMPROC pFunc, void *pParam)
{
	for (int i = 0; i < m_lResidentSettings.getCount(); i++) {
		int ret = pFunc(m_lResidentSettings[i], 0, (LPARAM)pParam);
		if (ret) return ret;
	}
	return 0;
}
