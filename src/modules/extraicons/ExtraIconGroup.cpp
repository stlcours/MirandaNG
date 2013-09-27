/*

Copyright (C) 2009 Ricardo Pescuma Domenecci
Copyright (C) 2012-13 Miranda NG Project

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

#include "extraicons.h"

ExtraIconGroup::ExtraIconGroup(const char *_name) :
	ExtraIcon(_name), setValidExtraIcon(false), insideApply(false)
{
	db_set_resident(MODULE_NAME, _name);
}

ExtraIconGroup::~ExtraIconGroup()
{
	items.clear();
}

void ExtraIconGroup::addExtraIcon(BaseExtraIcon *extra)
{
	items.push_back(extra);

	CMString description;
	for (unsigned int i = 0; i < items.size(); i++)
	{
		if (i > 0)
			description += _T(" / ");
		description += items[i]->getDescription();
	}

	tszDescription = mir_tstrdup(description);
}

void ExtraIconGroup::rebuildIcons()
{
	for (unsigned int i = 0; i < items.size(); i++)
		items[i]->rebuildIcons();
}

void ExtraIconGroup::applyIcon(HANDLE hContact)
{
	if (!isEnabled() || hContact == NULL)
		return;

	setValidExtraIcon = false;

	insideApply = true;

	unsigned int i;
	for (i = 0; i < items.size(); i++) {
		items[i]->applyIcon(hContact);
		if (setValidExtraIcon)
			break;
	}

	insideApply = false;

	db_set_dw(hContact, MODULE_NAME, name.c_str(), setValidExtraIcon ? items[i]->getID() : 0);
}

int ExtraIconGroup::getPosition() const
{
	int pos = INT_MAX;
	for (unsigned int i = 0; i < items.size(); i++)
		pos = MIN(pos, items[i]->getPosition());
	return pos;
}

void ExtraIconGroup::setSlot(int slot)
{
	ExtraIcon::setSlot(slot);

	for (unsigned int i = 0; i < items.size(); i++)
		items[i]->setSlot(slot);
}

ExtraIcon * ExtraIconGroup::getCurrentItem(HANDLE hContact) const
{
	int id = (int)db_get_dw(hContact, MODULE_NAME, name.c_str(), 0);
	if (id < 1)
		return NULL;

	for (unsigned int i = 0; i < items.size(); i++)
		if (id == items[i]->getID())
			return items[i];

	return NULL;
}

void ExtraIconGroup::onClick(HANDLE hContact)
{
	ExtraIcon *extra = getCurrentItem(hContact);
	if (extra != NULL)
		extra->onClick(hContact);
}

int ExtraIconGroup::setIcon(int id, HANDLE hContact, HANDLE value)
{
	return internalSetIcon(id, hContact, (void*)value, false);
}

int ExtraIconGroup::setIconByName(int id, HANDLE hContact, const char *value)
{
	return internalSetIcon(id, hContact, (void*)value, true);
}

int ExtraIconGroup::internalSetIcon(int id, HANDLE hContact, void *value, bool bByName)
{
	if (insideApply) {
		for (unsigned int i=0; i < items.size(); i++)
			if (items[i]->getID() == id) {
				if (bByName)
					return items[i]->setIconByName(id, hContact, (const char*)value);
				return items[i]->setIcon(id, hContact, (HANDLE)value);
			}

		return -1;
	}

	ExtraIcon *current = getCurrentItem(hContact);
	int currentPos = (int)items.size();
	int storePos = (int)items.size();
	for (unsigned int i = 0; i < items.size(); i++) {
		if (items[i]->getID() == id)
			storePos = i;

		if (items[i] == current)
			currentPos = i;
	}

	if (storePos == items.size())
		return -1;

	if (storePos > currentPos) {
		items[storePos]->storeIcon(hContact, value);
		return 0;
	}

	// Ok, we have to set the icon, but we have to assert it is a valid icon

	setValidExtraIcon = false;

	int ret;
	if (bByName)
		ret = items[storePos]->setIconByName(id, hContact, (const char*)value);
	else
		ret = items[storePos]->setIcon(id, hContact, (HANDLE)value);

	if (storePos < currentPos) {
		if (setValidExtraIcon)
			db_set_dw(hContact, MODULE_NAME, name.c_str(), items[storePos]->getID());
	}
	else if (storePos == currentPos) {
		if (!setValidExtraIcon) {
			db_set_dw(hContact, MODULE_NAME, name.c_str(), 0);

			insideApply = true;

			for (++storePos; storePos < (int)items.size(); ++storePos) {
				items[storePos]->applyIcon(hContact);
				if (setValidExtraIcon)
					break;
			}

			insideApply = false;

			if (setValidExtraIcon)
				db_set_dw(hContact, MODULE_NAME, name.c_str(), items[storePos]->getID());
		}
	}

	return ret;
}

const TCHAR *ExtraIconGroup::getDescription() const
{
	return tszDescription;
}

const char *ExtraIconGroup::getDescIcon() const
{
	for (unsigned int i = 0; i < items.size(); i++)
		if (!IsEmpty(items[i]->getDescIcon()))
			return items[i]->getDescIcon();

	return "";
}

int ExtraIconGroup::getType() const
{
	return EXTRAICON_TYPE_GROUP;
}

int ExtraIconGroup::ClistSetExtraIcon(HANDLE hContact, HANDLE hImage)
{
	if (hImage != INVALID_HANDLE_VALUE)
		setValidExtraIcon = true;

	return Clist_SetExtraIcon(hContact, slot, hImage);
}
