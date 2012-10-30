/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2009 Miranda ICQ/IM project, 
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

#include "..\..\core\commonheaders.h"
#include <m_protomod.h>

extern LIST<PROTOCOLDESCRIPTOR> filters;

static int GetProtocolP(HANDLE hContact, char *szBuf, int cbLen)
{
	if (currDb == NULL)
		return 1;

	DBCachedContact *cc = currDb->m_cache->GetCachedContact(hContact);
	if (cc == NULL)
		cc = currDb->m_cache->AddContactToCache(hContact);
	if (cc->szProto != NULL) {
		strncpy(szBuf, cc->szProto, cbLen);
		szBuf[cbLen-1] = 0;
		return 0;
	}

	DBVARIANT dbv;
	dbv.type = DBVT_ASCIIZ;
	dbv.pszVal = szBuf;
	dbv.cchVal = cbLen;

	DBCONTACTGETSETTING dbcgs;
	dbcgs.pValue = &dbv;
	dbcgs.szModule = "Protocol";
	dbcgs.szSetting = "p";

	int res = currDb->GetContactSettingStatic(hContact, &dbcgs);
	if (res == 0 && !cc->szProto)
		cc->szProto = currDb->m_cache->GetCachedSetting(NULL, szBuf, 0, strlen(szBuf));
	return res;
}

/////////////////////////////////////////////////////////////////////////////////////////

INT_PTR CallContactService(HANDLE hContact, const char *szProtoService, WPARAM wParam, LPARAM lParam)
{
	INT_PTR ret;
	CCSDATA ccs = { hContact, szProtoService, wParam, lParam };

	for (int i=0; i < filters.getCount(); i++) {
		if ((ret = CallProtoServiceInt(hContact, filters[i]->szName, szProtoService, i+1, (LPARAM)&ccs)) != CALLSERVICE_NOTFOUND) {
			//chain was started, exit
			return ret;
		}
	}

	char szProto[40];
	if ( GetProtocolP(hContact, szProto, sizeof(szProto)))
		return 1;

	PROTOACCOUNT *pa = Proto_GetAccount(szProto);
	if (pa == NULL || pa->ppro == NULL)
		return 1;

	if (pa->bOldProto)
		ret = CallProtoServiceInt(hContact, szProto, szProtoService, (WPARAM)(-1), (LPARAM)&ccs);
	else
		ret = CallProtoServiceInt(hContact, szProto, szProtoService, wParam, lParam);
	if (ret == CALLSERVICE_NOTFOUND)
		ret = 1;

	return ret;
}

/////////////////////////////////////////////////////////////////////////////////////////

INT_PTR Proto_CallContactService(WPARAM wParam, LPARAM lParam)
{
	CCSDATA *ccs = (CCSDATA*)lParam;
	INT_PTR ret;

	if (wParam == (WPARAM)(-1))
		return 1;

	for (int i = wParam; i < filters.getCount(); i++) {
		if ((ret = CallProtoServiceInt(NULL, filters[i]->szName, ccs->szProtoService, i+1, lParam)) != CALLSERVICE_NOTFOUND) {
			//chain was started, exit
			return ret;
		}
	}

	char szProto[40];
	if ( GetProtocolP(ccs->hContact, szProto, sizeof(szProto)))
		return 1;

	PROTOACCOUNT *pa = Proto_GetAccount(szProto);
	if (pa == NULL || pa->ppro == NULL)
		return 1;

	if (pa->bOldProto)
		ret = CallProtoServiceInt(ccs->hContact, szProto, ccs->szProtoService, (WPARAM)(-1), (LPARAM)ccs);
	else
		ret = CallProtoServiceInt(ccs->hContact, szProto, ccs->szProtoService, ccs->wParam, ccs->lParam);
	if (ret == CALLSERVICE_NOTFOUND)
		ret = 1;

	return ret;
}

/////////////////////////////////////////////////////////////////////////////////////////

static INT_PTR CallRecvChain(WPARAM wParam, LPARAM lParam)
{
	CCSDATA *ccs = (CCSDATA*)lParam;
	INT_PTR ret;

	if (wParam == (WPARAM)(-1)) return 1;   //shouldn't happen - sanity check
	if (wParam == 0)   //begin processing by finding end of chain
		wParam = filters.getCount();
	else
		wParam--;

	for (int i = wParam-1; i >= 0; i--) {
		if ((ret = CallProtoServiceInt(NULL, filters[i]->szName, ccs->szProtoService, i+1, lParam)) != CALLSERVICE_NOTFOUND) {
			//chain was started, exit
			return ret;
		}
	}

	//end of chain, call network protocol again
	char szProto[40];
	if ( GetProtocolP(ccs->hContact, szProto, sizeof(szProto)))
		return 1;

	PROTOACCOUNT *pa = Proto_GetAccount(szProto);
	if (pa == NULL || pa->ppro == NULL)
		return 1;

	if (pa->bOldProto)
		ret = CallProtoServiceInt(ccs->hContact, szProto, ccs->szProtoService, (WPARAM)(-1), (LPARAM)ccs);
	else
		ret = CallProtoServiceInt(ccs->hContact, szProto, ccs->szProtoService, ccs->wParam, ccs->lParam);
	if (ret == CALLSERVICE_NOTFOUND)
		ret = 1;

	return ret;
}

static INT_PTR Proto_ChainRecv(WPARAM wParam, LPARAM lParam)
{
	/* this will switch threads just like before */
	return CallServiceSync(MS_PROTO_CHAINRECV "ThreadSafe", wParam, lParam);
}

PROTOACCOUNT* __fastcall Proto_GetAccount(HANDLE hContact)
{
	if (hContact == NULL)
		return NULL;

	char szProto[40];
	if ( GetProtocolP(hContact, szProto, sizeof(szProto)))
		return NULL;

	return Proto_GetAccount(szProto);
}

static INT_PTR Proto_GetContactBaseProto(WPARAM wParam, LPARAM)
{
	PROTOACCOUNT *pa = Proto_GetAccount((HANDLE)wParam);
	return (INT_PTR)(Proto_IsAccountEnabled(pa) ? pa->szModuleName : NULL);
}

static INT_PTR Proto_GetContactBaseAccount(WPARAM wParam, LPARAM)
{
	PROTOACCOUNT *pa = Proto_GetAccount((HANDLE)wParam);
	return (INT_PTR)(pa ? pa->szModuleName : NULL);
}

static INT_PTR Proto_IsProtoOnContact(WPARAM wParam, LPARAM lParam)
{
	char *szProto = (char*)lParam;
	if (szProto == NULL)
		return 0;

	char szContactProto[40];
	if ( !GetProtocolP((HANDLE)wParam, szContactProto, sizeof(szContactProto)))
		if ( !_stricmp(szProto, szContactProto))
			return -1;

	for (int i=0; i < filters.getCount(); i++)
		if ( !strcmp(szProto, filters[i]->szName))
			return i+1;

	return 0;
}

static INT_PTR Proto_AddToContact(WPARAM wParam, LPARAM lParam)
{
	char *szProto = (char*)lParam;
	PROTOCOLDESCRIPTOR *pd = Proto_IsProtocolLoaded(szProto);
	if (pd == NULL) {
		PROTOACCOUNT *pa = Proto_GetAccount(szProto);
		if (pa) {
			db_set_s((HANDLE)wParam, "Protocol", "p", szProto);
			return 0;
		}
		return 1;
	}

	if (pd->type == PROTOTYPE_PROTOCOL)
		db_set_s((HANDLE)wParam, "Protocol", "p", szProto);

	return 0;
}

static INT_PTR Proto_RemoveFromContact(WPARAM wParam, LPARAM lParam)
{
	switch ( Proto_IsProtoOnContact(wParam, lParam)) {
	case 0:
		return 1;
	case -1:
		db_unset((HANDLE)wParam, "Protocol", "p");
	}

	return 0;
}

int LoadProtoChains(void)
{
	if ( !db_get_b(NULL, "Compatibility", "Filters", 0)) {
		CallService(MS_DB_MODULE_DELETE, 0, (LPARAM)"_Filters");
		db_set_b(NULL, "Compatibility", "Filters", 1);
	}

	CreateServiceFunction(MS_PROTO_CALLCONTACTSERVICE, Proto_CallContactService);
	CreateServiceFunction(MS_PROTO_CHAINSEND, Proto_CallContactService);
	CreateServiceFunction(MS_PROTO_CHAINRECV, Proto_ChainRecv);
	CreateServiceFunction(MS_PROTO_CHAINRECV "ThreadSafe", CallRecvChain);
	CreateServiceFunction(MS_PROTO_GETCONTACTBASEPROTO, Proto_GetContactBaseProto);
	CreateServiceFunction(MS_PROTO_GETCONTACTBASEACCOUNT, Proto_GetContactBaseAccount);
	CreateServiceFunction(MS_PROTO_ISPROTOONCONTACT, Proto_IsProtoOnContact);
	CreateServiceFunction(MS_PROTO_ADDTOCONTACT, Proto_AddToContact);
	CreateServiceFunction(MS_PROTO_REMOVEFROMCONTACT, Proto_RemoveFromContact);
	return 0;
}
