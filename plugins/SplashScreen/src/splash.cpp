/*
Splash Screen Plugin for Miranda NG (www.miranda-ng.org)
(c) 2004-2007 nullbie, (c) 2005-2007 Thief

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "headers.h"

BOOL bpreviewruns;
MyBitmap *SplashBmp, *tmpBmp;

LRESULT CALLBACK SplashWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_LOADED:
		{
			#ifdef _DEBUG
				logMessage(L"WM_LOADED", L"called");
			#endif

			if (!MyUpdateLayeredWindow) ShowWindow(hwndSplash, SW_SHOWNORMAL);
			if (!options.showtime) SetTimer(hwnd, 7, 2000, 0);

			break;
		}

		case WM_LBUTTONDOWN:
			PostMessage(hwnd, WM_CLOSE, 0, 0);
			break;

		case WM_TIMER:

			#ifdef _DEBUG
				TCHAR b [40];
				mir_sntprintf(b, SIZEOF(b), L"%d", wParam);
				logMessage(L"Timer ID", b);
				mir_sntprintf(b, SIZEOF(b), L"%d", options.showtime);
				logMessage(L"ShowTime value", b);
			#endif

			if (options.showtime > 0) //TimeToShow enabled
			{
				if (wParam == 6)
				{
					PostMessage(hwnd, WM_CLOSE, 0, 0);
					#ifdef _DEBUG
						logMessage(L"Showtime timer"), L"triggered");
					#endif
				}
			}
			else
			{
				PostMessage(hwnd, WM_CLOSE, 0, 0);
				if (wParam == 7)
				{
					#ifdef _DEBUG
						logMessage(L"On Modules Loaded timer", L"triggered");
					#endif
				}
				if (wParam == 8)
				{
					#ifdef _DEBUG
						logMessage(L"On Modules Loaded workaround", L"triggered");
					#endif
				}
			}

			break;

		case WM_RBUTTONDOWN:
		{
			ShowWindow(hwndSplash, SW_HIDE);
			DestroyWindow(hwndSplash);
			bpreviewruns = false; // preview is stopped.
			break;
		}

		case WM_CLOSE:
		{
			if (MyUpdateLayeredWindow) // Win 2000+
			{
				RECT rc; GetWindowRect(hwndSplash, &rc);
				POINT ptDst = { rc.left, rc.top };
				POINT ptSrc = { 0, 0 };
				SIZE sz = { rc.right - rc.left, rc.bottom - rc.top };

				BLENDFUNCTION blend;
				blend.BlendOp =             AC_SRC_OVER;
				blend.BlendFlags =          0;
				blend.SourceConstantAlpha = 255;
				blend.AlphaFormat =         AC_SRC_ALPHA;

				// Fade Out
				if (options.fadeout)
				{
					int i;
					for (i = 255; i>=0; i -= options.fosteps)
					{
						blend.SourceConstantAlpha = i;
						MyUpdateLayeredWindow(hwndSplash, NULL, &ptDst, &sz, SplashBmp->getDC(), &ptSrc, 0xffffffff, &blend, LWA_ALPHA);
						Sleep(1);
					}
				}
			}
			if (bserviceinvoked) bserviceinvoked = false;
			if (bpreviewruns) bpreviewruns = false;

			DestroyWindow(hwndSplash);
		}

		case WM_MOUSEMOVE:
		{
			if (bserviceinvoked)
				PostMessage(hwnd, WM_CLOSE, 0, 0);
			break;
		}

		case WM_DESTROY:
		{
			PostQuitMessage(0);
			#ifdef _DEBUG
				logMessage(L"WM_DESTROY", L"called");
			#endif
			break;
		}

		ShowWindow(hwndSplash, SW_HIDE);
		DestroyWindow(hwndSplash);
		break;
	}

	return DefWindowProc(hwnd, message, wParam, lParam);
}

int SplashThread(void *arg)
{
    IGraphBuilder *pGraph = NULL;
    IMediaControl *pControl = NULL;

	if (options.playsnd)
	{
		// Initialize the COM library.
		CoInitialize(NULL);

		// Create the filter graph manager and query for interfaces.
		CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, IID_IGraphBuilder, (void **)&pGraph);

		// Get MediaControl Interface
		pGraph->QueryInterface(IID_IMediaControl, (void **)&pControl);

		// Build the graph. IMPORTANT: Change this string to a file on your system.
		pGraph->RenderFile(szSoundFile, NULL);

		// Run the graph.
		pControl->Run();
	}

	WNDCLASSEX wcl;
	wcl.cbSize = sizeof(wcl);
	wcl.lpfnWndProc = SplashWindowProc;
	wcl.style = 0;
	wcl.cbClsExtra = 0;
	wcl.cbWndExtra = 0;
	wcl.hInstance = hInst;
	wcl.hIcon = NULL;
	wcl.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcl.hbrBackground = (HBRUSH)GetStockObject(LTGRAY_BRUSH);
	wcl.lpszMenuName = NULL;
	wcl.lpszClassName = _T(SPLASH_CLASS);
	wcl.hIconSm = NULL;
	RegisterClassEx(&wcl);

	RECT DesktopRect;
	int w = GetSystemMetrics(SM_CXSCREEN);
	int h = GetSystemMetrics(SM_CYSCREEN);
	DesktopRect.left = 0;
	DesktopRect.top = 0;
	DesktopRect.right = w;
	DesktopRect.bottom = h;

	RECT WindowRect;
	WindowRect.left = (DesktopRect.left + DesktopRect.right - SplashBmp->getWidth()) / 2;
	WindowRect.top  = (DesktopRect.top + DesktopRect.bottom - SplashBmp->getHeight()) / 2;
	WindowRect.right = WindowRect.left + SplashBmp->getWidth();
	WindowRect.bottom = WindowRect.top + SplashBmp->getHeight();

	hwndSplash = CreateWindowEx(
		WS_EX_TOOLWINDOW | WS_EX_TOPMOST,//dwStyleEx
		_T(SPLASH_CLASS), //Class name
		NULL, //Title
		DS_SETFONT | DS_FIXEDSYS | WS_POPUP, //dwStyle
		WindowRect.left, // x
		WindowRect.top, // y
		SplashBmp->getWidth(), // Width
		SplashBmp->getHeight(), // Height
		HWND_DESKTOP, //Parent
		NULL, //menu handle
		hInst, //Instance
		NULL);

	RECT rc; GetWindowRect(hwndSplash, &rc);
	POINT ptDst = {rc.left, rc.top};
	POINT ptSrc = {0, 0};
	SIZE sz = {rc.right - rc.left, rc.bottom - rc.top};
	bool splashWithMarkers = false;

	BLENDFUNCTION blend;
	blend.BlendOp =             AC_SRC_OVER;
	blend.BlendFlags =          0;
	blend.SourceConstantAlpha = 0;
	blend.AlphaFormat =         AC_SRC_ALPHA;

	if (options.showversion)
	{
		// locate text markers:
		int i, x = -1, y = -1;

		int splashWidth = SplashBmp->getWidth();
		for (i = 0; i < splashWidth; ++i)
			if (SplashBmp->getRow(0)[i] & 0xFF000000)
		{
			if (x < 0)
			{
				x = i-1; // 1 pixel for marker line
				splashWithMarkers = true;
			} else
			{
				x = -1;
				splashWithMarkers = false;
				break;
			}
		}
		int splashHeight = SplashBmp->getHeight();
		for (i = 0; splashWithMarkers && (i < splashHeight); ++i)
			if(SplashBmp->getRow(i)[0] & 0xFF000000)
			{
				if (y < 0)
				{
					y = i-1; // 1 pixel for marker line
					splashWithMarkers = true;
				} else
				{
					y = -1;
					splashWithMarkers = false;
					break;
				}
			}

			TCHAR verString[256] = {0};
			TCHAR* mirandaVerString = mir_a2t(szVersion);
			mir_sntprintf(verString, SIZEOF(verString), L"%s%s", szPrefix, mirandaVerString);
			mir_free(mirandaVerString);
			LOGFONT lf = {0};
			lf.lfHeight = 14;
			_tcscpy_s(lf.lfFaceName, L"Verdana");
			SelectObject(SplashBmp->getDC(), CreateFontIndirect(&lf));
			if (!splashWithMarkers)
			{
				SIZE v_sz = {0,0};
				GetTextExtentPoint32(SplashBmp->getDC(), verString, (int)_tcslen(verString), &v_sz);
				x = SplashBmp->getWidth()/2-(v_sz.cx/2);
				y = SplashBmp->getHeight()-(SplashBmp->getHeight()*(100-90)/100);
			}

			SetTextColor(SplashBmp->getDC(), (0xFFFFFFFFUL-SplashBmp->getRow(y)[x])&0x00FFFFFFUL);
			//SplashBmp->DrawText(verString,SplashBmp->getWidth()/2-(v_sz.cx/2),SplashBmp->getHeight()-30);	 
			SetBkMode(SplashBmp->getDC(), TRANSPARENT);
			SplashBmp->DrawText(verString, x, y);
			//free (ptr_verString);
	}

	if (MyUpdateLayeredWindow) // Win 2000+
	{
		SetWindowLongPtr(hwndSplash, GWL_EXSTYLE, GetWindowLongPtr(hwndSplash, GWL_EXSTYLE) | WS_EX_LAYERED);
		/*
		if (splashWithMarkers)
		{
		++ptSrc.x;
		++ptSrc.y;
		}
		*/
		MyUpdateLayeredWindow(hwndSplash, NULL, &ptDst, &sz, SplashBmp->getDC(), &ptSrc, 0xffffffff, &blend, LWA_ALPHA);

		ShowWindow(hwndSplash, SW_SHOWNORMAL);

		if (options.fadein)
		{
			// Fade in
			int i;
			for (i = 0; i < 255; i += options.fisteps)
			{
				blend.SourceConstantAlpha = i;
				MyUpdateLayeredWindow(hwndSplash, NULL, &ptDst, &sz, SplashBmp->getDC(), &ptSrc, 0xffffffff, &blend, LWA_ALPHA);
				Sleep(1);
			}
		}
		blend.SourceConstantAlpha = 255;
		MyUpdateLayeredWindow(hwndSplash, NULL, &ptDst, &sz, SplashBmp->getDC(), &ptSrc, 0xffffffff, &blend, LWA_ALPHA);
	}

	if (DWORD(arg) > 0)
	{
		if (SetTimer(hwndSplash, 6, DWORD(arg), 0))
		{
			#ifdef _DEBUG
				logMessage(L"Timer TimeToShow", L"set");
			#endif
		}
	}
	else
		if (bmodulesloaded)
		{
			if (SetTimer(hwndSplash, 8, 2000, 0))
			{
				#ifdef _DEBUG
					logMessage(L"Timer Modules loaded", L"set");
				#endif
			}
		}

	// The Message Pump
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0) == TRUE) //NULL means every window in the thread; == TRUE means a safe pump.
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	if (options.playsnd)
	{
		pControl->Release();
		pGraph->Release();
		CoUninitialize();
	}

	ExitThread(0);
	return 1;
}

BOOL ShowSplash(BOOL bpreview)
{
	if (bpreview && bpreviewruns) return 0;

	if (bpreview) bpreviewruns = true;
	unsigned long timeout = 0;

	SplashBmp = new MyBitmap;

	#ifdef _DEBUG
		logMessage(L"Loading splash file", szSplashFile);
	#endif

	SplashBmp->loadFromFile(szSplashFile, NULL);

	DWORD threadID;

	#ifdef _DEBUG
		logMessage(L"Thread", L"start");
	#endif

	if (bpreview)
	{
		ShowWindow(hwndSplash, SW_HIDE);
		DestroyWindow(hwndSplash);

		timeout = 2000;
		#ifdef _DEBUG
			logMessage(L"Preview", L"yes");
		#endif
	}
	else
	{
		timeout = options.showtime;
		#ifdef _DEBUG
			TCHAR b [40];
			mir_sntprintf(b, SIZEOF(b), L"%d", options.showtime);
			logMessage(L"Timeout", b);
		#endif
	}

	hSplashThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)SplashThread, (LPVOID)timeout, 0, &threadID);

	#ifdef _DEBUG
		logMessage(L"Thread", L"end");
	#endif

	CloseHandle(hSplashThread);
	hSplashThread = NULL;

	return 1;
}