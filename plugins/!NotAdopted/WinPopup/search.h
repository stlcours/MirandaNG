/*

WinPopup Protocol plugin for Miranda IM.

Copyright (C) 2004-2009 Nikolay Raspopov

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

#pragma once

class search
{
public:
	search();

	// ������ �������� ������
	INT_PTR StartSearch(LPCTSTR szId);

	// ������ �������� ������
	void AskForDestroy();

	// �������� �������� ������
	void Destroy();

protected:
	typedef struct _BasicSearchData
	{
		search*			me;				// ������� ������� ������
		HANDLE			cookie;			// ������������� ������� ���� ������� ��� ������
		CString			id;				// ��� ���� (uppercase)
		LPNETRESOURCE	root;			// ������ ��������� ���������� (mir_alloc)
	} BasicSearchData;

	HANDLE			m_event;			// ������� ������� ��� �������� ������
	volatile LONG	m_count;			// ������� ���������� �������

	// ������� ��������� ������
	void BasicSearch(const BasicSearchData* data);

	// ������� ��������� ������������ ������
	void BasicSearchJob(const BasicSearchData* data);

	// ������� ���� ������������ ������
	static void BasicSearchThread(LPVOID param);
};

extern search pluginSearch;
