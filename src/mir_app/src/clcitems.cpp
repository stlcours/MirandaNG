/*

Miranda NG: the free IM client for Microsoft* Windows*

Copyright (�) 2012-16 Miranda NG project (http://miranda-ng.org),
Copyright (c) 2000-12 Miranda IM project,
all portions of this codebase are copyrighted to the people
listed in contributors.txt.

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
*/

#include "stdafx.h"
#include "clc.h"

// routines for managing adding/removal of items in the list, including sorting

int fnAddItemToGroup(ClcGroup *group, int iAboveItem)
{
	ClcContact* newItem = cli.pfnCreateClcContact();
	newItem->type = CLCIT_DIVIDER;
	newItem->flags = 0;
	newItem->szText[0] = '\0';
	memset(newItem->iExtraImage, 0xFF, sizeof(newItem->iExtraImage));
	group->cl.insert(newItem, iAboveItem);
	return iAboveItem;
}

ClcGroup* fnAddGroup(HWND hwnd, ClcData *dat, const TCHAR *szName, DWORD flags, int groupId, int calcTotalMembers)
{
	dat->bNeedsResort = true;
	if (!(GetWindowLongPtr(hwnd, GWL_STYLE) & CLS_USEGROUPS))
		return &dat->list;

	ClcGroup *group = &dat->list;
	TCHAR *pNextField = NEWTSTR_ALLOCA(szName);
	do {
		TCHAR *pBackslash = _tcschr(pNextField, '\\'), *pThisField = pNextField;
		if (pBackslash == NULL) {
			pNextField = NULL;
		}
		else {
			*pBackslash = 0;
			pNextField = pBackslash + 1;
		}
		
		int i, compareResult = 1;
		for (i = 0; i < group->cl.getCount(); i++) {
			ClcContact *cc = group->cl[i];
			if (cc->type == CLCIT_CONTACT)
				break;
			if (cc->type != CLCIT_GROUP)
				continue;
			compareResult = mir_tstrcmp(pThisField, cc->szText);
			if (compareResult == 0) {
				if (pNextField == NULL && flags != (DWORD)-1) {
					cc->groupId = (WORD)groupId;
					group = cc->group;
					group->expanded = (flags & GROUPF_EXPANDED) != 0;
					group->hideOffline = (flags & GROUPF_HIDEOFFLINE) != 0;
					group->groupId = groupId;
				}
				else group = cc->group;
				break;
			}
			if (pNextField == NULL && cc->groupId == 0)
				break;
			if (!(dat->exStyle & CLS_EX_SORTGROUPSALPHA) && groupId && cc->groupId > groupId)
				break;
		}

		if (compareResult) { // not found
			if (groupId == 0)
				return NULL;

			ClcContact *cc = group->cl[cli.pfnAddItemToGroup(group, i)];
			cc->type = CLCIT_GROUP;
			mir_tstrncpy(cc->szText, pThisField, _countof(cc->szText));
			cc->groupId = (WORD)(pNextField ? 0 : groupId);
			cc->group = new ClcGroup(10);
			cc->group->parent = group;
			group = cc->group;

			if (flags == (DWORD)-1 || pNextField != NULL) {
				group->expanded = 0;
				group->hideOffline = 0;
			}
			else {
				group->expanded = (flags & GROUPF_EXPANDED) != 0;
				group->hideOffline = (flags & GROUPF_HIDEOFFLINE) != 0;
			}
			group->groupId = pNextField ? 0 : groupId;
			group->totalMembers = 0;
			if (flags != (DWORD)-1 && pNextField == NULL && calcTotalMembers) {
				DWORD style = GetWindowLongPtr(hwnd, GWL_STYLE);
				for (MCONTACT hContact = db_find_first(); hContact; hContact = db_find_next(hContact)) {
					ClcCacheEntry *cache = cli.pfnGetCacheEntry(hContact);
					if (!mir_tstrcmp(cache->tszGroup, szName) && (style & CLS_SHOWHIDDEN || !cache->bIsHidden))
						group->totalMembers++;
				}
			}
		}
	} while (pNextField);
	return group;
}

void fnFreeContact(ClcContact* p)
{
	if (p->type == CLCIT_GROUP) {
		cli.pfnFreeGroup(p->group);
		delete p->group; p->group = NULL;
	}
}

void fnFreeGroup(ClcGroup *group)
{
	if (!group)
		return;

	for (int i = 0; i < group->cl.getCount(); i++) {
		cli.pfnFreeContact(group->cl[i]);
		mir_free(group->cl[i]);
	}
	group->cl.destroy();
}

static int iInfoItemUniqueHandle = 0;
int fnAddInfoItemToGroup(ClcGroup *group, int flags, const TCHAR *pszText)
{
	int i = 0;

	if (flags & CLCIIF_BELOWCONTACTS)
		i = group->cl.getCount();
	else if (flags & CLCIIF_BELOWGROUPS) {
		for (; i < group->cl.getCount(); i++)
			if (group->cl[i]->type == CLCIT_CONTACT)
				break;
	}
	else
		for (; i < group->cl.getCount(); i++)
			if (group->cl[i]->type != CLCIT_INFO)
				break;
	i = cli.pfnAddItemToGroup(group, i);
	iInfoItemUniqueHandle = LOWORD(iInfoItemUniqueHandle + 1);
	if (iInfoItemUniqueHandle == 0)
		++iInfoItemUniqueHandle;
	group->cl[i]->type = CLCIT_INFO;
	group->cl[i]->flags = (BYTE)flags;
	group->cl[i]->hContact = (MCONTACT)++iInfoItemUniqueHandle;
	mir_tstrncpy(group->cl[i]->szText, pszText, _countof(group->cl[i]->szText));
	return i;
}

int fnAddContactToGroup(ClcData *dat, ClcGroup *group, MCONTACT hContact)
{
	int i, index = -1;

	dat->bNeedsResort = true;
	for (i = group->cl.getCount() - 1; i >= 0; i--) {
		if (group->cl[i]->hContact == hContact)
			return i;

		if (index == -1)
			if (group->cl[i]->type != CLCIT_INFO || !(group->cl[i]->flags & CLCIIF_BELOWCONTACTS))
				index = i;
	}

	char *szProto = GetContactProto(hContact);

	ClcCacheEntry *pce = cli.pfnGetCacheEntry(hContact);
	replaceStrT(pce->tszGroup, NULL);

	i = cli.pfnAddItemToGroup(group, index + 1);

	ClcContact *cc = group->cl[i];
	cc->type = CLCIT_CONTACT;
	cc->iImage = cli.pfnGetContactIcon(hContact);
	cc->hContact = hContact;
	cc->proto = szProto;
	cc->pce = pce;
	if (szProto != NULL && !cli.pfnIsHiddenMode(dat, db_get_w(hContact, szProto, "Status", ID_STATUS_OFFLINE)))
		cc->flags |= CONTACTF_ONLINE;
	WORD apparentMode = szProto != NULL ? db_get_w(hContact, szProto, "ApparentMode", 0) : 0;
	if (apparentMode == ID_STATUS_OFFLINE)
		cc->flags |= CONTACTF_INVISTO;
	else if (apparentMode == ID_STATUS_ONLINE)
		cc->flags |= CONTACTF_VISTO;
	else if (apparentMode)
		cc->flags |= CONTACTF_VISTO | CONTACTF_INVISTO;
	if (db_get_b(hContact, "CList", "NotOnList", 0))
		cc->flags |= CONTACTF_NOTONLIST;
	DWORD idleMode = szProto != NULL ? db_get_dw(hContact, szProto, "IdleTS", 0) : 0;
	if (idleMode)
		cc->flags |= CONTACTF_IDLE;
	mir_tstrncpy(cc->szText, cli.pfnGetContactDisplayName(hContact, 0), _countof(cc->szText));
	return i;
}

void fnAddContactToTree(HWND hwnd, ClcData *dat, MCONTACT hContact, int updateTotalCount, int checkHideOffline)
{
	DWORD style = GetWindowLongPtr(hwnd, GWL_STYLE);
	WORD status = ID_STATUS_OFFLINE;
	char *szProto = GetContactProto(hContact);

	dat->bNeedsResort = true;
	if (style & CLS_NOHIDEOFFLINE)
		checkHideOffline = 0;
	if (checkHideOffline)
		if (szProto != NULL)
			status = db_get_w(hContact, szProto, "Status", ID_STATUS_OFFLINE);

	int i;
	DWORD groupFlags;
	ClcGroup *group;
	ptrT tszGroup(db_get_tsa(hContact, "CList", "Group"));
	if (tszGroup == NULL)
		group = &dat->list;
	else {
		group = cli.pfnAddGroup(hwnd, dat, tszGroup, (DWORD)-1, 0, 0);
		if (group == NULL) {
			if (!(style & CLS_HIDEEMPTYGROUPS))
				return;

			if (checkHideOffline && cli.pfnIsHiddenMode(dat, status)) {
				for (i = 1;; i++) {
					TCHAR *szGroupName = Clist_GroupGetName(i, &groupFlags);
					if (szGroupName == NULL)
						return;

					if (!mir_tstrcmp(szGroupName, tszGroup))
						break;
				}
				if (groupFlags & GROUPF_HIDEOFFLINE)
					return;
			}
			for (i = 1;; i++) {
				TCHAR *szGroupName = Clist_GroupGetName(i, &groupFlags);
				if (szGroupName == NULL)
					return;

				if (!mir_tstrcmp(szGroupName, tszGroup))
					break;

				size_t len = mir_tstrlen(szGroupName);
				if (!_tcsncmp(szGroupName, tszGroup, len) && tszGroup[len] == '\\')
					cli.pfnAddGroup(hwnd, dat, szGroupName, groupFlags, i, 1);
			}
			group = cli.pfnAddGroup(hwnd, dat, tszGroup, groupFlags, i, 1);
		}
	}

	if (checkHideOffline) {
		if (cli.pfnIsHiddenMode(dat, status) && (style & CLS_HIDEOFFLINE || group->hideOffline)) {
			if (updateTotalCount)
				group->totalMembers++;
			return;
		}
	}
	cli.pfnAddContactToGroup(dat, group, hContact);
	if (updateTotalCount)
		group->totalMembers++;
}

ClcGroup* fnRemoveItemFromGroup(HWND hwnd, ClcGroup *group, ClcContact *contact, int updateTotalCount)
{
	int iContact = group->cl.indexOf(contact);
	if (iContact == -1)
		return group;

	if (contact->type == CLCIT_CONTACT) {
		if (updateTotalCount)
			group->totalMembers--;

		cli.pfnInvalidateDisplayNameCacheEntry(contact->hContact);
	}

	cli.pfnFreeContact(group->cl[iContact]);
	mir_free(group->cl[iContact]);
	group->cl.remove(iContact);

	if ((GetWindowLongPtr(hwnd, GWL_STYLE) & CLS_HIDEEMPTYGROUPS) && group->cl.getCount() == 0 && group->parent != NULL)
		for (int i = 0; i < group->parent->cl.getCount(); i++)
			if (group->parent->cl[i]->type == CLCIT_GROUP && group->parent->cl[i]->groupId == group->groupId)
				return cli.pfnRemoveItemFromGroup(hwnd, group->parent, group->parent->cl[i], 0);

	return group;
}

void fnDeleteItemFromTree(HWND hwnd, MCONTACT hItem)
{
	ClcData *dat = (ClcData*)GetWindowLongPtr(hwnd, 0);
	dat->bNeedsResort = true;

	ClcGroup *group;
	ClcContact *contact;
	if (!cli.pfnFindItem(hwnd, dat, hItem, &contact, &group, NULL)) {
		DBVARIANT dbv;
		int i, nameOffset;
		if (!IsHContactContact(hItem))
			return;
		if (db_get_ts(hItem, "CList", "Group", &dbv))
			return;

		//decrease member counts of all parent groups too
		group = &dat->list;
		nameOffset = 0;
		for (i = 0;; i++) {
			if (group->scanIndex == group->cl.getCount())
				break;

			ClcContact *cc = group->cl[i];
			if (cc->type == CLCIT_GROUP) {
				size_t len = mir_tstrlen(cc->szText);
				if (!_tcsncmp(cc->szText, dbv.ptszVal + nameOffset, len) &&
					(dbv.ptszVal[nameOffset + len] == '\\' || dbv.ptszVal[nameOffset + len] == '\0')) {
					group->totalMembers--;
					if (dbv.ptszVal[nameOffset + len] == '\0')
						break;
				}
			}
		}
		mir_free(dbv.ptszVal);
	}
	else cli.pfnRemoveItemFromGroup(hwnd, group, contact, 1);
}

void fnRebuildEntireList(HWND hwnd, ClcData *dat)
{
	DWORD style = GetWindowLongPtr(hwnd, GWL_STYLE);

	dat->list.expanded = 1;
	dat->list.hideOffline = db_get_b(NULL, "CLC", "HideOfflineRoot", 0) && (style & CLS_USEGROUPS);
	dat->list.cl.destroy();
	dat->list.totalMembers = 0;
	dat->selection = -1;

	for (int i = 1;; i++) {
		DWORD groupFlags;
		TCHAR *szGroupName = Clist_GroupGetName(i, &groupFlags);
		if (szGroupName == NULL)
			break;
		cli.pfnAddGroup(hwnd, dat, szGroupName, groupFlags, i, 0);
	}

	for (MCONTACT hContact = db_find_first(); hContact; hContact = db_find_next(hContact)) {
		if (style & CLS_SHOWHIDDEN || !db_get_b(hContact, "CList", "Hidden", 0)) {
			ClcGroup *group;
			ptrT tszGroupName(db_get_tsa(hContact, "CList", "Group"));
			if (tszGroupName == NULL)
				group = &dat->list;
			else {
				group = cli.pfnAddGroup(hwnd, dat, tszGroupName, (DWORD)-1, 0, 0);
				if (group == NULL && style & CLS_SHOWHIDDEN)
					group = &dat->list;
			}

			if (group != NULL) {
				group->totalMembers++;

				if (dat->bFilterSearch && dat->szQuickSearch[0] != '\0') {
					TCHAR *name = cli.pfnGetContactDisplayName(hContact, 0);
					TCHAR *lowered_name = CharLowerW(NEWTSTR_ALLOCA(name));
					TCHAR *lowered_search = CharLowerW(NEWTSTR_ALLOCA(dat->szQuickSearch));

					if (_tcsstr(lowered_name, lowered_search))
						cli.pfnAddContactToGroup(dat, group, hContact);
				}
				else if (!(style & CLS_NOHIDEOFFLINE) && (style & CLS_HIDEOFFLINE || group->hideOffline)) {
					char *szProto = GetContactProto(hContact);
					if (szProto == NULL) {
						if (!cli.pfnIsHiddenMode(dat, ID_STATUS_OFFLINE))
							cli.pfnAddContactToGroup(dat, group, hContact);
					}
					else if (!cli.pfnIsHiddenMode(dat, db_get_w(hContact, szProto, "Status", ID_STATUS_OFFLINE)))
						cli.pfnAddContactToGroup(dat, group, hContact);
				}
				else cli.pfnAddContactToGroup(dat, group, hContact);
			}
		}
	}

	if (style & CLS_HIDEEMPTYGROUPS) {
		ClcGroup *group = &dat->list;
		group->scanIndex = 0;
		for (;;) {
			if (group->scanIndex == group->cl.getCount()) {
				if ((group = group->parent) == NULL)
					break;
				group->scanIndex++;
				continue;
			}

			ClcContact *cc = group->cl[group->scanIndex];
			if (cc->type == CLCIT_GROUP) {
				if (cc->group->cl.getCount() == 0) {
					group = cli.pfnRemoveItemFromGroup(hwnd, group, cc, 0);
				}
				else {
					group = cc->group;
					group->scanIndex = 0;
				}
				continue;
			}
			group->scanIndex++;
		}
	}

	cli.pfnSortCLC(hwnd, dat, 0);
	cli.pfnSetAllExtraIcons(0);
}

int fnGetGroupContentsCount(ClcGroup *group, int visibleOnly)
{
	int count = group->cl.getCount();
	ClcGroup *topgroup = group;

	group->scanIndex = 0;
	for (;;) {
		if (group->scanIndex == group->cl.getCount()) {
			if (group == topgroup)
				break;
			group = group->parent;
			group->scanIndex++;
			continue;
		}

		ClcContact *cc = group->cl[group->scanIndex];
		if (cc->type == CLCIT_GROUP && (!visibleOnly || cc->group->expanded)) {
			group = cc->group;
			group->scanIndex = 0;
			count += group->cl.getCount();
			continue;
		}
		group->scanIndex++;
	}
	return count;
}

static int __cdecl GroupSortProc(const void* p1, const void* p2)
{
	ClcContact **contact1 = (ClcContact**)p1, **contact2 = (ClcContact**)p2;

	return mir_tstrcmpi(contact1[0]->szText, contact2[0]->szText);
}

static int __cdecl ContactSortProc(const void* p1, const void* p2)
{
	ClcContact **contact1 = (ClcContact**)p1, **contact2 = (ClcContact**)p2;

	int result = cli.pfnCompareContacts(contact1[0], contact2[0]);
	if (result)
		return result;
	//nothing to distinguish them, so make sure they stay in the same order
	return (int)((INT_PTR)contact2[0]->hContact - (INT_PTR)contact1[0]->hContact);
}

static void InsertionSort(ClcContact **pContactArray, int nArray, int(*CompareProc) (const void *, const void *))
{
	int i, j;
	ClcContact* testElement;

	for (i = 1; i < nArray; i++) {
		if (CompareProc(&pContactArray[i - 1], &pContactArray[i]) > 0) {
			testElement = pContactArray[i];
			for (j = i - 2; j >= 0; j--)
				if (CompareProc(&pContactArray[j], &testElement) <= 0)
					break;
			j++;
			memmove(&pContactArray[j + 1], &pContactArray[j], sizeof(void*) * (i - j));
			pContactArray[j] = testElement;
		}
	}
}

static void SortGroup(ClcData *dat, ClcGroup *group, int useInsertionSort)
{
	int i, sortCount;

	for (i = group->cl.getCount() - 1; i >= 0; i--) {
		if (group->cl[i]->type == CLCIT_DIVIDER) {
			mir_free(group->cl[i]);
			group->cl.remove(i);
		}
	}

	for (i = 0; i < group->cl.getCount(); i++)
		if (group->cl[i]->type != CLCIT_INFO)
			break;
	if (i > group->cl.getCount() - 2)
		return;
	if (group->cl[i]->type == CLCIT_GROUP) {
		if (dat->exStyle & CLS_EX_SORTGROUPSALPHA) {
			for (sortCount = 0; i + sortCount < group->cl.getCount(); sortCount++)
				if (group->cl[i + sortCount]->type != CLCIT_GROUP)
					break;
			qsort(group->cl.getArray() + i, sortCount, sizeof(void*), GroupSortProc);
			i = i + sortCount;
		}
		for (; i < group->cl.getCount(); i++)
			if (group->cl[i]->type == CLCIT_CONTACT)
				break;
		if (group->cl.getCount() - i < 2)
			return;
	}
	for (sortCount = 0; i + sortCount < group->cl.getCount(); sortCount++)
		if (group->cl[i + sortCount]->type != CLCIT_CONTACT)
			break;
	if (useInsertionSort)
		InsertionSort(group->cl.getArray() + i, sortCount, ContactSortProc);
	else
		qsort(group->cl.getArray() + i, sortCount, sizeof(void*), ContactSortProc);
	if (dat->exStyle & CLS_EX_DIVIDERONOFF) {
		int prevContactOnline = 0;
		for (i = 0; i < group->cl.getCount(); i++) {
			if (group->cl[i]->type != CLCIT_CONTACT)
				continue;
			if (group->cl[i]->flags & CONTACTF_ONLINE)
				prevContactOnline = 1;
			else {
				if (prevContactOnline) {
					i = cli.pfnAddItemToGroup(group, i);
					group->cl[i]->type = CLCIT_DIVIDER;
					mir_tstrcpy(group->cl[i]->szText, TranslateT("Offline"));
				}
				break;
			}
		}
	}
}

void fnSortCLC(HWND hwnd, ClcData *dat, int useInsertionSort)
{
	ClcGroup *group = &dat->list;

	if (dat->bNeedsResort) {
		MCONTACT hSelItem;
		ClcContact *selcontact;
		if (cli.pfnGetRowByIndex(dat, dat->selection, &selcontact, NULL) == -1)
			hSelItem = NULL;
		else
			hSelItem = (UINT_PTR)cli.pfnContactToHItem(selcontact);
		group->scanIndex = 0;
		SortGroup(dat, group, useInsertionSort);
		for (;;) {
			if (group->scanIndex == group->cl.getCount()) {
				if ((group = group->parent) == NULL)
					break;
				group->scanIndex++;
				continue;
			}

			ClcContact *cc = group->cl[group->scanIndex];
			if (cc->type == CLCIT_GROUP) {
				group = cc->group;
				group->scanIndex = 0;
				SortGroup(dat, group, useInsertionSort);
				continue;
			}
			group->scanIndex++;
		}

		if (hSelItem) {
			ClcGroup *selgroup;
			if (cli.pfnFindItem(hwnd, dat, hSelItem, &selcontact, &selgroup, NULL))
				dat->selection = cli.pfnGetRowsPriorTo(&dat->list, selgroup, selgroup->cl.indexOf(selcontact));
		}

		cli.pfnRecalcScrollBar(hwnd, dat);
	}
	dat->bNeedsResort = false;
	cli.pfnInvalidateRect(hwnd, NULL, FALSE);
}

struct SavedContactState_t
{
	MCONTACT hContact;
	WORD iExtraImage[EXTRA_ICON_COUNT];
	int checked;
};

struct SavedGroupState_t
{
	int groupId, expanded;
};

struct SavedInfoState_t
{
	int parentId;
	ClcContact contact;
};

void fnSaveStateAndRebuildList(HWND hwnd, ClcData *dat)
{
	int i, j;
	ClcGroup *group;
	ClcContact *contact;

	cli.pfnHideInfoTip(hwnd, dat);
	KillTimer(hwnd, TIMERID_INFOTIP);
	KillTimer(hwnd, TIMERID_RENAME);
	cli.pfnEndRename(hwnd, dat, 1);

	dat->bLockScrollbar = true;

	OBJLIST<SavedContactState_t> saveContact(10, NumericKeySortT);
	OBJLIST<SavedGroupState_t> saveGroup(100, NumericKeySortT);
	OBJLIST<SavedInfoState_t> saveInfo(10, NumericKeySortT);

	dat->bNeedsResort = true;
	group = &dat->list;
	group->scanIndex = 0;
	for (;;) {
		if (group->scanIndex == group->cl.getCount()) {
			if ((group = group->parent) == NULL)
				break;
			group->scanIndex++;
			continue;
		}

		ClcContact *cc = group->cl[group->scanIndex];
		if (cc->type == CLCIT_GROUP) {
			group = cc->group;
			group->scanIndex = 0;

			SavedGroupState_t *p = new SavedGroupState_t;
			p->groupId = group->groupId;
			p->expanded = group->expanded;
			saveGroup.insert(p);
			continue;
		}
		else if (cc->type == CLCIT_CONTACT) {
			SavedContactState_t *p = new SavedContactState_t;
			p->hContact = cc->hContact;
			memcpy(p->iExtraImage, cc->iExtraImage, sizeof(p->iExtraImage));
			p->checked = cc->flags & CONTACTF_CHECKED;
			saveContact.insert(p);
		}
		else if (cc->type == CLCIT_INFO) {
			SavedInfoState_t *p = new SavedInfoState_t;
			p->parentId = (group->parent == NULL) ? -1 : group->groupId;
			p->contact = *cc;
			saveInfo.insert(p);
		}
		group->scanIndex++;
	}

	cli.pfnFreeGroup(&dat->list);
	cli.pfnRebuildEntireList(hwnd, dat);

	group = &dat->list;
	group->scanIndex = 0;
	for (;;) {
		if (group->scanIndex == group->cl.getCount()) {
			if ((group = group->parent) == NULL)
				break;
			group->scanIndex++;
			continue;
		}

		ClcContact *cc = group->cl[group->scanIndex];
		if (cc->type == CLCIT_GROUP) {
			group = cc->group;
			group->scanIndex = 0;

			SavedGroupState_t tmp, *p;
			tmp.groupId = group->groupId;
			if ((p = saveGroup.find(&tmp)) != NULL)
				group->expanded = p->expanded;
			continue;
		}
		else if (cc->type == CLCIT_CONTACT) {
			SavedContactState_t tmp, *p;
			tmp.hContact = cc->hContact;
			if ((p = saveContact.find(&tmp)) != NULL) {
				memcpy(cc->iExtraImage, p->iExtraImage, sizeof(p->iExtraImage));
				if (p->checked)
					cc->flags |= CONTACTF_CHECKED;
			}
		}

		group->scanIndex++;
	}

	for (i = 0; i < saveInfo.getCount(); i++) {
		if (saveInfo[i].parentId == -1)
			group = &dat->list;
		else {
			if (!cli.pfnFindItem(hwnd, dat, saveInfo[i].parentId | HCONTACT_ISGROUP, &contact, NULL, NULL))
				continue;
			group = contact->group;
		}
		j = cli.pfnAddInfoItemToGroup(group, saveInfo[i].contact.flags, _T(""));
		*group->cl[j] = saveInfo[i].contact;
	}

	dat->bLockScrollbar = false;
	cli.pfnRecalculateGroupCheckboxes(hwnd, dat);

	cli.pfnRecalcScrollBar(hwnd, dat);

	NMCLISTCONTROL nm;
	nm.hdr.code = CLN_LISTREBUILT;
	nm.hdr.hwndFrom = hwnd;
	nm.hdr.idFrom = GetDlgCtrlID(hwnd);
	SendMessage(GetParent(hwnd), WM_NOTIFY, 0, (LPARAM)& nm);
}
