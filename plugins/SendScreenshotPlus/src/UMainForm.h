/*

Miranda IM: the free IM client for Microsoft* Windows*
Copyright 2000-2009 Miranda ICQ/IM project, 

This file is part of Send Screenshot Plus, a Miranda IM plugin.
Copyright (c) 2010 Ing.U.Horn

Parts of this file based on original sorce code
(c) 2004-2006 S�rgio Vieira Rolanski (portet from Borland C++)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#ifndef UMainFormH
#define UMainFormH

#define SS_JUSTSAVE		0
#define SS_FILESEND		1
#define SS_EMAIL		2
#define SS_HTTPSERVER	3
#define SS_FTPFILE		4
#define SS_IMAGESHACK	5

// Used for our own cheap TrackMouseEvent
#define BUTTON_POLLDELAY    50

// User Events
#define EVT_CaptureDone		1
#define EVT_SendFileDone	2
#define EVT_CheckOpenAgain	3

extern FI_INTERFACE *FIP;

typedef struct MyTabData {
	TCITEMHEADER tcih;
	HWND hwndMain;		//main window 
	HWND hwndTab;		//tab control 
	HWND hwndTabPage;	//current child dialog box 
}TAB_INFO;

//---------------------------------------------------------------------------
class TfrmMain{

	public:
		// Deklaration Standardkonstruktor/Standarddestructor
		TfrmMain();
		~TfrmMain();

		BYTE		m_opt_tabCapture;			//capure tab page
		BYTE		m_opt_btnDesc;				//TCheckBox *chkDesc;
		BYTE		m_opt_cboxDesktop;			//TRadioButton *rbtnDesktop;
		BYTE		m_opt_chkEditor;			//TCheckBox *chkEditor;
		BYTE		m_opt_chkTimed;				//TCheckBox *chkTimed;
		BYTE		m_opt_cboxSendBy;			//TComboBox *cboxSendBy;
		bool		m_bOnExitSave;

		static void Unload();
		void		Show(){ShowWindow(m_hWnd,SW_SHOW);}
		void		Hide(){ShowWindow(m_hWnd,SW_HIDE);}
		void		Close(){SendMessage(m_hWnd,WM_CLOSE,0,0);}
		void		Init(LPTSTR DestFolder, HANDLE Contact);
		void		btnCaptureClick();
		void		cboxSendByChange();

	private:
		HWND		m_hWnd;
		HANDLE		m_hContact;
		bool		m_bSelectingWindow, m_bDeleteAfterSend;
		bool		m_bFormAbout, m_bFormEdit;
		HWND		m_hTargetWindow, m_hLastWin;
		LPTSTR		m_FDestFolder;
		LPTSTR		m_pszFile;
		LPTSTR		m_pszFileDesc;
		FIBITMAP*	m_Screenshot;				//Graphics::TBitmap *Screenshot;
		RGBQUAD		m_AlphaColor;				//
		HCURSOR		m_hCursor;
		CSend*		m_cSend;

		void		chkTimedClick();
		void		imgTargetMouseDown();
		void		imgTargetMouseUp();
		void		btnAboutClick();
		void		btnAboutOnCloseWindow(HWND hWnd);
		void		btnExploreClick();
		void		LoadOptions(void);
		void		SaveOptions(void);
		INT_PTR		SaveScreenshot(FIBITMAP* dib);
		void		FormClose();
		static void	edtSizeUpdate(HWND hWnd, BOOL ClientArea, HWND hTarget, UINT Ctrl);
		static void	edtSizeUpdate(RECT rect, HWND hTarget, UINT Ctrl);

	protected:
		size_t			m_MonitorCount;
		MONITORINFOEX*	m_Monitors;
		RECT			m_VirtualScreen;

		BYTE			m_opt_chkOpenAgain;			//TCheckBox *chkOpenAgain;
		BYTE			m_opt_chkClientArea;		//TCheckBox *chkClientArea;
		BYTE			m_opt_edtQuality;			//TLabeledEdit *edtQuality;
		BYTE			m_opt_btnDeleteAfterSend;	//TCheckBox *chkDeleteAfterSend;
		BYTE			m_opt_cboxFormat;			//TComboBox *cboxFormat;
		BYTE			m_opt_edtTimed;				//TLabeledEdit *edtTimed;
		bool			m_bCapture;					//is capture activ

		typedef std::map<HWND, TfrmMain *> CHandleMapping;
		static CHandleMapping _HandleMapping;
		static LRESULT CALLBACK DlgTfrmMain(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

		LRESULT		wmInitdialog(WPARAM wParam, LPARAM lParam);
		LRESULT		wmCommand(WPARAM wParam, LPARAM lParam);
		LRESULT		wmClose(WPARAM wParam, LPARAM lParam);
		LRESULT		wmNotify(WPARAM wParam, LPARAM lParam);
		LRESULT		wmTimer(WPARAM wParam, LPARAM lParam);

		LRESULT		UMevent(WPARAM wParam, LPARAM lParam);
		LRESULT		UMClosing(WPARAM wParam, LPARAM lParam);
		LRESULT		UMTab1(WPARAM wParam, LPARAM lParam);

		HWND		m_hwndTab;		//TabControl handle
		HWND		m_hwndTabPage;	//TabControl activ page handle
		HIMAGELIST	m_himlTab;		//TabControl imagelist
		static INT_PTR CALLBACK DlgProc_CaptureWindow (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
		static INT_PTR CALLBACK DlgProc_CaptureDesktop(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
//		LRESULT CALLBACK DlgProc_UseLastFile   (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

};

//---------------------------------------------------------------------------

#endif
