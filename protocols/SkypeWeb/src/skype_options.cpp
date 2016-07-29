/*
Copyright (c) 2015-16 Miranda NG project (http://miranda-ng.org)

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

CSkypeOptionsMain::CSkypeOptionsMain(CSkypeProto *proto, int idDialog)
: CSkypeDlgBase(proto, idDialog, false),
	m_skypename(this, IDC_SKYPENAME),
	m_password(this, IDC_PASSWORD),
	m_group(this, IDC_GROUP),
	m_place(this, IDC_PLACE),
	m_autosync(this, IDC_AUTOSYNC),
	m_allasunread(this, IDC_MESASUREAD),
	m_usehostname(this, IDC_USEHOST),
	m_usebb(this, IDC_BBCODES),
	m_link(this, IDC_CHANGEPASS, "https://login.skype.com/recovery/password-change") // TODO : ...?username=%username%
{
	CreateLink(m_group, proto->m_opts.wstrCListGroup);
	CreateLink(m_autosync, proto->m_opts.bAutoHistorySync);
	CreateLink(m_allasunread, proto->m_opts.bMarkAllAsUnread);
	CreateLink(m_place, proto->m_opts.wstrPlace);
	CreateLink(m_usehostname, proto->m_opts.bUseHostnameAsPlace);
	CreateLink(m_usebb, proto->m_opts.bUseBBCodes);
	m_usehostname.OnChange = Callback(this, &CSkypeOptionsMain::OnUsehostnameCheck);
}

void CSkypeOptionsMain::OnInitDialog()
{
	CSkypeDlgBase::OnInitDialog();

	m_skypename.SetTextA(ptrA(m_proto->getStringA(SKYPE_SETTINGS_ID)));
	m_password.SetTextA(pass_ptrA(m_proto->getStringA("Password")));
	m_place.Enable(!m_proto->m_opts.bUseHostnameAsPlace);
	m_skypename.SendMsg(EM_LIMITTEXT, 32, 0);
	m_password.SendMsg(EM_LIMITTEXT, 128, 0);
	m_group.SendMsg(EM_LIMITTEXT, 64, 0);
}


void CSkypeOptionsMain::OnApply()
{
	ptrA szNewSkypename(m_skypename.GetTextA()),	 
		 szOldSkypename(m_proto->getStringA(SKYPE_SETTINGS_ID));
	pass_ptrA szNewPassword(m_password.GetTextA()),
			szOldPassword(m_proto->getStringA("Password"));
	if (mir_strcmpi(szNewSkypename, szOldSkypename) || mir_strcmp(szNewPassword, szOldPassword))
		m_proto->delSetting("TokenExpiresIn");
	m_proto->setString(SKYPE_SETTINGS_ID, szNewSkypename);
	m_proto->setString("Password", szNewPassword);
	ptrW group(m_group.GetText());
	if (mir_wstrlen(group) > 0 && !Clist_GroupExists(group))
		Clist_GroupCreate(0, group);
}

/////////////////////////////////////////////////////////////////////////////////

int CSkypeProto::OnOptionsInit(WPARAM wParam, LPARAM)
{
	OPTIONSDIALOGPAGE odp = { sizeof(odp) };
	odp.hInstance = g_hInstance;
	odp.pwszTitle = m_tszUserName;
	odp.flags = ODPF_BOLDGROUPS | ODPF_UNICODE | ODPF_DONTTRANSLATE;
	odp.pwszGroup = LPGENW("Network");

	odp.pwszTab = LPGENW("Account");
	odp.pDialog = CSkypeOptionsMain::CreateOptionsPage(this);
	Options_AddPage(wParam, &odp);

	return 0;
}

void CSkypeOptionsMain::OnUsehostnameCheck(CCtrlCheck*)
{
	m_place.Enable(!m_usehostname.GetState());
}