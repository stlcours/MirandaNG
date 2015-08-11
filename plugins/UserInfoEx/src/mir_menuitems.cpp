/*
UserinfoEx plugin for Miranda IM

Copyright:
� 2006-2010 DeathAxe, Yasnovidyashii, Merlin, K. Romanov, Kreol

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include "stdafx.h"

enum ECascadeType {
	MCAS_DISABLED	= 2,
	MCAS_ALL		= 4,
	MCAS_EXIMPORT	= 8,
	MCAS_NOTINITIATED = 128
};

HGENMENU hMenuItemRefresh = NULL;
HGENMENU *hMenuItemAccount	= NULL;

/**
 * Helper function to remove all menu items backward (first item second group).
 * Note: since miranda 0.8 genmenu is part of core (we don't need to check for).
 *
 * @param	pItems		- poiter to HGENMENU array
 * @param	Count		- number of array elements
 *
 * @return	0 on success, -1 on failure
 **/
INT_PTR RemoveMenuItems(HGENMENU *pItems, int Count)
{
	if (!Count || !pItems) {
		return -1;
	}
	while (Count--) {
		if (pItems[Count]) {
			Menu_RemoveItem(pItems[Count]);
			pItems[Count] = NULL;
		}
	}
	return 0;
}

/**
 * This function rebuilds the contactmenu. If user selected to cascade menus,
 * a root menu item is created which holds the popup for all the other items.
 * Note: since miranda 0.8 genmenu is part of core (we don't need to check for).
 *
 * @param	none
 *
 * @return	nothing
 **/
void RebuildContact()
{
	HGENMENU mhRoot = NULL;
	HGENMENU mhExIm = NULL;
	static HGENMENU hMenuItem[4] = { NULL, NULL, NULL, NULL };

	SvcEMailRebuildMenu();
	SvcHomepageRebuildMenu();

	// load options
	int flag = db_get_b(NULL, MODNAME, SET_MI_CONTACT, MCAS_NOTINITIATED);
	if (flag == MCAS_NOTINITIATED) {
		flag = MCAS_EXIMPORT | TRUE;
		db_set_b(NULL, MODNAME, SET_MI_CONTACT, flag);
	}

	// delete all MenuItems and set all bytes 0 to avoid problems
	RemoveMenuItems(hMenuItem, _countof(hMenuItem));

	// support new genmenu style
	CMenuItem mi;

	switch (flag) {
	case 3:
		//cascade off
		mhRoot = mhExIm = NULL;
		hMenuItem[0] = NULL;
		break;
	case 5:
		//cascade all
		mi.position = 1000050000;
		mi.hIcolibItem = IcoLib_GetIcon(ICO_COMMON_MAIN);
		mi.name.a = MODULELONGNAME;
		mhRoot = Menu_AddContactMenuItem(&mi);
		hMenuItem[0] = mhRoot;
		mhExIm = mhRoot;
		break;
	case 9:
		//cascade Ex/Import
		mi.position = 1000050100;
		mi.hIcolibItem = IcoLib_GetIcon(ICO_BTN_EXIMPORT);
		mi.name.a = LPGEN("Export/import contact");
		mhExIm = Menu_AddContactMenuItem(&mi);
		hMenuItem[0] = mhExIm;
		mhRoot = NULL;
		break;
	default:
		//disable Menu
		return;
	}

	// ContactDetailsPS's menuitem
	mi.root = mhRoot;
	mi.pszService = MS_USERINFO_SHOWDIALOG;
	mi.name.a = LPGEN("User &details");
	mi.position = 1000050000;
	mi.hIcolibItem = IcoLib_GetIcon(ICO_COMMON_MAIN);
	hMenuItem[1] = Menu_AddContactMenuItem(&mi);

	Menu_ConfigureItem(hMenuItem[1], MCI_OPT_HOTKEY, MAKELPARAM(VK_F3, MOD_ALT));

	// VCard's Ex/Import menuitems
	mi.root = mhExIm;

	// Export
	mi.pszService = MS_USERINFO_VCARD_EXPORT;
	mi.name.a = mhExIm != NULL ? LPGEN("&Export") : LPGEN("&Export user details");
	mi.position = 1000050200;
	mi.hIcolibItem = IcoLib_GetIcon(ICO_BTN_EXPORT);
	hMenuItem[2] = Menu_AddContactMenuItem(&mi);

	// Import
	mi.pszService = MS_USERINFO_VCARD_IMPORT;
	mi.name.a = mhExIm != NULL ? LPGEN("&Import") : LPGEN("&Import user details");
	mi.position = 1000050300;
	mi.hIcolibItem = IcoLib_GetIcon(ICO_BTN_IMPORT);
	hMenuItem[3] = Menu_AddContactMenuItem(&mi);
}

/**
 * This function rebuilds the mainmenu. If user selected to cascade menus,
 * a root menu item is created which holds the popup for all the other items.
 * Note: since miranda 0.8 genmenu is part of core (we don't need to check for).
 *
 * @param	none
 *
 * @return	nothing
 **/
void RebuildMain()
{
	BYTE item = 0;

	HGENMENU mhRoot = NULL;
	HGENMENU mhExIm = NULL;
	static HGENMENU hMenuItem[8] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };

	// load options
	int flag = db_get_b(NULL, MODNAME, SET_MI_MAIN, MCAS_NOTINITIATED);
	if (flag == MCAS_NOTINITIATED) {
		flag = MCAS_ALL | TRUE;
		db_set_b(NULL, MODNAME, SET_MI_MAIN, flag);
	}

	// delete all MenuItems and set all bytes 0 to avoid problems
	RemoveMenuItems(hMenuItem, _countof(hMenuItem));

	// support new genmenu style
	CMenuItem mi;

	switch (flag) {
	case 3:
		//cascade off
		mhRoot = mhExIm = NULL;
		hMenuItem[item++] = NULL;
		break;
	case 5:
		//cascade all
		mi.position = 500050000;
		mi.hIcolibItem = IcoLib_GetIcon(ICO_COMMON_MAIN);
		mi.name.a = MODULELONGNAME;
		mhRoot = Menu_AddMainMenuItem(&mi);
		hMenuItem[item++] = mhRoot;
		mhExIm = mhRoot;
		break;
	case 9:
		//cascade Ex/Import
		mi.position = 500050000;
		mi.hIcolibItem = IcoLib_GetIcon(ICO_BTN_EXIMPORT);
		mi.name.a = LPGEN("Export/import contact");
		mhExIm = Menu_AddMainMenuItem(&mi);
		hMenuItem[item++] = mhExIm;
		mhRoot = NULL;
		break;
	default:
		//disable Menue
		return;
	}

	// details dialog
	mi.root = mhRoot;
	mi.pszService = MS_USERINFO_SHOWDIALOG;
	mi.name.a = LPGEN("View/change my &details...");
	mi.position = 500050000;
	mi.hIcolibItem = IcoLib_GetIcon(ICO_COMMON_MAIN);
	hMenuItem[item++] = Menu_AddMainMenuItem(&mi);

	// VCard's Ex/Import menuitems
	mi.root = mhExIm;

	// Export
	mi.pszService = MS_USERINFO_VCARD_EXPORTALL;
	mi.name.a = LPGEN("Export all contacts");
	mi.position = 500150000;
	mi.hIcolibItem = IcoLib_GetIcon(ICO_BTN_EXPORT);
	hMenuItem[item++] = Menu_AddMainMenuItem(&mi);

	// Import
	mi.pszService = MS_USERINFO_VCARD_IMPORTALL;
	mi.name.a = LPGEN("Import all contacts");
	mi.position = 500151000;
	mi.hIcolibItem = IcoLib_GetIcon(ICO_BTN_IMPORT);
	hMenuItem[item++] = Menu_AddMainMenuItem(&mi);

	// reminder
	mi.root = mhRoot;
	const BYTE bRemindMenus =
		db_get_b(NULL, MODNAME, SET_REMIND_ENABLED, DEFVAL_REMIND_ENABLED) &&
		db_get_b(NULL, MODNAME, SET_REMIND_MENUENABLED, DEFVAL_REMIND_MENUENABLED);
	if (bRemindMenus) {
		// make backup of each protocol based birthday
		mi.pszService = MS_USERINFO_REMINDER_AGGRASIVEBACKUP;
		mi.name.a = LPGEN("Backup birthdays");
		mi.position = 500253000;
		mi.hIcolibItem = IcoLib_GetIcon(ICO_BTN_BDAY_BACKUP);
		hMenuItem[item++] = Menu_AddMainMenuItem(&mi);
		// Check anniversaries
		mi.pszService = MS_USERINFO_REMINDER_CHECK;
		mi.name.a = LPGEN("Check anniversaries");
		mi.position = 500251000;
		mi.hIcolibItem = IcoLib_GetIcon(ICO_BTN_SEARCH);
		hMenuItem[item++] = Menu_AddMainMenuItem(&mi);
	}
	else {
		hMenuItem[item++] = NULL;
		hMenuItem[item++] = NULL;
	}

	// Refresh Contact Details
	mi.pszService = MS_USERINFO_REFRESH;
	mi.name.a = LPGEN("Refresh contact details");
	mi.position = 500254000;
	mi.hIcolibItem = IcoLib_GetIcon(ICO_BTN_UPDATE);
	hMenuItem[item++] = Menu_AddMainMenuItem(&mi);

	// anniversary list
	mi.pszService = MS_USERINFO_REMINDER_LIST;
	mi.name.a = LPGEN("Anniversary list");
	mi.position = 500252000;
	mi.hIcolibItem = IcoLib_GetIcon(ICO_COMMON_ANNIVERSARY);
	hMenuItem[item++] = Menu_AddMainMenuItem(&mi);
}

/**
 * This function rebuilds the clist context menu (clist main groupmenu). If user selected to
 * cascade menus, a root menu item is created which holds the popup for all the other items.
 * Note: since miranda 0.8 genmenu is part of core (we don't need to check for).
 *
 * @param	none
 *
 * @return	nothing
 **/
void RebuildGroup()
{
	int flag = 0;
	BYTE item = 0;

	HGENMENU mhRoot = NULL;
	HGENMENU mhExIm = NULL;
	static HGENMENU hMenuItem[3] = { NULL, NULL, NULL };

	// load options
	flag = db_get_b(NULL, MODNAME, SET_MI_GROUP, MCAS_NOTINITIATED);
	if (flag == MCAS_NOTINITIATED) {
		flag = MCAS_EXIMPORT | TRUE;
		db_set_b(NULL, MODNAME, SET_MI_GROUP, flag);
	}

	// delete all MenuItems and set all bytes 0 to avoid problems
	RemoveMenuItems(hMenuItem, _countof(hMenuItem));

	// create service name main (prevent to generate {(Null)/Ex-/Import Group} in db) and set pointer to end it
	char text[200];
	mir_strcpy(text, "UserInfo");

	CMenuItem mi;
	mi.pszService = text;

	switch (flag) {
	case 3:
		//cascade off
		mhRoot = mhExIm = NULL;
		hMenuItem[item++] = NULL;
		break;
	case 5:
		//cascade all
		mi.position = 250000;
		mi.hIcolibItem = IcoLib_GetIcon(ICO_COMMON_MAIN);
		mi.name.a = MODULELONGNAME;
		mhRoot = Menu_AddGroupMenuItem(&mi);
		hMenuItem[item++] = mhRoot;
		mhExIm = mhRoot;
		break;
	case 9:
		//cascade Ex/Import
		mi.position = 250100;
		mi.hIcolibItem = IcoLib_GetIcon(ICO_BTN_EXIMPORT);
		mi.name.a = LPGEN("Export/import contact");
		mhExIm = Menu_AddGroupMenuItem(&mi);
		hMenuItem[item++] = mhExIm;
		mhRoot = NULL;
		break;
	default:
		//disable Menue
		return;
	}

	// VCard's Ex/Import menuitems
	mi.root = mhExIm;

	// Export
	mi.pszService = MS_USERINFO_VCARD_EXPORTALL;
	mi.name.a = LPGEN("Export all contacts");
	mi.position = 250200;
	mi.hIcolibItem = IcoLib_GetIcon(ICO_BTN_EXPORT);
	hMenuItem[item++] = Menu_AddGroupMenuItem(&mi);

	// Import
	mi.pszService = MS_USERINFO_VCARD_IMPORTALL;
	mi.name.a = LPGEN("Import all contacts");
	mi.position = 250300;
	mi.hIcolibItem = IcoLib_GetIcon(ICO_BTN_IMPORT);
	hMenuItem[item++] = Menu_AddGroupMenuItem(&mi);
}

/******************************
 * (Sub)GroupMenu
 ******************************/

/**
 * This function rebuilds the group context menu (clist main groupmenu). If user selected to
 * cascade menus, a root menu item is created which holds the popup for all the other items.
 * Note: since miranda 0.8 genmenu is part of core (we don't need to check for).
 *
 * @param	none
 *
 * @return	nothing
 **/
void RebuildSubGroup()
{
	int flag = 0;
	BYTE item = 0;
	GroupMenuParam gmp = { 0 };

	HGENMENU mhRoot = NULL;
	HGENMENU mhExIm = NULL;
	static HGENMENU hMenuItem[3] = { NULL, NULL, NULL };

	// load options
	flag = db_get_b(NULL, MODNAME, SET_MI_SUBGROUP, MCAS_NOTINITIATED);
	if (flag == MCAS_NOTINITIATED) {
		flag = MCAS_DISABLED | TRUE;
		db_set_b(NULL, MODNAME, SET_MI_SUBGROUP, flag);
	}

	// delete all MenuItems and set all bytes 0 to avoid problems
	RemoveMenuItems(hMenuItem, _countof(hMenuItem));

	// create service name main (prevent to generate {(Null)/Ex-/Import Group} in db) and set pointer to end it
	char text[200];
	mir_strcpy(text, "UserInfo");

	CMenuItem mi;
	mi.pszService = text;
	char* tDest = text + mir_strlen(text);

	switch (flag) {
	case 3:
		//cascade off
		mhRoot = mhExIm = NULL;
		hMenuItem[item++] = NULL;
		break;
	case 5:
		//cascade all
		mi.position = 1050000;
		mi.hIcolibItem = IcoLib_GetIcon(ICO_COMMON_MAIN);
		mi.name.a = MODULELONGNAME;
		mhRoot = Menu_AddSubGroupMenuItem(&mi);
		hMenuItem[item++] = mhRoot;
		mhExIm = mhRoot;
		break;
	case 9:
		//cascade Ex/Import
		mi.position = 1050100;
		mi.hIcolibItem = IcoLib_GetIcon(ICO_BTN_EXIMPORT);
		mi.name.a = LPGEN("Export/import group");
		mhExIm = Menu_AddSubGroupMenuItem(&mi);
		hMenuItem[item++] = mhExIm;
		mhRoot = NULL;
		break;
	default:
		//disable Menue
		return;
	}

	// VCard's Ex/Import menuitems
	mi.root = mhExIm;

	// Export
	mir_strcpy(tDest, "/ExportGroup");		//mi.pszService
	if (!ServiceExists(mi.pszService)) CreateServiceFunction(mi.pszService, svcExIm_Group_Service);
	mi.name.a = mhExIm != NULL ? LPGEN("&Export") : LPGEN("&Export group");
	mi.position = 1050200;
	mi.hIcolibItem = IcoLib_GetIcon(ICO_BTN_EXPORT);
	gmp.wParam = TRUE;
	hMenuItem[item++] = Menu_AddSubGroupMenuItem(&mi, &gmp);

	// Import
	mir_strcpy(tDest, "/ImportGroup");		//mi.pszService
	if (!ServiceExists(mi.pszService)) CreateServiceFunction(mi.pszService, svcExIm_Group_Service);
	mi.name.a = mhExIm != NULL ? LPGEN("&Import") : LPGEN("&Import group");
	mi.position = 1050300;
	mi.hIcolibItem = IcoLib_GetIcon(ICO_BTN_IMPORT);
	gmp.wParam = FALSE;
	hMenuItem[item++] = Menu_AddSubGroupMenuItem(&mi, &gmp);
}

/******************************
 * Account Menu
 ******************************/

/**
 * This function rebuilds the account context menu (clist status <account>). If user selected to
 * cascade menus, a root menu item is created which holds the popup for all the other items.
 * Note: since miranda 0.8 genmenu is part of core (we don't need to check for).
 *
 * @param	wParam				- 0 not used
 * @param	lParam				- clear bit for old menu items
 *								  0 don't delete old items (its calld by ME_CLIST_PREBUILDSTATUSMENU hook)
 *								  other then 0 delete old items first
 *
 * @return	always 0
 **/
INT_PTR RebuildAccount(WPARAM, LPARAM lParam)
{
	const BYTE mItems = 3;				// menuitems to create
	BYTE item = 0;

	int mProtoCount = pcli->menuProtoCount;

	// on call by hook or first start
	if (!lParam || !hMenuItemAccount) {
		size_t sizeNew = mItems * mProtoCount * sizeof(HGENMENU);
		hMenuItemAccount = (HGENMENU*)mir_realloc(hMenuItemAccount, sizeNew);
		// set all bytes 0 to avoid problems
		memset(hMenuItemAccount, 0, sizeNew);
	}
	// on options change
	else // delete all MenuItems backward (first item second group)
		RemoveMenuItems(hMenuItemAccount, mItems * mProtoCount);

	// load options
	int flag = db_get_b(NULL, MODNAME, SET_MI_ACCOUNT, MCAS_NOTINITIATED);
	if (flag == MCAS_NOTINITIATED) {
		flag = MCAS_EXIMPORT | TRUE;
		db_set_b(NULL, MODNAME, SET_MI_ACCOUNT, flag);
	}

	// loop for all account names
	for (int i = 0; i < mProtoCount; i++) {
		// set all bytes 0 to avoid problems
		item = 0;

		HGENMENU mhRoot = pcli->menuProtos[i].pMenu, mhExIm;
		if (mhRoot == NULL)
			break;

		PROTOACCOUNT *pa = Proto_GetAccount(pcli->menuProtos[i].szProto);

		// create service name main (account module name) and set pointer to end it
		char text[200];
		mir_strcpy(text, pcli->menuProtos[i].szProto);

		CMenuItem mi;
		mi.pszService = text;
		char* tDest = text + mir_strlen(text);

		// support new genmenu style
		mi.root = mhRoot;

		switch (flag) {
		case 3:
			// cascade off
			mhExIm = mhRoot;
			// seperator
			mi.position = 50100;
			hMenuItemAccount[mItems*i + item++] = Menu_AddStatusMenuItem(&mi, pa->szModuleName);
			break;
		case 5:
			// cascade all
			mi.position = 50100;
			mi.hIcolibItem = IcoLib_GetIcon(ICO_COMMON_MAIN);
			mi.name.a = MODULELONGNAME;
			hMenuItemAccount[mItems*i + item] = Menu_AddStatusMenuItem(&mi, pa->szModuleName);
			mhRoot = hMenuItemAccount[mItems*i + item++];
			mhExIm = mhRoot;
			break;
		case 9:
			// cascade Ex/Import
			mi.position = 50100;
			mi.hIcolibItem = IcoLib_GetIcon(ICO_BTN_EXIMPORT);
			mi.name.a = LPGEN("Export/import");
			hMenuItemAccount[mItems*i + item] = Menu_AddStatusMenuItem(&mi, pa->szModuleName);
			mhRoot = hMenuItemAccount[mItems*i + item++];
			mhExIm = mhRoot;
			break;
		default:
			// disable Menue
			return 0;
		}

		// VCard's Ex/Import menuitems
		mi.root = mhExIm;

		// Export
		mir_strcpy(tDest, "/ExportAccount");
		CreateServiceFunction(mi.pszService, svcExIm_Account_Service);
		mi.name.a = LPGEN("&Export xml");
		mi.position = 50200;
		mi.hIcolibItem = IcoLib_GetIcon(ICO_BTN_EXPORT);
		hMenuItemAccount[mItems*i + item++] = Menu_AddStatusMenuItem(&mi, pa->szModuleName);

		// Import
		mir_strcpy(tDest, "/ImportAccount");
		CreateServiceFunction(mi.pszService, svcExIm_Account_Service);
		mi.name.a = LPGEN("&Import xml");
		mi.position = 50300;
		mi.hIcolibItem = IcoLib_GetIcon(ICO_BTN_IMPORT);
		hMenuItemAccount[mItems*i + item++] = Menu_AddStatusMenuItem(&mi, pa->szModuleName);
	}
	return 0;
}

void RebuildMenu()
{
	RebuildMain();
	RebuildContact();
	RebuildGroup();
	RebuildSubGroup();
	RebuildAccount(NULL, 1);
}
