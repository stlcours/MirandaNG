/*
	NewXstatusNotify YM - Plugin for Miranda IM
	Copyright (c) 2001-2004 Luca Santarelli
	Copyright (c) 2005-2007 Vasilich
	Copyright (c) 2007-2011 yaho

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
	*/

#ifndef UTILS_H
#define UTILS_H

bool CheckMsgWnd(MCONTACT hContact);
int DBGetStringDefault(MCONTACT hContact, const char *szModule, const char *szSetting, wchar_t *setting, int size, const wchar_t *defaultValue);
void ShowLog(wchar_t *file);
BOOL StatusHasAwayMessage(char *szProto, int status);
void LogToFile(wchar_t *stzText);
void AddCR(CMString &str, const wchar_t *statusmsg);

#endif
