#pragma once

#include "skype.h"
#include "skype_subclassing.h"

struct CSkypeProto;

typedef void    (__cdecl CSkypeProto::*SkypeThreadFunc) (void*);
typedef INT_PTR (__cdecl CSkypeProto::*SkypeServiceFunc)(WPARAM, LPARAM);
typedef int     (__cdecl CSkypeProto::*SkypeEventFunc)(WPARAM, LPARAM);
typedef INT_PTR (__cdecl CSkypeProto::*SkypeServiceFuncParam)(WPARAM, LPARAM, LPARAM);


struct CSkypeProto : public PROTO_INTERFACE, public MZeroedObject
{
public:
	CSkypeProto(const char*, const TCHAR*);
	~CSkypeProto();

	// PROTO_INTERFACE
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
	virtual	HICON  __cdecl GetIcon( int iconIndex );
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

	int __cdecl OnModulesLoaded(WPARAM, LPARAM);
	int __cdecl OnPreShutdown(WPARAM, LPARAM);
	int __cdecl OnOptionsInit(WPARAM, LPARAM);
	int __cdecl OnAccountManagerInit(WPARAM wParam, LPARAM lParam);

	char* ModuleName();
	bool IsOffline();

protected:
	bool isOffline;
	CAccount::Ref account;

	HANDLE hNetlibUser;
	void Log( const char* fmt, ... );

	
	
	
	void	CreateService(const char* szService, SkypeServiceFunc serviceProc);
	//void	CreateServiceParam(const char* szService, SkypeServiceFunc serviceProc, LPARAM lParam);
	
	//HANDLE	CreateHookableEvent(const char* szService);
	void	HookEvent(const char*, SkypeEventFunc);
	//int		SendBroadcast(HANDLE hContact, int type, int result, HANDLE hProcess, LPARAM lParam);

	//void	ForkThread(SkypeThreadFunc, void*);
	//HANDLE	ForkThreadEx(SkypeThreadFunc, void*, UINT* threadID = NULL);


	TCHAR* GetSettingString(const char *szSetting, TCHAR* defVal = NULL);
	TCHAR* GetSettingString(HANDLE hContact, const char *szSetting, TCHAR* defVal = NULL);

	TCHAR* GetDecodeSettingString(const char *szSetting, TCHAR* defVal = NULL);
	TCHAR* GetDecodeSettingString(HANDLE hContact, const char *szSetting, TCHAR* defVal = NULL);


	static INT_PTR CALLBACK SkypeAccountProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
	static INT_PTR CALLBACK SkypeOptionsProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
};