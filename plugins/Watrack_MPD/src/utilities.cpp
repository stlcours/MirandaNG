// Copyright � 2008 sss, chaos.persei
// 
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.


#include "commonheaders.h"

TCHAR* __stdcall UniGetContactSettingUtf(HANDLE hContact, const char *szModule,const char* szSetting, TCHAR* szDef)
{
  DBVARIANT dbv = {DBVT_DELETED};
  TCHAR* szRes = NULL;
  if (db_get_ts(hContact, szModule, szSetting, &dbv))
	 return _tcsdup(szDef);
  else if(dbv.pszVal)
	  szRes = _tcsdup(dbv.ptszVal);
  else
	  szRes = _tcsdup(szDef);
  db_free(&dbv);
  return szRes;
}

HANDLE NetLib_CreateConnection(HANDLE hUser, NETLIBOPENCONNECTION* nloc) //from icq )
{
	HANDLE hConnection;

	nloc->cbSize = sizeof(NETLIBOPENCONNECTION);
	nloc->flags |= NLOCF_V2;

	hConnection = (HANDLE)CallService(MS_NETLIB_OPENCONNECTION, (WPARAM)hUser, (LPARAM)nloc);
	if (!hConnection && (GetLastError() == 87))
	{ // this ensures, an old Miranda will be able to connect also
		nloc->cbSize = NETLIBOPENCONNECTION_V1_SIZE;
		hConnection = (HANDLE)CallService(MS_NETLIB_OPENCONNECTION, (WPARAM)hConnection, (LPARAM)nloc);
	}
	return hConnection;	
}


