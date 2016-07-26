#if !defined(HISTORYSTATS_GUARD_BANDCTRL_H)
#define HISTORYSTATS_GUARD_BANDCTRL_H

#include "stdafx.h"
#include "bandctrldefs.h"

/*
 * BandCtrl
 */ 

class BandCtrl
	: public BandCtrlDefs
	, private pattern::NotCopyable<BandCtrl>
{
private:
	HWND m_hBandWnd;

public:
	explicit BandCtrl(HWND hBandWnd = NULL) : m_hBandWnd(hBandWnd) { }
	~BandCtrl() { }

public:
	const BandCtrl& operator <<(HWND hBandWnd) { m_hBandWnd = hBandWnd; return *this; }
	operator HWND() { return m_hBandWnd; }

public:
	void setLayout(int nLayout);
	HANDLE addButton(DWORD dwFlags, HICON hIcon, INT_PTR dwData, const wchar_t* szTooltip = NULL, const wchar_t* szText = NULL);
	bool isButtonChecked(HANDLE hButton);
    void checkButton(HANDLE hButton, bool bCheck);
	DWORD getButtonData(HANDLE hButton);
	void setButtonData(HANDLE hButton, INT_PTR dwData);
	bool isButtonVisisble(HANDLE hButton);
	void showButton(HANDLE hButton, bool bShow);
	RECT getButtonRect(HANDLE hButton);
	bool isButtonEnabled(HANDLE hButton);
	void enableButton(HANDLE hButton, bool bEnable);
};

#endif // HISTORYSTATS_GUARD_BANDCTRL_H
