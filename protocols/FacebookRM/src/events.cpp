/*

Facebook plugin for Miranda Instant Messenger
_____________________________________________

Copyright � 2009-11 Michal Zelinka, 2011-16 Robert P�sel

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "stdafx.h"

HWND FacebookProto::NotifyEvent(wchar_t* title, wchar_t* info, MCONTACT contact, EventType type, std::string *url, std::string *notification_id, const char *icon)
{
	if (title == NULL || info == NULL)
		return NULL;

	char name[256];

	switch (type)
	{
	case EVENT_CLIENT:
		mir_snprintf(name, "%s_%s", m_szModuleName, "Client");
		break;

	case EVENT_NEWSFEED:
		mir_snprintf(name, "%s_%s", m_szModuleName, "Newsfeed");
		break;

	case EVENT_NOTIFICATION:
		mir_snprintf(name, "%s_%s", m_szModuleName, "Notification");
		SkinPlaySound("Notification");
		break;

	case EVENT_OTHER:
		mir_snprintf(name, "%s_%s", m_szModuleName, "Other");
		SkinPlaySound("OtherEvent");
		break;

	case EVENT_FRIENDSHIP:
		mir_snprintf(name, "%s_%s", m_szModuleName, "Friendship");
		SkinPlaySound("Friendship");
		break;

	case EVENT_TICKER:
		mir_snprintf(name, "%s_%s", m_szModuleName, "Ticker");
		SkinPlaySound("Ticker");
		break;

	case EVENT_ON_THIS_DAY:
		mir_snprintf(name, "%s_%s", m_szModuleName, "Memories");		
		break;
	}

	if (!getByte(FACEBOOK_KEY_SYSTRAY_NOTIFY, DEFAULT_SYSTRAY_NOTIFY))
	{
		if (ServiceExists(MS_POPUP_ADDPOPUPCLASS)) {

			// TODO: if popup with particular ID is already showed, just update his content
			// ... but f***ed up Popup Classes won't allow it now - they need to return hPopupWindow somehow
			/* if (popup exists) {
				if (PUChangeTextT(hWndPopup, info) > 0) // success
				return;
				}*/

			POPUPDATACLASS pd = { sizeof(pd) };
			pd.pwszTitle = title;
			pd.pwszText = info;
			pd.pszClassName = name;
			pd.hContact = contact;
			if (icon != NULL) {
				// pd.hIcon = IcoLib_GetIconByHandle(GetIconHandle(icon)); // FIXME: Uncomment when implemented in Popup+ and YAPP correctly
			}

			if (url != NULL || notification_id != NULL) {
				popup_data *data = new popup_data(this);
				if (url != NULL)
					data->url = *url;
				if (notification_id != NULL)
					data->notification_id = *notification_id;
				pd.PluginData = data;
			}

			return (HWND)CallService(MS_POPUP_ADDPOPUPCLASS, 0, (LPARAM)&pd);
		}
	}
	else {
		if (ServiceExists(MS_CLIST_SYSTRAY_NOTIFY))
		{
			MIRANDASYSTRAYNOTIFY err;
			err.szProto = m_szModuleName;
			err.cbSize = sizeof(err);
			err.dwInfoFlags = NIIF_INTERN_UNICODE | (type == EVENT_CLIENT ? NIIF_WARNING : NIIF_INFO);
			err.tszInfoTitle = title;
			err.tszInfo = info;
			err.uTimeout = 10000;

			if (CallService(MS_CLIST_SYSTRAY_NOTIFY, 0, (LPARAM)&err) == 0)
				return NULL;
		}
	}

	if (type == EVENT_CLIENT)
		MessageBox(NULL, info, title, MB_OK | MB_ICONERROR);

	return NULL;
}
