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

#ifndef _TrafficCounter_H
#define _TrafficCounter_H

int TrafficRecv(WPARAM wParam,LPARAM lParam);
int TrafficSend(WPARAM wParam,LPARAM lParam);

//---------------------------------------------------------------------------------------------
// ����� ���������� � ����������� � ����
//---------------------------------------------------------------------------------------------
#define TRAFFIC_SETTINGS_GROUP "TrafficCounter"

#define SETTINGS_COUNTER_FORMAT "CounterFormat"
#define SETTINGS_TOOLTIP_FORMAT "TooltipFormat"
#define SETTINGS_STAT_ACC_OPT "Stat_SelAcc"
#define SETTINGS_ADDITION_SPACE "traffic_addition_space"
//
//notify
#define SETTINGS_POPUP_BKCOLOR "popup_bkcolor"
#define SETTINGS_POPUP_FONTCOLOR "popup_font_color"
#define SETTINGS_POPUP_NOTIFY_SIZE_VALUE "popup_size_value"
#define SETTINGS_POPUP_NOTIFY_TIME_VALUE "popup_time_value"
#define SETTINGS_POPUP_TIMEOUT_DEFAULT "popup_timeout_default"
#define SETTINGS_POPUP_TIMEOUT_VALUE "popup_timeout_value"
//timer notify
#define TIMER_SAVE_TICK (3001 + 1)
#define TIMER_NOTIFY_TICK (3001 + 2)
//popup menu
#define POPUPMENU_HIDE (3001 + 4)
#define POPUPMENU_ONTOP (3001 + 5)
#define POPUPMENU_CLEAR_NOW (3001 + 8)
//time counter
#define TIMER_REDRAW 3300
#define TIMER_TOOLTIP 3301
#define SETTINGS_TOTAL_ONLINE_TIME "TotalOnlineTime"
// ������ ������
#define SETTINGS_WHAT_DRAW "WhatDraw"

// ��������� ��� ���������� - ����������� � ����� ���������
#define SETTINGS_PROTO_FLAGS "tc_Flags"

/*====== ������� ������ =====*/
int TrafficCounterOptInitialise(WPARAM wParam,LPARAM lParam);
int TrafficCounterModulesLoaded(WPARAM wParam,LPARAM lParam);
int TrafficCounterShutdown(WPARAM wParam,LPARAM lParam);
int ModuleLoad(WPARAM wParam, LPARAM lParam);
void SaveSettings(BYTE);
void CreateTrafficWindow(HWND);
//popup
void UpdateNotifyTimer(void);
void NotifyOnSend(void);
void NotifyOnRecv(void);
//time counter
int ProtocolAckHook(WPARAM wParam,LPARAM lParam);
void CreateProtocolList(void);
void DestroyProtocolList(void);
int OnAccountsListChange(WPARAM wParam, LPARAM lParam);
//font service suppot
int UpdateFonts(WPARAM wParam, LPARAM lParam);
void UpdateTrafficWindowSize(void);
unsigned short int TrafficWindowHeight(void);
// ���������
int TrafficCounter_Draw(HWND, HDC);
int PaintTrafficCounterWindow(HWND, HDC);
// ��������������� �������
INT_PTR MenuCommand_TrafficShowHide(WPARAM, LPARAM);
void Traffic_AddMainMenuItem(void);

typedef	struct
{
	BYTE Hour, Day, Month;
	WORD Year;
	DWORD Incoming, Outgoing;
	WORD Time;
} HOURLYSTATS;

typedef struct tagTimer
{
	DWORD TimeAtStart; // ����� � ������ ������� ������� - � �������������.
	DWORD Timer; // ���������� ������ �� ������� ������� �������.
} TIMER;

typedef struct
{
	char *name; // ��� ��������.

	TIMER Session; // ������ ������� ������ (�������� � �������).
	TIMER Total; // ������ �����.

	DWORD TotalRecvTraffic, // ����� ������ ��������� (�� ��������� ������)
		  TotalSentTraffic,
		  CurrentRecvTraffic, // ������� ������ ��������� (�� ������)
		  CurrentSentTraffic;
	union
	{
		BYTE Flags;
		struct
		{
			unsigned int Reserv0:1; // ���������� �������� ����� - ���������� ������ �� ���� ���������.
			unsigned int Visible:1; // = 1 - ������� ����� ������������ �� ������ ���������
			unsigned int Enabled:1; // = 1 - ������� ������� � �� ��������
			unsigned int State:1;   // = 1 - ������� ������ ������
			unsigned int Reserv1:3;
		};
	};

	// ��������� � ������ 0.1.1.0.
	DWORD NumberOfRecords; // ���������� ����� � ����� ����������.
	HOURLYSTATS *AllStatistics; // ������ ���������� ������ �� ����������� �������.
	HANDLE hFile; // ���� � ���������� ����������� ������� ���������.

	DWORD StartIndex; // ����� ������ � ����������, ������ ���������� �� ������ �������.
	DWORD StartIncoming; // �������� ��������� ������� �� ������ �������.
	DWORD StartOutgoing; // �������� ���������� ������� �� ������ �������.

	// 0.1.1.5.
	DWORD Shift;	// ����� ������ � ���������� ���������� ���������� ��������,
					// ���� ������� ������������� ������ ���������� ������� ��������.

	// 0.1.1.6
	TCHAR *tszAccountName; // ������������ ��� �������� ��� ������������� � ����������� ����������.
} PROTOLIST;


#endif;
