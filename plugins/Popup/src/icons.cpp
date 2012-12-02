/*
Popup Plus plugin for Miranda IM

Copyright	� 2002 Luca Santarelli,
			� 2004-2007 Victor Pavlychko
			� 2010 MPK
			� 2010 Merlin_de

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

===============================================================================

File name      : $HeadURL: http://svn.miranda.im/mainrepo/popup/trunk/src/icons.cpp $
Revision       : $Revision: 1610 $
Last change on : $Date: 2010-06-23 00:55:13 +0300 (Ср, 23 июн 2010) $
Last change by : $Author: Merlin_de $

===============================================================================
*/

#include "headers.h"

static IconItem iconList[] =
{
	//toolbar
	{ "Popups are enabled",          ICO_TB_POPUP_ON,    IDI_POPUP          },
	{ "Popups are disabled",         ICO_TB_POPUP_OFF,   IDI_NOPOPUP        },

	//                               common popup
	{ "Popups are enabled",          ICO_POPUP_ON,       IDI_POPUP          },
	{ "Popups are disabled",         ICO_POPUP_OFF,      IDI_NOPOPUP        },
	{ "With \"favourite\" overlay",  ICO_FAV,            IDI_PU_FAVOURITE   },
	{ "With \"fullscreen\" overlay", ICO_FULLSCREEN,     IDI_PU_FULLSCREEN  },
	{ "Popup History",               ICO_HISTORY,        IDI_HISTORY        },

	//                               option
	{ "Refresh skin list",           ICO_OPT_RELOAD,     IDI_RELOAD         },
	{ "Popup Placement",             ICO_OPT_RESIZE,     IDI_RESIZE         },
	{ "OK",                          ICO_OPT_OK,         IDI_ACT_OK         },
	{ "Cancel",                      ICO_OPT_CANCEL,     IDI_ACT_CLOSE      },
	{ "Popup Group",                 ICO_OPT_GROUP,      IDI_OPT_GROUP      },
	{ "Show default",                ICO_OPT_DEF,        IDI_ACT_OK         },
	{ "Favorite Contact",            ICO_OPT_FAV,        IDI_OPT_FAVORITE   },
	{ "Show in Fullscreen",          ICO_OPT_FULLSCREEN, IDI_OPT_FULLSCREEN },
	{ "Blocked Contact",             ICO_OPT_BLOCK,      IDI_OPT_BLOCK      },

	//                               action
	{ "Quick Reply",                 ICO_ACT_REPLY,      IDI_ACT_REPLY      },
	{ "Pin Popup",                   ICO_ACT_PIN,        IDI_ACT_PIN        },
	{ "Pinned Popup",                ICO_ACT_PINNED,     IDI_ACT_PINNED     },
	{ "Send Message",                ICO_ACT_MESS,       IDI_ACT_MESSAGE    },
	{ "User Details",                ICO_ACT_INFO,       IDI_ACT_INFO       },
	{ "Contact Menu",                ICO_ACT_MENU,       IDI_ACT_MENU       },
	{ "Add Contact Permanently",     ICO_ACT_ADD,        IDI_ACT_ADD        },
	{ "Dismiss Popup",               ICO_ACT_CLOSE,      IDI_ACT_CLOSE      },
	{ "Copy to clipboard",           ICO_ACT_COPY,       IDI_ACT_COPY       }

};

/**
 * Returns a icon, identified by a name
 * @param	pszIcon		- name of the icon
 * @param	big			- bool big icon (default = false)
 * @return:	HICON if the icon is loaded, NULL otherwise
 **/

HICON IcoLib_GetIcon(LPCSTR pszIcon, bool big)
{
	return (pszIcon) ? Skin_GetIcon(pszIcon, big) : NULL;
}

void InitIcons()
{
	Icon_Register(hInst, SECT_TOLBAR, iconList, 2);
	Icon_Register(hInst, SECT_POPUP,  iconList+2, 5);
	Icon_Register(hInst, SECT_POPUP SECT_POPUP_OPT,  iconList+7, 9);
	Icon_Register(hInst, SECT_POPUP SECT_POPUP_ACT,  iconList+16, 9);
}
