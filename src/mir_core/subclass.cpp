/*
Copyright (C) 2012-13 Miranda NG team (http://miranda-ng.org)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation version 2
of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include "commonheaders.h"

struct MSubclassData
{
	HWND     m_hWnd;

	int      m_iHooks;
	WNDPROC *m_hooks;
	WNDPROC  m_origWndProc;

	~MSubclassData()
	{
		free(m_hooks);
	}
};

static LIST<MSubclassData> arSubclass(10, LIST<MSubclassData>::FTSortFunc(HandleKeySortT));

static LRESULT CALLBACK MSubclassWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	MSubclassData *p = arSubclass.find((MSubclassData*)&hwnd);
	if (p != NULL) {
		if (p->m_iHooks)
			return p->m_hooks[p->m_iHooks-1](hwnd, uMsg, wParam, lParam);

		return p->m_origWndProc(hwnd, uMsg, wParam, lParam);
	}		

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

MIR_CORE_DLL(void) mir_subclassWindow(HWND hWnd, WNDPROC wndProc)
{
	MSubclassData *p = arSubclass.find((MSubclassData*)&hWnd);
	if (p == NULL) {
		p = new MSubclassData;
		p->m_hWnd = hWnd;
		p->m_origWndProc = (WNDPROC)SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)MSubclassWndProc);
		p->m_iHooks = 0;
		p->m_hooks = (WNDPROC*)malloc( sizeof(WNDPROC));
		arSubclass.insert(p);
	}
	else {
		for (int i=0; i < p->m_iHooks; i++)
			if (p->m_hooks[i] == wndProc)
				return;

		p->m_hooks = (WNDPROC*)realloc(p->m_hooks, (p->m_iHooks+1)*sizeof(WNDPROC));
	}

	p->m_hooks[p->m_iHooks++] = wndProc;		
}

MIR_CORE_DLL(void) mir_subclassWindowFull(HWND hWnd, WNDPROC wndProc, WNDPROC oldWndProc)
{
	MSubclassData *p = arSubclass.find((MSubclassData*)&hWnd);
	if (p == NULL) {
		p = new MSubclassData;
		p->m_hWnd = hWnd;
		p->m_origWndProc = oldWndProc;
		p->m_iHooks = 0;
		p->m_hooks = (WNDPROC*)malloc( sizeof(WNDPROC));
		arSubclass.insert(p);

		SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)MSubclassWndProc);
	}
	else {
		for (int i=0; i < p->m_iHooks; i++)
			if (p->m_hooks[i] == wndProc)
				return;

		p->m_hooks = (WNDPROC*)realloc(p->m_hooks, (p->m_iHooks+1)*sizeof(WNDPROC));
	}

	p->m_hooks[p->m_iHooks++] = wndProc;		
}

MIR_CORE_DLL(LRESULT) mir_callNextSubclass(HWND hWnd, WNDPROC wndProc, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	MSubclassData *p = arSubclass.find((MSubclassData*)&hWnd);
	if (p) {
		for (int i=p->m_iHooks-1; i >= 0; i--) {
			if (p->m_hooks[i] == wndProc) {
				// next hook exitst, call it 
				if (i != 0)
					return p->m_hooks[i-1](hWnd, uMsg, wParam, lParam);

				// last hook called, ping the default window procedure
				if (uMsg == WM_DESTROY) {
					WNDPROC saveProc = p->m_origWndProc;
					arSubclass.remove(p);
					delete p;

					SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)saveProc);
					return CallWindowProc(saveProc, hWnd, uMsg, wParam, lParam);
				}

				return CallWindowProc(p->m_origWndProc, hWnd, uMsg, wParam, lParam);
			}
		}
		
		// invalid / closed hook
		return 0;
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

MIR_CORE_DLL(void) KillModuleSubclassing(HMODULE hInst)
{
	for (int i=0; i < arSubclass.getCount(); i++) {
		MSubclassData *p = arSubclass[i];
		for (int j=0; j < p->m_iHooks; ) {
			if ( GetInstByAddress(p->m_hooks[j]) == hInst) {
				WNDPROC saveProc = p->m_hooks[j];

				// untie hook from a window to prevent calling mir_callNextSubclass from saveProc
				for (int k=j+1; k < p->m_iHooks; k++)
					p->m_hooks[k-1] = p->m_hooks[k];
				p->m_iHooks--;

				// emulate window destruction
				saveProc(p->m_hWnd, WM_DESTROY, 0, 0);
				saveProc(p->m_hWnd, WM_NCDESTROY, 0, 0);
			}
			else j++;
		}
	}
}
