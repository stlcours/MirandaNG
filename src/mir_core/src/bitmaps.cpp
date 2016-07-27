/*

Miranda NG: the free IM client for Microsoft* Windows*

Copyright (C) 2012-16 Miranda NG project,
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

#include <m_imgsrvc.h>

MIR_CORE_DLL(HBITMAP) Bitmap_Load(const wchar_t *ptszFileName)
{
	wchar_t szFilename[MAX_PATH];
	if (!PathToAbsoluteT(ptszFileName, szFilename))
		wcsncpy_s(szFilename, ptszFileName, _TRUNCATE);

	if (!ServiceExists(MS_IMG_LOAD))
		return NULL;

	return (HBITMAP)CallService(MS_IMG_LOAD, (WPARAM)szFilename, IMGL_TCHAR);
}

MIR_CORE_DLL(void) Bitmap_GetFilter(wchar_t *dest, size_t destLen)
{
	if (dest == NULL)
		return;

	mir_snwprintf(dest, destLen,
		L"%s (*.bmp;*.jpg;*.gif;*.png)%c*.BMP;*.RLE;*.JPG;*.JPEG;*.GIF;*.PNG%c"
		L"%s (*.bmp;*.rle)%c*.BMP;*.RLE%c"
		L"%s (*.jpg;*.jpeg)%c*.JPG;*.JPEG%c"
		L"%s (*.gif)%c*.GIF%c"
		L"%s (*.png)%c*.PNG%c%c",
		TranslateT("All Files"), 0, 0, TranslateT("Windows bitmaps"), 0, 0, TranslateT("JPEG bitmaps"), 0, 0, TranslateT("GIF bitmaps"), 0, 0, TranslateT("PNG bitmaps"), 0, 0, 0);
}
