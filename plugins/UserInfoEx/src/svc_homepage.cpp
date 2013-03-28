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

#include "commonheaders.h"

static HANDLE ghMenuItem			= NULL;
static HANDLE ghExtraIconDef		= INVALID_HANDLE_VALUE;
static HANDLE ghExtraIconSvc		= INVALID_HANDLE_VALUE;

static HANDLE hChangedHook			= NULL;
static HANDLE hApplyIconHook		= NULL;
static HANDLE hRebuildIconsHook		= NULL;
/**
 * This function reads the homepage address of the contact.
 *
 * @param	hContact	- handle to contact to read email from
 *
 * @retval	URL to contacts homepage
 * @retval	NULL if contact provides no homepage
 **/
static LPSTR Get(HANDLE hContact)
{
	// ignore owner
	if (hContact != NULL) 
	{
		LPCSTR pszProto = DB::Contact::Proto(hContact);
		
		if (pszProto != NULL) 
		{
			LPCSTR	e[2] = { SET_CONTACT_HOMEPAGE, SET_CONTACT_COMPANY_HOMEPAGE };
			LPSTR pszHomepage;

			int i;

			for (i = 0; i < 2; i++)
			{
				pszHomepage = DB::Setting::GetAStringEx(hContact, USERINFO, pszProto, e[i]);
				if (pszHomepage)
					return pszHomepage;
			}
		}
	}
	return NULL;
}

/**
 * Service function that opens the default browser and displays the homepage.
 *
 * @param	wParam		- handle to contact to send an email to
 * @param	lParam		- not used
 *
 * @retval	0 if email was sent
 * @retval	1 if no email can be sent
 **/
static INT_PTR MenuCommand(WPARAM wParam, LPARAM lParam)
{
	LPSTR szUrl = Get((HANDLE)wParam);

	if (szUrl) 
	{
		CallService(MS_UTILS_OPENURL, 1, (LPARAM)szUrl);
		mir_free(szUrl);
	}
	else
	{
		MessageBox((HWND)lParam, 
			TranslateT("User has no valid homepage"),
			TranslateT("View Homepage"), MB_OK);
	}
	return 0;
}

/***********************************************************************************************************
 * Event Handler functions
 ***********************************************************************************************************/

/**
 * Notification handler for clist extra icons to be applied for a contact.
 *
 * @param	wParam		- handle to the contact whose extra icon is to apply
 * @param	lParam		- not used
 **/
static int OnCListApplyIcons(HANDLE hContact, LPARAM)
{
	LPSTR val = Get(hContact);
	if (ghExtraIconSvc != INVALID_HANDLE_VALUE)
		ExtraIcon_SetIcon(ghExtraIconSvc, hContact, (val) ? ICO_BTN_GOTO : NULL);
	MIR_FREE(val);
	return 0;
}

/**
 * Notification handler for changed contact settings
 *
 * @param	wParam		- (HANDLE)hContact
 * @param	lParam		- (DBCONTACTWRITESETTING*)pdbcws
 **/
static int OnContactSettingChanged(HANDLE hContact, DBCONTACTWRITESETTING* pdbcws)
{
	if (hContact && pdbcws && pdbcws->szSetting && 
			((pdbcws->value.type & DBVTF_VARIABLELENGTH) || (pdbcws->value.type == DBVT_DELETED)) &&
			(!strncmp(pdbcws->szSetting, SET_CONTACT_HOMEPAGE, 8) ||
			 !strncmp(pdbcws->szSetting, SET_CONTACT_COMPANY_HOMEPAGE, 15)))
		OnCListApplyIcons(hContact, 0);

	return 0;
}

/**
 * This function decides whether to show menuitem for sending emails or not.
 *
 * @param	wParam		- handle to contact to send an email to
 * @param	lParam		- not used
 *
 * @return	always 0
 **/
static int OnPreBuildMenu(WPARAM wParam, LPARAM lParam)
{
	LPSTR val;
	
	CLISTMENUITEM mi = { sizeof(mi) };
	mi.flags = CMIM_FLAGS;

	val = Get((HANDLE)wParam);
	if (val) 
	{
		MIR_FREE(val);
	}
	else mi.flags = CMIM_FLAGS | CMIF_HIDDEN;

	CallService(MS_CLIST_MODIFYMENUITEM, (WPARAM)ghMenuItem, (LPARAM)&mi);
	return 0;
}

/***********************************************************************************************************
 * public Module Interface functions
 ***********************************************************************************************************/

/**
 * enable or disable menuitem
 *
 * @param	not used
 * @return	nothing
 **/
void SvcHomepageRebuildMenu()
{
	static HANDLE hPrebuildMenuHook = NULL;

	if (!hPrebuildMenuHook)
		hPrebuildMenuHook = HookEvent(ME_CLIST_PREBUILDCONTACTMENU, OnPreBuildMenu);

	if (!ghMenuItem) {
		// insert contact menuitem
		CLISTMENUITEM mi = { sizeof(mi) };
		mi.position = -2000010000;
		mi.hIcon = IcoLib_GetIcon(ICO_BTN_GOTO);
		mi.pszName = LPGEN("&Homepage");
		mi.pszService = MS_USERINFO_HOMEPAGE_OPENURL;
		ghMenuItem = Menu_AddContactMenuItem(&mi);
	}
}

/**
 * Force all icons to be reloaded.
 *
 * @param	wParam		- handle to the contact whose extra icon is to apply
 * @param	lParam		- not used
 **/
void SvcHomepageApplyCListIcons()
{
	HANDLE hContact;

	//walk through all the contacts stored in the DB
	for (hContact = DB::Contact::FindFirst();	hContact != NULL;	hContact = DB::Contact::FindNext(hContact))
	{
		OnCListApplyIcons(hContact, 0);
	}
}

/**
 * Enable or disable the replacement of clist extra icons.
 *
 * @param	bEnable		- determines whether icons are enabled or not
 * @param	bUpdateDB	- if true the database setting is updated, too.
 **/
void SvcHomepageEnableExtraIcons(BYTE bEnable, BYTE bUpdateDB) 
{
	if (bUpdateDB)
		DB::Setting::WriteByte(SET_CLIST_EXTRAICON_HOMEPAGE, bEnable);

	if (bEnable) {
		// hook events
		if (hChangedHook == NULL) 
			hChangedHook = HookEvent(ME_DB_CONTACT_SETTINGCHANGED, (MIRANDAHOOK)OnContactSettingChanged);

		if (hApplyIconHook == NULL) 
			hApplyIconHook = HookEvent(ME_CLIST_EXTRA_IMAGE_APPLY, (MIRANDAHOOK)OnCListApplyIcons);

		if (ghExtraIconSvc == INVALID_HANDLE_VALUE)
			ghExtraIconSvc = ExtraIcon_Register("homepage", LPGEN("Homepage (uinfoex)"), ICO_BTN_GOTO);
	}
	else {
		if (hChangedHook) {
			UnhookEvent(hChangedHook); 
			hChangedHook = NULL;
		}			
		if (hApplyIconHook) {
			UnhookEvent(hApplyIconHook); 
			hApplyIconHook = NULL;
		}			
		if (hRebuildIconsHook) {
			UnhookEvent(hRebuildIconsHook); 
			hRebuildIconsHook = NULL;
		}
	}
	SvcHomepageApplyCListIcons();
}

/**
 * This function initially loads the module uppon startup.
 *
 * @param	not used
 * @return	nothing
 **/
void SvcHomepageLoadModule()
{
	CreateServiceFunction(MS_USERINFO_HOMEPAGE_OPENURL, MenuCommand);
	SvcHomepageEnableExtraIcons(
		DB::Setting::GetByte(SET_CLIST_EXTRAICON_HOMEPAGE, DEFVAL_CLIST_EXTRAICON_HOMEPAGE), FALSE);
}

/**
 * This function unloads the Email module.
 *
 * @param	none
 *
 * @return	nothing
 **/
void SvcHomepageUnloadModule()
{	
	// unhook event handlers
	UnhookEvent(hChangedHook);		hChangedHook		= NULL;
	UnhookEvent(hApplyIconHook);	hApplyIconHook		= NULL;
	UnhookEvent(hRebuildIconsHook);	hRebuildIconsHook	= NULL;
}
