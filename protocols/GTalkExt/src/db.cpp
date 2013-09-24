//***************************************************************************************
//
//   Google Extension plugin for the Miranda IM's Jabber protocol
//   Copyright (c) 2011 bems@jabber.org, George Hazan (ghazan@jabber.ru)
//
//   This program is free software; you can redistribute it and/or
//   modify it under the terms of the GNU General Public License
//   as published by the Free Software Foundation; either version 2
//   of the License, or (at your option) any later version.
//
//   This program is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with this program; if not, write to the Free Software
//   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
//***************************************************************************************

#include "StdAfx.h"
#include "options.h"

LPTSTR ReadJidSetting(LPCSTR name, LPCTSTR jid)
{
	return db_get_tsa(0, name, _T2A(jid));
}

void WriteJidSetting(LPCSTR name, LPCTSTR jid, LPCTSTR setting)
{
	db_set_ts(0, name, _T2A(jid), setting);
}

void RenewPseudocontactHandles()
{
	int count = 0;
	PROTOACCOUNT **protos;
	ProtoEnumAccounts(&count, &protos);
	for (int i = 0; i < count; i++) {
		db_unset(0, protos[i]->szModuleName, PSEUDOCONTACT_LINK);
		db_unset(0, protos[i]->szModuleName, "GMailExtNotifyContact");	// remove this
	}

	for (HANDLE hContact = db_find_first(); hContact; hContact = db_find_next(hContact)) {
		if (db_get_b(hContact, SHORT_PLUGIN_NAME, PSEUDOCONTACT_FLAG, 0)) {
			LPCSTR proto = (LPCSTR)GetContactProto(hContact);
			db_set_dw(NULL, proto, PSEUDOCONTACT_LINK, (DWORD)hContact);
		}
	}
}
