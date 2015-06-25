#ifndef M_GENMENU_H
#define M_GENMENU_H

#ifndef M_CLIST_H__
   #include <m_clist.h>
#endif

extern int hLangpack;

// Group MENU
typedef struct
{
	int wParam;
	int lParam;
}
GroupMenuParam,*lpGroupMenuParam;

//builds the SubGroup menu
//wParam=lParam=0
//returns a HMENU identifying the menu.
#define MS_CLIST_MENUBUILDSUBGROUP							"CList/MenuBuildSubGroup"

//add a new item to the SubGroup menus
//wParam=lpGroupMenuParam, params to call when exec menuitem
//lParam=(LPARAM)(CLISTMENUITEM*)&mi

__forceinline HGENMENU Menu_AddSubGroupMenuItem(lpGroupMenuParam gmp, CLISTMENUITEM *mi)
{	mi->hLangpack = hLangpack;
	return (HGENMENU)CallService("CList/AddSubGroupMenuItem", (WPARAM)gmp, (LPARAM)mi);
}

//the SubGroup menu is about to be built
//wParam=lParam=0
#define ME_CLIST_PREBUILDSUBGROUPMENU						"CList/PreBuildSubGroupMenu"

// Group MENU

//builds the Group menu
//wParam=lParam=0
//returns a HMENU identifying the menu.
#define MS_CLIST_MENUBUILDGROUP							"CList/MenuBuildGroup"

//add a new item to the Group menus
//wParam=lpGroupMenuParam, params to call when exec menuitem
//lParam=(LPARAM)(CLISTMENUITEM*)&mi

__forceinline HGENMENU Menu_AddGroupMenuItem(lpGroupMenuParam gmp, CLISTMENUITEM *mi)
{	mi->hLangpack = hLangpack;
	return (HGENMENU)CallService("CList/AddGroupMenuItem", (WPARAM)gmp, (LPARAM)mi);
}

//the Group menu is about to be built
//wParam=lParam=0
#define ME_CLIST_PREBUILDGROUPMENU						"CList/PreBuildGroupMenu"

// TRAY MENU

//builds the tray menu
//wParam=lParam=0
//returns a HMENU identifying the menu.
#define MS_CLIST_MENUBUILDTRAY						"CList/MenuBuildTray"

//add a new item to the tray menus
//wParam=0
//lParam=(LPARAM)(CLISTMENUITEM*)&mi

__forceinline HGENMENU Menu_AddTrayMenuItem(CLISTMENUITEM *mi)
{	mi->hLangpack = hLangpack;
	return (HGENMENU)CallService("CList/AddTrayMenuItem", 0, (LPARAM)mi);
}

//the tray menu is about to be built
//wParam=lParam=0
#define ME_CLIST_PREBUILDTRAYMENU					"CList/PreBuildTrayMenu"

// STATUS MENU

//the status menu is about to be built
//wParam=lParam=0
#define ME_CLIST_PREBUILDSTATUSMENU "CList/PreBuildStatusMenu"

//builds the main menu
//wParam=lParam=0
//returns a HMENU identifying the menu.
#define MS_CLIST_MENUBUILDMAIN						"CList/MenuBuildMain"

//the main menu is about to be built
//wParam=lParam=0
#define ME_CLIST_PREBUILDMAINMENU					"CList/PreBuildMainMenu"

/*GENMENU_MODULE*/

#define SETTING_NOOFFLINEBOTTOM_DEFAULT 0

struct TMO_MenuItem
{
	int position;
	HGENMENU root;
	MAllStrings name;
	int flags;
	union {
		HICON hIcon;
		HANDLE hIcolibItem;
	};
	void *ownerdata;
	int   hLangpack;
};

/*
This structure passed to CheckService.
*/

struct TCheckProcParam
{
	void *MenuItemOwnerData;
	HGENMENU MenuItemHandle;
	WPARAM wParam;
	LPARAM lParam;
};

struct ProcessCommandParam
{
	HMENU menu;
	int ident;
	LPARAM lParam;
};

/////////////////////////////////////////////////////////////////////////////////////////
// Builds a menu from menu object's description
// Returns hMenu on success or NULL on failure

EXTERN_C MIR_APP_DLL(HMENU) Menu_Build(HMENU parent, HANDLE hMenuObject, WPARAM wParam = 0, LPARAM lParam = 0);

/////////////////////////////////////////////////////////////////////////////////////////
// Passes custom lParam to the ExecMenuService for the specified menu item
// Returns TRUE if command was processed, FALSE otherwise

EXTERN_C MIR_APP_DLL(BOOL) Menu_ProcessCommand(HGENMENU hMenuItem, LPARAM lParam);

/////////////////////////////////////////////////////////////////////////////////////////
// if menu not known call this
// LOWORD(wparam) menuident (from WM_COMMAND message)
// It automatically finds right menuobject and menuitem and calls Menu_ProcessCommand
// returns TRUE if command was processed, FALSE otherwise

EXTERN_C MIR_APP_DLL(BOOL) Menu_ProcessCommandById(int command, LPARAM lParam);

/////////////////////////////////////////////////////////////////////////////////////////
// Removes a menu item from genmenu
// Returns 0 on success,-1 on failure.
// You must free ownerdata before this call.
// If MenuItemHandle is root, all children will be removed too.

EXTERN_C MIR_APP_DLL(int) Menu_RemoveItem(HGENMENU hMenuItem);

/////////////////////////////////////////////////////////////////////////////////////////
// Adds a menu item to genmenu
// Returns MenuItemHandle on success, or NULL on failure

EXTERN_C MIR_APP_DLL(HGENMENU) Menu_AddItem(HANDLE hMenuObject, TMO_MenuItem *pItem);

/////////////////////////////////////////////////////////////////////////////////////////
// Retrieves user info from a menu item
// Returns ownerdata on success, NULL on failure
// Useful to get and free ownerdata before delete menu item.

EXTERN_C MIR_APP_DLL(void*) Menu_GetItemData(HGENMENU hMenuItem);

/////////////////////////////////////////////////////////////////////////////////////////
// Retrieves menu item structure by handle.
// Returns 0 and filled TMO_MenuItem structure on success, or -1 on failure

EXTERN_C MIR_APP_DLL(int) Menu_GetItemInfo(HGENMENU hMenuItem, TMO_MenuItem &pInfo);

/////////////////////////////////////////////////////////////////////////////////////////
// Retrieves a default menu item for the menu passed
// Returns a menu handle on success or NULL on failure

EXTERN_C MIR_APP_DLL(HGENMENU) Menu_GetDefaultItem(HGENMENU hMenu);

/////////////////////////////////////////////////////////////////////////////////////////
// Tries to process a keystroke
// returns TRUE if a key was, FALSE otherwise
// this should be called in WM_KEYDOWN

EXTERN_C MIR_APP_DLL(BOOL) Menu_ProcessHotKey(HANDLE hMenuObject, int key);

//process a WM_MEASUREITEM message for user context menus   v0.1.1.0+
//wParam, lParam, return value as for WM_MEASUREITEM
//This is for displaying the icons by the menu items. If you don't call this
//and clist/menudrawitem whne drawing a menu returned by one of the three menu
//services below then it'll work but you won't get any icons

EXTERN_C MIR_APP_DLL(BOOL) Menu_MeasureItem(MEASUREITEMSTRUCT *mis);

//process a WM_DRAWITEM message for user context menus      v0.1.1.0+
//wParam, lParam, return value as for WM_MEASUREITEM
//See comments for clist/menumeasureitem

EXTERN_C MIR_APP_DLL(BOOL) Menu_DrawItem(DRAWITEMSTRUCT *dis);

/////////////////////////////////////////////////////////////////////////////////////////
// Creates a new menu object
// szName = unique menu object identifier
// szDisplayName = menu display name (auto-translated by core)
// szCheckService = this service called when module build menu(MO_BUILDMENU).
//    Service is called with params wparam = PCheckProcParam, lparam = 0
//    if service returns FALSE, item is skipped.
// szExecService = this service called when user select menu item.
//    Service called with params wparam = ownerdata; lparam = lParam from MO_PROCESSCOMMAND
// 
// returns = MenuObjectHandle on success, NULL on failure

EXTERN_C MIR_APP_DLL(HANDLE) Menu_AddObject(LPCSTR szName, LPCSTR szDisplayName, LPCSTR szCheckService, LPCSTR szExecService);

/////////////////////////////////////////////////////////////////////////////////////////
// Removes the whole menu object with all submenus
// returns 0 on success, nonzero on failure
// Note: you must free all ownerdata structures, before you
// call this function. Menu_RemoveObject DOES NOT free it.

EXTERN_C MIR_APP_DLL(int) Menu_RemoveObject(HANDLE hMenuObject);

/////////////////////////////////////////////////////////////////////////////////////////
// tunes the whold menu object
// returns TRUE if it processed the command, FALSE otherwise

// enable ability user to edit menuitems via options page.
#define MCO_OPT_USERDEFINEDITEMS 1

// Set FreeService for menuobject. When freeing menuitem it will be called with
// wParam = MenuItemHandle
// lParam = mi.ownerdata
#define MCO_OPT_FREE_SERVICE 2

// Set onAddService for menuobject.
#define MCO_OPT_ONADD_SERVICE 3

// Set menu check service
#define MCO_OPT_CHECK_SERVICE 4

EXTERN_C MIR_APP_DLL(int) Menu_ConfigureObject(HANDLE hMenu, int iSetting, INT_PTR value);

__forceinline int Menu_ConfigureObject(HANDLE hMenu, int iSetting, LPCSTR pszValue)
{	return Menu_ConfigureObject(hMenu, iSetting, INT_PTR(pszValue));
}

/////////////////////////////////////////////////////////////////////////////////////////
// tunes a menu item
// returns TRUE if it processed the command, FALSE otherwise

#define MCI_OPT_UNIQUENAME 1 // a unique name to menuitem(used to store it in database when enabled OPT_USERDEFINEDITEMS)
#define MCI_OPT_HOTKEY     2 // DWORD value = MAKELONG(VK_*, VK_SHIFT)

EXTERN_C MIR_APP_DLL(int) Menu_ConfigureItem(HGENMENU hItem, int iOption, INT_PTR value);

__forceinline int Menu_ConfigureItem(HGENMENU hMenu, int iSetting, LPCSTR pszValue)
{	return Menu_ConfigureItem(hMenu, iSetting, INT_PTR(pszValue));
}

/////////////////////////////////////////////////////////////////////////////////////////
// returns HGENMENU of the root item or NULL

EXTERN_C MIR_APP_DLL(HGENMENU) Menu_GetProtocolRoot(const char *szProtoName);

/////////////////////////////////////////////////////////////////////////////////////////
// kills all menu items & submenus that belong to the hLangpack given

EXTERN_C MIR_APP_DLL(void) KillModuleMenus(int hLangpack);

#endif
