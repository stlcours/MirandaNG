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

===============================================================================

File name      : $HeadURL: https://userinfoex.googlecode.com/svn/trunk/svc_gender.cpp $
Revision       : $Revision: 187 $
Last change on : $Date: 2010-09-08 16:05:54 +0400 (Ср, 08 сен 2010) $
Last change by : $Author: ing.u.horn $

===============================================================================
*/
#include "commonheaders.h"

static HANDLE ghExtraIconF			= INVALID_HANDLE_VALUE;
static HANDLE ghExtraIconM			= INVALID_HANDLE_VALUE;
static HANDLE ghExtraIconSvc		= INVALID_HANDLE_VALUE;

static HANDLE hChangedHook			= NULL;
static HANDLE hApplyIconHook		= NULL;
static HANDLE hRebuildIconsHook		= NULL;

BYTE GenderOf(HANDLE hContact, LPCSTR pszProto)
{
	DBVARIANT dbv;

	if (DB::Setting::GetAsIsEx(hContact, USERINFO, pszProto, SET_CONTACT_GENDER, &dbv) == 0)
	{
		// gender must be byte and either M or F
		if (dbv.type == DBVT_BYTE && (dbv.bVal == 'M' || dbv.bVal == 'F'))
		{
			return dbv.bVal;	
		}
		DB::Variant::Free(&dbv);
	}
	return 0;
}

/**
 * This function gets the gender of the contact from the database.
 *
 * @param	hContact		- handle to contact to read email from
 *
 * @retval	F	- contact is female
 * @retval	M	- contact is male
 * @retval	0	- contact does not provide its gender
 **/
BYTE GenderOf(HANDLE hContact)
{
	return GenderOf(hContact, DB::Contact::Proto(hContact));
}

/***********************************************************************************************************
 * Event Handler functions
 ***********************************************************************************************************/

/**
 * Notification handler for clist extra icons to be applied for a contact.
 *
 * @param	wParam			- handle to the contact whose extra icon is to apply
 * @param	lParam			- not used
 **/

static INT OnCListApplyIcons(HANDLE hContact, LPARAM)
{
	if (ghExtraIconSvc != INVALID_HANDLE_VALUE) {
		EXTRAICON ico = { sizeof(ico) };
		ico.hContact = hContact;
		ico.hExtraIcon = ghExtraIconSvc;
		switch (GenderOf(hContact)) {
		case 'M': 
			ico.icoName = ICO_COMMON_MALE;
			break;
		case 'F': 
			ico.icoName = ICO_COMMON_FEMALE;
			break;
		default:
			ico.icoName = NULL;
		}
		CallService(MS_EXTRAICON_SET_ICON, (WPARAM)&ico, 0);
	}
	return 0;
}

/**
 * Notification handler for changed contact settings
 *
 * @param	wParam			- (HANDLE)hContact
 * @param	lParam			- (DBCONTACTWRITESETTING*)pdbcws
 **/
static INT OnContactSettingChanged(HANDLE hContact, DBCONTACTWRITESETTING* pdbcws)
{
	if (hContact && pdbcws && (pdbcws->value.type <= DBVT_BYTE) && !mir_stricmp(pdbcws->szSetting, SET_CONTACT_GENDER))
	{
		OnCListApplyIcons(hContact, 0);
	}
	return 0;
}

/***********************************************************************************************************
 * public Module Interface functions
 ***********************************************************************************************************/

/**
 * Force all icons to be reloaded.
 *
 * @param	wParam			- handle to the contact whose extra icon is to apply
 * @param	lParam			- not used
 **/
VOID SvcGenderApplyCListIcons()
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
 * @param	bEnable			- determines whether icons are enabled or not
 * @param	bUpdateDB		- if true the database setting is updated, too.
 **/
VOID SvcGenderEnableExtraIcons(BYTE bColumn, BOOLEAN bUpdateDB) 
{
	bool bEnable = (bColumn!=((BYTE)-1));
	if (myGlobals.HaveCListExtraIcons)
	{
		if (bUpdateDB)
			DB::Setting::WriteByte(SET_CLIST_EXTRAICON_GENDER2, bColumn);

		if (bEnable)	// Gender checkt or dropdown select
		{
			if (ghExtraIconSvc == INVALID_HANDLE_VALUE) {
				EXTRAICON_INFO ico = { sizeof(ico) };
				ico.type = EXTRAICON_TYPE_ICOLIB;
				ico.name = "gender";	//must be the same as the group name in extraicon
				ico.description="Gender (uinfoex)";
				ico.descIcon = ICO_COMMON_MALE;
				ghExtraIconSvc = (HANDLE)CallService(MS_EXTRAICON_REGISTER, (WPARAM)&ico, 0);
			}

			// hook events
			if (hChangedHook == NULL) 
				hChangedHook = HookEvent(ME_DB_CONTACT_SETTINGCHANGED, (MIRANDAHOOK)OnContactSettingChanged);

			if (hApplyIconHook == NULL) 
				hApplyIconHook = HookEvent(ME_CLIST_EXTRA_IMAGE_APPLY, (MIRANDAHOOK)OnCListApplyIcons);
		}
		else
		{
			if (hChangedHook)
			{
				UnhookEvent(hChangedHook); 
				hChangedHook = NULL;
			}
			if (hApplyIconHook)
			{
				UnhookEvent(hApplyIconHook); 
				hApplyIconHook = NULL;
			}
			if (hRebuildIconsHook)
			{
				UnhookEvent(hRebuildIconsHook); 
				hRebuildIconsHook = NULL;
			}
		}
		SvcGenderApplyCListIcons();
	}
}

/**
 * This function initially loads the module uppon startup.
 **/
VOID SvcGenderLoadModule()
{
	SvcGenderEnableExtraIcons(DB::Setting::GetByte(SET_CLIST_EXTRAICON_GENDER2, 0), FALSE);
}

/**
 * This function unloads the module.
 *
 * @param	none
 *
 * @return	nothing
 **/
VOID SvcGenderUnloadModule()
{	
	// unhook event handlers
	UnhookEvent(hChangedHook);		hChangedHook		= NULL;
	UnhookEvent(hApplyIconHook);	hApplyIconHook		= NULL;
	UnhookEvent(hRebuildIconsHook);	hRebuildIconsHook	= NULL;
}
