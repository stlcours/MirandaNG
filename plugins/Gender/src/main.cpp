/*
   Show Contact Gender plugin for Miranda-IM (www.miranda-im.org)
   (c) 2006-2011 by Thief

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

   File name      : $URL: http://svn.miranda.im/mainrepo/gender/trunk/main.cpp $
   Revision       : $Rev: 1688 $
   Last change on : $Date: 2011-01-22 19:44:43 +0200 (Сб, 22 янв 2011) $
   Last change by : $Author: Thief $

*/

#include "commonheaders.h"

HINSTANCE g_hInst;

static HANDLE hContactMenu = NULL, hContactMenuMale = NULL, hContactMenuFemale = NULL, hContactMenuNotDef = NULL;
static HANDLE hSetMale = NULL, hSetFemale = NULL, hSetUndef = NULL, hGenderGetIcon = NULL;

HANDLE g_hExtraIcon = NULL;
HANDLE g_hIconMale, g_hIconFemale, g_hIconMenu;

byte bEnableClistIcon = 1; // do we need clist icon?
byte bDrawNoGenderIcon = 0; // enable icon when no info?
byte bContactMenuItems = 1; // do we need a contact menu items?
byte bMetaAvail = 0; // metacontacts installed?
int hLangpack;

extern int onOptInitialise(WPARAM wParam, LPARAM lParam);

PLUGININFOEX pluginInfo={
sizeof(PLUGININFOEX),
	"Show Contact Gender",
	PLUGIN_MAKE_VERSION(0,0,2,1),
	"Shows contacts gender as an icon in contact list",
	"Thief, idea by nile, icons by a0x",
	"thief@miranda.im",
	"2006-2011 Alexander Turyak",
	"http://miranda-ng.org/",
	UNICODE_AWARE,      //doesn't replace anything built-in
	{0xfb1c17e0, 0x77fc, 0x45a7, {0x9c, 0x8b, 0xe2, 0xbe, 0xf4, 0xf5, 0x6b, 0x28}} /* FB1C17E0-77FC-45A7-9C8B-E2BEF4F56B28 */
};

extern "C" BOOL WINAPI DllMain(HINSTANCE hinstDLL,DWORD fdwReason,LPVOID lpvReserved)
{
	g_hInst = hinstDLL;
	return TRUE;
}

/* 0FF3991A-5505-479D-A2E0-53DD31C6DFA7 */
#define MIID_GENDER {0x0ff3991a, 0x5505, 0x479d, {0xa2, 0xe0, 0x53, 0xdd, 0x31, 0xc6, 0xdf, 0xa7}}

extern "C" __declspec(dllexport) const MUUID MirandaInterfaces[] = {MIID_GENDER, MIID_LAST};

extern "C" __declspec(dllexport) PLUGININFOEX* MirandaPluginInfoEx(DWORD mirandaVersion)
{
	return &pluginInfo;
}

void setIcon(HANDLE hContact, unsigned int gender)
{
	char *ico;
	switch (gender) {
		case 77: ico = "male_icon"; break;
		case 70: ico = "female_icon"; break;
		default: ico = (bDrawNoGenderIcon ? "menu_icon" : NULL); break;
	}
	ExtraIcon_SetIcon(g_hExtraIcon, hContact, ico);
}

int applyExtraImage(HANDLE hContact)
{
	if (!bEnableClistIcon) return 0;

	if (bMetaAvail) {
		HANDLE hMetacontact = (HANDLE)CallService(MS_MC_GETMOSTONLINECONTACT, (WPARAM)hContact, 0);
		if (hMetacontact != NULL)
			hContact = hMetacontact;
	}
		
	char *proto = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
	if (!proto)
		return 0;

	if ( DBGetContactSettingByte(hContact, proto, "ChatRoom", 0))
		return 0;
	
	unsigned int gender = DBGetContactSettingByte(hContact, "UserInfo", "Gender", DBGetContactSettingByte(hContact, proto, "Gender", 0));

	setIcon(hContact, gender);
	return 0;
}

// Returns gender icon for specific contact
INT_PTR GetIcon(WPARAM wParam, LPARAM lParam)
{
	char *proto = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, wParam, 0);
	unsigned short gender = DBGetContactSettingByte((HANDLE)wParam, "UserInfo", "Gender", DBGetContactSettingByte((HANDLE)wParam, proto, "Gender", 0));
	
	if (gender > 0) {
		if (gender == 77)
			return (INT_PTR)Skin_GetIconByHandle(g_hIconMale); 
		if (gender == 70)
			return (INT_PTR)Skin_GetIconByHandle(g_hIconFemale);
	}
	
	return 0;
}

int onPrebuildContactMenu(WPARAM wParam, LPARAM lParam)
{
	char *proto = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, wParam, 0);

	if (!proto) return 0;

	CLISTMENUITEM mi = {0};
	mi.cbSize = sizeof(CLISTMENUITEM);
	
	
	if (DBGetContactSettingByte((HANDLE)wParam, proto, "ChatRoom", 0) || !(CallProtoService(proto, PS_GETCAPS, PFLAGNUM_1, 0) & PF1_IMSEND))
		mi.flags = CMIM_FLAGS | CMIF_HIDDEN;
	else
		mi.flags = CMIM_FLAGS;
	
	CallService(MS_CLIST_MODIFYMENUITEM, (WPARAM)hContactMenu, (LPARAM)&mi);
	
	unsigned short gender = DBGetContactSettingByte((HANDLE)wParam, proto, "Gender", DBGetContactSettingByte((HANDLE)wParam, "UserInfo", "Gender", 0));
	
	CLISTMENUITEM mitem = {0};
	mitem.cbSize = sizeof(CLISTMENUITEM);
	mitem.flags = CMIM_FLAGS;
	
	CallService(MS_CLIST_MODIFYMENUITEM, (WPARAM)hContactMenuMale, (LPARAM)&mitem);
	CallService(MS_CLIST_MODIFYMENUITEM, (WPARAM)hContactMenuFemale, (LPARAM)&mitem);
	CallService(MS_CLIST_MODIFYMENUITEM, (WPARAM)hContactMenuNotDef, (LPARAM)&mitem);
	
	mitem.flags = CMIM_FLAGS | CMIF_CHECKED;
	
	switch (gender)
	{
		case 77:
			CallService(MS_CLIST_MODIFYMENUITEM, (WPARAM)hContactMenuMale, (LPARAM)&mitem);
		break;
		case 70:
			CallService(MS_CLIST_MODIFYMENUITEM, (WPARAM)hContactMenuFemale, (LPARAM)&mitem);
		break;
		case 0:
			CallService(MS_CLIST_MODIFYMENUITEM, (WPARAM)hContactMenuNotDef, (LPARAM)&mitem);
		break;
	}	
	
	return 0;
}

void setGender(HANDLE hContact, unsigned int gender)
{
	DBWriteContactSettingByte(hContact, "UserInfo", "Gender", gender);
	setIcon(hContact, gender);

	int metasnum = (bMetaAvail ? CallService(MS_MC_GETNUMCONTACTS,(WPARAM)hContact,0) : 0);
	for(int i=0; i<metasnum; i++)
	{
		HANDLE hSubContact = (HANDLE)CallService(MS_MC_GETSUBCONTACT, (WPARAM)hContact, i);
		DBWriteContactSettingByte(hSubContact, "UserInfo", "Gender", gender);
		setIcon(hSubContact, gender);
	}
}

INT_PTR onSetMale(WPARAM wParam,LPARAM lParam)
{
	setGender((HANDLE)wParam, 77);
	return 0;
}

INT_PTR onSetFemale(WPARAM wParam,LPARAM lParam)
{
	setGender((HANDLE)wParam, 70);
	return 0;
}

INT_PTR onSetUndef(WPARAM wParam,LPARAM lParam)
{
	DBDeleteContactSetting((HANDLE)wParam, "UserInfo", "Gender");
	applyExtraImage((HANDLE)wParam);

	int metasnum = (bMetaAvail ? CallService(MS_MC_GETNUMCONTACTS,wParam,0) : 0);
	for(int i=0; i<metasnum; i++)
	{
		HANDLE hContact = (HANDLE)CallService(MS_MC_GETSUBCONTACT, wParam, i);
		DBDeleteContactSetting(hContact, "UserInfo", "Gender");
		applyExtraImage(hContact);
	}
	
	return 0;
}

int onModulesLoaded(WPARAM wParam,LPARAM lParam)
{
	HookEvent(ME_OPT_INITIALISE, onOptInitialise);
	if (bContactMenuItems)
		HookEvent(ME_CLIST_PREBUILDCONTACTMENU, onPrebuildContactMenu);
	bMetaAvail = (ServiceExists(MS_MC_GETMETACONTACT) != 0); 
	
	TCHAR szFile[MAX_PATH];
	GetModuleFileName(g_hInst, szFile, MAX_PATH);

	//IcoLib support
	SKINICONDESC sid = {0};
	sid.cbSize = sizeof(sid);
	sid.flags = SIDF_ALL_TCHAR;	
	sid.ptszSection = LPGENT("Gender");
	sid.ptszDefaultFile = szFile;
		
	sid.ptszDescription = LPGENT("Male");
	sid.pszName = "male_icon";
	sid.iDefaultIndex = -IDI_MALE;
	g_hIconMale = Skin_AddIcon(&sid);
		
	sid.ptszDescription = LPGENT("Female");
	sid.pszName = "female_icon";
	sid.iDefaultIndex = -IDI_FEMALE;
	g_hIconFemale = Skin_AddIcon(&sid);
		
	sid.ptszDescription = LPGENT("No info");
	sid.pszName = "menu_icon";
	sid.iDefaultIndex = -IDI_UNDEF;
	g_hIconMenu = Skin_AddIcon(&sid);
		
	// Adding clist extra icon
	g_hExtraIcon = ExtraIcon_Register("gender", "Gender", "menu_icon");

	HANDLE hContact = db_find_first();
	while (hContact != NULL) {
		applyExtraImage(hContact);
		hContact = db_find_next(hContact);
	}
	
	// Adding menu items, submenu even if clist supports that
	if (bContactMenuItems)
	{
		if(ServiceExists(MS_CLIST_MENUBUILDSUBGROUP))
		{
			CLISTMENUITEM mi = {0};
			mi.cbSize = sizeof(CLISTMENUITEM);
			mi.flags = CMIF_ROOTPOPUP | CMIF_ICONFROMICOLIB | CMIF_TCHAR;
			mi.icolibItem = g_hIconMenu;
			mi.pszPopupName = (char*)-1;
			mi.position = 203;
			mi.ptszName = LPGENT("Set Gender");
			mi.pszService = NULL;
			hContactMenu = Menu_AddContactMenuItem(&mi);
			
			mi.flags = CMIF_CHILDPOPUP | CMIF_ICONFROMICOLIB | CMIF_TCHAR;
			mi.position = 1001;
			mi.pszPopupName = (char*)hContactMenu;
			
			mi.ptszName = LPGENT("Male");
			mi.icolibItem = g_hIconMale;
			mi.pszService = "Gender/MenuItemSetMale";
			hContactMenuMale = Menu_AddContactMenuItem(&mi);
			
			mi.ptszName = LPGENT("Female");
			mi.icolibItem = g_hIconFemale;
			mi.pszService = "Gender/MenuItemSetFemale";
			hContactMenuFemale = Menu_AddContactMenuItem(&mi);
			
			mi.ptszName = LPGENT("Undefined");
			mi.hIcon = NULL;
			mi.pszService = "Gender/MenuItemSetUndef";
			hContactMenuNotDef = Menu_AddContactMenuItem(&mi);
		}
		else
		{
			CLISTMENUITEM mi = {0};
			mi.cbSize = sizeof(CLISTMENUITEM);
			mi.flags = CMIF_ICONFROMICOLIB | CMIF_TCHAR;
			mi.position = 1001;
			mi.ptszName = LPGENT("Set Male");
			mi.icolibItem = g_hIconMale;
			mi.pszService = "Gender/MenuItemSetMale";
			hContactMenuMale = Menu_AddContactMenuItem(&mi);
			
			mi.flags = CMIF_ICONFROMICOLIB | CMIF_TCHAR;
			mi.position = 1002;
			mi.ptszName = LPGENT("Set Female");
			mi.icolibItem = g_hIconFemale;
			mi.pszService = "Gender/MenuItemSetFemale";
			hContactMenuFemale = Menu_AddContactMenuItem(&mi);
			
			mi.flags =  CMIF_TCHAR;
			mi.position = 1003;
			mi.ptszName = LPGENT("Set Undefined");
			mi.pszService = "Gender/MenuItemSetUndef";
			mi.hIcon = NULL;
			hContactMenuNotDef = Menu_AddContactMenuItem(&mi);
		}
	}
	
	return 0;
}

int onSystemOKToExit(WPARAM wParam,LPARAM lParam)
{
	DestroyServiceFunction(hSetMale);
	DestroyServiceFunction(hSetFemale);
	DestroyServiceFunction(hSetUndef);
	DestroyServiceFunction(hGenderGetIcon);
	return 0;
}

extern "C" int __declspec(dllexport) Load(void)
{
	mir_getLP(&pluginInfo);
	
	HookEvent(ME_SYSTEM_MODULESLOADED, onModulesLoaded);
	HookEvent(ME_SYSTEM_OKTOEXIT,onSystemOKToExit);
	
	bEnableClistIcon  = DBGetContactSettingByte(NULL, MODULENAME, "ClistIcon", 1);
	bContactMenuItems = DBGetContactSettingByte(NULL, MODULENAME, "MenuItems", 1);
	bDrawNoGenderIcon = DBGetContactSettingByte(NULL, MODULENAME, "NoGenderIcon", 0);

	hSetMale    = CreateServiceFunction("Gender/MenuItemSetMale", onSetMale);
	hSetFemale  = CreateServiceFunction("Gender/MenuItemSetFemale", onSetFemale);
	hSetUndef   = CreateServiceFunction("Gender/MenuItemSetUndef", onSetUndef);
	
	hGenderGetIcon = CreateServiceFunction(MS_GENDER_GETICON, GetIcon);
	return 0;
}

extern "C" int __declspec(dllexport) Unload(void)
{   
	return 0;
}
