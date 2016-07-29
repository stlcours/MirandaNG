/*

'File Association Manager'-Plugin for Miranda IM

Copyright (C) 2005-2007 H. Herkenrath

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program (AssocMgr-License.txt); if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "stdafx.h"

extern HINSTANCE hInst;

#ifdef _DEBUG
/* Debug: Ensure all registry calls do succeed and have valid parameters. 
 * Shows a details message box otherwise. */
static __inline LONG regchk(LONG res, const char *pszFunc, const void *pszInfo, BOOL fInfoUnicode, const char *pszFile, unsigned int nLine)
{
	if (res != ERROR_SUCCESS && res != ERROR_FILE_NOT_FOUND && res != ERROR_NO_MORE_ITEMS) {
		wchar_t szMsg[1024], *pszInfo2;
		char *pszErr;
		pszErr = GetWinErrorDescription(res);
		pszInfo2 = s2t(pszInfo, fInfoUnicode, FALSE);  /* does NULL check */
		mir_snwprintf(szMsg, TranslateT("Access failed:\n%.64hs(%.128s)\n%.250hs(%u)\n%.256hs (%u)"), pszFunc, pszInfo2, pszFile, nLine, pszErr, res);
		MessageBox(NULL, szMsg, TranslateT("Registry warning"), MB_OK | MB_ICONINFORMATION | MB_SETFOREGROUND | MB_TOPMOST | MB_TASKMODAL);
		if (pszErr != NULL) LocalFree(pszErr);
		mir_free(pszInfo2);  /* does NULL check */
	}
	return res;
}
#undef  RegCloseKey
#define RegCloseKey(hKey) \
	regchk(RegCloseKey(hKey),"RegCloseKey",NULL,FALSE,__FILE__,__LINE__)
#undef  RegOpenKeyExA
#define RegOpenKeyExA(hKey,szSubkey,opt,rights,phKey) \
	regchk(RegOpenKeyExA(hKey,szSubkey,opt,rights,phKey),"RegOpenKeyExA",szSubkey,FALSE,__FILE__,__LINE__)
#undef  RegCreateKeyExA
#define RegCreateKeyExA(hKey,szSubkey,x,y,opt,rights,sec,phKey,pDisp) \
	regchk(RegCreateKeyExA(hKey,szSubkey,x,y,opt,rights,sec,phKey,pDisp),"RegCreateKeyExA",szSubkey,FALSE,__FILE__,__LINE__)
#undef  RegDeleteKeyA
#define RegDeleteKeyA(hKey,szName) \
	regchk(RegDeleteKeyA(hKey,szName),"RegDeleteKeyA",szName,FALSE,__FILE__,__LINE__)
#undef  RegSetValueExA
#define RegSetValueExA(hSubKey,szName,x,type,pVal,size) \
	regchk(RegSetValueExA(hSubKey,szName,x,type,pVal,size),"RegSetValueExA",szName,FALSE,__FILE__,__LINE__)
#undef  RegQueryValueExA
#define RegQueryValueExA(hKey,szName,x,pType,pVal,pSize) \
	regchk(RegQueryValueExA(hKey,szName,x,pType,pVal,pSize),"RegQueryValueExA",szName,FALSE,__FILE__,__LINE__)
#undef  RegQueryInfoKeyA
#define RegQueryInfoKeyA(hKey,x,y,z,pnKeys,pnKeyLen,a,pnVals,pnNames,pnValLen,sec,pTime) \
	regchk(RegQueryInfoKeyA(hKey,x,y,z,pnKeys,pnKeyLen,a,pnVals,pnNames,pnValLen,sec,pTime),"RegQueryInfoKeyA",NULL,FALSE,__FILE__,__LINE__)
#undef  RegEnumKeyExA
#define RegEnumKeyExA(hKey,idx,pName,pnName,x,y,z,pTime) \
	regchk(RegEnumKeyExA(hKey,idx,pName,pnName,x,y,z,pTime),"RegEnumKeyExA",NULL,FALSE,__FILE__,__LINE__)
#undef  RegDeleteValueA
#define RegDeleteValueA(hKey,szName) \
	regchk(RegDeleteValueA(hKey,szName),"RegDeleteValueA",szName,FALSE,__FILE__,__LINE__)
#undef  RegOpenKeyExW
#define RegOpenKeyExW(hKey,szSubkey,x,sam,phKey) \
	regchk(RegOpenKeyExW(hKey,szSubkey,x,sam,phKey),"RegOpenKeyExW",szSubkey,TRUE,__FILE__,__LINE__)
#undef  RegCreateKeyExW
#define RegCreateKeyExW(hKey,szSubkey,x,y,z,rights,p,phKey,q) \
	regchk(RegCreateKeyExW(hKey,szSubkey,x,y,z,rights,p,phKey,q),"RegCreateKeyExW",szSubkey,TRUE,__FILE__,__LINE__)
#undef  RegDeleteKeyW
#define RegDeleteKeyW(hKey,szName) \
	regchk(RegDeleteKeyW(hKey,szName),"RegDeleteKeyW",szName,TRUE,__FILE__,__LINE__)
#undef  RegSetValueExW
#define RegSetValueExW(hSubKey,szName,x,type,pVal,size) \
	regchk(RegSetValueExW(hSubKey,szName,x,type,pVal,size),"RegSetValueExW",szName,TRUE,__FILE__,__LINE__)
#undef  RegQueryValueExW
#define RegQueryValueExW(hKey,szName,x,pType,pVal,pSize) \
	regchk(RegQueryValueExW(hKey,szName,x,pType,pVal,pSize),"RegQueryValueExW",szName,TRUE,__FILE__,__LINE__)
#undef  RegQueryInfoKeyW
#define RegQueryInfoKeyW(hKey,x,y,z,pnKeys,pnKeyLen,a,pnVals,pnNames,pnValLen,sec,pTime) \
	regchk(RegQueryInfoKeyW(hKey,x,y,z,pnKeys,pnKeyLen,a,pnVals,pnNames,pnValLen,sec,pTime),"RegQueryInfoKeyW",NULL,TRUE,__FILE__,__LINE__)
#undef  RegEnumKeyExW
#define RegEnumKeyExW(hKey,idx,pName,pnName,x,y,z,pTime) \
	regchk(RegEnumKeyExW(hKey,idx,pName,pnName,x,y,z,pTime),"RegEnumKeyExW",NULL,TRUE,__FILE__,__LINE__)
#undef  RegDeleteValueW
#define RegDeleteValueW(hKey,szName) \
	regchk(RegDeleteValueW(hKey,szName),"RegDeleteValueW",szName,TRUE,__FILE__,__LINE__)
#endif // _DEBUG

/************************* Strings ********************************/

// mir_free() the return value
char *MakeFileClassName(const char *pszFileExt)
{
	size_t cbLen = mir_strlen(pszFileExt)+12;
	char *pszClass = (char*)mir_alloc(cbLen);
	if (pszClass != NULL)
		/* using correctly formated PROGID */
		mir_snprintf(pszClass, cbLen, "miranda%sfile", pszFileExt); /* includes dot, buffer safe */
	return pszClass;
}

// mir_free() the return value
char *MakeUrlClassName(const char *pszUrl)
{
	char *pszClass = mir_strdup(pszUrl);
	if (pszClass != NULL)
		/* remove trailing : */
		pszClass[mir_strlen(pszClass)-1]=0;
	return pszClass;
}

static BOOL IsFileClassName(char *pszClassName, char **ppszFileExt)
{
	*ppszFileExt = strchr(pszClassName,'.');
	return *ppszFileExt!=NULL;
}

// mir_free() the return value
wchar_t *MakeRunCommand(BOOL fMirExe,BOOL fFixedDbProfile)
{
	wchar_t szDbFile[MAX_PATH], szExe[MAX_PATH], *pszFmt;
	if (fFixedDbProfile) {
		if ( CallService(MS_DB_GETPROFILENAMEW, _countof(szDbFile), (LPARAM)szDbFile))
			return NULL;
		wchar_t *p = wcsrchr(szDbFile, '.');
		if (p)
			*p = 0;
	}
	else mir_wstrcpy(szDbFile, L"%1"); /* buffer safe */

	if ( !GetModuleFileName(fMirExe ? NULL : hInst, szExe, _countof(szExe)))
		return NULL;

	if (fMirExe)
		/* run command for miranda32.exe */
			pszFmt = L"\"%s\" \"/profile:%s\"";
	else {
		/* run command for rundll32.exe calling WaitForDDE */
		pszFmt = L"rundll32.exe %s,WaitForDDE \"/profile:%s\"";
		/* ensure the command line is not too long */ 
		GetShortPathName(szExe, szExe, _countof(szExe));
		/* surround by quotes if failed */
		size_t len = mir_wstrlen(szExe);
		if ( wcschr(szExe,' ') != NULL && (len+2) < _countof(szExe)) {
			memmove(szExe, szExe+1, (len+1)*sizeof(wchar_t));
			szExe[len+2] = szExe[0] = '\"';
			szExe[len+3] = 0;
		}
	}

	wchar_t tszBuffer[1024];
	mir_snwprintf(tszBuffer, pszFmt, szExe, szDbFile);
	return mir_wstrdup(tszBuffer);
}

static BOOL IsValidRunCommand(const wchar_t *pszRunCmd)
{
	wchar_t *buf,*pexe,*pargs;
	wchar_t szFullExe[MAX_PATH],*pszFilePart;
	buf=mir_wstrcpy((wchar_t*)_alloca((mir_wstrlen(pszRunCmd)+1)*sizeof(wchar_t)),pszRunCmd);
	/* split into executable path and arguments */
	if (buf[0]=='\"') {
		pargs=wcschr(&buf[1],'\"');
		if (pargs!=NULL) *(pargs++)=0;
		pexe=&buf[1];
		if (*pargs==' ') ++pargs;
	} else {
		pargs=wcschr(buf,' ');
		if (pargs!=NULL) *pargs=0;
		pexe=buf;
	}
	if (SearchPath(NULL,pexe,L".exe",_countof(szFullExe),szFullExe,&pszFilePart)) {
		if (pszFilePart!=NULL)
			if (!mir_wstrcmpi(pszFilePart,L"rundll32.exe") || !mir_wstrcmpi(pszFilePart,L"rundll.exe")) {
				/* split into dll path and arguments */
				if (pargs[0]=='\"') {
					++pargs;
					pexe=wcschr(&pargs[1],'\"');
					if (pexe!=NULL) *pexe=0;
				} else {
					pexe=wcschr(pargs,',');
					if (pexe!=NULL) *pexe=0;
				}
				return SearchPath(NULL,pargs,L".dll",0,NULL,NULL)!=0;
			}
			return TRUE;
	}
	return FALSE;
}

// mir_free() the return value
wchar_t *MakeIconLocation(HMODULE hModule,WORD nIconResID)
{
	wchar_t szModule[MAX_PATH],*pszIconLoc=NULL;
	int cch;
	if ((cch=GetModuleFileName(hModule,szModule,_countof(szModule))) != 0) {
		pszIconLoc=(wchar_t*)mir_alloc((cch+=8)*sizeof(wchar_t));
		if (pszIconLoc!=NULL)
			mir_snwprintf(pszIconLoc, cch, L"%s,%i", szModule, -(int)nIconResID); /* id may be 0, buffer safe */
	}
	return pszIconLoc;
}

// mir_free() the return value
wchar_t *MakeAppFileName(BOOL fMirExe)
{
	wchar_t szExe[MAX_PATH],*psz;
	if (GetModuleFileName(fMirExe?NULL:hInst,szExe,_countof(szExe))) {
		psz=wcsrchr(szExe,'\\');
		if (psz!=NULL) ++psz;
		else psz=szExe;
		return mir_wstrdup(psz);
	}
	return NULL;
}

/************************* Helpers ********************************/

static LONG DeleteRegSubTree(HKEY hKey,const wchar_t *pszSubKey)
{
	LONG res;
	DWORD nMaxSubKeyLen,cchSubKey;
	wchar_t *pszSubKeyBuf;
	HKEY hSubKey;
	if ((res=RegOpenKeyEx(hKey,pszSubKey,0,KEY_QUERY_VALUE|KEY_ENUMERATE_SUB_KEYS|DELETE,&hSubKey))==ERROR_SUCCESS) {
		if ((res=RegQueryInfoKey(hSubKey,NULL,NULL,NULL,NULL,&nMaxSubKeyLen,NULL,NULL,NULL,NULL,NULL,NULL))==ERROR_SUCCESS) {
			pszSubKeyBuf=(wchar_t*)mir_alloc((nMaxSubKeyLen+1)*sizeof(wchar_t));
			if (pszSubKeyBuf==NULL) res=ERROR_NOT_ENOUGH_MEMORY;
			while(!res) {
				cchSubKey=nMaxSubKeyLen+1;
				if ((res=RegEnumKeyEx(hSubKey,0,pszSubKeyBuf,&cchSubKey,NULL,NULL,NULL,NULL))==ERROR_SUCCESS)
					res=DeleteRegSubTree(hSubKey,pszSubKeyBuf); /* recursion */
			}
			mir_free(pszSubKeyBuf); /* does NULL check */
			if (res==ERROR_NO_MORE_ITEMS) res=ERROR_SUCCESS;
		}
		RegCloseKey(hSubKey);
	}
	if (!res) res=RegDeleteKey(hKey,pszSubKey);
	return res;
}

// hMainKey must have been opened with KEY_CREATE_SUB_KEY access right
static LONG SetRegSubKeyStrDefValue(HKEY hMainKey,const wchar_t *pszSubKey,const wchar_t *pszVal)
{
	HKEY hSubKey;
	LONG res=RegCreateKeyEx(hMainKey,pszSubKey,0,NULL,0,KEY_SET_VALUE|KEY_QUERY_VALUE,NULL,&hSubKey,NULL);
	if (!res) {
		res=RegSetValueEx(hSubKey,NULL,0,REG_SZ,(BYTE*)pszVal,(int)(mir_wstrlen(pszVal)+1)*sizeof(wchar_t));
		RegCloseKey(hSubKey);
	}
	return res;
}

// hKey must have been opened with KEY_SET_VALUE access right
static void SetRegStrPrefixValue(HKEY hKey,const wchar_t *pszValPrefix,const wchar_t *pszVal)
{
	size_t dwSize = (mir_wstrlen(pszVal)+mir_wstrlen(pszValPrefix)+1)*sizeof(wchar_t);
	wchar_t *pszStr = (wchar_t*)_alloca(dwSize);
	mir_wstrcat(mir_wstrcpy(pszStr, pszValPrefix), pszVal); /* buffer safe */
	RegSetValueEx(hKey, NULL, 0, REG_SZ, (BYTE*)pszStr, (int)dwSize);
}

// hKey must have been opened with KEY_QUERY_VALUE access right
// mir_free() the return value
static wchar_t *GetRegStrValue(HKEY hKey,const wchar_t *pszValName)
{
	wchar_t *pszVal,*pszVal2;
	DWORD dwSize,dwType;
	/* get size */
	if (!RegQueryValueEx(hKey,pszValName,NULL,NULL,NULL,&dwSize) && dwSize>sizeof(wchar_t)) {
		pszVal=(wchar_t*)mir_alloc(dwSize+sizeof(wchar_t));
		if (pszVal!=NULL) {
			/* get value */
			if (!RegQueryValueEx(hKey,pszValName,NULL,&dwType,(BYTE*)pszVal,&dwSize)) {
				pszVal[dwSize/sizeof(wchar_t)]=0;
				if (dwType==REG_EXPAND_SZ) {
					dwSize=MAX_PATH;
					pszVal2=(wchar_t*)mir_alloc(dwSize*sizeof(wchar_t));
					if (ExpandEnvironmentStrings(pszVal,pszVal2,dwSize)) {
						mir_free(pszVal);
						return pszVal2;
					}
					mir_free(pszVal2);
				} else if (dwType==REG_SZ)
					return pszVal;
			}
			mir_free(pszVal);
		}
	}
	return NULL;
}

// hKey must have been opened with KEY_QUERY_VALUE access right
static BOOL IsRegStrValue(HKEY hKey,const wchar_t *pszValName,const wchar_t *pszCmpVal)
{
	BOOL fSame=FALSE;
	wchar_t *pszVal=GetRegStrValue(hKey,pszValName);
	if (pszVal!=NULL) {
		fSame=!mir_wstrcmp(pszVal,pszCmpVal);
		mir_free(pszVal);
	}
	return fSame;
}

// hKey must have been opened with KEY_QUERY_VALUE access right
static BOOL IsRegStrValueA(HKEY hKey,const wchar_t *pszValName,const char *pszCmpVal)
{
	BOOL fSame=FALSE;
	char *pszValA;
	wchar_t *pszVal=GetRegStrValue(hKey,pszValName);
	if (pszVal!=NULL) {
		pszValA=t2a(pszVal);
		if (pszValA!=NULL)
			fSame=!mir_strcmp(pszValA,pszCmpVal);
		mir_free(pszValA); /* does NULL check */
		mir_free(pszVal);
	}
	return fSame;
}

/************************* Backup to DB ***************************/

#define REGF_ANSI  0x80000000 /* this bit is set in dwType for ANSI registry data */ 

// pData must always be Unicode data, registry supports Unicode even on Win95
static void WriteDbBackupData(const char *pszSetting,DWORD dwType,BYTE *pData,DWORD cbData)
{
	size_t cbLen = cbData + sizeof(DWORD);
	PBYTE buf = (PBYTE)mir_alloc(cbLen);
	if (buf) {
		*(DWORD*)buf = dwType;
		memcpy(buf+sizeof(DWORD), pData, cbData);
		db_set_blob(NULL, "AssocMgr", pszSetting, buf, (unsigned)cbLen);
		mir_free(buf);
	}
}

// mir_free() the value returned in ppData 
static BOOL ReadDbBackupData(const char *pszSetting,DWORD *pdwType,BYTE **ppData,DWORD *pcbData)
{
	DBVARIANT dbv;
	if (!db_get(0, "AssocMgr", pszSetting, &dbv)) {
		if (dbv.type==DBVT_BLOB && dbv.cpbVal>=sizeof(DWORD)) {
			*pdwType=*(DWORD*)dbv.pbVal;
			*ppData=dbv.pbVal;
			*pcbData=dbv.cpbVal-sizeof(DWORD);
			memmove(*ppData,*ppData+sizeof(DWORD),*pcbData);
			return TRUE;
		}
		db_free(&dbv);
	}
	return FALSE;
}

struct BackupRegTreeParam {
	char **ppszDbPrefix;
	DWORD *pdwDbPrefixSize;
	int level;
};

static void BackupRegTree_Worker(HKEY hKey,const char *pszSubKey,struct BackupRegTreeParam *param)
{
	LONG res;
	DWORD nMaxSubKeyLen,nMaxValNameLen,nMaxValSize;
	DWORD index,cchName,dwType,cbData;
	BYTE *pData;
	char *pszName;
	register wchar_t *ptszName;
	DWORD nDbPrefixLen;
	if ((res=RegOpenKeyExA(hKey,pszSubKey,0,KEY_QUERY_VALUE|KEY_ENUMERATE_SUB_KEYS,&hKey))==ERROR_SUCCESS) {
		if ((res=RegQueryInfoKey(hKey,NULL,NULL,NULL,NULL,&nMaxSubKeyLen,NULL,NULL,&nMaxValNameLen,&nMaxValSize,NULL,NULL))==ERROR_SUCCESS) {
			if (nMaxSubKeyLen>nMaxValNameLen) nMaxValNameLen=nMaxSubKeyLen;
			/* prepare buffer */
			nDbPrefixLen = (DWORD)(mir_strlen(*param->ppszDbPrefix) + mir_strlen(pszSubKey) + 1);
			cchName = nDbPrefixLen + nMaxValNameLen + 3;
			if (cchName>*param->pdwDbPrefixSize) {
				pszName=(char*)mir_realloc(*param->ppszDbPrefix,cchName);
				if (pszName==NULL) return;
				*param->ppszDbPrefix=pszName;
				*param->pdwDbPrefixSize=cchName;
			}
			mir_strcat(mir_strcat(*param->ppszDbPrefix,pszSubKey),"\\"); /* buffer safe */
			/* enum values */
			pszName=(char*)mir_alloc(nMaxValNameLen+1);
			if (nMaxValSize==0) nMaxValSize=1;
			pData=(BYTE*)mir_alloc(nMaxValSize);
			if (pszName!=NULL && pData!=NULL) {
				index=0;
				while(!res) {
					cchName=nMaxValNameLen+1;
					cbData=nMaxValSize;
					if ((res=RegEnumValueA(hKey,index++,pszName,&cchName,NULL,NULL,NULL,NULL))==ERROR_SUCCESS) {
						(*param->ppszDbPrefix)[nDbPrefixLen]=0;
						mir_strcat(*param->ppszDbPrefix,pszName); /* buffer safe */
						ptszName=a2t(pszName);
						if (ptszName!=NULL) {
							if (!RegQueryValueEx(hKey,ptszName,NULL,&dwType,pData,&cbData)) {

								WriteDbBackupData(*param->ppszDbPrefix,dwType,pData,cbData);

							}
							mir_free(ptszName);
						}
					}
				}
				if (res==ERROR_NO_MORE_ITEMS) res=ERROR_SUCCESS;
			}
			mir_free(pData); /* does NULL check */
			/* enum subkeys */
			if (param->level<32 && pszName!=NULL) {
				++param->level; /* can be max 32 levels deep (after prefix), restriction of RegCreateKeyEx() */
				index=0;
				while(!res) {
					cchName=nMaxSubKeyLen+1;
					if ((res=RegEnumKeyExA(hKey,index++,pszName,&cchName,NULL,NULL,NULL,NULL))==ERROR_SUCCESS) {
						(*param->ppszDbPrefix)[nDbPrefixLen]=0;
						BackupRegTree_Worker(hKey,pszName,param); /* recursion */
					}
				}
			}
			if (res==ERROR_NO_MORE_ITEMS) res=ERROR_SUCCESS;
			mir_free(pszName); /* does NULL check */
		}
		RegCloseKey(hKey);
	}
}

static void BackupRegTree(HKEY hKey, const char *pszSubKey, const char *pszDbPrefix)
{
	char *prefix = mir_strdup(pszDbPrefix);
	struct BackupRegTreeParam param;
	DWORD dwDbPrefixSize;
	param.level = 0;
	param.pdwDbPrefixSize = &dwDbPrefixSize;
	param.ppszDbPrefix = (char**)&prefix;
	dwDbPrefixSize = (int)mir_strlen(prefix) + 1;
	BackupRegTree_Worker(hKey, pszSubKey, &param);
	mir_free(prefix);
}

static LONG RestoreRegTree(HKEY hKey,const char *pszSubKey,const char *pszDbPrefix)
{
	char **ppszSettings,*pszSuffix;
	int nSettingsCount,i;
	char *pslash=NULL,*pnext,*pkeys;
	char *pszValName;
	WCHAR *pwszValName;
	HKEY hSubKey;
	DWORD dwType,cbData;
	BYTE *pData;

	int nDbPrefixLen = (int)mir_strlen(pszDbPrefix);
	int nPrefixWithSubKeyLen = nDbPrefixLen + (int)mir_strlen(pszSubKey) + 1;
	char *pszPrefixWithSubKey = (char*)mir_alloc(nPrefixWithSubKeyLen+1);
	if (pszPrefixWithSubKey == NULL)
		return ERROR_OUTOFMEMORY;
	
	mir_strcat(mir_strcat(mir_strcpy(pszPrefixWithSubKey,pszDbPrefix),pszSubKey),"\\"); /* buffer safe */
	LONG res=ERROR_NO_MORE_ITEMS;
	if (pszPrefixWithSubKey!=NULL) {
		if (EnumDbPrefixSettings("AssocMgr",pszPrefixWithSubKey,&ppszSettings,&nSettingsCount)) {
			for(i=0;i<nSettingsCount;++i) {
				pszSuffix=&ppszSettings[i][nDbPrefixLen];
				/* key hierachy */
				pkeys=mir_strcpy((char*)_alloca(mir_strlen(pszSuffix)+1),pszSuffix);
				pnext=pkeys;
				while((pnext=strchr(pnext+1,'\\'))!=NULL) pslash=pnext;
				if (pslash!=NULL) {
					/* create subkey */
					*(pslash++)=0;
					hSubKey=hKey;
					if (pslash!=pkeys+1)
						if ((res=RegCreateKeyExA(hKey,pkeys,0,NULL,0,KEY_SET_VALUE,NULL,&hSubKey,NULL))!=ERROR_SUCCESS)
							break;
					pszValName=pslash;
					/* read data */
					if (ReadDbBackupData(ppszSettings[i],&dwType,&pData,&cbData)) {
						/* set value */
						if (!(dwType&REGF_ANSI)) {
							pwszValName=a2u(pszValName,FALSE);
							if (pwszValName!=NULL) res=RegSetValueExW(hSubKey,pwszValName,0,dwType,pData,cbData);
							else res=ERROR_NOT_ENOUGH_MEMORY;
							mir_free(pwszValName); /* does NULL check */
						} else res=RegSetValueExA(hSubKey,pszValName,0,dwType&~REGF_ANSI,pData,cbData);
						mir_free(pData);
					} else res=ERROR_INVALID_DATA;
					if (res) break;
					db_unset(NULL,"AssocMgr",ppszSettings[i]);
					if (hSubKey!=hKey) RegCloseKey(hSubKey);
				}
				mir_free(ppszSettings[i]);
			}
			mir_free(ppszSettings);
		}
		mir_free(pszPrefixWithSubKey);
	}
	return res;
}

static void DeleteRegTreeBackup(const char *pszSubKey,const char *pszDbPrefix)
{
	char **ppszSettings;
	int nSettingsCount,i;

	char *pszPrefixWithSubKey=(char*)mir_alloc(mir_strlen(pszDbPrefix)+mir_strlen(pszSubKey)+2);
	if (pszPrefixWithSubKey==NULL) return;
	mir_strcat(mir_strcat(mir_strcpy(pszPrefixWithSubKey,pszDbPrefix),pszSubKey),"\\"); /* buffer safe */
	if (pszPrefixWithSubKey!=NULL) {
		if (EnumDbPrefixSettings("AssocMgr",pszPrefixWithSubKey,&ppszSettings,&nSettingsCount)) {
			for(i=0;i<nSettingsCount;++i) {
				db_unset(NULL,"AssocMgr",ppszSettings[i]);
				mir_free(ppszSettings[i]);
			}
			mir_free(ppszSettings);
		}
		mir_free(pszPrefixWithSubKey);
	}
}

void CleanupRegTreeBackupSettings(void)
{
	/* delete old bak_* settings and try to restore backups */
	int nSettingsCount;
	char **ppszSettings;
	if ( !EnumDbPrefixSettings("AssocMgr", "bak_", &ppszSettings, &nSettingsCount))
		return;

	for(int i=0; i < nSettingsCount; ++i) {
		char *pszClassName = &ppszSettings[i][4];
		char *pszBuf = strchr(pszClassName,'\\');
		if (pszBuf != NULL) {
			*pszBuf = '\0';

			/* remove others in list with same class name */
			if(i < nSettingsCount-1){
				for(int j=i+1; j < nSettingsCount; ++j) {
					pszBuf = strchr(&ppszSettings[j][4],'\\');
					if (pszBuf != NULL) *pszBuf='\0';
					if (mir_strcmp(pszClassName, &ppszSettings[j][4])){
						if (pszBuf != NULL) *pszBuf='\\';
						continue;
					}

					mir_free(ppszSettings[j]);
					memmove(&ppszSettings[j], &ppszSettings[j+1], ((--nSettingsCount)-j) * sizeof(char*));
					--j; /* reiterate current index */
				}
			}

			/* no longer registered? */
			if (!IsRegisteredAssocItem(pszClassName)) {
				char *pszFileExt;
				if (IsFileClassName(pszClassName, &pszFileExt))
					RemoveRegFileExt(pszFileExt, pszClassName);
				else
					RemoveRegClass(pszClassName);
			}
		}
		mir_free(ppszSettings[i]);
	}
	mir_free(ppszSettings);
}

/************************* Class **********************************/

/*
* Add a new file class to the class list.
* This either represents a superclass for several file extensions or
* the the url object.
* Urls just need a class named after their prefix e.g. "http".
* File extensions should follow the rule "appname.extension". 
*/

// pszIconLoc, pszVerbDesc and pszDdeCmd are allowed to be NULL
// call GetLastError() on error to get more error details
BOOL AddRegClass(const char *pszClassName,const wchar_t *pszTypeDescription,const wchar_t *pszIconLoc,const wchar_t *pszAppName,const wchar_t *pszRunCmd,const wchar_t *pszDdeCmd,const wchar_t *pszDdeApp,const wchar_t *pszDdeTopic,const wchar_t *pszVerbDesc,BOOL fBrowserAutoOpen,BOOL fUrlProto,BOOL fIsShortcut)
{
	LONG res;
	HKEY hRootKey,hClassKey,hShellKey,hVerbKey,hDdeKey;

	/* some error checking for disallowed values (to avoid errors in registry) */
	if (strchr(pszClassName,'\\')!=NULL || strchr(pszClassName,' ')!=NULL) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}
	/* try to open interactive user's classes key */
	if (RegOpenKeyEx(HKEY_CURRENT_USER,L"Software\\Classes",0,KEY_CREATE_SUB_KEY,&hRootKey))
		hRootKey=HKEY_CLASSES_ROOT; /* might be write protected by security settings */

	/* class */
	if ((res=RegCreateKeyExA(hRootKey,pszClassName,0,NULL,0,KEY_SET_VALUE|KEY_CREATE_SUB_KEY|DELETE|KEY_QUERY_VALUE,NULL,&hClassKey,NULL))==ERROR_SUCCESS) {
		/* backup class if shared */
		if (fUrlProto) BackupRegTree(hRootKey,pszClassName,"bak_");
		/* type description */
		if (fUrlProto) SetRegStrPrefixValue(hClassKey,L"URL:",pszTypeDescription);
		else RegSetValueEx(hClassKey,NULL,0,REG_SZ,(BYTE*)pszTypeDescription,(int)(mir_wstrlen(pszTypeDescription)+1)*sizeof(wchar_t));
		/* default icon */
		if (pszIconLoc!=NULL) SetRegSubKeyStrDefValue(hClassKey,L"DefaultIcon",pszIconLoc);
		/* url protocol */
		if (!fUrlProto) RegDeleteValue(hClassKey,L"URL Protocol");
		else RegSetValueEx(hClassKey,L"URL Protocol",0,REG_SZ,NULL,0);
		/* moniker clsid */
		RegDeleteKey(hClassKey,L"CLSID");
		/* edit flags */
		{	DWORD dwFlags=0,dwSize=sizeof(dwFlags);
		RegQueryValueEx(hClassKey,L"EditFlags",NULL,NULL,(BYTE*)&dwFlags,&dwSize);
		if (fBrowserAutoOpen) dwFlags=(dwFlags&~FTA_AlwaysUnsafe)|FTA_OpenIsSafe;
		if (!fUrlProto) dwFlags|=FTA_HasExtension;
		else dwFlags=(dwFlags&~FTA_HasExtension)|FTA_Show; /* show classes without extension */
		RegSetValueEx(hClassKey,L"EditFlags",0,REG_DWORD,(BYTE*)&dwFlags,sizeof(dwFlags));
		}
		if (fIsShortcut) {
			RegSetValueExA(hClassKey,"IsShortcut",0,REG_SZ,NULL,0);
			RegSetValueExA(hClassKey,"NeverShowExt",0,REG_SZ,NULL,0);
		}
		/* shell */
		if ((res=RegCreateKeyEx(hClassKey,L"shell",0,NULL,0,KEY_SET_VALUE|KEY_CREATE_SUB_KEY,NULL,&hShellKey,NULL))==ERROR_SUCCESS) {
			/* default verb (when empty "open" is used) */
			RegSetValueEx(hShellKey,NULL,0,REG_SZ,(BYTE*)L"open",5*sizeof(wchar_t));
			/* verb */
			if ((res=RegCreateKeyEx(hShellKey,L"open",0,NULL,0,KEY_SET_VALUE|KEY_CREATE_SUB_KEY|DELETE,NULL,&hVerbKey,NULL))==ERROR_SUCCESS) {
				/* verb description */
				if (pszVerbDesc==NULL) RegDeleteValue(hVerbKey,NULL);
				else RegSetValueEx(hVerbKey,NULL,0,REG_SZ,(BYTE*)pszVerbDesc,(int)(mir_wstrlen(pszVerbDesc)+1)*sizeof(wchar_t));
				/* friendly appname (mui string) */
				RegSetValueEx(hVerbKey,L"FriendlyAppName",0,REG_SZ,(BYTE*)pszAppName,(int)(mir_wstrlen(pszAppName)+1)*sizeof(wchar_t));
				/* command */
				SetRegSubKeyStrDefValue(hVerbKey,L"command",pszRunCmd);
				/* ddeexec */
				if (pszDdeCmd!=NULL) {
					if (!RegCreateKeyEx(hVerbKey,L"ddeexec",0,NULL,0,KEY_SET_VALUE|KEY_CREATE_SUB_KEY|DELETE,NULL,&hDdeKey,NULL)) {
						/* command */
						RegSetValueEx(hDdeKey,NULL,0,REG_SZ,(BYTE*)pszDdeCmd,(int)(mir_wstrlen(pszDdeCmd)+1)*sizeof(wchar_t));
						/* application */
						SetRegSubKeyStrDefValue(hDdeKey,L"application",pszDdeApp);
						/* topic */
						SetRegSubKeyStrDefValue(hDdeKey,L"topic",pszDdeTopic);
						/* ifexec */
						RegDeleteKey(hDdeKey,L"ifexec");
						RegCloseKey(hDdeKey);
					}
				} else {
					if (!RegOpenKeyEx(hVerbKey,L"ddeexec",0,DELETE,&hDdeKey)) {
						/* application */
						RegDeleteKey(hDdeKey,L"application");
						/* topic */
						RegDeleteKey(hDdeKey,L"topic");
						/* ifexec */
						RegDeleteKey(hDdeKey,L"ifexec");
						RegCloseKey(hDdeKey);
					}
					RegDeleteKey(hVerbKey,L"ddeexec");
				}
				/* drop target (WinXP+) */
				RegDeleteKey(hVerbKey,L"DropTarget");
				RegCloseKey(hVerbKey);
			}
			RegCloseKey(hShellKey);
		}
		RegCloseKey(hClassKey);
	}

	if (hRootKey!=HKEY_CLASSES_ROOT)
		RegCloseKey(hRootKey);

	if (res) SetLastError(res);
	return !res;
}

BOOL RemoveRegClass(const char *pszClassName)
{
	LONG res;
	HKEY hRootKey,hClassKey,hShellKey,hVerbKey;
	wchar_t *ptszClassName,*ptszPrevRunCmd;

	/* try to open interactive user's classes key */
	if (RegOpenKeyEx(HKEY_CURRENT_USER,L"Software\\Classes",0,DELETE,&hRootKey))
		hRootKey=HKEY_CLASSES_ROOT;

	/* class name */
	ptszClassName=a2t(pszClassName);
	if (ptszClassName!=NULL)
		res=DeleteRegSubTree(hRootKey,ptszClassName);
	else res=ERROR_OUTOFMEMORY;
	mir_free(ptszClassName); /* does NULL check */

	/* backup only saved/restored for fUrlProto */
	if (!res) {
		if ((res=RestoreRegTree(hRootKey,pszClassName,"bak_"))==ERROR_SUCCESS)
			/* class */
				if (!RegOpenKeyExA(hRootKey,pszClassName,0,KEY_QUERY_VALUE,&hClassKey)) {
					/* shell */
					if (!RegOpenKeyEx(hClassKey,L"shell",0,KEY_QUERY_VALUE,&hShellKey)) {
						/* verb */
						if (!RegOpenKeyEx(hShellKey,L"open",0,KEY_QUERY_VALUE,&hVerbKey)) {
							/* command */
							ptszPrevRunCmd=GetRegStrValue(hVerbKey,L"command");
							if (ptszPrevRunCmd!=NULL && !IsValidRunCommand(ptszPrevRunCmd))
								res=DeleteRegSubTree(hRootKey,ptszClassName); /* backup outdated, remove all */
							mir_free(ptszPrevRunCmd); /* does NULL check */
							RegCloseKey(hVerbKey);
						}
						RegCloseKey(hShellKey);
					}
					RegCloseKey(hClassKey);
				}
	} else DeleteRegTreeBackup(pszClassName,"bak_");

	if (hRootKey!=HKEY_CLASSES_ROOT)
		RegCloseKey(hRootKey);

	if (res==ERROR_SUCCESS || res==ERROR_FILE_NOT_FOUND || res==ERROR_NO_MORE_ITEMS) return TRUE;
	SetLastError(res);
	return FALSE;
}

/*
* Test if a given class belongs to the current process
* specified via its run command.
* This is especially needed for Urls where the same class name "http" can be
* registered and thus be overwritten by multiple applications.
*/

BOOL IsRegClass(const char *pszClassName,const wchar_t *pszRunCmd)
{
	BOOL fSuccess=FALSE;
	HKEY hClassKey,hShellKey,hVerbKey,hCmdKey;

	/* using the merged view classes key for reading */
	/* class */
	if (!RegOpenKeyExA(HKEY_CLASSES_ROOT,pszClassName,0,KEY_QUERY_VALUE,&hClassKey)) {
		/* shell */
		if (!RegOpenKeyEx(hClassKey,L"shell",0,KEY_QUERY_VALUE,&hShellKey)) {
			/* verb */
			if (!RegOpenKeyEx(hShellKey,L"open",0,KEY_QUERY_VALUE,&hVerbKey)) {
				/* command */
				if (!RegOpenKeyEx(hVerbKey,L"command",0,KEY_QUERY_VALUE,&hCmdKey)) {
					/* it is enough to check if the command is right */
					fSuccess=IsRegStrValue(hCmdKey,NULL,pszRunCmd);
					RegCloseKey(hCmdKey);
				}
				RegCloseKey(hVerbKey);
			}
			RegCloseKey(hShellKey);
		}
		RegCloseKey(hClassKey);
	}
	return fSuccess;
}

/*
* Extract the icon name of the class from the registry and load it.
* For uses especially with url classes.
*/

// DestroyIcon() the return value
HICON LoadRegClassSmallIcon(const char *pszClassName)
{
	HICON hIcon=NULL;
	HKEY hClassKey,hIconKey;
	wchar_t *pszIconLoc,*p;

	/* using the merged view classes key for reading */
	/* class */
	if (!RegOpenKeyExA(HKEY_CLASSES_ROOT,pszClassName,0,KEY_QUERY_VALUE,&hClassKey)) {
		/* default icon */
		if (!RegOpenKeyEx(hClassKey,L"DefaultIcon",0,KEY_QUERY_VALUE,&hIconKey)) {
			/* extract icon */
			pszIconLoc=GetRegStrValue(hIconKey,NULL);
			if (pszIconLoc!=NULL) {
				p=wcsrchr(pszIconLoc,',');
				if (p!=NULL) {
					*(p++)=0;
					ExtractIconEx(pszIconLoc,_wtoi(p),NULL,&hIcon,1);
				}
				mir_free(pszIconLoc);
			}
			RegCloseKey(hIconKey);
		}
		RegCloseKey(hClassKey);
	}

	return hIcon;
}

/************************* Extension ******************************/

/*
* Add a new file extension to the class list.
* The file extension needs to be associated with a class
* that has been registered previously.
* Multiple file extensions can be assigned to the same class.
* The class contains most settings as the run command etc.
*/

// pszMimeType is allowed to be NULL
BOOL AddRegFileExt(const char *pszFileExt,const char *pszClassName,const char *pszMimeType,BOOL fIsText)
{
	BOOL fSuccess=FALSE;
	HKEY hRootKey,hExtKey,hOpenWithKey;

	/* some error checking for disallowed values (to avoid errors in registry) */
	if (strchr(pszFileExt,'\\')!=NULL || strchr(pszFileExt,' ')!=NULL)
		return FALSE;

	/* try to open interactive user's classes key */
	if (RegOpenKeyEx(HKEY_CURRENT_USER,L"Software\\Classes",0,KEY_CREATE_SUB_KEY,&hRootKey))
		hRootKey=HKEY_CLASSES_ROOT;

	/* file ext */
	if (!RegCreateKeyExA(hRootKey,pszFileExt,0,NULL,0,KEY_SET_VALUE|KEY_QUERY_VALUE|KEY_CREATE_SUB_KEY,NULL,&hExtKey,NULL)) {
		/* backup previous app */
		BackupRegTree(hRootKey,pszFileExt,"bak_");
		/* remove any no-open flag */
		RegDeleteValue(hExtKey,L"NoOpen");
		/* open with progids */
		wchar_t *pszPrevClass=GetRegStrValue(hExtKey,NULL);
		if (pszPrevClass!=NULL && !IsRegStrValueA(hExtKey,NULL,pszClassName))
			if (!RegCreateKeyEx(hExtKey,L"OpenWithProgids",0,NULL,0,KEY_SET_VALUE,NULL,&hOpenWithKey,NULL)) {
				/* previous class (backup) */
				RegSetValueEx(hOpenWithKey,pszPrevClass,0,REG_NONE,NULL,0);
				RegCloseKey(hOpenWithKey);
			}
			mir_free(pszPrevClass); /* does NULL check */
			/* class name */
			fSuccess=!RegSetValueExA(hExtKey, NULL, 0, REG_SZ, (BYTE*)pszClassName, (int)mir_strlen(pszClassName)+1);
			/* mime type e.g. "application/x-icq" */
			if (pszMimeType!=NULL) RegSetValueExA(hExtKey, "Content Type", 0, REG_SZ, (BYTE*)pszMimeType, (int)mir_strlen(pszMimeType)+1);
			/* perceived type e.g. text (WinXP+) */
			if (fIsText) RegSetValueEx(hExtKey,L"PerceivedType",0,REG_SZ,(BYTE*)L"text",5*sizeof(wchar_t));
			RegCloseKey(hExtKey);
	}

	if (hRootKey!=HKEY_CLASSES_ROOT)
		RegCloseKey(hRootKey);
	return fSuccess;
}

void RemoveRegFileExt(const char *pszFileExt,const char *pszClassName)
{
	HKEY hRootKey,hExtKey,hSubKey;
	DWORD nOpenWithCount;
	wchar_t *pszPrevClassName=NULL;
	BOOL fRestored=FALSE;

	/* try to open interactive user's classes key */
	if (RegOpenKeyEx(HKEY_CURRENT_USER,L"Software\\Classes",0,DELETE,&hRootKey))
		hRootKey=HKEY_CLASSES_ROOT;

	/* file ext */
	if (!RegOpenKeyExA(hRootKey,pszFileExt,0,KEY_QUERY_VALUE|KEY_SET_VALUE|DELETE,&hExtKey)) {
		/* class name (the important part) */
		if (!RestoreRegTree(hRootKey,pszFileExt,"bak_")) {
			pszPrevClassName=GetRegStrValue(hExtKey,NULL);
			if (pszPrevClassName!=NULL) {
				/* previous class name still exists? */
				/* using the merged view classes key for reading */
				if (!RegOpenKeyEx(HKEY_CLASSES_ROOT,pszPrevClassName,0,KEY_QUERY_VALUE,&hSubKey)) {
					fRestored=TRUE;
					RegCloseKey(hSubKey);
				} else RegDeleteValue(hExtKey,NULL);
				mir_free(pszPrevClassName);
			}
		}
		if (pszPrevClassName==NULL) RegDeleteValue(hExtKey,NULL);
		/* open with progids (remove if empty) */
		nOpenWithCount=0;
		if (!RegOpenKeyEx(hExtKey,L"OpenWithProgids",0,KEY_SET_VALUE|KEY_QUERY_VALUE,&hSubKey)) {
			/* remove current class (if set by another app) */
			RegDeleteValueA(hSubKey,pszClassName);
			RegQueryInfoKey(hSubKey,NULL,NULL,NULL,NULL,NULL,NULL,NULL,&nOpenWithCount,NULL,NULL,NULL);
			RegCloseKey(hSubKey);
		}
		if (!nOpenWithCount) RegDeleteKey(hExtKey,L"OpenWithProgids"); /* delete if no values */
		RegCloseKey(hExtKey);
	} else DeleteRegTreeBackup(pszFileExt,"bak_");
	if (!fRestored) RegDeleteKeyA(hRootKey,pszFileExt); /* try to remove it all */

	if (hRootKey!=HKEY_CLASSES_ROOT)
		RegCloseKey(hRootKey);
}

/*
* Test if a given file extension belongs to the given class name.
* If it does not belong to the class name, it got reassigned and thus
* overwritten by another application.
*/

BOOL IsRegFileExt(const char *pszFileExt,const char *pszClassName)
{
	BOOL fSuccess=FALSE;
	HKEY hExtKey;

	/* using the merged view classes key for reading */
	/* file ext */
	if (!RegOpenKeyExA(HKEY_CLASSES_ROOT,pszFileExt,0,KEY_QUERY_VALUE,&hExtKey)) {
		/* class name */
		/* it is enough to check if the class is right */
		fSuccess=IsRegStrValueA(hExtKey,NULL,pszClassName);
		RegCloseKey(hExtKey);
	}
	return fSuccess;
}

/************************* Mime Type ******************************/

/*
* Add a given mime type to the global mime database.
*/

// returns TRUE if the mime type was not yet registered on the system,
// it needs to be removed when the file extension gets removed
BOOL AddRegMimeType(const char *pszMimeType,const char *pszFileExt)
{
	BOOL fSuccess=FALSE;
	HKEY hRootKey,hDbKey,hTypeKey;

	/* some error checking for disallowed values (to avoid errors in registry) */
	if (strchr(pszMimeType,'\\')!=NULL || strchr(pszMimeType,' ')!=NULL)
		return FALSE;

	/* try to open interactive user's classes key */
	if (RegOpenKeyEx(HKEY_CURRENT_USER,L"Software\\Classes",0,KEY_QUERY_VALUE,&hRootKey))
		hRootKey=HKEY_CLASSES_ROOT;

	/* database */
	if (!RegOpenKeyEx(hRootKey,L"MIME\\Database\\Content Type",0,KEY_CREATE_SUB_KEY,&hDbKey)) {
		/* mime type */
		if (!RegCreateKeyExA(hDbKey,pszMimeType,0,NULL,0,KEY_QUERY_VALUE|KEY_SET_VALUE,NULL,&hTypeKey,NULL)) {
			/* file ext */
			if (RegQueryValueExA(hTypeKey,"Extension",NULL,NULL,NULL,NULL)) /* only set if not present */
				fSuccess=!RegSetValueExA(hTypeKey,"Extension",0,REG_SZ,(BYTE*)pszFileExt,(int)mir_strlen(pszFileExt)+1);
			RegCloseKey(hTypeKey);
		}
		RegCloseKey(hDbKey);
	}

	if (hRootKey!=HKEY_CLASSES_ROOT)
		RegCloseKey(hRootKey);
	return fSuccess;
}

void RemoveRegMimeType(const char *pszMimeType,const char *pszFileExt)
{
	HKEY hRootKey,hDbKey,hTypeKey;
	BOOL fDelete=TRUE;

	/* try to open interactive user's classes key */
	if (RegOpenKeyEx(HKEY_CURRENT_USER,L"Software\\Classes",0,KEY_QUERY_VALUE,&hRootKey))
		hRootKey=HKEY_CLASSES_ROOT;

	/* database */
	if (!RegOpenKeyEx(hRootKey,L"MIME\\Database\\Content Type",0,DELETE,&hDbKey)) {
		/* mime type */
		if (!RegOpenKeyExA(hDbKey,pszMimeType,0,KEY_QUERY_VALUE,&hTypeKey)) {
			/* file ext */
			fDelete=IsRegStrValueA(hTypeKey,L"Extension",pszFileExt);
			RegCloseKey(hTypeKey);
		}
		if (fDelete) RegDeleteKeyA(hDbKey,pszMimeType);
		RegCloseKey(hDbKey);
	}

	if (hRootKey!=HKEY_CLASSES_ROOT)
		RegCloseKey(hRootKey);
}

/************************* Open-With App **************************/

/*
* Add Miranda as an option to the advanced "Open With..." dialog.
*/

// pszDdeCmd is allowed to be NULL
void AddRegOpenWith(const wchar_t *pszAppFileName,BOOL fAllowOpenWith,const wchar_t *pszAppName,const wchar_t *pszIconLoc,const wchar_t *pszRunCmd,const wchar_t *pszDdeCmd,const wchar_t *pszDdeApp,const wchar_t *pszDdeTopic)
{
	HKEY hRootKey,hAppsKey,hExeKey,hShellKey,hVerbKey,hDdeKey;

	/* try to open interactive user's classes key */
	if (RegOpenKeyEx(HKEY_CURRENT_USER,L"Software\\Classes",0,KEY_QUERY_VALUE,&hRootKey))
		hRootKey=HKEY_CLASSES_ROOT;

	/* database */
	if (!RegCreateKeyEx(hRootKey,L"Applications",0,NULL,0,KEY_CREATE_SUB_KEY,NULL,&hAppsKey,NULL)) {
		/* filename */
		if (!RegCreateKeyEx(hAppsKey,pszAppFileName,0,NULL,0,KEY_SET_VALUE|KEY_CREATE_SUB_KEY,NULL,&hExeKey,NULL)) {
			/* appname */
			RegSetValueEx(hExeKey,NULL,0,REG_SZ,(BYTE*)pszAppName,(int)(mir_wstrlen(pszAppName)+1)*sizeof(wchar_t));
			/* no open-with flag */
			if (fAllowOpenWith) RegDeleteValue(hExeKey,L"NoOpenWith");
			else RegSetValueEx(hExeKey,L"NoOpenWith",0,REG_SZ,NULL,0);
			/* default icon */
			if (pszIconLoc!=NULL) SetRegSubKeyStrDefValue(hExeKey,L"DefaultIcon",pszIconLoc);
			/* shell */
			if (!RegCreateKeyEx(hExeKey,L"shell",0,NULL,0,KEY_SET_VALUE|KEY_CREATE_SUB_KEY,NULL,&hShellKey,NULL)) {
				/* default verb (when empty "open" is used) */
				RegSetValueEx(hShellKey,NULL,0,REG_SZ,(BYTE*)L"open",5*sizeof(wchar_t));
				/* verb */
				if (!RegCreateKeyEx(hShellKey,L"open",0,NULL,0,KEY_SET_VALUE|KEY_CREATE_SUB_KEY,NULL,&hVerbKey,NULL)) {
					/* friendly appname (mui string) */
					RegSetValueEx(hVerbKey,L"FriendlyAppName",0,REG_SZ,(BYTE*)pszAppName,(int)(mir_wstrlen(pszAppName)+1)*sizeof(wchar_t));
					/* command */
					SetRegSubKeyStrDefValue(hVerbKey,L"command",pszRunCmd);
					/* ddeexec */
					if (pszDdeCmd!=NULL)
						if (!RegCreateKeyEx(hVerbKey,L"ddeexec",0,NULL,0,KEY_SET_VALUE|KEY_CREATE_SUB_KEY,NULL,&hDdeKey,NULL)) {
							/* command */
							RegSetValueEx(hDdeKey,NULL,0,REG_SZ,(BYTE*)pszDdeCmd,(int)(mir_wstrlen(pszDdeCmd)+1)*sizeof(wchar_t));
							/* application */
							SetRegSubKeyStrDefValue(hDdeKey,L"application",pszDdeApp);
							/* topic */
							SetRegSubKeyStrDefValue(hDdeKey,L"topic",pszDdeTopic);
							RegCloseKey(hDdeKey);
						}
						RegCloseKey(hVerbKey);
				}
				RegCloseKey(hShellKey);
			}
			RegCloseKey(hExeKey);
		}
		RegCloseKey(hAppsKey);
	}

	if (hRootKey!=HKEY_CLASSES_ROOT)
		RegCloseKey(hRootKey);
}

void RemoveRegOpenWith(const wchar_t *pszAppFileName)
{
	HKEY hRootKey,hAppsKey,hExeKey,hShellKey,hVerbKey,hDdeKey;

	/* try to open interactive user's classes key */
	if (RegOpenKeyEx(HKEY_CURRENT_USER,L"Software\\Classes",0,KEY_QUERY_VALUE,&hRootKey))
		hRootKey=HKEY_CLASSES_ROOT;

	/* applications */
	if (!RegOpenKeyEx(hRootKey,L"Applications",0,DELETE,&hAppsKey)) {
		/* filename */
		if (!RegOpenKeyEx(hAppsKey,pszAppFileName,0,DELETE,&hExeKey)) {
			/* default icon */
			RegDeleteKey(hExeKey,L"DefaultIcon");
			/* shell */
			if (!RegOpenKeyEx(hExeKey,L"shell",0,DELETE,&hShellKey)) {
				/* verb */
				if (!RegOpenKeyEx(hShellKey,L"open",0,DELETE,&hVerbKey)) {
					/* command */
					RegDeleteKey(hVerbKey,L"command");
					/* ddeexec */
					if (!RegOpenKeyEx(hVerbKey,L"ddeexec",0,DELETE,&hDdeKey)) {
						/* application */
						RegDeleteKey(hDdeKey,L"application");
						/* topic */
						RegDeleteKey(hDdeKey,L"topic");
						RegCloseKey(hDdeKey);
					}
					RegDeleteKey(hVerbKey,L"ddeexec");
					RegCloseKey(hVerbKey);
				}
				RegDeleteKey(hShellKey,L"open");
				RegCloseKey(hShellKey);
			}
			RegDeleteKey(hExeKey,L"shell");
			/* supported types */
			RegDeleteKey(hExeKey,L"SupportedTypes");
			RegCloseKey(hExeKey);
		}
		RegDeleteKey(hAppsKey,pszAppFileName);
		RegCloseKey(hAppsKey);
	}

	if (hRootKey!=HKEY_CLASSES_ROOT)
		RegCloseKey(hRootKey);
}

/*
* Tell the "Open With..." dialog we support a given file extension.
*/

void AddRegOpenWithExtEntry(const wchar_t *pszAppFileName,const char *pszFileExt,const wchar_t *pszFileDesc)
{
	HKEY hRootKey,hAppsKey,hExeKey,hTypesKey;

	/* try to open interactive user's classes key */
	if (RegOpenKeyEx(HKEY_CURRENT_USER,L"Software\\Classes",0,KEY_QUERY_VALUE,&hRootKey))
		hRootKey=HKEY_CLASSES_ROOT;

	/* applications */
	if (!RegOpenKeyEx(hRootKey,L"Applications",0,KEY_QUERY_VALUE|KEY_ENUMERATE_SUB_KEYS,&hAppsKey)) {
		/* filename */
		if (!RegOpenKeyEx(hAppsKey,pszAppFileName,0,KEY_CREATE_SUB_KEY,&hExeKey)) {
			/* supported types */
			if (!RegCreateKeyEx(hExeKey,L"SupportedTypes",0,NULL,0,KEY_SET_VALUE,NULL,&hTypesKey,NULL)) {	
				wchar_t *ptszFileExt;
				ptszFileExt=a2t(pszFileExt);
				if (ptszFileExt!=NULL)
					RegSetValueEx(hTypesKey,ptszFileExt,0,REG_SZ,(BYTE*)pszFileDesc,(int)(mir_wstrlen(pszFileDesc)+1)*sizeof(wchar_t));
				mir_free(ptszFileExt); /* does NULL check */
				RegCloseKey(hTypesKey);
			}
			RegCloseKey(hExeKey);
		}
		RegCloseKey(hAppsKey);
	}

	if (hRootKey!=HKEY_CLASSES_ROOT)
		RegCloseKey(hRootKey);
}

void RemoveRegOpenWithExtEntry(const wchar_t *pszAppFileName,const char *pszFileExt)
{
	HKEY hRootKey,hAppsKey,hExeKey,hTypesKey;

	/* try to open interactive user's classes key */
	if (RegOpenKeyEx(HKEY_CURRENT_USER,L"Software\\Classes",0,KEY_QUERY_VALUE,&hRootKey))
		hRootKey=HKEY_CLASSES_ROOT;

	/* applications */
	if (!RegOpenKeyEx(hRootKey,L"Applications",0,KEY_QUERY_VALUE,&hAppsKey)) {
		/* filename */
		if (!RegOpenKeyEx(hAppsKey,pszAppFileName,0,KEY_QUERY_VALUE,&hExeKey)) {
			/* supported types */
			if (!RegOpenKeyEx(hExeKey,L"SupportedTypes",0,KEY_SET_VALUE,&hTypesKey)) {	
				RegDeleteValueA(hTypesKey,pszFileExt);
				RegCloseKey(hTypesKey);
			}
			RegCloseKey(hExeKey);
		}
		RegCloseKey(hAppsKey);
	}

	if (hRootKey!=HKEY_CLASSES_ROOT)
		RegCloseKey(hRootKey);
}

/************************* Autostart ******************************/

/*
* Add Miranda to the autostart list in the registry.
*/

BOOL AddRegRunEntry(const wchar_t *pszAppName,const wchar_t *pszRunCmd)
{
	BOOL fSuccess=FALSE;
	HKEY hRunKey;

	/* run */
	if (!RegCreateKeyEx(HKEY_CURRENT_USER,L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",0,NULL,0,KEY_SET_VALUE,NULL,&hRunKey,NULL)) {
		/* appname */
		fSuccess=!RegSetValueEx(hRunKey,pszAppName,0,REG_SZ,(BYTE*)pszRunCmd,(int)(mir_wstrlen(pszRunCmd)+1)*sizeof(wchar_t));
		RegCloseKey(hRunKey);
	}
	return fSuccess;
}

BOOL RemoveRegRunEntry(const wchar_t *pszAppName,const wchar_t *pszRunCmd)
{
	HKEY hRunKey;

	/* run */
	LONG res = RegOpenKeyEx(HKEY_CURRENT_USER,L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",0,KEY_QUERY_VALUE|KEY_SET_VALUE,&hRunKey);
	if (!res) {
		/* appname */
		if (IsRegStrValue(hRunKey,pszAppName,pszRunCmd))
			res=RegDeleteValue(hRunKey,pszAppName); /* only remove if current */
		RegCloseKey(hRunKey);
	}
	return res==ERROR_SUCCESS || res==ERROR_FILE_NOT_FOUND;
}

/*
* Check if the autostart item belongs to the current instance of Miranda.
*/

BOOL IsRegRunEntry(const wchar_t *pszAppName,const wchar_t *pszRunCmd)
{
	BOOL fState=FALSE;
	HKEY hRunKey;

	/* Run */
	if (!RegOpenKeyEx(HKEY_CURRENT_USER,L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",0,KEY_QUERY_VALUE,&hRunKey)) {
		/* appname */
		fState=IsRegStrValue(hRunKey,pszAppName,pszRunCmd);
		RegCloseKey(hRunKey);
	}
	return fState;
}
