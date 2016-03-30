/*
Copyright (c) 2013-16 Miranda NG project (http://miranda-ng.org)

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

#include "stdafx.h"

////////////////////////////////// IDD_CAPTCHAFORM ////////////////////////////////////////

CaptchaForm::CaptchaForm(CVkProto *proto, CAPTCHA_FORM_PARAMS* param) :
	CVkDlgBase(proto, IDD_CAPTCHAFORM, false),
	m_instruction(this, IDC_INSTRUCTION),
	m_edtValue(this, IDC_VALUE),
	m_btnOpenInBrowser(this, IDOPENBROWSER),
	m_btnOk(this, IDOK),
	m_param(param)
{
	m_btnOpenInBrowser.OnClick = Callback(this, &CaptchaForm::On_btnOpenInBrowser_Click);
	m_btnOk.OnClick = Callback(this, &CaptchaForm::On_btnOk_Click);
	m_edtValue.OnChange = Callback(this, &CaptchaForm::On_edtValue_Change);
}

void CaptchaForm::OnInitDialog()
{
	SendMessage(m_hwnd, WM_SETICON, ICON_BIG, (LPARAM)IcoLib_GetIconByHandle(GetIconHandle(IDI_KEYS), TRUE));
	SendMessage(m_hwnd, WM_SETICON, ICON_SMALL, (LPARAM)IcoLib_GetIconByHandle(GetIconHandle(IDI_KEYS)));

	m_btnOk.Disable();
	m_btnOpenInBrowser.Enable((m_param->bmp != NULL));
	m_instruction.SetText(TranslateT("Enter the text you see"));
}

INT_PTR CaptchaForm::DlgProc(UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_CTLCOLORSTATIC:
		switch (GetWindowLongPtr((HWND)lParam, GWL_ID)) {
		case IDC_WHITERECT:
		case IDC_INSTRUCTION:
		case IDC_TITLE:
			return (INT_PTR)GetStockObject(WHITE_BRUSH);
		}
		return NULL;

	case WM_PAINT:
		if (m_param->bmp) {
			PAINTSTRUCT ps;
			HDC hdc, hdcMem;
			RECT rc;

			GetClientRect(m_hwnd, &rc);
			hdc = BeginPaint(m_hwnd, &ps);
			hdcMem = CreateCompatibleDC(hdc);
			HGDIOBJ hOld = SelectObject(hdcMem, m_param->bmp);

			int y = (rc.bottom + rc.top - m_param->h) / 2;
			int x = (rc.right + rc.left - m_param->w) / 2;
			BitBlt(hdc, x, y, m_param->w, m_param->h, hdcMem, 0, 0, SRCCOPY);
			SelectObject(hdcMem, hOld);
			DeleteDC(hdcMem);
			EndPaint(m_hwnd, &ps);

			if (m_proto->getBool("AlwaysOpenCaptchaInBrowser", false))
				m_proto->ShowCaptchaInBrowser(m_param->bmp);
		}
		break;

	}
	return CDlgBase::DlgProc(msg, wParam, lParam);
}

void CaptchaForm::OnDestroy()
{
	IcoLib_ReleaseIcon((HICON)SendMessage(m_hwnd, WM_SETICON, ICON_BIG, 0));
	IcoLib_ReleaseIcon((HICON)SendMessage(m_hwnd, WM_SETICON, ICON_SMALL, 0));
}

void CaptchaForm::On_btnOpenInBrowser_Click(CCtrlButton*)
{
	m_proto->ShowCaptchaInBrowser(m_param->bmp);
}

void CaptchaForm::On_btnOk_Click(CCtrlButton*)
{
	m_edtValue.GetTextA(m_param->Result, _countof(m_param->Result));
	EndDialog(m_hwnd, 1);
}

void CaptchaForm::On_edtValue_Change(CCtrlEdit*)
{
	m_btnOk.Enable(!IsEmpty(ptrA(m_edtValue.GetTextA())));
}

////////////////////////////////// IDD_WALLPOST ///////////////////////////////////////////

WallPostForm::WallPostForm(CVkProto * proto, WALLPOST_FORM_PARAMS * param) :
	CVkDlgBase(proto, IDD_WALLPOST, false),
	m_edtMsg(this, IDC_ED_MSG),
	m_edtUrl(this, IDC_ED_URL),
	m_cbOnlyForFriends(this, IDC_ONLY_FRIENDS),
	m_btnShare(this, IDOK),
	m_param(param)
{
	m_btnShare.OnClick = Callback(this, &WallPostForm::On_btnShare_Click);
	m_edtMsg.OnChange = Callback(this, &WallPostForm::On_edtValue_Change);
	m_edtUrl.OnChange = Callback(this, &WallPostForm::On_edtValue_Change);
}

void WallPostForm::OnInitDialog()
{
	SendMessage(m_hwnd, WM_SETICON, ICON_BIG, (LPARAM)IcoLib_GetIconByHandle(GetIconHandle(IDI_WALL), TRUE));
	SendMessage(m_hwnd, WM_SETICON, ICON_SMALL, (LPARAM)IcoLib_GetIconByHandle(GetIconHandle(IDI_WALL)));
	
	CMString tszTitle(FORMAT, _T("%s %s"), TranslateT("Wall message for"), m_param->ptszNick);
	SetCaption(tszTitle);
	
	m_btnShare.Disable();
}

void WallPostForm::OnDestroy()
{
	IcoLib_ReleaseIcon((HICON)SendMessage(m_hwnd, WM_SETICON, ICON_BIG, 0));
	IcoLib_ReleaseIcon((HICON)SendMessage(m_hwnd, WM_SETICON, ICON_SMALL, 0));
}

void WallPostForm::On_btnShare_Click(CCtrlButton *)
{
	m_param->ptszUrl = mir_tstrdup(m_edtUrl.GetText());
	m_param->ptszMsg = mir_tstrdup(m_edtMsg.GetText());
	m_param->bFriendsOnly = m_cbOnlyForFriends.GetState();

	EndDialog(m_hwnd, 1);
}

void WallPostForm::On_edtValue_Change(CCtrlEdit *)
{
	m_btnShare.Enable(!IsEmpty(ptrT(m_edtMsg.GetText())) || !IsEmpty(ptrT(m_edtUrl.GetText())));
}
