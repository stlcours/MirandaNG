////////////////////////////////////////////////////////////////////////////////
// Gadu-Gadu Plugin for Miranda IM
//
// Copyright (c) 2003-2009 Adam Strzelecki <ono+miranda@java.pl>
// Copyright (c) 2009-2012 Bartosz Bia�ek
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
////////////////////////////////////////////////////////////////////////////////

#ifndef GGPROTO_H
#define GGPROTO_H

struct GGPROTO;
typedef void    ( __cdecl GGPROTO::*GGThreadFunc )( void* );
typedef int     ( __cdecl GGPROTO::*GGEventFunc )( WPARAM, LPARAM );
typedef INT_PTR ( __cdecl GGPROTO::*GGServiceFunc )( WPARAM, LPARAM );

struct GGPROTO : public PROTO_INTERFACE, public MZeroedObject
{
				GGPROTO( const char*, const TCHAR* );
				~GGPROTO();

	//====================================================================================
	// PROTO_INTERFACE
	//====================================================================================

	virtual	HANDLE __cdecl AddToList( int flags, PROTOSEARCHRESULT* psr );
	virtual	HANDLE __cdecl AddToListByEvent( int flags, int iContact, HANDLE hDbEvent );

	virtual	int    __cdecl Authorize( HANDLE hDbEvent );
	virtual	int    __cdecl AuthDeny( HANDLE hDbEvent, const TCHAR* szReason );
	virtual	int    __cdecl AuthRecv( HANDLE hContact, PROTORECVEVENT* );
	virtual	int    __cdecl AuthRequest( HANDLE hContact, const TCHAR* szMessage );

	virtual	HANDLE __cdecl ChangeInfo( int iInfoType, void* pInfoData );

	virtual	HANDLE __cdecl FileAllow( HANDLE hContact, HANDLE hTransfer, const TCHAR* szPath );
	virtual	int    __cdecl FileCancel( HANDLE hContact, HANDLE hTransfer );
	virtual	int    __cdecl FileDeny( HANDLE hContact, HANDLE hTransfer, const TCHAR* szReason );
	virtual	int    __cdecl FileResume( HANDLE hTransfer, int* action, const TCHAR** szFilename );

	virtual	DWORD_PTR __cdecl GetCaps( int type, HANDLE hContact = NULL );
	virtual	int    __cdecl GetInfo( HANDLE hContact, int infoType );

	virtual	HANDLE __cdecl SearchBasic( const TCHAR* id );
	virtual	HANDLE __cdecl SearchByEmail( const TCHAR* email );
	virtual	HANDLE __cdecl SearchByName( const TCHAR* nick, const TCHAR* firstName, const TCHAR* lastName );
	virtual	HWND   __cdecl SearchAdvanced( HWND owner );
	virtual	HWND   __cdecl CreateExtendedSearchUI( HWND owner );

	virtual	int    __cdecl RecvContacts( HANDLE hContact, PROTORECVEVENT* );
	virtual	int    __cdecl RecvFile( HANDLE hContact, PROTORECVFILET* );
	virtual	int    __cdecl RecvMsg( HANDLE hContact, PROTORECVEVENT* );
	virtual	int    __cdecl RecvUrl( HANDLE hContact, PROTORECVEVENT* );

	virtual	int    __cdecl SendContacts( HANDLE hContact, int flags, int nContacts, HANDLE* hContactsList );
	virtual	HANDLE __cdecl SendFile( HANDLE hContact, const TCHAR* szDescription, TCHAR** ppszFiles );
	virtual	int    __cdecl SendMsg( HANDLE hContact, int flags, const char* msg );
	virtual	int    __cdecl SendUrl( HANDLE hContact, int flags, const char* url );

	virtual	int    __cdecl SetApparentMode( HANDLE hContact, int mode );
	virtual	int    __cdecl SetStatus( int iNewStatus );

	virtual	HANDLE __cdecl GetAwayMsg( HANDLE hContact );
	virtual	int    __cdecl RecvAwayMsg( HANDLE hContact, int mode, PROTORECVEVENT* evt );
	virtual	int    __cdecl SendAwayMsg( HANDLE hContact, HANDLE hProcess, const char* msg );
	virtual	int    __cdecl SetAwayMsg( int m_iStatus, const TCHAR* msg );

	virtual	int    __cdecl UserIsTyping( HANDLE hContact, int type );

	virtual	int    __cdecl OnEvent( PROTOEVENTTYPE eventType, WPARAM wParam, LPARAM lParam );

	//////////////////////////////////////////////////////////////////////////////////////
	//  Services

	INT_PTR  __cdecl blockuser(WPARAM wParam, LPARAM lParam);
	INT_PTR  __cdecl getmyawaymsg(WPARAM wParam, LPARAM lParam);
	INT_PTR  __cdecl get_acc_mgr_gui(WPARAM wParam, LPARAM lParam);
	INT_PTR  __cdecl leavechat(WPARAM wParam, LPARAM lParam);

	void     __cdecl sendackthread(void *);
	void     __cdecl searchthread(void *);
	void     __cdecl cmdgetinfothread(void *hContact);
	void     __cdecl getawaymsgthread(void *hContact);
	void     __cdecl dccmainthread(void *);
	void     __cdecl ftfailthread(void *param);
	void     __cdecl remindpasswordthread(void *param);

	//////////////////////////////////////////////////////////////////////////////////////

	/* Helper functions */
	int status_m2gg(int status, int descr);
	int status_gg2m(int status);
	void checknewuser(uin_t uin, const char* passwd);

	/* Thread functions */
	void forkthread(GGThreadFunc pFunc, void *param);
	HANDLE forkthreadex(GGThreadFunc pFunc, void *param, UINT *threadId);
	void threadwait(GGTHREAD *thread);

#ifdef DEBUGMODE
	volatile int extendedLogging;
#endif
	void gg_EnterCriticalSection(CRITICAL_SECTION* mutex, char* callingFunction, int sectionNumber, char* mutexName, int logging);
	void gg_LeaveCriticalSection(CRITICAL_SECTION* mutex, char* callingFunction, int sectionNumber, int returnNumber, char* mutexName, int logging);
	void gg_sleep(DWORD miliseconds, BOOL alterable, char* callingFunction, int sleepNumber, int logging);

	/* Global GG functions */
	void notifyuser(HANDLE hContact, int refresh);
	void setalloffline();
	void disconnect();
	HANDLE getcontact(uin_t uin, int create, int inlist, TCHAR *nick);
	void __cdecl mainthread(void *empty);
	int isonline();
	int refreshstatus(int status);

	void broadcastnewstatus(int newStatus);
	void cleanuplastplugin(DWORD version);
	int contactdeleted(WPARAM wParam, LPARAM lParam);
	int dbsettingchanged(WPARAM wParam, LPARAM lParam);
	void notifyall();
	void changecontactstatus(uin_t uin, int status, const TCHAR *idescr, int time, uint32_t remote_ip, uint16_t remote_port, uint32_t version);
	TCHAR *getstatusmsg(int status);
	void dccstart();
	void dccconnect(uin_t uin);
	int gettoken(GGTOKEN *token);
	void parsecontacts(char *contacts);
	void remindpassword(uin_t uin, const char *email);
	void menus_init();

	/* Avatar functions */
	void getAvatarFilename(HANDLE hContact, TCHAR *pszDest, int cbLen);
	void requestAvatarTransfer(HANDLE hContact, char *szAvatarURL);
	void requestAvatarInfo(HANDLE hContact, int iWaitFor);
	void getUserAvatar();
	void setAvatar(const TCHAR *szFilename);
	void getAvatarFileInfo(uin_t uin, char **avatarurl, char **avatarts);

	INT_PTR  __cdecl getavatarcaps(WPARAM wParam, LPARAM lParam);
	INT_PTR  __cdecl getavatarinfo(WPARAM wParam, LPARAM lParam);
	INT_PTR  __cdecl getmyavatar(WPARAM wParam, LPARAM lParam);
	INT_PTR  __cdecl setmyavatar(WPARAM wParam, LPARAM lParam);

	void initavatarrequestthread();
	void uninitavatarrequestthread();

	void     __cdecl avatarrequestthread(void*);
	void     __cdecl getuseravatarthread(void*);
	void     __cdecl setavatarthread(void*);

	/* File transfer functions */
	HANDLE fileallow(HANDLE hContact, HANDLE hTransfer, const PROTOCHAR* szPath);
	int filecancel(HANDLE hContact, HANDLE hTransfer);
	int filedeny(HANDLE hContact, HANDLE hTransfer, const PROTOCHAR* szReason);
	int recvfile(HANDLE hContact, PROTOFILEEVENT* pre);
	HANDLE sendfile(HANDLE hContact, const PROTOCHAR* szDescription, PROTOCHAR** ppszFiles);

	HANDLE dccfileallow(HANDLE hTransfer, const PROTOCHAR* szPath);
	HANDLE dcc7fileallow(HANDLE hTransfer, const PROTOCHAR* szPath);

	int dccfiledeny(HANDLE hTransfer);
	int dcc7filedeny(HANDLE hTransfer);

	int dccfilecancel(HANDLE hTransfer);
	int dcc7filecancel(HANDLE hTransfer);

	/* Import module */
	void import_init(HGENMENU hRoot);

	INT_PTR  __cdecl import_server(WPARAM wParam, LPARAM lParam);
	INT_PTR  __cdecl import_text(WPARAM wParam, LPARAM lParam);
	INT_PTR  __cdecl remove_server(WPARAM wParam, LPARAM lParam);
	INT_PTR  __cdecl export_server(WPARAM wParam, LPARAM lParam);
	INT_PTR  __cdecl export_text(WPARAM wParam, LPARAM lParam);

	/* Keep-alive module */
	void keepalive_init();
	void keepalive_destroy();

	/* Image reception functions */
	int img_init();
	int img_destroy();
	int img_shutdown();
	int img_sendonrequest(gg_event* e);
	BOOL img_opened(uin_t uin);
	void *img_loadpicture(gg_event* e, TCHAR *szFileName);
	int img_display(HANDLE hContact, void *img);
	int img_displayasmsg(HANDLE hContact, void *img);

	void __cdecl img_dlgcallthread(void *param);

	INT_PTR __cdecl img_recvimage(WPARAM wParam, LPARAM lParam);
	INT_PTR __cdecl img_sendimg(WPARAM wParam, LPARAM lParam);

	void links_instance_init();

	/* OAuth functions */
	char *oauth_header(const char *httpmethod, const char *url);
	int oauth_checktoken(int force);
	int oauth_receivetoken();

	/* UI page initializers */
	int __cdecl options_init(WPARAM wParam, LPARAM lParam);
	int __cdecl details_init(WPARAM wParam, LPARAM lParam);

	/* Groupchat functions */
	int gc_init();
	void gc_menus_init(HGENMENU hRoot);
	int gc_destroy();
	TCHAR * gc_getchat(uin_t sender, uin_t *recipients, int recipients_count);
	GGGC *gc_lookup(TCHAR *id);
	int gc_changenick(HANDLE hContact, TCHAR *ptszNick);

	int __cdecl gc_event(WPARAM wParam, LPARAM lParam);

	INT_PTR __cdecl gc_openconf(WPARAM wParam, LPARAM lParam);
	INT_PTR __cdecl gc_clearignored(WPARAM wParam, LPARAM lParam);

	/* Popups functions */
	void initpopups();
	void showpopup(const TCHAR* nickname, const TCHAR* msg, int flags);

	/* Sessions functions */
	INT_PTR __cdecl sessions_view(WPARAM wParam, LPARAM lParam);
	void sessions_updatedlg();
	BOOL sessions_closedlg();
	void sessions_menus_init(HGENMENU hRoot);

	/* Event helpers */
	void   createObjService(const char* szService, GGServiceFunc serviceProc);
	void   createProtoService(const char* szService, GGServiceFunc serviceProc);
	HANDLE hookProtoEvent(const char*, GGEventFunc);
	void   forkThread(GGThreadFunc, void* );
	HANDLE forkThreadEx(GGThreadFunc, void*, UINT* threadID = NULL);

	// Debug functions
	int netlog(const char *fmt, ...);

	void block_init();
	void block_uninit();

	//////////////////////////////////////////////////////////////////////////////////////

	CRITICAL_SECTION ft_mutex, sess_mutex, img_mutex, modemsg_mutex, avatar_mutex, sessions_mutex;
	list_t watches, transfers, requests, chats, imagedlgs, avatar_requests, avatar_transfers, sessions;
	int gc_enabled, gc_id, is_list_remove, check_first_conn;
	uin_t next_uin;
	unsigned long last_crc;
	GGTHREAD pth_dcc;
	GGTHREAD pth_sess;
	GGTHREAD pth_avatar;
	struct gg_session *sess;
	struct gg_dcc *dcc;
	HANDLE hEvent;
	HANDLE hConnStopEvent;
	SOCKET sock;
	UINT_PTR timer;
	struct
	{
		TCHAR *online;
		TCHAR *away;
		TCHAR *dnd;
		TCHAR *freechat;
		TCHAR *invisible;
		TCHAR *offline;
	} modemsg;
	HANDLE netlib;
	HGENMENU hMenuRoot;
	HGENMENU hMainMenu[7];
	HANDLE hPrebuildMenuHook;
	HANDLE hBlockMenuItem;
	HANDLE hImageMenuItem;
	HANDLE hInstanceMenuItem;
	HANDLE hAvatarsFolder;
	HANDLE hImagesFolder;
	HWND   hwndSessionsDlg;
};

typedef struct
{
	int mode;
	uin_t uin;
	char *pass;
	char *email;
	GGPROTO *gg;
} GGUSERUTILDLGDATA;


inline void GGPROTO::gg_EnterCriticalSection(CRITICAL_SECTION* mutex, char* callingFunction, int sectionNumber, char* mutexName, int logging)
{
#ifdef DEBUGMODE
	int logAfter = 0;
	extendedLogging = 1;
	if(logging == 1 && mutex->LockCount != -1) {
		logAfter = 1;
		netlog("%s(): %i before EnterCriticalSection %s LockCount=%ld RecursionCount=%ld OwningThread=%ld", callingFunction, sectionNumber, mutexName, mutex->LockCount, mutex->RecursionCount, mutex->OwningThread);
	}
#endif
	EnterCriticalSection(mutex);
#ifdef DEBUGMODE
	if(logging == 1 && logAfter == 1) netlog("%s(): %i after EnterCriticalSection %s LockCount=%ld RecursionCount=%ld OwningThread=%ld", callingFunction, sectionNumber, mutexName, mutex->LockCount, mutex->RecursionCount, mutex->OwningThread);
	extendedLogging = 0;
#endif

}

inline void GGPROTO::gg_LeaveCriticalSection(CRITICAL_SECTION* mutex, char* callingFunction, int sectionNumber, int returnNumber, char* mutexName, int logging) /*0-never, 1-debug, 2-all*/
{
#ifdef DEBUGMODE
	if(logging == 1 && extendedLogging == 1) netlog("%s(): %i.%i LeaveCriticalSection %s", callingFunction, sectionNumber, returnNumber, mutexName);
#endif
	LeaveCriticalSection(mutex);
}

inline void GGPROTO::gg_sleep(DWORD miliseconds, BOOL alterable, char* callingFunction, int sleepNumber, int logging){
	SleepEx(miliseconds, alterable);
#ifdef DEBUGMODE
	if(logging == 1 && extendedLogging == 1) netlog("%s(): %i after SleepEx(%ld,%u)", callingFunction, sleepNumber, miliseconds, alterable);
#endif
}


#endif
