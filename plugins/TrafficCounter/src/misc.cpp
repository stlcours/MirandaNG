/*
Traffic Counter plugin for Miranda IM 
Copyright 2007-2011 Mironych.

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

/* ������� ��������� ������ � ���������� ������ ����� � ��������������� �� �����.
���������:
InputString - ������ ��� �������;
RowItemsList - ������ ��������� ���������.
������������ �������� - ���������� ��������� � �������. */

WORD GetRowItems(wchar_t *InputString, RowItemInfo **RowItemsList)
{
	wchar_t *begin, *end;
	WORD c = 0;

	// ���� ����� ����������� ������.
	begin = wcschr(InputString, '{');
	// ���� ������ �������...
	if (begin) {
		// �������� ������ ��� ���������
		*RowItemsList = (RowItemInfo*)mir_alloc(sizeof(RowItemInfo));
	}
	else return 0;

	do {
		// ����� ����� �� ��� ���� �����������.
		end = wcschr(begin, '}');

		// �������� ������ ��� ���������
		*RowItemsList = (RowItemInfo*)mir_realloc(*RowItemsList, sizeof(RowItemInfo) * (c + 1));

		// ��������� ���.
		swscanf(begin + 1, L"%c%hd",
			&((*RowItemsList)[c].Alignment),
			&((*RowItemsList)[c].Interval));

		// ���� ����� ����������� ������ - ��� ����� ������, ��������������� ����.
		begin = wcschr(end, '{');

		if (begin) {
			// �������� ������ ��� ������.
			(*RowItemsList)[c].String = (wchar_t*)mir_alloc(sizeof(wchar_t) * (begin - end));
			// �������� ������.
			wcsncpy((*RowItemsList)[c].String, end + 1, begin - end - 1);
			(*RowItemsList)[c].String[begin - end - 1] = 0;
		}
		else {
			// �������� ������ ��� ������.
			(*RowItemsList)[c].String = (wchar_t*)mir_alloc(sizeof(wchar_t) * mir_tstrlen(end));
			// �������� ������.
			wcsncpy((*RowItemsList)[c].String, end + 1, mir_tstrlen(end));
		}

		c++;
	} while (begin);

	return c;
}

/* ������� ���������� ���������� ���� � ��������� ������ ���������� ����. */
BYTE DaysInMonth(BYTE Month, WORD Year)
{
	switch (Month) {
	case 1:
	case 3:
	case 5:
	case 7:
	case 8:
	case 10:
	case 12: return 31;
	case 4:
	case 6:
	case 9:
	case 11: return 30;
	case 2: return 28 + (BYTE)!((Year % 4) && ((Year % 100) || !(Year % 400)));
	}
	return 0;
}

// ������� ���������� ���� ������ �� ����
// 7 - ��, 1 - �� � �. �.
BYTE DayOfWeek(BYTE Day, BYTE Month, WORD Year)
{
	WORD a, y, m;

	a = (14 - Month) / 12;
	y = Year - a;
	m = Month + 12 * a - 2;

	a = (7000 + (Day + y + (y >> 2) - y / 100 + y / 400 + (31 * m) / 12)) % 7;
	if (!a) a = 7;

	return a;
}

/*
���������:
Value - ���������� ����;
Unit - ������� ��������� (0 - �����, 1 - ���������, 2 - ���������, 3 - �������������);
Buffer - ����� ������ ��� ������ ����������;
Size - ������ ������.
������������ ��������: ��������� ������ ������.
*/
size_t GetFormattedTraffic(DWORD Value, BYTE Unit, wchar_t *Buffer, size_t Size)
{
	wchar_t Str1[32], szUnit[4] = { ' ', 0 };
	DWORD Divider;
	NUMBERFMT nf = { 0, 1, 3, L",", L" ", 0 };
	wchar_t *Res; // ������������� ���������.

	switch (Unit) {
	case 0: //bytes
		Divider = 1;
		nf.NumDigits = 0;
		szUnit[0] = 0;
		break;
	case 1: // KB
		Divider = 0x400;
		nf.NumDigits = 2;
		break;
	case 2: // MB
		Divider = 0x100000;
		nf.NumDigits = 2;
		break;
	case 3: // Adaptive
		nf.NumDigits = 2;
		if (Value < 0x100000) { Divider = 0x400; szUnit[1] = 'K'; szUnit[2] = 'B'; }
		else { Divider = 0x100000; szUnit[1] = 'M'; szUnit[2] = 'B'; }
		break;
	default:
		return 0;
	}

	mir_sntprintf(Str1, L"%d.%d", Value / Divider, Value % Divider);
	size_t l = GetNumberFormat(LOCALE_USER_DEFAULT, 0, Str1, &nf, NULL, 0);
	if (!l) return 0;
	l += mir_tstrlen(szUnit) + 1;
	Res = (wchar_t*)malloc(l * sizeof(wchar_t));
	if (!Res) return 0;
	GetNumberFormat(LOCALE_USER_DEFAULT, 0, Str1, &nf, Res, (int)l);
	mir_tstrcat(Res, szUnit);

	if (Size && Buffer) {
		mir_tstrcpy(Buffer, Res);
		l = mir_tstrlen(Buffer);
	}
	else l = mir_tstrlen(Res) + 1;

	free(Res);
	return l;
}

/* �������������� ��������� ������� � ��� ��������� �������������
���������:
Duration: �������� ������� � ��������;
Format: ������ �������;
Buffer: ����� ������, ���� ������� �������� ���������.
Size - ������ ������. */

size_t GetDurationFormatM(DWORD Duration, wchar_t *Format, wchar_t *Buffer, size_t Size)
{
	size_t Length;
	DWORD q;
	WORD TokenIndex, FormatIndex;
	wchar_t Token[256],  // �����������.
		*Res; // ������������� ���������.

	Res = (wchar_t*)malloc(sizeof(wchar_t)); // �������� ����-���� ������ ��� ���������, �� ��� ������ ������.
	//SecureZeroMemory(Res, sizeof(wchar_t));
	Res[0] = 0;

	for (FormatIndex = 0; Format[FormatIndex];) {
		// ���� ������. ���������, ��� ����� - ������ �����.
		TokenIndex = 0;
		q = iswalpha(Format[FormatIndex]);
		// �������� ������� � ����������� �� ����� �����.
		do {
			Token[TokenIndex++] = Format[FormatIndex++];
		} while (q == iswalpha(Format[FormatIndex]));
		Token[TokenIndex] = 0;

		// ��� �������� � ������������?
		if (!mir_tstrcmp(Token, L"d")) {
			q = Duration / (60 * 60 * 24);
			mir_sntprintf(Token, L"%d", q);
			Duration -= q * 60 * 60 * 24;
		}
		else if (!mir_tstrcmp(Token, L"h")) {
			q = Duration / (60 * 60);
			mir_sntprintf(Token, L"%d", q);
			Duration -= q * 60 * 60;
		}
		else if (!mir_tstrcmp(Token, L"hh")) {
			q = Duration / (60 * 60);
			mir_sntprintf(Token, L"%02d", q);
			Duration -= q * 60 * 60;
		}
		else if (!mir_tstrcmp(Token, L"m")) {
			q = Duration / 60;
			mir_sntprintf(Token, L"%d", q);
			Duration -= q * 60;
		}
		else if (!mir_tstrcmp(Token, L"mm")) {
			q = Duration / 60;
			mir_sntprintf(Token, L"%02d", q);
			Duration -= q * 60;
		}
		else if (!mir_tstrcmp(Token, L"s")) {
			q = Duration;
			mir_sntprintf(Token, L"%d", q);
			Duration -= q;
		}
		else if (!mir_tstrcmp(Token, L"ss")) {
			q = Duration;
			mir_sntprintf(Token, L"%02d", q);
			Duration -= q;
		}

		// ������� ������, ���� �����.
		Length = mir_tstrlen(Res) + mir_tstrlen(Token) + 1;
		Res = (wchar_t*)realloc(Res, Length * sizeof(wchar_t));
		mir_tstrcat(Res, Token);
	}

	if (Size && Buffer) {
		wcsncpy(Buffer, Res, Size);
		Length = mir_tstrlen(Buffer);
	}
	else Length = mir_tstrlen(Res) + 1;

	free(Res);
	return Length;
}

/* ���������:
-1 - st1 < st2
0 - st1 = st2
+1 - st1 > st2
*/
signed short int TimeCompare(SYSTEMTIME st1, SYSTEMTIME st2)
{
	signed short int a, b, c, d;

	a = st1.wYear - st2.wYear;
	b = st1.wMonth - st2.wMonth;
	c = st1.wDay - st2.wDay;
	d = st1.wHour - st2.wHour;

	if (a < 0) return -1;
	if (a > 0) return +1;

	if (b < 0) return -1;
	if (b > 0) return +1;

	if (c < 0) return -1;
	if (c > 0) return +1;

	if (d < 0) return -1;
	if (d > 0) return +1;
	return 0;
}
