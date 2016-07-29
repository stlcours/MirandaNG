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

////////////////////// Account manager dialog ////////////////////////////////

INT_PTR CVkProto::SvcCreateAccMgrUI(WPARAM, LPARAM lParam)
{
	CVkAccMgrForm *dlg = new CVkAccMgrForm(this, (HWND)lParam);
	dlg->Show();
	return (INT_PTR)dlg->GetHwnd();
}

//////////////////////////////////////////////////////////////////////////////

CVkAccMgrForm::CVkAccMgrForm(CVkProto *proto, HWND hwndParent):
	CVkDlgBase(proto, IDD_ACCMGRUI, false),
	m_edtLogin(this, IDC_LOGIN),
	m_edtPassword(this, IDC_PASSWORD),
	m_hlLink(this, IDC_URL, "https://vk.com/")
{
	SetParent(hwndParent);

	CreateLink(m_edtLogin, "Login", L"");
}

void CVkAccMgrForm::OnInitDialog()
{
	CSuper::OnInitDialog();

	m_pwszOldLogin = m_edtLogin.GetText();
	m_edtLogin.SendMsg(EM_LIMITTEXT, 1024, 0);

	m_pwszOldPass = m_proto->GetUserStoredPassword();
	m_edtPassword.SetText(m_pwszOldPass);
	m_edtPassword.SendMsg(EM_LIMITTEXT, 1024, 0);
}

void CVkAccMgrForm::OnApply()
{
	pass_ptrW pwszNewPass(m_edtPassword.GetText());
	bool bPassChanged = mir_wstrcmp(m_pwszOldPass, pwszNewPass) != 0;
	if (bPassChanged) {
		T2Utf szRawPasswd(pwszNewPass);
		m_proto->setString("Password", szRawPasswd);
		pass_ptrA pszPass(szRawPasswd.detach());
		m_pwszOldPass = pwszNewPass;
	}

	ptrW pwszNewLogin(m_edtLogin.GetText());
	if (bPassChanged || mir_wstrcmpi(m_pwszOldLogin, pwszNewLogin))
		m_proto->ClearAccessToken();
	m_pwszOldLogin = pwszNewLogin;
}

////////////////////// Options ///////////////////////////////////////////////

int CVkProto::OnOptionsInit(WPARAM wParam, LPARAM)
{
	OPTIONSDIALOGPAGE odp = { 0 };
	odp.hInstance = hInst;
	odp.pwszTitle = m_tszUserName;
	odp.dwInitParam = LPARAM(this);
	odp.flags = ODPF_BOLDGROUPS | ODPF_UNICODE | ODPF_DONTTRANSLATE;
	odp.pwszGroup = LPGENW("Network");

	odp.pwszTab = LPGENW("Account");
	odp.position = 1;
	odp.pDialog = new CVkOptionAccountForm(this);
	Options_AddPage(wParam, &odp);

	odp.pwszTab = LPGENW("Advanced");
	odp.position = 2;
	odp.pDialog = new CVkOptionAdvancedForm(this);
	Options_AddPage(wParam, &odp);

	odp.pwszTab = LPGENW("News and notifications");
	odp.position = 3;
	odp.pDialog = new CVkOptionFeedsForm(this);
	Options_AddPage(wParam, &odp);

	odp.pwszTab = LPGENW("View");
	odp.position = 4;
	odp.pDialog = new CVkOptionViewForm(this);
	Options_AddPage(wParam, &odp);

	odp.pwszTab = LPGENW("Menu");
	odp.position = 5;
	odp.pDialog = new CVkOptionMenuForm(this);
	Options_AddPage(wParam, &odp);

	return 0;
}

////////////////////// Account page //////////////////////////////////////////

static CVKLang vkLangCodes[] = {
	{ NULL, LPGENW("Account language") },
	{ L"en", LPGENW("English") },
	{ L"ru", LPGENW("Russian") },
	{ L"be", LPGENW("Belarusian") },
	{ L"ua", LPGENW("Ukrainian") },
	{ L"es", LPGENW("Spanish") },
	{ L"fi", LPGENW("Finnish") },
	{ L"de", LPGENW("German") },
	{ L"it", LPGENW("Italian") },
};

static CVKSync vkHistorySyncMethods[] = 
{
	{ TranslateT("off"), SyncHistoryMetod::syncOff },
	{ TranslateT("automatically"), SyncHistoryMetod::syncAuto },
	{ TranslateT("for last 1 day"), SyncHistoryMetod::sync1Days },
	{ TranslateT("for last 3 days"), SyncHistoryMetod::sync3Days }
};

static CVKMarkMsgRead vkMarkMsgAsReadMethods[] =
{
	{ TranslateT("on read"), MarkMsgReadOn::markOnRead },
	{ TranslateT("on receive"), MarkMsgReadOn::markOnReceive },
	{ TranslateT("on reply"), MarkMsgReadOn::markOnReply },
	{ TranslateT("on typing"), MarkMsgReadOn::markOnTyping }
};

CVkOptionAccountForm::CVkOptionAccountForm(CVkProto *proto):
	CVkDlgBase(proto, IDD_OPT_MAIN, false),
	m_edtLogin(this, IDC_LOGIN),
	m_edtPassword(this, IDC_PASSWORD),
	m_hlLink(this, IDC_URL, "https://vk.com/"),
	m_edtGroupName(this, IDC_GROUPNAME),
	m_cbxVKLang(this, IDC_COMBO_LANGUAGE),
	m_cbDelivery(this, IDC_DELIVERY),
	m_cbUseLocalTime(this, IDC_USE_LOCAL_TIME),
	m_cbAutoClean(this, IDC_AUTOCLEAN),
	m_cbxMarkAsRead(this, IDC_COMBO_MARKASREAD),
	m_cbxSyncHistory(this, IDC_COMBO_SYNCHISTORY)
{
	CreateLink(m_edtLogin, "Login", L"");
	CreateLink(m_edtGroupName, m_proto->m_vkOptions.pwszDefaultGroup);
	CreateLink(m_cbDelivery, m_proto->m_vkOptions.bServerDelivery);
	CreateLink(m_cbUseLocalTime, m_proto->m_vkOptions.bUseLocalTime);
	CreateLink(m_cbAutoClean, m_proto->m_vkOptions.bAutoClean);
}

void CVkOptionAccountForm::OnInitDialog()
{
	m_pwszOldLogin = m_edtLogin.GetText();
	m_edtLogin.SendMsg(EM_LIMITTEXT, 1024, 0);

	m_pwszOldPass = m_proto->GetUserStoredPassword();
	m_edtPassword.SetText(m_pwszOldPass);
	m_edtPassword.SendMsg(EM_LIMITTEXT, 1024, 0);

	m_pwszOldGroup = m_edtGroupName.GetText();

	for (size_t i = 0; i < _countof(vkMarkMsgAsReadMethods); i++) {
		int cur = m_cbxMarkAsRead.AddString(vkMarkMsgAsReadMethods[i].type, vkMarkMsgAsReadMethods[i].data);
		if (vkMarkMsgAsReadMethods[i].data == m_proto->m_vkOptions.iMarkMessageReadOn)
			m_cbxMarkAsRead.SetCurSel(cur);
	}

	for (size_t i = 0; i < _countof(vkHistorySyncMethods); i++)	{
		int cur = m_cbxSyncHistory.AddString(vkHistorySyncMethods[i].type, vkHistorySyncMethods[i].data);
		if (vkHistorySyncMethods[i].data == m_proto->m_vkOptions.iSyncHistoryMetod)
			m_cbxSyncHistory.SetCurSel(cur);
	}
	
	for (size_t i = 0; i < _countof(vkLangCodes); i++) {
		int cur = m_cbxVKLang.AddString(TranslateW(vkLangCodes[i].szDescription), (LPARAM)vkLangCodes[i].szCode);
		if (!mir_wstrcmpi(vkLangCodes[i].szCode, m_proto->m_vkOptions.pwszVKLang))
			m_cbxVKLang.SetCurSel(cur);
	}
	
}

void CVkOptionAccountForm::OnApply()
{
	m_proto->m_vkOptions.iSyncHistoryMetod = m_cbxSyncHistory.GetItemData(m_cbxSyncHistory.GetCurSel());
	m_proto->m_vkOptions.iMarkMessageReadOn = m_cbxMarkAsRead.GetItemData(m_cbxMarkAsRead.GetCurSel());
	m_proto->m_vkOptions.pwszVKLang = (wchar_t *)m_cbxVKLang.GetItemData(m_cbxVKLang.GetCurSel());
	
	ptrW pwszGroupName(m_edtGroupName.GetText());
	if (mir_wstrcmp(m_pwszOldGroup, pwszGroupName)) {
		Clist_GroupCreate(NULL, pwszGroupName);
		m_pwszOldGroup = pwszGroupName;
	}

	pass_ptrW pwszNewPass(m_edtPassword.GetText());
	bool bPassChanged = mir_wstrcmp(m_pwszOldPass, pwszNewPass) != 0;
	if (bPassChanged) {
		T2Utf szRawPasswd(pwszNewPass);
		m_proto->setString("Password", szRawPasswd);
		pass_ptrA pszPass(szRawPasswd.detach());
		m_pwszOldPass = pwszNewPass;
	}

	ptrW pwszNewLogin(m_edtLogin.GetText());
	if (bPassChanged || mir_wstrcmpi(m_pwszOldLogin, pwszNewLogin))
		m_proto->ClearAccessToken();
	m_pwszOldLogin = pwszNewLogin;
	
}

////////////////////// Advanced page /////////////////////////////////////////

CVkOptionAdvancedForm::CVkOptionAdvancedForm(CVkProto *proto):
	CVkDlgBase(proto, IDD_OPT_ADV, false),
	m_cbHideChats(this, IDC_HIDECHATS),
	m_cbSyncReadMessageStatusFromServer(this, IDC_SYNC_MSG_STATUS),
	m_cbMesAsUnread(this, IDC_MESASUREAD),
	m_cbForceInvisibleStatus(this, IDC_FORCE_ONLINE_ON_ACT),
	m_edtInvInterval(this, IDC_ED_INT_INVIS),
	m_spInvInterval(this, IDC_SPIN_INT_INVIS),
	m_cbSendVKLinksAsAttachments(this, IDC_SENDVKURLSASATTACH),
	m_cbLoadSentAttachments(this, IDC_LOADSENTATTACH),
	m_cbUseNonStandardUrlEncode(this, IDC_USENOSTDURLENCODE),
	m_cbReportAbuse(this, IDC_REPORT_ABUSE),
	m_cbClearServerHistory(this, IDC_CLEAR_SERVER_HISTORY),
	m_cbRemoveFromFrendlist(this, IDC_REMOVE_FROM_FRENDLIST),
	m_cbRemoveFromCList(this, IDC_REMOVE_FROM_CLIST),
	m_cbMusicSendOff(this, IDC_SEND_MUSIC_NONE),
	m_cbMusicSendBroadcastAndStatus(this, IDC_SEND_MUSIC_BROADCAST_AND_STATUS),
	m_cbSendMetodBroadcast(this, IDC_SEND_MUSIC_BROADCAST),
	m_cbMusicSendStatus(this, IDC_SEND_MUSIC_STATUS),
	m_edtReturnChatMessage(this, IDC_RET_CHAT_MES)
{
	CreateLink(m_cbHideChats, m_proto->m_vkOptions.bHideChats);
	CreateLink(m_cbSyncReadMessageStatusFromServer, m_proto->m_vkOptions.bSyncReadMessageStatusFromServer);
	CreateLink(m_cbMesAsUnread, m_proto->m_vkOptions.bMesAsUnread);
	CreateLink(m_cbForceInvisibleStatus, m_proto->m_vkOptions.bUserForceInvisibleOnActivity);
	CreateLink(m_edtInvInterval, m_proto->m_vkOptions.iInvisibleInterval);
	CreateLink(m_cbSendVKLinksAsAttachments, m_proto->m_vkOptions.bSendVKLinksAsAttachments);
	CreateLink(m_cbLoadSentAttachments, m_proto->m_vkOptions.bLoadSentAttachments);
	CreateLink(m_cbUseNonStandardUrlEncode, m_proto->m_vkOptions.bUseNonStandardUrlEncode);
	CreateLink(m_cbReportAbuse, m_proto->m_vkOptions.bReportAbuse);
	CreateLink(m_cbClearServerHistory, m_proto->m_vkOptions.bClearServerHistory);
	CreateLink(m_cbRemoveFromFrendlist, m_proto->m_vkOptions.bRemoveFromFrendlist);
	CreateLink(m_cbRemoveFromCList, m_proto->m_vkOptions.bRemoveFromCList);
	CreateLink(m_edtReturnChatMessage, m_proto->m_vkOptions.pwszReturnChatMessage);

	m_cbForceInvisibleStatus.OnChange = Callback(this, &CVkOptionAdvancedForm::On_cbForceInvisibleStatusChange);
	m_cbSendVKLinksAsAttachments.OnChange = Callback(this, &CVkOptionAdvancedForm::On_cbSendVKLinksAsAttachmentsChange);
}

void CVkOptionAdvancedForm::OnInitDialog()
{
	m_cbMusicSendOff.SetState(m_proto->m_vkOptions.iMusicSendMetod == MusicSendMetod::sendNone);
	m_cbMusicSendBroadcastAndStatus.SetState(m_proto->m_vkOptions.iMusicSendMetod == MusicSendMetod::sendBroadcastAndStatus);
	m_cbSendMetodBroadcast.SetState(m_proto->m_vkOptions.iMusicSendMetod == MusicSendMetod::sendBroadcastOnly);
	m_cbMusicSendStatus.SetState(m_proto->m_vkOptions.iMusicSendMetod == MusicSendMetod::sendStatusOnly);

	m_spInvInterval.SendMsg(UDM_SETRANGE, 0, MAKELONG(15, 0));
	m_spInvInterval.SendMsg(UDM_SETPOS, 0, m_proto->m_vkOptions.iInvisibleInterval);

	On_cbForceInvisibleStatusChange(&m_cbForceInvisibleStatus);
	On_cbSendVKLinksAsAttachmentsChange(&m_cbSendVKLinksAsAttachments);
}

void CVkOptionAdvancedForm::OnApply()
{
	if (m_cbMusicSendOff.GetState())
		m_proto->m_vkOptions.iMusicSendMetod = MusicSendMetod::sendNone;
	if (m_cbMusicSendBroadcastAndStatus.GetState())
		m_proto->m_vkOptions.iMusicSendMetod = MusicSendMetod::sendBroadcastAndStatus;
	if (m_cbSendMetodBroadcast.GetState())
		m_proto->m_vkOptions.iMusicSendMetod = MusicSendMetod::sendBroadcastOnly;
	if (m_cbMusicSendStatus.GetState())
		m_proto->m_vkOptions.iMusicSendMetod = MusicSendMetod::sendStatusOnly;

	if (m_cbSendVKLinksAsAttachments.GetState() == 0)
		m_proto->m_vkOptions.bLoadSentAttachments = false;
}

void CVkOptionAdvancedForm::On_cbForceInvisibleStatusChange(CCtrlCheck *)
{
	bool bEnable = m_cbForceInvisibleStatus.GetState() != 0;

	m_edtInvInterval.Enable(bEnable);
	m_spInvInterval.Enable(bEnable);
}

void CVkOptionAdvancedForm::On_cbSendVKLinksAsAttachmentsChange(CCtrlCheck *)
{
	m_cbLoadSentAttachments.Enable(m_cbSendVKLinksAsAttachments.GetState());
}

////////////////////// News and notifications ////////////////////////////////

CVkOptionFeedsForm::CVkOptionFeedsForm(CVkProto *proto):
	CVkDlgBase(proto, IDD_OPT_FEEDS, false),
	m_cbNewsEnabled(this, IDC_NEWS_ENBL),
	m_edtNewsInterval(this, IDC_ED_INT_NEWS),
	m_spNewsInterval(this, IDC_SPIN_INT_NEWS),
	m_cbNotificationsEnabled(this, IDC_NOTIF_ENBL),
	m_edtNotificationsInterval(this, IDC_ED_INT_NOTIF),
	m_spNotificationsInterval(this, IDC_SPIN_INT_NOTIF),
	m_cbNotificationsMarkAsViewed(this, IDC_NOTIF_MARK_VIEWED),
	m_cbSpecialContactAlwaysEnabled(this, IDC_SPEC_CONT_ENBL),
	m_cbNewsAutoClearHistory(this, IDC_NEWSAUTOCLEAR),
	m_cbNewsFilterPosts(this, IDC_F_POSTS),
	m_cbNewsFilterPhotos(this, IDC_F_PHOTOS),
	m_cbNewsFilterTags(this, IDC_F_TAGS),
	m_cbNewsFilterWallPhotos(this, IDC_F_WALLPHOTOS),
	m_cbNewsSourceFriends(this, IDC_S_FRIENDS),
	m_cbNewsSourceGroups(this, IDC_S_GROUPS),
	m_cbNewsSourcePages(this, IDC_S_PAGES),
	m_cbNewsSourceFollowing(this, IDC_S_FOLLOWING),
	m_cbNewsSourceIncludeBanned(this, IDC_S_BANNED),
	m_cbNewsSourceNoReposts(this, IDC_S_NOREPOSTES),
	m_cbNotificationFilterComments(this, IDC_N_COMMENTS),
	m_cbNotificationFilterLikes(this, IDC_N_LIKES),
	m_cbNotificationFilterReposts(this, IDC_N_REPOSTS),
	m_cbNotificationFilterMentions(this, IDC_N_MENTIONS),
	m_cbNotificationFilterInvites(this, IDC_N_INVITES),
	m_cbNotificationFilterAcceptedFriends(this, IDC_N_FRIENDACCEPTED)
{
	CreateLink(m_cbNewsEnabled, m_proto->m_vkOptions.bNewsEnabled);
	CreateLink(m_edtNewsInterval, m_proto->m_vkOptions.iNewsInterval);
	CreateLink(m_cbNotificationsEnabled, m_proto->m_vkOptions.bNotificationsEnabled);
	CreateLink(m_edtNotificationsInterval, m_proto->m_vkOptions.iNotificationsInterval);
	CreateLink(m_cbNotificationsMarkAsViewed, m_proto->m_vkOptions.bNotificationsMarkAsViewed);
	CreateLink(m_cbSpecialContactAlwaysEnabled, m_proto->m_vkOptions.bSpecialContactAlwaysEnabled);
	CreateLink(m_cbNewsAutoClearHistory, m_proto->m_vkOptions.bNewsAutoClearHistory);
	CreateLink(m_cbNewsFilterPosts, m_proto->m_vkOptions.bNewsFilterPosts);
	CreateLink(m_cbNewsFilterPhotos, m_proto->m_vkOptions.bNewsFilterPhotos);
	CreateLink(m_cbNewsFilterTags, m_proto->m_vkOptions.bNewsFilterTags);
	CreateLink(m_cbNewsFilterWallPhotos, m_proto->m_vkOptions.bNewsFilterWallPhotos);
	CreateLink(m_cbNewsSourceFriends, m_proto->m_vkOptions.bNewsSourceFriends);
	CreateLink(m_cbNewsSourceGroups, m_proto->m_vkOptions.bNewsSourceGroups);
	CreateLink(m_cbNewsSourcePages, m_proto->m_vkOptions.bNewsSourcePages);
	CreateLink(m_cbNewsSourceFollowing, m_proto->m_vkOptions.bNewsSourceFollowing);
	CreateLink(m_cbNewsSourceIncludeBanned, m_proto->m_vkOptions.bNewsSourceIncludeBanned);
	CreateLink(m_cbNewsSourceNoReposts, m_proto->m_vkOptions.bNewsSourceNoReposts);
	CreateLink(m_cbNotificationFilterComments, m_proto->m_vkOptions.bNotificationFilterComments);
	CreateLink(m_cbNotificationFilterLikes, m_proto->m_vkOptions.bNotificationFilterLikes);
	CreateLink(m_cbNotificationFilterReposts, m_proto->m_vkOptions.bNotificationFilterReposts);
	CreateLink(m_cbNotificationFilterMentions, m_proto->m_vkOptions.bNotificationFilterMentions);
	CreateLink(m_cbNotificationFilterInvites, m_proto->m_vkOptions.bNotificationFilterInvites);
	CreateLink(m_cbNotificationFilterAcceptedFriends, m_proto->m_vkOptions.bNotificationFilterAcceptedFriends);

	m_cbNewsEnabled.OnChange = Callback(this, &CVkOptionFeedsForm::On_cbNewsEnabledChange);
	m_cbNotificationsEnabled.OnChange = Callback(this, &CVkOptionFeedsForm::On_cbNotificationsEnabledChange);

}

void CVkOptionFeedsForm::OnInitDialog()
{
	m_spNewsInterval.SendMsg(UDM_SETRANGE, 0, MAKELONG(60 * 24, 1));
	m_spNewsInterval.SendMsg(UDM_SETPOS, 0, m_proto->m_vkOptions.iNewsInterval);

	m_spNotificationsInterval.SendMsg(UDM_SETRANGE, 0, MAKELONG(60 * 24, 1));
	m_spNotificationsInterval.SendMsg(UDM_SETPOS, 0, m_proto->m_vkOptions.iNotificationsInterval);

	On_cbNewsEnabledChange(&m_cbNewsEnabled);
	On_cbNotificationsEnabledChange(&m_cbNotificationsEnabled);
}

void CVkOptionFeedsForm::On_cbNewsEnabledChange(CCtrlCheck*)
{
	bool bState = m_cbNewsEnabled.GetState() != 0;

	m_edtNewsInterval.Enable(bState);
	m_spNewsInterval.Enable(bState);
	m_cbNewsFilterPosts.Enable(bState);
	m_cbNewsFilterPhotos.Enable(bState);
	m_cbNewsFilterTags.Enable(bState);
	m_cbNewsFilterWallPhotos.Enable(bState);
	m_cbNewsSourceFriends.Enable(bState);
	m_cbNewsSourceGroups.Enable(bState);
	m_cbNewsSourcePages.Enable(bState);
	m_cbNewsSourceFollowing.Enable(bState);
	m_cbNewsSourceIncludeBanned.Enable(bState);
	m_cbNewsSourceNoReposts.Enable(bState);

}

void CVkOptionFeedsForm::On_cbNotificationsEnabledChange(CCtrlCheck*)
{
	bool bState = m_cbNotificationsEnabled.GetState() != 0;

	m_cbNotificationsMarkAsViewed.Enable(bState);
	m_edtNotificationsInterval.Enable(bState);
	m_spNotificationsInterval.Enable(bState);
	m_cbNotificationFilterComments.Enable(bState);
	m_cbNotificationFilterLikes.Enable(bState);
	m_cbNotificationFilterReposts.Enable(bState);
	m_cbNotificationFilterMentions.Enable(bState);
	m_cbNotificationFilterInvites.Enable(bState);
}

////////////////////// View page /////////////////////////////////////////////

CVkOptionViewForm::CVkOptionViewForm(CVkProto *proto):
	CVkDlgBase(proto, IDD_OPT_VIEW, false),
	m_cbIMGBBCSupportOff(this, IDC_IMG_OFF),
	m_cbIMGBBCSupportFullSize(this, IDC_IMG_FULLSIZE),
	m_cbIMGBBCSupport130(this, IDC_IMG_130),
	m_cbIMGBBCSupport604(this, IDC_IMG_604),
	m_cbBBCForNewsOff(this, IDC_NEWSBBC_OFF),
	m_cbBBCForNewsBasic(this, IDC_NEWSBBC_BASIC),
	m_cbBBCForNewsAdvanced(this, IDC_NEWSBBC_ADV),
	m_cbUseBBCOnAttacmentsAsNews(this, IDC_BBC_ATT_NEWS),
	m_cbBBCForAttachmentsOff(this, IDC_ATTBBC_OFF),
	m_cbBBCForAttachmentsBasic(this, IDC_ATTBBC_BASIC),
	m_cbBBCForAttachmentsAdvanced(this, IDC_ATTBBC_ADV),
	m_cbStikersAsSmyles(this, IDC_STICKERS_AS_SMYLES),
	m_cbShortenLinksForAudio(this, IDC_SHOTEN_LINKS_FOR_AUDIO),
	m_cbUseNonStandardNotifications(this, IDC_USENOSTDPOPUPS)
{
	CreateLink(m_cbUseBBCOnAttacmentsAsNews, m_proto->m_vkOptions.bUseBBCOnAttacmentsAsNews);
	CreateLink(m_cbStikersAsSmyles, m_proto->m_vkOptions.bStikersAsSmyles);
	CreateLink(m_cbShortenLinksForAudio, m_proto->m_vkOptions.bShortenLinksForAudio);
	CreateLink(m_cbUseNonStandardNotifications, m_proto->m_vkOptions.bUseNonStandardNotifications);
}

void CVkOptionViewForm::OnInitDialog()
{
	m_cbIMGBBCSupportOff.SetState(m_proto->m_vkOptions.iIMGBBCSupport == IMGBBCSypport::imgNo);
	m_cbIMGBBCSupportFullSize.SetState(m_proto->m_vkOptions.iIMGBBCSupport == IMGBBCSypport::imgFullSize);
	m_cbIMGBBCSupport130.SetState(m_proto->m_vkOptions.iIMGBBCSupport == IMGBBCSypport::imgPreview130);
	m_cbIMGBBCSupport604.SetState(m_proto->m_vkOptions.iIMGBBCSupport == IMGBBCSypport::imgPreview604);

	m_cbBBCForNewsOff.SetState(m_proto->m_vkOptions.iBBCForNews == BBCSupport::bbcNo);
	m_cbBBCForNewsBasic.SetState(m_proto->m_vkOptions.iBBCForNews == BBCSupport::bbcBasic);
	m_cbBBCForNewsAdvanced.SetState(m_proto->m_vkOptions.iBBCForNews == BBCSupport::bbcAdvanced);
	
	m_cbBBCForAttachmentsOff.SetState(m_proto->m_vkOptions.iBBCForAttachments == BBCSupport::bbcNo);
	m_cbBBCForAttachmentsBasic.SetState(m_proto->m_vkOptions.iBBCForAttachments == BBCSupport::bbcBasic);
	m_cbBBCForAttachmentsAdvanced.SetState(m_proto->m_vkOptions.iBBCForAttachments == BBCSupport::bbcAdvanced);
}

void CVkOptionViewForm::OnApply()
{
	if (m_cbIMGBBCSupportOff.GetState())
		m_proto->m_vkOptions.iIMGBBCSupport = IMGBBCSypport::imgNo;
	if (m_cbIMGBBCSupportFullSize.GetState())
		m_proto->m_vkOptions.iIMGBBCSupport = IMGBBCSypport::imgFullSize;
	if (m_cbIMGBBCSupport130.GetState())
		m_proto->m_vkOptions.iIMGBBCSupport = IMGBBCSypport::imgPreview130;
	if (m_cbIMGBBCSupport604.GetState())
		m_proto->m_vkOptions.iIMGBBCSupport = IMGBBCSypport::imgPreview604;

	if (m_cbBBCForNewsOff.GetState())
		m_proto->m_vkOptions.iBBCForNews = BBCSupport::bbcNo;
	if (m_cbBBCForNewsBasic.GetState())
		m_proto->m_vkOptions.iBBCForNews = BBCSupport::bbcBasic;
	if (m_cbBBCForNewsAdvanced.GetState())
		m_proto->m_vkOptions.iBBCForNews = BBCSupport::bbcAdvanced;

	if (m_cbBBCForAttachmentsOff.GetState())
		m_proto->m_vkOptions.iBBCForAttachments = BBCSupport::bbcNo;
	if (m_cbBBCForAttachmentsBasic.GetState())
		m_proto->m_vkOptions.iBBCForAttachments = BBCSupport::bbcBasic;
	if (m_cbBBCForAttachmentsAdvanced.GetState())
		m_proto->m_vkOptions.iBBCForAttachments = BBCSupport::bbcAdvanced;
}

////////////////////// Menu page /////////////////////////////////////////////

CVkOptionMenuForm::CVkOptionMenuForm(CVkProto *proto):
	CVkDlgBase(proto, IDD_OPT_MENU, false),
	m_cbMenuEnabled0(this, IDC_SHOW_MENU0),
	m_cbMenuEnabled1(this, IDC_SHOW_MENU1),
	m_cbMenuEnabled2(this, IDC_SHOW_MENU2),
	m_cbMenuEnabled3(this, IDC_SHOW_MENU3),
	m_cbMenuEnabled4(this, IDC_SHOW_MENU4),
	m_cbMenuEnabled5(this, IDC_SHOW_MENU5),
	m_cbMenuEnabled6(this, IDC_SHOW_MENU6)
{
	CreateLink(m_cbMenuEnabled0, m_proto->m_vkOptions.bShowProtoMenuItem0);
	CreateLink(m_cbMenuEnabled1, m_proto->m_vkOptions.bShowProtoMenuItem1);
	CreateLink(m_cbMenuEnabled2, m_proto->m_vkOptions.bShowProtoMenuItem2);
	CreateLink(m_cbMenuEnabled3, m_proto->m_vkOptions.bShowProtoMenuItem3);
	CreateLink(m_cbMenuEnabled4, m_proto->m_vkOptions.bShowProtoMenuItem4);
	CreateLink(m_cbMenuEnabled5, m_proto->m_vkOptions.bShowProtoMenuItem5);
	CreateLink(m_cbMenuEnabled6, m_proto->m_vkOptions.bShowProtoMenuItem6);
}

void CVkOptionMenuForm::OnApply()
{
	if (MessageBox(NULL, 
		TranslateT("These changes will take effect after Miranda NG restart.\nWould you like to restart it now?"),
		TranslateT("VKontakte protocol"), MB_ICONWARNING | MB_YESNO | MB_DEFBUTTON2) == IDYES) 
		CallServiceSync(MS_SYSTEM_RESTART, 1,  0);
}


