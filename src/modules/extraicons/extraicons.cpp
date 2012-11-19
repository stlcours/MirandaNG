/*
 Copyright (C) 2009 Ricardo Pescuma Domenecci

 This is free software; you can redistribute it and/or
 modify it under the terms of the GNU Library General Public
 License as published by the Free Software Foundation; either
 version 2 of the License, or (at your option) any later version.

 This is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Library General Public License for more details.

 You should have received a copy of the GNU Library General Public
 License along with this file; see the file license.txt.  If
 not, write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 Boston, MA 02111-1307, USA.
 */

#include "..\..\core\commonheaders.h"

#include "m_cluiframes.h"

#include "BaseExtraIcon.h"
#include "CallbackExtraIcon.h"
#include "DefaultExtraIcons.h"
#include "ExtraIcon.h"
#include "ExtraIconGroup.h"
#include "IcolibExtraIcon.h"

#include "extraicons.h"
#include "usedIcons.h"
#include "..\clist\clc.h"

// Prototypes ///////////////////////////////////////////////////////////////////////////

vector<BaseExtraIcon*> registeredExtraIcons;
vector<ExtraIcon*> extraIconsByHandle;
vector<ExtraIcon*> extraIconsBySlot;

BOOL clistRebuildAlreadyCalled = FALSE;
BOOL clistApplyAlreadyCalled = FALSE;

int clistFirstSlot = 0;
int clistSlotCount = 0;

// Functions ////////////////////////////////////////////////////////////////////////////

int InitOptionsCallback(WPARAM wParam, LPARAM lParam);

// Called when all the modules are loaded
int ModulesLoaded(WPARAM wParam, LPARAM lParam)
{
	// add our modules to the KnownModules list
	CallService("DBEditorpp/RegisterSingleModule", (WPARAM) MODULE_NAME, 0);
	CallService("DBEditorpp/RegisterSingleModule", (WPARAM) MODULE_NAME "Groups", 0);

	HookEvent(ME_OPT_INITIALISE, InitOptionsCallback);
	return 0;
}

int PreShutdown(WPARAM wParam, LPARAM lParam)
{
	DefaultExtraIcons_Unload();
	return 0;
}

int GetNumberOfSlots()
{
	return clistSlotCount;
}

int ConvertToClistSlot(int slot)
{
	if (slot < 0)
		return slot;

	return clistFirstSlot + slot;
}

int ExtraImage_ExtraIDToColumnNum(int extra)
{
	return (extra < 1 || extra > EXTRA_ICON_COUNT) ? -1 : extra-1;
}

int Clist_SetExtraIcon(HANDLE hContact, int slot, HANDLE hImage)
{
	if (cli.hwndContactTree == 0)
		return -1;

	int icol = ExtraImage_ExtraIDToColumnNum( ConvertToClistSlot(slot));
	if (icol == -1)
		return -1;

	HANDLE hItem = (HANDLE)SendMessage(cli.hwndContactTree, CLM_FINDCONTACT, (WPARAM)hContact, 0);
	if (hItem == 0)
		return -1;

	SendMessage(cli.hwndContactTree, CLM_SETEXTRAIMAGE, (WPARAM)hItem, MAKELPARAM(icol,hImage));
	return 0;
}

ExtraIcon* GetExtraIcon(HANDLE id)
{
	unsigned int i = (int)id;

	if (i < 1 || i > extraIconsByHandle.size())
		return NULL;

	return extraIconsByHandle[i - 1];
}

ExtraIcon* GetExtraIconBySlot(int slot)
{
	for (unsigned int i = 0; i < extraIconsBySlot.size(); i++) {
		ExtraIcon *extra = extraIconsBySlot[i];
		if (extra->getSlot() == slot)
			return extra;
	}
	return NULL;
}

BaseExtraIcon* GetExtraIconByName(const char *name)
{
	for (unsigned int i = 0; i < registeredExtraIcons.size(); i++) {
		BaseExtraIcon *extra = registeredExtraIcons[i];
		if (strcmp(name, extra->getName()) == 0)
			return extra;
	}
	return NULL;
}

static void LoadGroups(vector<ExtraIconGroup *> &groups)
{
	unsigned int count = db_get_w(NULL, MODULE_NAME "Groups", "Count", 0);
	for (unsigned int i = 0; i < count; i++) {
		char setting[512];
		mir_snprintf(setting, SIZEOF(setting), "%d_count", i);
		unsigned int items = db_get_w(NULL, MODULE_NAME "Groups", setting, 0);
		if (items < 1)
			continue;

		mir_snprintf(setting, SIZEOF(setting), "__group_%d", i);
		ExtraIconGroup *group = new ExtraIconGroup(setting);

		for (unsigned int j = 0; j < items; j++) {
			mir_snprintf(setting, SIZEOF(setting), "%d_%d", i, j);

			DBVARIANT dbv = { 0 };
			if (!DBGetContactSettingString(NULL, MODULE_NAME "Groups", setting, &dbv)) {
				if (!IsEmpty(dbv.pszVal)) {
					BaseExtraIcon *extra = GetExtraIconByName(dbv.pszVal);
					if (extra != NULL) {
						group->items.push_back(extra);

						if (extra->getSlot() >= 0)
							group->setSlot(extra->getSlot());
					}
				}
				db_free(&dbv);
			}
		}

		if (group->items.size() < 2) {
			delete group;
			continue;
		}

		groups.push_back(group);
	}
}

static ExtraIconGroup * IsInGroup(vector<ExtraIconGroup *> &groups, BaseExtraIcon *extra)
{
	for (unsigned int i = 0; i < groups.size(); i++) {
		ExtraIconGroup *group = groups[i];
		for (unsigned int j = 0; j < group->items.size(); j++) {
			if (extra == group->items[j])
				return group;
		}
	}
	return NULL;
}

struct compareFunc : std::binary_function<const ExtraIcon *, const ExtraIcon *, bool>
{
	bool operator()(const ExtraIcon * one, const ExtraIcon * two) const
	{
		return *one < *two;
	}
};

void RebuildListsBasedOnGroups(vector<ExtraIconGroup *> &groups)
{
	unsigned int i;
	for (i = 0; i < extraIconsByHandle.size(); i++)
		extraIconsByHandle[i] = registeredExtraIcons[i];

	for (i = 0; i < extraIconsBySlot.size(); i++) {
		ExtraIcon *extra = extraIconsBySlot[i];
		if (extra->getType() != EXTRAICON_TYPE_GROUP)
			continue;

		delete extra;
	}
	extraIconsBySlot.clear();

	for (i = 0; i < groups.size(); i++) {
		ExtraIconGroup *group = groups[i];

		for (unsigned int j = 0; j < group->items.size(); j++)
			extraIconsByHandle[group->items[j]->getID() - 1] = group;

		extraIconsBySlot.push_back(group);
	}

	for (i = 0; i < extraIconsByHandle.size(); i++) {
		ExtraIcon *extra = extraIconsByHandle[i];
		if (extra->getType() != EXTRAICON_TYPE_GROUP)
			extraIconsBySlot.push_back(extra);
	}

	std::sort(extraIconsBySlot.begin(), extraIconsBySlot.end(), compareFunc());
}

///////////////////////////////////////////////////////////////////////////////

int ClistExtraListRebuild(WPARAM wParam, LPARAM lParam)
{
	clistRebuildAlreadyCalled = TRUE;

	ResetIcons();

	for (unsigned int i = 0; i < extraIconsBySlot.size(); i++)
		extraIconsBySlot[i]->rebuildIcons();

	return 0;
}

int ClistExtraImageApply(WPARAM wParam, LPARAM lParam)
{
	HANDLE hContact = (HANDLE)wParam;
	if (hContact == NULL)
		return 0;

	clistApplyAlreadyCalled = TRUE;

	for (unsigned int i = 0; i < extraIconsBySlot.size(); i++)
		extraIconsBySlot[i]->applyIcon(hContact);

	return 0;
}

int ClistExtraClick(WPARAM wParam, LPARAM lParam)
{
	HANDLE hContact = (HANDLE)wParam;
	if (hContact == NULL)
		return 0;

	int clistSlot = (int)lParam;

	for (unsigned int i = 0; i < extraIconsBySlot.size(); i++) {
		ExtraIcon *extra = extraIconsBySlot[i];
		if (ConvertToClistSlot(extra->getSlot()) == clistSlot) {
			extra->onClick(hContact);
			break;
		}
	}

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Extra image list functions

HANDLE hEventExtraImageListRebuilding, hEventExtraImageApplying, hEventExtraClick;

static bool bImageCreated = false;
static int g_mutex_bSetAllExtraIconsCycle = 0;
static HIMAGELIST hExtraImageList;

HANDLE ExtraIcon_Add(HICON hIcon)
{
	if (hExtraImageList == 0 || hIcon == 0)
		return INVALID_HANDLE_VALUE;

	int res = ImageList_AddIcon(hExtraImageList, hIcon);
	return (res > 0xFFFE) ? INVALID_HANDLE_VALUE : (HANDLE)res;
}

void fnReloadExtraIcons()
{
	SendMessage(cli.hwndContactTree, CLM_SETEXTRASPACE, db_get_b(NULL,"CLUI","ExtraColumnSpace",18), 0);
	SendMessage(cli.hwndContactTree, CLM_SETEXTRAIMAGELIST, 0, 0);

	if (hExtraImageList)
		ImageList_Destroy(hExtraImageList);

	hExtraImageList = ImageList_Create(GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),ILC_COLOR32|ILC_MASK,1,256);

	SendMessage(cli.hwndContactTree, CLM_SETEXTRAIMAGELIST, 0, (LPARAM)hExtraImageList);
	SendMessage(cli.hwndContactTree, CLM_SETEXTRACOLUMNS, EXTRA_ICON_COUNT, 0);
	NotifyEventHooks(hEventExtraImageListRebuilding,0,0);
	bImageCreated = true;
}

void fnSetAllExtraIcons(HWND hwndList, HANDLE hContact)
{
	if (cli.hwndContactTree == 0)
		return;

	g_mutex_bSetAllExtraIconsCycle = 1;
	bool hcontgiven = (hContact != 0);

	if (!bImageCreated)
		cli.pfnReloadExtraIcons();

	SendMessage(cli.hwndContactTree, CLM_SETEXTRACOLUMNS, EXTRA_ICON_COUNT, 0);

	if (hContact == NULL)
		hContact = db_find_first();

	do {
		HANDLE hItem = hContact;
		if (hItem == 0)
			continue;

		ClcCacheEntry* pdnce = (ClcCacheEntry*)cli.pfnGetCacheEntry(hItem);
		if (pdnce == NULL)
			continue;

		NotifyEventHooks(hEventExtraImageApplying, (WPARAM)hContact, 0);
		if (hcontgiven) break;
		Sleep(0);
	}
		while(hContact = db_find_next(hContact));

	g_mutex_bSetAllExtraIconsCycle = 0;
	cli.pfnInvalidateRect(hwndList, NULL, FALSE);
	Sleep(0);
}

///////////////////////////////////////////////////////////////////////////////
// Services

INT_PTR ExtraIcon_Register(WPARAM wParam, LPARAM lParam)
{
	if (wParam == 0)
		return 0;

	EXTRAICON_INFO *ei = (EXTRAICON_INFO *) wParam;
	if (ei->cbSize < (int)sizeof(EXTRAICON_INFO))
		return 0;
	if (ei->type != EXTRAICON_TYPE_CALLBACK && ei->type != EXTRAICON_TYPE_ICOLIB)
		return 0;
	if ( IsEmpty(ei->name) || IsEmpty(ei->description))
		return 0;
	if (ei->type == EXTRAICON_TYPE_CALLBACK && (ei->ApplyIcon == NULL || ei->RebuildIcons == NULL))
		return 0;

	TCHAR *desc = Langpack_PcharToTchar(ei->description);

	BaseExtraIcon *extra = GetExtraIconByName(ei->name);
	if (extra != NULL) {
		if (ei->type != extra->getType() || ei->type != EXTRAICON_TYPE_ICOLIB)
			return 0;

		// Found one, now merge it
		if ( _tcsicmp(extra->getDescription(), desc)) {
			tstring newDesc = extra->getDescription();
			newDesc += _T(" / ");
			newDesc += desc;
			extra->setDescription(newDesc.c_str());
		}

		if (!IsEmpty(ei->descIcon))
			extra->setDescIcon(ei->descIcon);

		if (ei->OnClick != NULL)
			extra->setOnClick(ei->OnClick, ei->onClickParam);

		if (extra->getSlot() > 0) {
			if (clistRebuildAlreadyCalled)
				extra->rebuildIcons();
			if (clistApplyAlreadyCalled)
				extraIconsByHandle[extra->getID() - 1]->applyIcons();
		}

		return extra->getID();
	}

	int id = (int)registeredExtraIcons.size() + 1;

	switch (ei->type) {
	case EXTRAICON_TYPE_CALLBACK:
		extra = new CallbackExtraIcon(id, ei->name, desc, ei->descIcon == NULL ? "" : ei->descIcon,
			ei->RebuildIcons, ei->ApplyIcon, ei->OnClick, ei->onClickParam);
		break;
	case EXTRAICON_TYPE_ICOLIB:
		extra = new IcolibExtraIcon(id, ei->name, desc, ei->descIcon == NULL ? "" : ei->descIcon, ei->OnClick,
			ei->onClickParam);
		break;
	default:
		return 0;
	}

	char setting[512];
	mir_snprintf(setting, SIZEOF(setting), "Position_%s", ei->name);
	extra->setPosition(db_get_w(NULL, MODULE_NAME, setting, 1000));

	mir_snprintf(setting, SIZEOF(setting), "Slot_%s", ei->name);
	int slot = db_get_w(NULL, MODULE_NAME, setting, 1);
	if (slot == (WORD) -1)
		slot = -1;
	extra->setSlot(slot);

	registeredExtraIcons.push_back(extra);
	extraIconsByHandle.push_back(extra);

	vector<ExtraIconGroup *> groups;
	LoadGroups(groups);

	ExtraIconGroup *group = IsInGroup(groups, extra);
	if (group != NULL)
		RebuildListsBasedOnGroups(groups);
	else {
		for (unsigned int i = 0; i < groups.size(); i++)
			delete groups[i];

		extraIconsBySlot.push_back(extra);
		std::sort(extraIconsBySlot.begin(), extraIconsBySlot.end(), compareFunc());
	}

	if (slot >= 0 || group != NULL) {
		if (clistRebuildAlreadyCalled)
			extra->rebuildIcons();

		slot = 0;
		for (unsigned int i = 0; i < extraIconsBySlot.size(); i++) {
			ExtraIcon *ex = extraIconsBySlot[i];
			if (ex->getSlot() < 0)
				continue;

			int oldSlot = ex->getSlot();
			ex->setSlot(slot++);

			if (clistApplyAlreadyCalled && (ex == group || ex == extra || oldSlot != slot))
				extra->applyIcons();
		}
	}

	return id;
}

INT_PTR ExtraIcon_SetIcon(WPARAM wParam, LPARAM lParam)
{
	if (wParam == 0)
		return -1;

	EXTRAICON *ei = (EXTRAICON*)wParam;
	if (ei->cbSize < (int)sizeof(EXTRAICON) || ei->hExtraIcon == NULL || ei->hContact == NULL)
		return -1;

	ExtraIcon *extra = GetExtraIcon(ei->hExtraIcon);
	if (extra == NULL)
		return -1;

	return extra->setIcon((int)ei->hExtraIcon, ei->hContact, ei->hImage);
}

INT_PTR ExtraIcon_SetIconByName(WPARAM wParam, LPARAM lParam)
{
	if (wParam == 0)
		return -1;

	EXTRAICON *ei = (EXTRAICON*)wParam;
	if (ei->cbSize < (int)sizeof(EXTRAICON) || ei->hExtraIcon == NULL || ei->hContact == NULL)
		return -1;

	ExtraIcon *extra = GetExtraIcon(ei->hExtraIcon);
	if (extra == NULL)
		return -1;

	return extra->setIconByName((int)ei->hExtraIcon, ei->hContact, ei->icoName);
}

static INT_PTR svcExtraIcon_Add(WPARAM wParam, LPARAM lParam)
{
	return (INT_PTR)ExtraIcon_Add((HICON)wParam);
}

///////////////////////////////////////////////////////////////////////////////

void LoadExtraIconsModule()
{
	DWORD ret = CallService(MS_CLUI_GETCAPS, CLUICAPS_FLAGS2, 0);
	clistFirstSlot = HIWORD(ret);
	clistSlotCount = LOWORD(ret);

	// Services
	CreateServiceFunction(MS_EXTRAICON_REGISTER, &ExtraIcon_Register);
	CreateServiceFunction(MS_EXTRAICON_SET_ICON, &ExtraIcon_SetIcon);
	CreateServiceFunction(MS_EXTRAICON_SET_ICON_BY_NAME, &ExtraIcon_SetIconByName);

	CreateServiceFunction(MS_CLIST_EXTRA_ADD_ICON, &svcExtraIcon_Add);

	hEventExtraClick = CreateHookableEvent(ME_CLIST_EXTRA_CLICK);
	hEventExtraImageApplying = CreateHookableEvent(ME_CLIST_EXTRA_IMAGE_APPLY);
	hEventExtraImageListRebuilding = CreateHookableEvent(ME_CLIST_EXTRA_LIST_REBUILD);

	// Icons
	TCHAR tszFile[MAX_PATH];
	GetModuleFileName(NULL, tszFile, MAX_PATH);

	SKINICONDESC sid = {0};
	sid.cbSize = sizeof(SKINICONDESC);
	sid.flags = SIDF_PATH_TCHAR;
	sid.ptszDefaultFile = tszFile;
	sid.pszSection = "Contact List";

	sid.pszName = "ChatActivity";
	sid.pszDescription = LPGEN("Chat Activity");
	sid.iDefaultIndex = -IDI_CHAT;
	Skin_AddIcon(&sid);

	sid.pszName = "gender_male";
	sid.pszDescription = LPGEN("Male");
	sid.iDefaultIndex = -IDI_MALE;
	Skin_AddIcon(&sid);

	sid.pszName = "gender_female";
	sid.pszDescription = LPGEN("Female");
	sid.iDefaultIndex = -IDI_FEMALE;
	Skin_AddIcon(&sid);

	// Hooks
	HookEvent(ME_SYSTEM_MODULESLOADED, &ModulesLoaded);
	HookEvent(ME_SYSTEM_PRESHUTDOWN, &PreShutdown);

	HookEvent(ME_CLIST_EXTRA_LIST_REBUILD, &ClistExtraListRebuild);
	HookEvent(ME_CLIST_EXTRA_IMAGE_APPLY, &ClistExtraImageApply);
	HookEvent(ME_CLIST_EXTRA_CLICK, &ClistExtraClick);

	DefaultExtraIcons_Load();
}

void UnloadExtraIconsModule(void)
{
	for (size_t i=0; i < registeredExtraIcons.size(); i++)
		delete registeredExtraIcons[i];
}
