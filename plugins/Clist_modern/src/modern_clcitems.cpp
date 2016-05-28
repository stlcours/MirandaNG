/*

Miranda NG: the free IM client for Microsoft* Windows*

Copyright (�) 2012-16 Miranda NG project (http://miranda-ng.org),
Copyright (c) 2000-08 Miranda ICQ/IM project,
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

void AddSubcontacts(ClcData *dat, ClcContact *cont, BOOL showOfflineHereGroup)
{
	cont->bSubExpanded = db_get_b(cont->hContact, "CList", "Expanded", 0) && db_get_b(NULL, "CLC", "MetaExpanding", SETTING_METAEXPANDING_DEFAULT);
	int subcount = db_mc_getSubCount(cont->hContact);
	if (subcount <= 0) {
		cont->iSubNumber = 0;
		cont->subcontacts = NULL;
		cont->iSubAllocated = 0;
		return;
	}

	cont->iSubNumber = 0;
	mir_free(cont->subcontacts);
	cont->subcontacts = (ClcContact *)mir_calloc(sizeof(ClcContact)*subcount);
	cont->iSubAllocated = subcount;
	int i = 0;
	int bHideOffline = db_get_b(NULL, "CList", "HideOffline", SETTING_HIDEOFFLINE_DEFAULT);
	for (int j = 0; j < subcount; j++) {
		MCONTACT hsub = db_mc_getSub(cont->hContact, j);
		ClcCacheEntry *cacheEntry = pcli->pfnGetCacheEntry(hsub);
		WORD wStatus = cacheEntry->getStatus();

		if (!showOfflineHereGroup && bHideOffline && !cacheEntry->m_bNoHiddenOffline && wStatus == ID_STATUS_OFFLINE)
			continue;

		ClcContact& p = cont->subcontacts[i];
		p.hContact = cacheEntry->hContact;
		p.pce = cacheEntry;

		p.avatar_pos = AVATAR_POS_DONT_HAVE;
		Cache_GetAvatar(dat, &p);

		p.iImage = corecli.pfnGetContactIcon(cacheEntry->hContact);
		memset(p.iExtraImage, 0xFF, sizeof(p.iExtraImage));
		p.proto = cacheEntry->m_pszProto;
		p.type = CLCIT_CONTACT;
		p.flags = 0;//CONTACTF_ONLINE;
		p.iSubNumber = i + 1;
		p.lastPaintCounter = 0;
		p.subcontacts = cont;
		p.bImageIsSpecial = false;
		//p.status = cacheEntry->status;
		Cache_GetTimezone(dat, (&p)->hContact);
		Cache_GetText(dat, &p);

		char *szProto = cacheEntry->m_pszProto;
		if (szProto != NULL && !pcli->pfnIsHiddenMode(dat, wStatus))
			p.flags |= CONTACTF_ONLINE;
		int apparentMode = szProto != NULL ? cacheEntry->ApparentMode : 0;
		if (apparentMode == ID_STATUS_OFFLINE)	p.flags |= CONTACTF_INVISTO;
		else if (apparentMode == ID_STATUS_ONLINE) p.flags |= CONTACTF_VISTO;
		else if (apparentMode) p.flags |= CONTACTF_VISTO | CONTACTF_INVISTO;
		if (cacheEntry->NotOnList) p.flags |= CONTACTF_NOTONLIST;
		int idleMode = szProto != NULL ? cacheEntry->IdleTS : 0;
		if (idleMode) p.flags |= CONTACTF_IDLE;
		i++;
	}

	cont->iSubAllocated = i;
	if (!i && cont->subcontacts != NULL)
		mir_free_and_nil(cont->subcontacts);
}

void cli_FreeContact(ClcContact *p)
{
	if (p->iSubAllocated) {
		if (p->subcontacts && !p->iSubNumber) {
			for (int i = 0; i < p->iSubAllocated; i++) {
				p->subcontacts[i].ssText.DestroySmileyList();
				if (p->subcontacts[i].avatar_pos == AVATAR_POS_ANIMATED)
					AniAva_RemoveAvatar(p->subcontacts[i].hContact);
				p->subcontacts[i].avatar_pos = AVATAR_POS_DONT_HAVE;
			}
			mir_free_and_nil(p->subcontacts);
		}
		p->iSubAllocated = 0;
	}

	p->ssText.DestroySmileyList();
	if (p->avatar_pos == AVATAR_POS_ANIMATED)
		AniAva_RemoveAvatar(p->hContact);
	p->avatar_pos = AVATAR_POS_DONT_HAVE;
	corecli.pfnFreeContact(p);
}

static void _LoadDataToContact(ClcContact *cont, ClcGroup *group, ClcData *dat, MCONTACT hContact)
{
	if (!cont)
		return;

	ClcCacheEntry *cacheEntry = pcli->pfnGetCacheEntry(hContact);
	char *szProto = cacheEntry->m_pszProto;

	cont->type = CLCIT_CONTACT;
	cont->pce = cacheEntry;
	cont->iSubAllocated = 0;
	cont->iSubNumber = 0;
	cont->subcontacts = NULL;
	cont->szText[0] = 0;
	cont->lastPaintCounter = 0;
	cont->bImageIsSpecial = false;
	cont->hContact = hContact;
	cont->proto = szProto;

	if (szProto != NULL && !pcli->pfnIsHiddenMode(dat, cacheEntry->m_iStatus))
		cont->flags |= CONTACTF_ONLINE;

	WORD apparentMode = szProto != NULL ? cacheEntry->ApparentMode : 0;
	if (apparentMode)
		switch (apparentMode) {
		case ID_STATUS_OFFLINE:
			cont->flags |= CONTACTF_INVISTO;
			break;
		case ID_STATUS_ONLINE:
			cont->flags |= CONTACTF_VISTO;
			break;
		default:
			cont->flags |= CONTACTF_VISTO | CONTACTF_INVISTO;
		}

	if (cacheEntry->NotOnList)
		cont->flags |= CONTACTF_NOTONLIST;

	DWORD idleMode = szProto != NULL ? cacheEntry->IdleTS : 0;
	if (idleMode)
		cont->flags |= CONTACTF_IDLE;

	// Add subcontacts
	if (szProto)
		if (dat->IsMetaContactsEnabled && !mir_strcmp(cont->proto, META_PROTO))
			AddSubcontacts(dat, cont, CLCItems_IsShowOfflineGroup(group));

	cont->lastPaintCounter = 0;
	cont->avatar_pos = AVATAR_POS_DONT_HAVE;
	Cache_GetAvatar(dat, cont);
	Cache_GetText(dat, cont);
	Cache_GetTimezone(dat, cont->hContact);
	cont->iImage = corecli.pfnGetContactIcon(hContact);
	cont->bContactRate = db_get_b(hContact, "CList", "Rate", 0);
}

static ClcContact* AddContactToGroup(ClcData *dat, ClcGroup *group, MCONTACT hContact)
{
	if (group == NULL || dat == NULL)
		return NULL;
	
	dat->bNeedsResort = true;
	int i;
	for (i = group->cl.getCount() - 1; i >= 0; i--)
		if (group->cl[i]->type != CLCIT_INFO || !(group->cl[i]->flags & CLCIIF_BELOWCONTACTS))
			break;

	i = pcli->pfnAddItemToGroup(group, i + 1);

	_LoadDataToContact(group->cl[i], group, dat, hContact);
	return group->cl[i];
}

void cli_AddContactToTree(HWND hwnd, ClcData *dat, MCONTACT hContact, int updateTotalCount, int checkHideOffline)
{
	ClcCacheEntry *cacheEntry = pcli->pfnGetCacheEntry(hContact);
	if (dat->IsMetaContactsEnabled && cacheEntry && cacheEntry->m_bIsSub)
		return;		//contact should not be added

	if (!dat->IsMetaContactsEnabled && cacheEntry && !mir_strcmp(cacheEntry->m_pszProto, META_PROTO))
		return;

	corecli.pfnAddContactToTree(hwnd, dat, hContact, updateTotalCount, checkHideOffline);

	ClcGroup *group;
	ClcContact *cont;
	if (FindItem(hwnd, dat, hContact, &cont, &group, NULL, false))
		_LoadDataToContact(cont, group, dat, hContact);
}

bool CLCItems_IsShowOfflineGroup(ClcGroup *group)
{
	if (!group) return false;
	if (group->hideOffline) return false;

	DWORD groupFlags = 0;
	Clist_GroupGetName(group->groupId, &groupFlags);
	return (groupFlags & GROUPF_SHOWOFFLINE) != 0;
}

MCONTACT SaveSelection(ClcData *dat)
{
	ClcContact *selcontact = NULL;
	if (pcli->pfnGetRowByIndex(dat, dat->selection, &selcontact, NULL) == -1)
		return NULL;

	return (DWORD_PTR)pcli->pfnContactToHItem(selcontact);
}

int RestoreSelection(ClcData *dat, MCONTACT hSelected)
{
	ClcGroup *selgroup = NULL;
	ClcContact *selcontact = NULL;
	if (!hSelected || !pcli->pfnFindItem(dat->hWnd, dat, hSelected, &selcontact, &selgroup, NULL)) {
		dat->selection = -1;
		return dat->selection;
	}

	if (!selcontact->iSubNumber)
		dat->selection = pcli->pfnGetRowsPriorTo(&dat->list, selgroup, selgroup->cl.indexOf(selcontact));
	else {
		dat->selection = pcli->pfnGetRowsPriorTo(&dat->list, selgroup, selgroup->cl.indexOf(selcontact->subcontacts));
		if (dat->selection != -1)
			dat->selection += selcontact->iSubNumber;
	}
	return dat->selection;
}

void cliRebuildEntireList(HWND hwnd, ClcData *dat)
{
	DWORD style = GetWindowLongPtr(hwnd, GWL_STYLE);
	ClcGroup *group = NULL;

	BOOL PlaceOfflineToRoot = db_get_b(NULL, "CList", "PlaceOfflineToRoot", SETTING_PLACEOFFLINETOROOT_DEFAULT);
	KillTimer(hwnd, TIMERID_REBUILDAFTER);
	pcli->bAutoRebuild = false;

	ImageArray_Clear(&dat->avatar_cache);
	RowHeights_Clear(dat);
	RowHeights_GetMaxRowHeight(dat, hwnd);

	dat->list.expanded = 1;
	dat->list.hideOffline = db_get_b(NULL, "CLC", "HideOfflineRoot", SETTING_HIDEOFFLINEATROOT_DEFAULT) && style & CLS_USEGROUPS;
	dat->list.cl.destroy();
	dat->bNeedsResort = true;

	MCONTACT hSelected = SaveSelection(dat);
	dat->selection = -1;
	dat->HiLightMode = db_get_b(NULL, "CLC", "HiLightMode", SETTING_HILIGHTMODE_DEFAULT);

	for (int i = 1;; i++) {
		DWORD groupFlags;
		TCHAR *szGroupName = Clist_GroupGetName(i, &groupFlags);
		if (szGroupName == NULL)
			break;
		pcli->pfnAddGroup(hwnd, dat, szGroupName, groupFlags, i, 0);
	}

	for (MCONTACT hContact = db_find_first(); hContact; hContact = db_find_next(hContact)) {
		ClcContact *cont = NULL;
		ClcCacheEntry *cacheEntry = pcli->pfnGetCacheEntry(hContact);

		int nHiddenStatus = CLVM_GetContactHiddenStatus(hContact, NULL, dat);
		if ((style & CLS_SHOWHIDDEN && nHiddenStatus != -1) || !nHiddenStatus) {
			if (mir_tstrlen(cacheEntry->tszGroup) == 0)
				group = &dat->list;
			else
				group = pcli->pfnAddGroup(hwnd, dat, cacheEntry->tszGroup, (DWORD)-1, 0, 0);

			if (group != NULL) {
				if (cacheEntry->m_iStatus == ID_STATUS_OFFLINE && PlaceOfflineToRoot)
					group = &dat->list;

				group->totalMembers++;

				if (!(style & CLS_NOHIDEOFFLINE) && (style & CLS_HIDEOFFLINE || group->hideOffline)) {
					if (cacheEntry->m_pszProto == NULL) {
						if (!pcli->pfnIsHiddenMode(dat, ID_STATUS_OFFLINE) || cacheEntry->m_bNoHiddenOffline || CLCItems_IsShowOfflineGroup(group))
							cont = AddContactToGroup(dat, group, hContact);
					}
					else if (!pcli->pfnIsHiddenMode(dat, cacheEntry->m_iStatus) || cacheEntry->m_bNoHiddenOffline || CLCItems_IsShowOfflineGroup(group))
						cont = AddContactToGroup(dat, group, hContact);
				}
				else cont = AddContactToGroup(dat, group, hContact);
			}
		}
		if (cont) {
			cont->iSubAllocated = 0;
			if (cont->proto && dat->IsMetaContactsEnabled  && mir_strcmp(cont->proto, META_PROTO) == 0)
				AddSubcontacts(dat, cont, CLCItems_IsShowOfflineGroup(group));
		}
	}

	if (style & CLS_HIDEEMPTYGROUPS) {
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
				if (cc->group->cl.getCount() == 0)
					group = pcli->pfnRemoveItemFromGroup(hwnd, group, cc, 0);
				else {
					group = cc->group;
					group->scanIndex = 0;
				}
				continue;
			}
			group->scanIndex++;
		}
	}

	pcli->pfnSortCLC(hwnd, dat, 0);

	RestoreSelection(dat, hSelected);
}

void cli_SortCLC(HWND hwnd, ClcData *dat, int useInsertionSort)
{
	MCONTACT hSelected = SaveSelection(dat);
	corecli.pfnSortCLC(hwnd, dat, useInsertionSort);
	RestoreSelection(dat, hSelected);
}

/////////////////////////////////////////////////////////////////////////////////////////

int GetNewSelection(ClcGroup *group, int selection, int direction)
{
	if (selection < 0)
		return 0;

	int lastcount = 0, count = 0;

	group->scanIndex = 0;
	for (;;) {
		if (group->scanIndex == group->cl.getCount()) {
			if ((group = group->parent) == NULL)
				break;
			group->scanIndex++;
			continue;
		}

		if (count >= selection)
			return count;

		lastcount = count;
		count++;
		if (!direction && count > selection)
			return lastcount;

		ClcContact *cc = group->cl[group->scanIndex];
		if (cc->type == CLCIT_GROUP && (cc->group->expanded)) {
			group = cc->group;
			group->scanIndex = 0;
			continue;
		}
		group->scanIndex++;
	}
	return lastcount;
}

/////////////////////////////////////////////////////////////////////////////////////////

ClcContact* cliCreateClcContact()
{
	ClcContact *contact = (ClcContact*)mir_calloc(sizeof(ClcContact));
	memset(contact->iExtraImage, 0xFF, sizeof(contact->iExtraImage));
	return contact;
}

ClcCacheEntry* cliCreateCacheItem(MCONTACT hContact)
{
	if (hContact == NULL)
		return NULL;

	ClcCacheEntry *pdnce = (ClcCacheEntry *)mir_calloc(sizeof(ClcCacheEntry));
	if (pdnce == NULL)
		return NULL;

	pdnce->hContact = hContact;
	pdnce->m_pszProto = GetContactProto(hContact);
	pdnce->bIsHidden = db_get_b(hContact, "CList", "Hidden", 0);
	pdnce->m_bIsSub = db_mc_isSub(hContact) != 0;
	pdnce->m_bNoHiddenOffline = db_get_b(hContact, "CList", "noOffline", 0);
	pdnce->IdleTS = db_get_dw(hContact, pdnce->m_pszProto, "IdleTS", 0);
	pdnce->ApparentMode = db_get_w(hContact, pdnce->m_pszProto, "ApparentMode", 0);
	pdnce->NotOnList = db_get_b(hContact, "CList", "NotOnList", 0);
	pdnce->IsExpanded = db_get_b(hContact, "CList", "Expanded", 0);
	pdnce->dwLastMsgTime = -1;
	return pdnce;
}

/////////////////////////////////////////////////////////////////////////////////////////

void cliInvalidateDisplayNameCacheEntry(MCONTACT hContact)
{
	if (hContact != INVALID_CONTACT_ID) {
		ClcCacheEntry *p = pcli->pfnGetCacheEntry(hContact);
		if (p)
			p->m_iStatus = 0;
	}

	corecli.pfnInvalidateDisplayNameCacheEntry(hContact);
}

void cli_SetContactCheckboxes(ClcContact *cc, int checked)
{
	corecli.pfnSetContactCheckboxes(cc, checked);

	for (int i = 0; i < cc->iSubAllocated; i++)
		corecli.pfnSetContactCheckboxes(&cc->subcontacts[i], checked);
}

int cliGetGroupContentsCount(ClcGroup *group, int visibleOnly)
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
		if (cc->type == CLCIT_GROUP && (!(visibleOnly & 0x01) || cc->group->expanded)) {
			group = cc->group;
			group->scanIndex = 0;
			count += group->cl.getCount();
			continue;
		}
		if (cc->type == CLCIT_CONTACT && cc->subcontacts != NULL && (cc->bSubExpanded || !visibleOnly))
			count += cc->iSubAllocated;

		group->scanIndex++;
	}
	return count;
}

/////////////////////////////////////////////////////////////////////////////////////////
// checks the currently active view mode filter and returns true, if the contact should be hidden
// if no view mode is active, it returns the CList/Hidden setting
// also cares about sub contacts (if meta is active)

int __fastcall CLVM_GetContactHiddenStatus(MCONTACT hContact, char *szProto, ClcData *dat)
{
	int dbHidden = db_get_b(hContact, "CList", "Hidden", 0);		// default hidden state, always respect it.
	int filterResult = 1;
	int searchResult = 0;
	DBVARIANT dbv = { 0 };
	char szTemp[64];
	TCHAR szGroupMask[256];
	DWORD dwLocalMask;
	ClcCacheEntry *pdnce = pcli->pfnGetCacheEntry(hContact);

	// always hide subcontacts (but show them on embedded contact lists)
	if (dat != NULL && dat->IsMetaContactsEnabled && db_mc_isSub(hContact))
		return -1; //subcontact

	if (pdnce && pdnce->m_bIsUnknown && dat != NULL && !dat->force_in_dialog)
		return 1; //'Unknown Contact'

	if (dat != NULL && dat->bFilterSearch && pdnce && pdnce->tszName) {
		// search filtering
		TCHAR *lowered_name = CharLowerW(NEWTSTR_ALLOCA(pdnce->tszName));
		TCHAR *lowered_search = CharLowerW(NEWTSTR_ALLOCA(dat->szQuickSearch));
		searchResult = _tcsstr(lowered_name, lowered_search) ? 0 : 1;
	}
	
	if (pdnce && g_CluiData.bFilterEffective && dat != NULL && !dat->force_in_dialog) {
		if (szProto == NULL)
			szProto = GetContactProto(hContact);
		// check stickies first (priority), only if we really have stickies defined (CLVM_STICKY_CONTACTS is set).
		if (g_CluiData.bFilterEffective & CLVM_STICKY_CONTACTS) {
			if ((dwLocalMask = db_get_dw(hContact, CLVM_MODULE, g_CluiData.current_viewmode, 0)) != 0) {
				if (g_CluiData.bFilterEffective & CLVM_FILTER_STICKYSTATUS) {
					WORD wStatus = db_get_w(hContact, szProto, "Status", ID_STATUS_OFFLINE);
					return !((1 << (wStatus - ID_STATUS_OFFLINE)) & HIWORD(dwLocalMask)) | searchResult;
				}
				return 0 | searchResult;
			}
		}

		// check the proto, use it as a base filter result for all further checks
		if (g_CluiData.bFilterEffective & CLVM_FILTER_PROTOS) {
			mir_snprintf(szTemp, "%s|", szProto);
			filterResult = strstr(g_CluiData.protoFilter, szTemp) ? 1 : 0;
		}

		if (g_CluiData.bFilterEffective & CLVM_FILTER_GROUPS) {
			if (!db_get_ts(hContact, "CList", "Group", &dbv)) {
				mir_sntprintf(szGroupMask, _T("%s|"), &dbv.ptszVal[0]);
				filterResult = (g_CluiData.filterFlags & CLVM_PROTOGROUP_OP) ? (filterResult | (_tcsstr(g_CluiData.groupFilter, szGroupMask) ? 1 : 0)) : (filterResult & (_tcsstr(g_CluiData.groupFilter, szGroupMask) ? 1 : 0));
				mir_free(dbv.ptszVal);
			}
			else if (g_CluiData.filterFlags & CLVM_INCLUDED_UNGROUPED)
				filterResult = (g_CluiData.filterFlags & CLVM_PROTOGROUP_OP) ? filterResult : filterResult & 1;
			else
				filterResult = (g_CluiData.filterFlags & CLVM_PROTOGROUP_OP) ? filterResult : filterResult & 0;
		}

		if (g_CluiData.bFilterEffective & CLVM_FILTER_STATUS) {
			WORD wStatus = db_get_w(hContact, szProto, "Status", ID_STATUS_OFFLINE);
			filterResult = (g_CluiData.filterFlags & CLVM_GROUPSTATUS_OP) ? ((filterResult | ((1 << (wStatus - ID_STATUS_OFFLINE)) & g_CluiData.statusMaskFilter ? 1 : 0))) : (filterResult & ((1 << (wStatus - ID_STATUS_OFFLINE)) & g_CluiData.statusMaskFilter ? 1 : 0));
		}

		if (g_CluiData.bFilterEffective & CLVM_FILTER_LASTMSG) {
			if (pdnce->dwLastMsgTime != -1) {
				DWORD now = g_CluiData.t_now;
				now -= g_CluiData.lastMsgFilter;
				if (g_CluiData.bFilterEffective & CLVM_FILTER_LASTMSG_OLDERTHAN)
					filterResult = filterResult & (pdnce->dwLastMsgTime < now);
				else if (g_CluiData.bFilterEffective & CLVM_FILTER_LASTMSG_NEWERTHAN)
					filterResult = filterResult & (pdnce->dwLastMsgTime > now);
			}
		}
		return (dbHidden | !filterResult | searchResult);
	}

	return dbHidden | searchResult;
}
