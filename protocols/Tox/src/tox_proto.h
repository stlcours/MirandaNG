#ifndef _TOX_PROTO_H_
#define _TOX_PROTO_H_

#include "common.h"

#define TOX_SEARCH_BYUID 1001

struct CToxProto : public PROTO<CToxProto>
{
public:

	//////////////////////////////////////////////////////////////////////////////////////
	//Ctors

	CToxProto(const char *protoName, const wchar_t *userName);
	~CToxProto();

	//////////////////////////////////////////////////////////////////////////////////////
	// Virtual functions

	virtual	MCONTACT __cdecl AddToList(int flags, PROTOSEARCHRESULT* psr);
	virtual	MCONTACT __cdecl AddToListByEvent(int flags, int iContact, HANDLE hDbEvent);

	virtual	int      __cdecl Authorize(HANDLE hDbEvent);
	virtual	int      __cdecl AuthDeny(HANDLE hDbEvent, const PROTOCHAR* szReason);
	virtual	int      __cdecl AuthRecv(MCONTACT hContact, PROTORECVEVENT*);
	virtual	int      __cdecl AuthRequest(MCONTACT hContact, const PROTOCHAR* szMessage);

	virtual	HANDLE   __cdecl ChangeInfo(int iInfoType, void* pInfoData);

	virtual	HANDLE   __cdecl FileAllow(MCONTACT hContact, HANDLE hTransfer, const PROTOCHAR* szPath);
	virtual	int      __cdecl FileCancel(MCONTACT hContact, HANDLE hTransfer);
	virtual	int      __cdecl FileDeny(MCONTACT hContact, HANDLE hTransfer, const PROTOCHAR* szReason);
	virtual	int      __cdecl FileResume(HANDLE hTransfer, int* action, const PROTOCHAR** szFilename);

	virtual	DWORD_PTR __cdecl GetCaps(int type, MCONTACT hContact = NULL);
	virtual	int       __cdecl GetInfo(MCONTACT hContact, int infoType);

	virtual	HANDLE    __cdecl SearchBasic(const PROTOCHAR* id);
	virtual	HANDLE    __cdecl SearchByEmail(const PROTOCHAR* email);
	virtual	HANDLE    __cdecl SearchByName(const PROTOCHAR* nick, const PROTOCHAR* firstName, const PROTOCHAR* lastName);
	virtual	HWND      __cdecl SearchAdvanced(HWND owner);
	virtual	HWND      __cdecl CreateExtendedSearchUI(HWND owner);

	virtual	int       __cdecl RecvContacts(MCONTACT hContact, PROTORECVEVENT*);
	virtual	int       __cdecl RecvFile(MCONTACT hContact, PROTOFILEEVENT*);
	virtual	int       __cdecl RecvMsg(MCONTACT hContact, PROTORECVEVENT*);
	virtual	int       __cdecl RecvUrl(MCONTACT hContact, PROTORECVEVENT*);

	virtual	int       __cdecl SendContacts(MCONTACT hContact, int flags, int nContacts, MCONTACT* hContactsList);
	virtual	HANDLE    __cdecl SendFile(MCONTACT hContact, const PROTOCHAR* szDescription, PROTOCHAR** ppszFiles);
	virtual	int       __cdecl SendMsg(MCONTACT hContact, int flags, const char* msg);
	virtual	int       __cdecl SendUrl(MCONTACT hContact, int flags, const char* url);

	virtual	int       __cdecl SetApparentMode(MCONTACT hContact, int mode);
	virtual	int       __cdecl SetStatus(int iNewStatus);

	virtual	HANDLE    __cdecl GetAwayMsg(MCONTACT hContact);
	virtual	int       __cdecl RecvAwayMsg(MCONTACT hContact, int mode, PROTORECVEVENT* evt);
	virtual	int       __cdecl SetAwayMsg(int iStatus, const PROTOCHAR* msg);

	virtual	int       __cdecl UserIsTyping(MCONTACT hContact, int type);

	virtual	int       __cdecl OnEvent(PROTOEVENTTYPE iEventType, WPARAM wParam, LPARAM lParam);

	// instances
	static CToxProto* InitProtoInstance(const char* protoName, const wchar_t* userName);
	static int        UninitProtoInstance(CToxProto* ppro);

	static CToxProto* GetContactInstance(MCONTACT hContact);
	static void       UninitInstances();

private:

	// instances
	static LIST<CToxProto> instanceList;
	static int CompareProtos(const CToxProto *p1, const CToxProto *p2);

	static void CALLBACK TimerProc(HWND, UINT, UINT_PTR idEvent, DWORD);

	// Instance data:
	Tox *_tox;
	UINT_PTR _timer;

	//services
	INT_PTR __cdecl CreateAccMgrUI(WPARAM, LPARAM);

	//events
	static void OnFriendRequest(uint8_t *userId, uint8_t *message, uint16_t messageSize, void *arg);
    static void OnFriendMessage(Tox *tox, int friendId, uint8_t *message, uint16_t messageSize, void *arg);
    static void OnFriendNameChange(Tox *tox, int friendId, uint8_t *name, uint16_t nameSize, void *arg);
    static void OnStatusMessageChanged(Tox *tox, int friendId, uint8_t* message, uint16_t messageSize, void *arg);
    static void OnUserStatusChanged(Tox *tox, int friendId, TOX_USERSTATUS userStatus, void *arg);
    static void OnConnectionStatusChanged(Tox *tox, int friendId, uint8_t status, void *arg);
    static void OnAction(Tox *tox, int friendId, uint8_t *message, uint16_t messageSize, void *arg);

	// contacts
	bool IsProtoContact(MCONTACT hContact);
	MCONTACT GetContactByUserId(const wchar_t *userId);
	MCONTACT CToxProto::AddContact(const wchar_t*userId, const wchar_t *nick, bool isHidden = false);
	
	void __cdecl SearchByUidAsync(void* arg);

	// dialogs
	static INT_PTR CALLBACK AccountManagerProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
};

#endif //_TOX_PROTO_H_