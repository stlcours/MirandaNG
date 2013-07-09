/*
 * $Id: services.cpp 14142 2012-03-09 21:11:27Z george.hazan $
 *
 * myYahoo Miranda Plugin 
 *
 * Authors: Gennady Feldman (aka Gena01) 
 *          Laurent Marechal (aka Peorth)
 *
 * This code is under GPL and is based on AIM, MSN and Miranda source code.
 * I want to thank Robert Rainwater and George Hazan for their code and support
 * and for answering some of my questions during development of this plugin.
 */

#include "yahoo.h"

#include <m_protomod.h>
#include <m_protosvc.h>
#include <m_langpack.h>
#include <m_skin.h>
#include <m_utils.h>
#include <m_options.h>
#include <m_popup.h>

#include "avatar.h"
#include "resource.h"
#include "file_transfer.h"
#include "im.h"
#include "search.h"

void CYahooProto::logoff_buddies()
{
	//set all contacts to 'offline'
	for (HANDLE hContact = db_find_first(m_szModuleName); hContact; hContact = db_find_next(hContact, m_szModuleName)) {
		setWord( hContact, "Status", ID_STATUS_OFFLINE );
		setDword(hContact, "IdleTS", 0);
		setDword(hContact, "PictLastCheck", 0);
		setDword(hContact, "PictLoading", 0);
		db_unset(hContact, "CList", "StatusMsg");
		db_unset(hContact, m_szModuleName, "YMsg");
		db_unset(hContact, m_szModuleName, "YGMsg");
	}	
}

//=======================================================
//Broadcast the user status
//=======================================================
void CYahooProto::BroadcastStatus(int s)
{
	int oldStatus = m_iStatus;
	if (oldStatus == s)
		return;

	//m_iStatus = s;
	switch (s) {
	case ID_STATUS_OFFLINE:
	case ID_STATUS_CONNECTING:
	case ID_STATUS_ONLINE:
	case ID_STATUS_AWAY:
	case ID_STATUS_NA:
	case ID_STATUS_OCCUPIED:
	case ID_STATUS_ONTHEPHONE:
	case ID_STATUS_OUTTOLUNCH:
	case ID_STATUS_INVISIBLE:
		m_iStatus = s;
		break;
	case ID_STATUS_DND:
		m_iStatus = ID_STATUS_OCCUPIED;
		break;
	default:
		m_iStatus = ID_STATUS_ONLINE;
	}

	DebugLog("[yahoo_util_broadcaststatus] Old Status: %s (%d), New Status: %s (%d)",
		NEWSTR_ALLOCA((char *) CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, oldStatus, 0)), oldStatus,
		NEWSTR_ALLOCA((char *) CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, m_iStatus, 0)), m_iStatus);	
	ProtoBroadcastAck(NULL, ACKTYPE_STATUS, ACKRESULT_SUCCESS, (HANDLE) oldStatus, (LPARAM)m_iStatus);
}

//=======================================================
//Contact deletion event
//=======================================================
int __cdecl CYahooProto::OnContactDeleted( WPARAM wParam, LPARAM lParam )
{
	DBVARIANT dbv;
	HANDLE hContact = (HANDLE) wParam;
	
	DebugLog("[YahooContactDeleted]");
	
	if ( !m_bLoggedIn )  {//should never happen for Yahoo contacts
		DebugLog("[YahooContactDeleted] We are not Logged On!!!");
		return 0;
	}

	// he is not a permanent contact!
	if (db_get_b(hContact, "CList", "NotOnList", 0) != 0) {
		DebugLog("[YahooContactDeleted] Not a permanent buddy!!!");
		return 0;
	}
	
	if (!getString(hContact, YAHOO_LOGINID, &dbv)) {
		DebugLog("[YahooContactDeleted] Removing %s", dbv.pszVal);
		remove_buddy(dbv.pszVal, getWord(hContact, "yprotoid", 0));
		
		db_free( &dbv );
	} else {
		DebugLog("[YahooContactDeleted] Can't retrieve contact Yahoo ID");
	}
	return 0;
}

//=======================================================
//Custom status message windows handling
//=======================================================
static INT_PTR CALLBACK DlgProcSetCustStat(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	DBVARIANT dbv;

	switch (msg) {
	case WM_INITDIALOG:
		TranslateDialogDefault( hwndDlg );
		{
			CYahooProto* ppro = ( CYahooProto* )lParam;
			SetWindowLongPtr( hwndDlg, GWLP_USERDATA, lParam );

			SendMessage( hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)ppro->LoadIconEx("yahoo", true ));
			SendMessage( hwndDlg, WM_SETICON, ICON_SMALL, (LPARAM)ppro->LoadIconEx("yahoo"));

			if ( !db_get_s( NULL, ppro->m_szModuleName, YAHOO_CUSTSTATDB, &dbv )) {
				SetDlgItemTextA( hwndDlg, IDC_CUSTSTAT, dbv. pszVal );

				EnableWindow( GetDlgItem( hwndDlg, IDOK ), lstrlenA(dbv.pszVal) > 0);
				db_free( &dbv );
			}
			else {
				SetDlgItemTextA( hwndDlg, IDC_CUSTSTAT, "");
				EnableWindow( GetDlgItem( hwndDlg, IDOK ), FALSE );
			}

			CheckDlgButton( hwndDlg, IDC_CUSTSTATBUSY,  ppro->getByte("BusyCustStat", 0));
		}
		return TRUE;

	case WM_COMMAND:
		switch(wParam) {
		case IDOK:
			{
				char str[ 255 + 1 ];
				CYahooProto* ppro = ( CYahooProto* )GetWindowLongPtr( hwndDlg, GWLP_USERDATA );

				/* Get String from dialog */
				GetDlgItemTextA( hwndDlg, IDC_CUSTSTAT, str, sizeof( str ));

				/* Save it for later use */
				ppro->setString( YAHOO_CUSTSTATDB, str );
				ppro->setByte("BusyCustStat", ( BYTE )IsDlgButtonChecked( hwndDlg, IDC_CUSTSTATBUSY ));

				/* set for Idle/AA */
				if (ppro->m_startMsg) mir_free(ppro->m_startMsg);
				ppro->m_startMsg = mir_strdup(str);

				/* notify Server about status change */
				ppro->set_status(YAHOO_CUSTOM_STATUS, str, ( BYTE )IsDlgButtonChecked( hwndDlg, IDC_CUSTSTATBUSY ));

				/* change local/miranda status */
				ppro->BroadcastStatus(( BYTE )IsDlgButtonChecked( hwndDlg, IDC_CUSTSTATBUSY ) ? ID_STATUS_AWAY : ID_STATUS_ONLINE);
			}
		case IDCANCEL:
			DestroyWindow( hwndDlg );
			break;
		}

		if ( HIWORD( wParam ) == EN_CHANGE && ( HWND )lParam == GetFocus()) {
			if (LOWORD( wParam ) == IDC_CUSTSTAT) {
				char str[ 255 + 1 ];

				BOOL toSet;

				toSet = GetDlgItemTextA( hwndDlg, IDC_CUSTSTAT, str, sizeof( str )) != 0;

				EnableWindow( GetDlgItem( hwndDlg, IDOK ), toSet );
			}			
		}
		break; /* case WM_COMMAND */

	case WM_CLOSE:
		DestroyWindow( hwndDlg );
		break;

	case WM_DESTROY:
		{
			CYahooProto* ppro = ( CYahooProto* )GetWindowLongPtr( hwndDlg, GWLP_USERDATA );
			ppro->ReleaseIconEx("yahoo", true);
			ppro->ReleaseIconEx("yahoo");
		}
		break;
	}
	return FALSE;
}

INT_PTR __cdecl CYahooProto::SetCustomStatCommand( WPARAM wParam, LPARAM lParam )
{
	if ( !m_bLoggedIn ) {
		ShowNotification( TranslateT("Yahoo Error"), TranslateT("You need to be connected to set the custom message"), NIIF_ERROR);
		return 0;
	}
	
	HWND hwndSetCustomStatus = CreateDialogParam(hInstance, MAKEINTRESOURCE( IDD_SETCUSTSTAT ), NULL, DlgProcSetCustStat, (LPARAM)this );
	SetForegroundWindow( hwndSetCustomStatus );
	SetFocus( hwndSetCustomStatus );
 	ShowWindow( hwndSetCustomStatus, SW_SHOW );
	return 0;
}

//=======================================================
//Open URL
//=======================================================
void CYahooProto::OpenURL(const char *url, int autoLogin)
{
	char tUrl[ 4096 ];

	DebugLog("[YahooOpenURL] url: %s Auto Login: %d", url, autoLogin);
	
	if (autoLogin && getByte("MailAutoLogin", 0) && m_bLoggedIn && m_id > 0) {
		char  *y, *t, *u;
		
		y = yahoo_urlencode(yahoo_get_cookie(m_id, "y"));
		t = yahoo_urlencode(yahoo_get_cookie(m_id, "t"));
		u = yahoo_urlencode(url);
		_snprintf( tUrl, sizeof( tUrl ), 
				"http://msg.edit.yahoo.com/config/reset_cookies?&.y=Y=%s&.t=T=%s&.ver=2&.done=http%%3a//us.rd.yahoo.com/messenger/client/%%3f%s",
				y, t, u);
				
		FREE(y);
		FREE(t);
		FREE(u);
	} else {
		_snprintf( tUrl, sizeof( tUrl ), url );
	}

	DebugLog("[YahooOpenURL] url: %s Final URL: %s", url, tUrl);
	
	CallService(MS_UTILS_OPENURL, TRUE, (LPARAM)tUrl);
}

//=======================================================
//Show buddy profile
//=======================================================
INT_PTR __cdecl CYahooProto::OnShowProfileCommand( WPARAM wParam, LPARAM lParam )
{
	char tUrl[ 4096 ];
	DBVARIANT dbv;

	/**
	 * We don't show profile for users using other IM clients through the IM server bridge
	 */
	if (getWord(( HANDLE )wParam, "yprotoid", 0) != 0) {
		return 0;
	}
	
	if (getString((HANDLE)wParam, YAHOO_LOGINID, &dbv))
		return 0;
		
	_snprintf( tUrl, sizeof( tUrl ), "http://profiles.yahoo.com/%s", dbv.pszVal  );
	db_free( &dbv );
	
	OpenURL(tUrl, 0);
	return 0;
}

INT_PTR __cdecl CYahooProto::OnEditMyProfile( WPARAM wParam, LPARAM lParam )
{
	OpenURL("http://edit.yahoo.com/config/eval_profile", 1);
	return 0;
}

//=======================================================
//Show My profile
//=======================================================
INT_PTR __cdecl CYahooProto::OnShowMyProfileCommand( WPARAM wParam, LPARAM lParam )
{
	DBVARIANT dbv;
	
	if ( getString( YAHOO_LOGINID, &dbv ) != 0)	{
		ShowError( TranslateT("Yahoo Error"), TranslateT("Please enter your yahoo id in Options/Network/Yahoo"));
		return 0;
	}

	char tUrl[ 4096 ];
	_snprintf( tUrl, sizeof( tUrl ), "http://profiles.yahoo.com/%s", dbv.pszVal  );
	db_free( &dbv );

	OpenURL(tUrl, 0);
	return 0;
}

//=======================================================
//Show Goto mailbox
//=======================================================
INT_PTR __cdecl CYahooProto::OnGotoMailboxCommand( WPARAM wParam, LPARAM lParam )
{
	if (getByte("YahooJapan", 0))
		OpenURL("http://mail.yahoo.co.jp/", 1);
	else
		OpenURL("http://mail.yahoo.com/", 1);
	
	return 0;
}

INT_PTR __cdecl CYahooProto::OnABCommand( WPARAM wParam, LPARAM lParam )
{
	OpenURL("http://address.yahoo.com/yab/", 1);
	return 0;
}

INT_PTR __cdecl CYahooProto::OnCalendarCommand( WPARAM wParam, LPARAM lParam )
{
	OpenURL("http://calendar.yahoo.com/", 1);		
	return 0;
}

//=======================================================
//Refresh Yahoo
//=======================================================
INT_PTR __cdecl CYahooProto::OnRefreshCommand( WPARAM wParam, LPARAM lParam )
{
	if ( !m_bLoggedIn ) {
		ShowNotification(TranslateT("Yahoo Error"), TranslateT("You need to be connected to refresh your buddy list"), NIIF_ERROR);
		return 0;
	}

	yahoo_refresh(m_id);
	return 0;
}

int __cdecl CYahooProto::OnIdleEvent(WPARAM wParam, LPARAM lParam)
{
	BOOL bIdle = (lParam & IDF_ISIDLE);

	DebugLog("[YAHOO_IDLE_EVENT] Idle: %s", bIdle ?"Yes":"No");

	if ( lParam & IDF_PRIVACY ) 
		return 0; /* we support Privacy settings */

	if (m_bLoggedIn) {
		/* set me to idle or back */
		if (m_iStatus == ID_STATUS_INVISIBLE)
			DebugLog("[YAHOO_IDLE_EVENT] WARNING: INVISIBLE! Don't change my status!");
		else
			set_status(m_iStatus,m_startMsg,(bIdle) ? 2 : (m_iStatus == ID_STATUS_ONLINE) ? 0 : 1);
	} else {
		DebugLog("[YAHOO_IDLE_EVENT] WARNING: NOT LOGGED IN???");
	}

	return 0;
}

INT_PTR __cdecl CYahooProto::GetUnreadEmailCount(WPARAM wParam, LPARAM lParam)
{
	if ( !m_bLoggedIn )
		return 0;

	return m_unreadMessages;
}

/////////////////////////////////////////////////////////////////////////////////////////

void CYahooProto::MenuMainInit( void )
{
	char servicefunction[ 100 ];
	lstrcpyA( servicefunction, m_szModuleName);
	char* tDest = servicefunction + lstrlenA( servicefunction );
	
	CLISTMENUITEM mi = { sizeof(mi) };
	mi.pszService = servicefunction;

	HGENMENU hRoot = MO_GetProtoRootMenu( m_szModuleName);
	if ( hRoot == NULL) {
		mi.position = 500015000;
		mi.hParentMenu = HGENMENU_ROOT;
		mi.flags = CMIF_ROOTPOPUP | CMIF_TCHAR | CMIF_KEEPUNTRANSLATED;
		mi.icolibItem = GetIconHandle( IDI_YAHOO );
		mi.ptszName = m_tszUserName;
		hRoot = mainMenuRoot = Menu_AddProtoMenuItem(&mi);
	}
	else {
		if (mainMenuRoot)
			CallService(MS_CLIST_REMOVEMAINMENUITEM, (WPARAM)mainMenuRoot, 0);
		mainMenuRoot = NULL;
	}
		
	mi.flags = CMIF_CHILDPOPUP;
	mi.hParentMenu = hRoot;
	
	// Show custom status menu    
	lstrcpyA( tDest, YAHOO_SET_CUST_STAT );
	CreateService( YAHOO_SET_CUST_STAT, &CYahooProto::SetCustomStatCommand );
	
	mi.position = 290000;
	mi.icolibItem = GetIconHandle( IDI_SET_STATUS );
	mi.pszName = LPGEN("Set &Custom Status");
	
	menuItemsAll[0] = Menu_AddProtoMenuItem(&mi);

	// Edit My profile
	lstrcpyA( tDest, YAHOO_EDIT_MY_PROFILE );
	CreateService( YAHOO_EDIT_MY_PROFILE, &CYahooProto::OnEditMyProfile );

	mi.position = 290005;
	mi.icolibItem = GetIconHandle( IDI_PROFILE );
	mi.pszName = LPGEN("&Edit My Profile");
	menuItemsAll[1] = Menu_AddProtoMenuItem(&mi);

	// Show My profile
	lstrcpyA( tDest, YAHOO_SHOW_MY_PROFILE );
	CreateService( YAHOO_SHOW_MY_PROFILE, &CYahooProto::OnShowMyProfileCommand );

	mi.position = 290006;
	mi.icolibItem = GetIconHandle( IDI_PROFILE );
	mi.pszName = LPGEN("&My Profile");
	menuItemsAll[2] = Menu_AddProtoMenuItem(&mi);

	// Show Yahoo mail 
	strcpy( tDest, YAHOO_YAHOO_MAIL );
	CreateService( YAHOO_YAHOO_MAIL, &CYahooProto::OnGotoMailboxCommand );

	mi.position = 290010;
	mi.icolibItem = GetIconHandle( IDI_INBOX );
	mi.pszName = LPGEN("&Yahoo Mail");
	menuItemsAll[3] = Menu_AddProtoMenuItem(&mi);

	// Show Address Book    
	strcpy( tDest, YAHOO_AB );
	CreateService( YAHOO_AB, &CYahooProto::OnABCommand );

	mi.position = 290015;
	mi.icolibItem = GetIconHandle( IDI_YAB );
	mi.pszName = LPGEN("&Address Book");
	menuItemsAll[4] = Menu_AddProtoMenuItem(&mi);

	// Show Calendar
	strcpy( tDest, YAHOO_CALENDAR );
	CreateService( YAHOO_CALENDAR, &CYahooProto::OnCalendarCommand );

	mi.position = 290017;
	mi.icolibItem = GetIconHandle( IDI_CALENDAR );
	mi.pszName = LPGEN("&Calendar");
	menuItemsAll[5] = Menu_AddProtoMenuItem(&mi);
	strcpy( tDest, "/JoinChatRoom");
	CreateService("/JoinChatRoom", &CYahooProto::CreateConference);

	mi.position = 290018;
	mi.icolibItem = GetIconHandle(IDI_YAHOO);
	mi.pszName = LPGEN("Create Conference");
	menuItemsAll[6] = Menu_AddProtoMenuItem(&mi);


	// Show Refresh     
	/*strcpy( tDest, YAHOO_REFRESH );
	CreateService( YAHOO_REFRESH, &CYahooProto::OnRefreshCommand );

	mi.position = 500090015;
	mi.icolibItem = GetIconHandle( IDI_REFRESH );
	mi.pszName = LPGEN("&Refresh");
	menuItemsAll[7] = Menu_AddProtoMenuItem(&mi);
	*/
}
	
void CYahooProto::MenuContactInit( void )
{
	char servicefunction[ 100 ];
	lstrcpyA( servicefunction, m_szModuleName);
	char* tDest = servicefunction + lstrlenA( servicefunction );
	
	CLISTMENUITEM mi = { sizeof(mi) };
	mi.pszService = servicefunction;
	mi.pszContactOwner = m_szModuleName;

	// Show Profile 
	strcpy( tDest, YAHOO_SHOW_PROFILE );
	CreateService( YAHOO_SHOW_PROFILE, &CYahooProto::OnShowProfileCommand );

	mi.position = -2000006000;
	mi.icolibItem = GetIconHandle( IDI_PROFILE );
	mi.pszName = LPGEN("&Show Profile");
	hShowProfileMenuItem = Menu_AddContactMenuItem(&mi);
	
}

void CYahooProto::MenuUninit( void )
{
	if ( mainMenuRoot )
		CallService(MS_CLIST_REMOVEMAINMENUITEM, (WPARAM)mainMenuRoot, 0);
	
	CallService(MS_CLIST_REMOVECONTACTMENUITEM, (WPARAM)hShowProfileMenuItem, 0);
}

int __cdecl CYahooProto::OnPrebuildContactMenu(WPARAM wParam, LPARAM)
{
	const HANDLE hContact = (HANDLE)wParam;
	if (!IsMyContact(hContact)) {
		DebugLog("[OnPrebuildContactMenu] Not a Yahoo Contact!!!");
		return 0;
	}

	Menu_ShowItem(hShowProfileMenuItem, getWord(hContact, "yprotoid", 0) == 0);
	return 0;
}


//=======================================================================================
// Load the yahoo service/plugin
//=======================================================================================
void CYahooProto::LoadYahooServices( void )
{
	char path[MAX_PATH];
	
	//----| Events hooking |--------------------------------------------------------------
	HookEvent( ME_OPT_INITIALISE, &CYahooProto::OnOptionsInit );

	//----| Create nudge event |----------------------------------------------------------
	mir_snprintf(path, SIZEOF(path), "%s/Nudge", m_szModuleName);
	hYahooNudge = CreateHookableEvent( path );

	//----| Service creation |------------------------------------------------------------
	CreateService( PS_CREATEACCMGRUI, &CYahooProto::SvcCreateAccMgrUI);
	
	CreateService( PS_GETAVATARINFOT, &CYahooProto::GetAvatarInfo );
	CreateService( PS_GETMYAVATART,   &CYahooProto::GetMyAvatar );
	CreateService( PS_SETMYAVATART,   &CYahooProto::SetMyAvatar );
	CreateService( PS_GETAVATARCAPS,  &CYahooProto::GetAvatarCaps );

	CreateService(	PS_GETMYAWAYMSG, &CYahooProto::GetMyAwayMsg);
	CreateService( YAHOO_SEND_NUDGE, &CYahooProto::SendNudge );
	
	CreateService( YAHOO_GETUNREAD_EMAILCOUNT, &CYahooProto::GetUnreadEmailCount);

	//----| Set resident variables |------------------------------------------------------
	
	db_set_resident(m_szModuleName, "Mobile");
	db_set_resident(m_szModuleName, "IdleTS");
	db_set_resident(m_szModuleName, "PictLastCheck");
	db_set_resident(m_szModuleName, "PictLoading");
	db_set_resident(m_szModuleName, "Status");

	db_set_resident(m_szModuleName, "YAway");
	db_set_resident(m_szModuleName, "YGMsg");
	db_set_resident(m_szModuleName, "YStatus");
}
