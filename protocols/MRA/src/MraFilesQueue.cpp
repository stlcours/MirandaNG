#include "Mra.h"
#include "MraFilesQueue.h"
#include "proto.h"

#define MRA_FT_HELLO	"MRA_FT_HELLO"
#define MRA_FT_GET_FILE	"MRA_FT_GET_FILE"

struct MRA_FILES_QUEUE : public LIST_MT
{
	DWORD			dwSendTimeOutInterval;
};

struct MRA_FILES_QUEUE_FILE
{
	LPWSTR		lpwszName;
	size_t		dwNameLen;
	DWORDLONG	dwSize;
};

struct MRA_FILES_QUEUE_ITEM : public LIST_MT_ITEM
{
	// internal
	BOOL                  bIsWorking;
	DWORD                 dwSendTime;

	// external
	CMraProto            *ppro;
	DWORD                 dwIDRequest;
	DWORD                 dwFlags;
	HANDLE                hContact;
	DWORDLONG             dwFilesCount;
	DWORDLONG             dwFilesTotalSize;
	MRA_FILES_QUEUE_FILE *pmfqfFiles;
	LPWSTR                pwszFilesList;
	LPWSTR                pwszDescription;
	MRA_ADDR_LIST         malAddrList;
	LPWSTR                lpwszPath;
	size_t                dwPathSize;
	BOOL                  bSending;
	HANDLE                hConnection;
	HANDLE                hListen;
	HANDLE                hThread;
	HANDLE                hWaitHandle;
	HANDLE                hMraMrimProxyData;
};

struct MRA_FILES_THREADPROC_PARAMS
{
	HANDLE					hFilesQueueHandle;
	MRA_FILES_QUEUE_ITEM	*dat;
};

DWORD  MraFilesQueueItemFindByID(HANDLE hFilesQueueHandle, DWORD dwIDRequest, MRA_FILES_QUEUE_ITEM **ppmrafqFilesQueueItem);
void   MraFilesQueueItemFree(MRA_FILES_QUEUE_ITEM *dat);

/////////////////////////////////////////////////////////////////////////////////////////
// File transfer options

static WORD wMraFilesControlsList[] = {
	IDC_FILE_SEND_NOOUTCONNECTIONONRECEIVE,
	IDC_FILE_SEND_NOOUTCONNECTIONONSEND,
	IDC_FILE_SEND_IGNORYADDITIONALPORTS,
	IDC_FILE_SEND_HIDE_MY_ADDRESSES,
	IDC_FILE_SEND_ADD_EXTRA_ADDRESS,
	IDC_FILE_SEND_EXTRA_ADDRESS
};

void MraFilesQueueDlgEnableDirectConsControls(HWND hWndDlg, BOOL bEnabled)
{
	EnableControlsArray(hWndDlg, (WORD*)&wMraFilesControlsList, SIZEOF(wMraFilesControlsList), bEnabled);
	EnableWindow(GetDlgItem(hWndDlg, IDC_FILE_SEND_EXTRA_ADDRESS), (bEnabled && IS_DLG_BUTTON_CHECKED(hWndDlg, IDC_FILE_SEND_ADD_EXTRA_ADDRESS)));
}

INT_PTR CALLBACK MraFilesQueueDlgProcOpts(HWND hWndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	CMraProto *ppro = (CMraProto*)GetWindowLongPtr(hWndDlg, GWLP_USERDATA);

	switch (msg) {
	case WM_INITDIALOG:
		TranslateDialogDefault(hWndDlg);
		SetWindowLongPtr(hWndDlg, GWLP_USERDATA, lParam);
		ppro = (CMraProto*)lParam;
		{
			WCHAR szBuff[MAX_PATH];
			CHECK_DLG_BUTTON(hWndDlg, IDC_FILE_SEND_ENABLE_DIRECT_CONN, ppro->getByte(NULL, "FileSendEnableDirectConn", MRA_DEF_FS_ENABLE_DIRECT_CONN));
			CHECK_DLG_BUTTON(hWndDlg, IDC_FILE_SEND_NOOUTCONNECTIONONRECEIVE, ppro->getByte(NULL, "FileSendNoOutConnOnRcv", MRA_DEF_FS_NO_OUT_CONN_ON_RCV));
			CHECK_DLG_BUTTON(hWndDlg, IDC_FILE_SEND_NOOUTCONNECTIONONSEND, ppro->getByte(NULL, "FileSendNoOutConnOnSend", MRA_DEF_FS_NO_OUT_CONN_ON_SEND));
			CHECK_DLG_BUTTON(hWndDlg, IDC_FILE_SEND_IGNORYADDITIONALPORTS, ppro->getByte(NULL, "FileSendIgnoryAdditionalPorts", MRA_DEF_FS_IGNORY_ADDITIONAL_PORTS));
			CHECK_DLG_BUTTON(hWndDlg, IDC_FILE_SEND_HIDE_MY_ADDRESSES, ppro->getByte(NULL, "FileSendHideMyAddresses", MRA_DEF_FS_HIDE_MY_ADDRESSES));
			CHECK_DLG_BUTTON(hWndDlg, IDC_FILE_SEND_ADD_EXTRA_ADDRESS, ppro->getByte(NULL, "FileSendAddExtraAddresses", MRA_DEF_FS_ADD_EXTRA_ADDRESSES));

			if (ppro->mraGetStaticStringW(NULL, "FileSendExtraAddresses", szBuff, SIZEOF(szBuff), NULL))
				SET_DLG_ITEM_TEXT(hWndDlg, IDC_FILE_SEND_EXTRA_ADDRESS, szBuff);

			CHECK_DLG_BUTTON(hWndDlg, IDC_FILE_SEND_ENABLE_MRIMPROXY_CONS, ppro->getByte(NULL, "FileSendEnableMRIMProxyCons", MRA_DEF_FS_ENABLE_MRIM_PROXY_CONS));

			SetDlgItemInt(hWndDlg, IDC_FILE_SEND_BLOCK_SIZE, ppro->getDword(NULL, "FileSendBlockSize", MRA_DEFAULT_FILE_SEND_BLOCK_SIZE), FALSE);

			MraFilesQueueDlgEnableDirectConsControls(hWndDlg, IS_DLG_BUTTON_CHECKED(hWndDlg, IDC_FILE_SEND_ENABLE_DIRECT_CONN));
		}
		return TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDC_FILE_SEND_ENABLE_DIRECT_CONN)
			MraFilesQueueDlgEnableDirectConsControls(hWndDlg, IS_DLG_BUTTON_CHECKED(hWndDlg, IDC_FILE_SEND_ENABLE_DIRECT_CONN));

		if (LOWORD(wParam) == IDC_FILE_SEND_ADD_EXTRA_ADDRESS)
			EnableWindow(GetDlgItem(hWndDlg, IDC_FILE_SEND_EXTRA_ADDRESS), IS_DLG_BUTTON_CHECKED(hWndDlg, IDC_FILE_SEND_ADD_EXTRA_ADDRESS));

		if ((LOWORD(wParam) == IDC_FILE_SEND_EXTRA_ADDRESS || LOWORD(wParam) == IDC_FILE_SEND_BLOCK_SIZE) && (HIWORD(wParam) != EN_CHANGE || (HWND)lParam != GetFocus())) return FALSE;
		SendMessage(GetParent(hWndDlg), PSM_CHANGED, 0, 0);
		break;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code) {
		case PSN_APPLY:
			{
				WCHAR szBuff[MAX_PATH];

				ppro->setByte(NULL, "FileSendEnableDirectConn", IS_DLG_BUTTON_CHECKED(hWndDlg, IDC_FILE_SEND_ENABLE_DIRECT_CONN));
				ppro->setByte(NULL, "FileSendNoOutConnOnRcv", IS_DLG_BUTTON_CHECKED(hWndDlg, IDC_FILE_SEND_NOOUTCONNECTIONONRECEIVE));
				ppro->setByte(NULL, "FileSendNoOutConnOnSend", IS_DLG_BUTTON_CHECKED(hWndDlg, IDC_FILE_SEND_NOOUTCONNECTIONONSEND));
				ppro->setByte(NULL, "FileSendIgnoryAdditionalPorts", IS_DLG_BUTTON_CHECKED(hWndDlg, IDC_FILE_SEND_IGNORYADDITIONALPORTS));
				ppro->setByte(NULL, "FileSendHideMyAddresses", IS_DLG_BUTTON_CHECKED(hWndDlg, IDC_FILE_SEND_HIDE_MY_ADDRESSES));
				ppro->setByte(NULL, "FileSendAddExtraAddresses", IS_DLG_BUTTON_CHECKED(hWndDlg, IDC_FILE_SEND_ADD_EXTRA_ADDRESS));
				GET_DLG_ITEM_TEXT(hWndDlg, IDC_FILE_SEND_EXTRA_ADDRESS, szBuff, SIZEOF(szBuff));
				ppro->mraSetStringW(NULL, "FileSendExtraAddresses", szBuff);
				ppro->setDword(NULL, "FileSendBlockSize", (DWORD)GetDlgItemInt(hWndDlg, IDC_FILE_SEND_BLOCK_SIZE, NULL, FALSE));
				ppro->setByte(NULL, "FileSendEnableMRIMProxyCons", IS_DLG_BUTTON_CHECKED(hWndDlg, IDC_FILE_SEND_ENABLE_MRIMPROXY_CONS));
			}
			return TRUE;
		}
		break;
	}
	return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MRA files' queue

DWORD MraFilesQueueInitialize(DWORD dwSendTimeOutInterval, HANDLE *phFilesQueueHandle)
{
	if (!phFilesQueueHandle)
		return ERROR_INVALID_HANDLE;

	MRA_FILES_QUEUE *pmrafqFilesQueue = (MRA_FILES_QUEUE*)mir_calloc(sizeof(MRA_FILES_QUEUE));
	if (!pmrafqFilesQueue)
		return GetLastError();

	DWORD dwRetErrorCode = ListMTInitialize(pmrafqFilesQueue, 0);
	if (dwRetErrorCode == NO_ERROR) {
		pmrafqFilesQueue->dwSendTimeOutInterval = dwSendTimeOutInterval;
		*phFilesQueueHandle = (HANDLE)pmrafqFilesQueue;
	}
	else mir_free(pmrafqFilesQueue);

	return dwRetErrorCode;
}

void MraFilesQueueDestroy(HANDLE hFilesQueueHandle)
{
	if ( !hFilesQueueHandle)
		return;

	MRA_FILES_QUEUE *pmrafqFilesQueue = (MRA_FILES_QUEUE*)hFilesQueueHandle;
	MRA_FILES_QUEUE_ITEM *dat;
	{
		mt_lock l(pmrafqFilesQueue);
		while ( ListMTItemGetFirst(pmrafqFilesQueue, NULL, (LPVOID*)&dat) == NO_ERROR)
			MraFilesQueueItemFree(dat);
	}
	ListMTDestroy(pmrafqFilesQueue);
	mir_free(pmrafqFilesQueue);
}

DWORD MraFilesQueueItemFindByID(HANDLE hFilesQueueHandle, DWORD dwIDRequest, MRA_FILES_QUEUE_ITEM **ppmrafqFilesQueueItem)
{
	if (!hFilesQueueHandle)
		return ERROR_INVALID_HANDLE;

	MRA_FILES_QUEUE *pmrafqFilesQueue = (MRA_FILES_QUEUE*)hFilesQueueHandle;
	MRA_FILES_QUEUE_ITEM *dat;
	LIST_MT_ITERATOR lmtiIterator;

	mt_lock l(pmrafqFilesQueue);
	ListMTIteratorMoveFirst(pmrafqFilesQueue, &lmtiIterator);
	do {
		if (ListMTIteratorGet(&lmtiIterator, NULL, (LPVOID*)&dat) == NO_ERROR)
		if (dat->dwIDRequest == dwIDRequest) {
			if (ppmrafqFilesQueueItem)
				*ppmrafqFilesQueueItem = dat;
			return 0;
		}
	}
		while (ListMTIteratorMoveNext(&lmtiIterator));

	return ERROR_NOT_FOUND;
}

HANDLE MraFilesQueueItemProxyByID(HANDLE hFilesQueueHandle, DWORD dwIDRequest)
{
	MRA_FILES_QUEUE_ITEM *dat;
	if ( !MraFilesQueueItemFindByID(hFilesQueueHandle, dwIDRequest, &dat))
		return dat->hMraMrimProxyData;

	return NULL;
}

void MraFilesQueueItemFree(MRA_FILES_QUEUE_ITEM *dat)
{
 	LIST_MT *plmtListMT = (LIST_MT*)dat->lpListMT;

	for (size_t i = 0;i<dat->dwFilesCount;i++)
		mir_free(dat->pmfqfFiles[i].lpwszName);

	mir_free(dat->pmfqfFiles);
	mir_free(dat->pwszFilesList);
	mir_free(dat->pwszDescription);
	MraAddrListFree(&dat->malAddrList);
	MraMrimProxyFree(dat->hMraMrimProxyData);
	mir_free(dat->lpwszPath);
	{
		mt_lock l(plmtListMT);
		ListMTItemDelete(plmtListMT, dat);
	}
	mir_free(dat);
}

size_t CMraProto::MraFilesQueueGetLocalAddressesList(LPSTR lpszBuff, size_t dwBuffSize, DWORD dwPort)
{
	CHAR szHostName[MAX_PATH] = {0};
	LPSTR lpszCurPos = lpszBuff;

	if (getByte(NULL, "FileSendHideMyAddresses", MRA_DEF_FS_HIDE_MY_ADDRESSES))
	{// �� ����� ����� ��� IP �����!!! :)
		if (getByte(NULL, "FileSendAddExtraAddresses", MRA_DEF_FS_ADD_EXTRA_ADDRESSES) == FALSE)
		{// ������ ���� �� ��������� ����� �������
			lpszCurPos += mir_snprintf(lpszCurPos, (dwBuffSize-((size_t)lpszCurPos-(size_t)lpszBuff)), MRA_FILES_NULL_ADDRR);
		}
	}
	else {// ������ ������ ����� IP �������
		BYTE btAddress[32];
		DWORD dwSelfExternalIP;
		size_t dwAdapter = 0;
		hostent *sh;

		dwSelfExternalIP = NTOHL(getDword(NULL, "IP", 0));
		if (dwSelfExternalIP) {
			memmove(&btAddress, &dwSelfExternalIP, sizeof(DWORD));
			lpszCurPos += mir_snprintf(lpszCurPos, (dwBuffSize-((size_t)lpszCurPos-(size_t)lpszBuff)), "%lu.%lu.%lu.%lu:%lu;", btAddress[0], btAddress[1], btAddress[2], btAddress[3], dwPort);
		}

		if (gethostname(szHostName, SIZEOF(szHostName)) == 0)
		if ((sh = gethostbyname((LPSTR)&szHostName))) {
			while (sh->h_addr_list[dwAdapter]) {
				lpszCurPos += mir_snprintf(lpszCurPos, (dwBuffSize-((size_t)lpszCurPos-(size_t)lpszBuff)), "%s:%lu;", inet_ntoa(*((struct in_addr*)sh->h_addr_list[dwAdapter])), dwPort);
				dwAdapter++;
			}
		}
	}

	if (getByte(NULL, "FileSendAddExtraAddresses", MRA_DEF_FS_ADD_EXTRA_ADDRESSES))// ��������� ������������ �����
	if (mraGetStaticStringA(NULL, "FileSendExtraAddresses", szHostName, SIZEOF(szHostName), NULL))
		lpszCurPos += mir_snprintf(lpszCurPos, (dwBuffSize-((size_t)lpszCurPos-(size_t)lpszBuff)), "%s:%lu;", szHostName, dwPort);

	return lpszCurPos - lpszBuff;
}

DWORD CMraProto::MraFilesQueueAccept(HANDLE hFilesQueueHandle, DWORD dwIDRequest, LPCWSTR lpwszPath, size_t dwPathSize)
{
	if ( !hFilesQueueHandle || !lpwszPath || !dwPathSize)
		return ERROR_INVALID_HANDLE;

	MRA_FILES_QUEUE *pmrafqFilesQueue = (MRA_FILES_QUEUE*)hFilesQueueHandle;
	MRA_FILES_QUEUE_ITEM *dat;

	mt_lock l(pmrafqFilesQueue);
	DWORD dwRetErrorCode = MraFilesQueueItemFindByID(hFilesQueueHandle, dwIDRequest, &dat);
	if (dwRetErrorCode == NO_ERROR) {
		MRA_FILES_THREADPROC_PARAMS *pmftpp = (MRA_FILES_THREADPROC_PARAMS*)mir_calloc(sizeof(MRA_FILES_THREADPROC_PARAMS));
		dat->lpwszPath = (LPWSTR)mir_calloc((dwPathSize*sizeof(WCHAR)));
		dat->dwPathSize = dwPathSize;
		memmove(dat->lpwszPath, lpwszPath, (dwPathSize*sizeof(WCHAR)));

		if ( (*(WCHAR*)(dat->lpwszPath+(dat->dwPathSize-1))) != '\\')
		{// add slash at the end if needed
			(*(WCHAR*)(dat->lpwszPath+dat->dwPathSize)) = '\\';
			dat->dwPathSize++;
			(*(WCHAR*)(dat->lpwszPath+dat->dwPathSize)) = 0;
		}

		pmftpp->hFilesQueueHandle = hFilesQueueHandle;
		pmftpp->dat = dat;

		dat->hThread = ForkThreadEx(&CMraProto::MraFilesQueueRecvThreadProc, pmftpp, 0);
	}
	return dwRetErrorCode;
}

DWORD CMraProto::MraFilesQueueCancel(HANDLE hFilesQueueHandle, DWORD dwIDRequest, BOOL bSendDecline)
{
	if (!hFilesQueueHandle)
		return ERROR_INVALID_HANDLE;

	MRA_FILES_QUEUE *pmrafqFilesQueue = (MRA_FILES_QUEUE*)hFilesQueueHandle;
	MRA_FILES_QUEUE_ITEM *dat;

	mt_lock l(pmrafqFilesQueue);
	DWORD dwRetErrorCode = MraFilesQueueItemFindByID(hFilesQueueHandle, dwIDRequest, &dat);
	if (dwRetErrorCode == NO_ERROR) { //***deb closesocket, send message to thread
		InterlockedExchange((volatile LONG*)&dat->bIsWorking, FALSE);

		if (bSendDecline) {
			CHAR szEMail[MAX_EMAIL_LEN];
			size_t dwEMailSize;

			if (mraGetStaticStringA(dat->hContact, "e-mail", szEMail, SIZEOF(szEMail), &dwEMailSize))
				MraFileTransferAck(FILE_TRANSFER_STATUS_DECLINE, szEMail, dwEMailSize, dwIDRequest, NULL, 0);
		}

		MraMrimProxyCloseConnection(dat->hMraMrimProxyData);

		Netlib_CloseHandle(dat->hListen);
		dat->hListen = NULL;

		Netlib_CloseHandle(dat->hConnection);
		dat->hConnection = NULL;

		SetEvent(dat->hWaitHandle);

		if (dat->hThread == NULL)
			MraFilesQueueItemFree(dat);
	}
	return dwRetErrorCode;
}

DWORD CMraProto::MraFilesQueueStartMrimProxy(HANDLE hFilesQueueHandle, DWORD dwIDRequest)
{
	if (!hFilesQueueHandle || !getByte(NULL, "FileSendEnableMRIMProxyCons", MRA_DEF_FS_ENABLE_MRIM_PROXY_CONS))
		return ERROR_INVALID_HANDLE;

	MRA_FILES_QUEUE *pmrafqFilesQueue = (MRA_FILES_QUEUE*)hFilesQueueHandle;
	MRA_FILES_QUEUE_ITEM *dat;

	mt_lock l(pmrafqFilesQueue);
	if ( !MraFilesQueueItemFindByID(hFilesQueueHandle, dwIDRequest, &dat))
		if (dat->bSending == FALSE)
			SetEvent(dat->hWaitHandle);// cancel wait incomming connection

	return 0;
}

DWORD MraFilesQueueFree(HANDLE hFilesQueueHandle, DWORD dwIDRequest)
{
	if (!hFilesQueueHandle)
		return ERROR_INVALID_HANDLE;

	MRA_FILES_QUEUE *pmrafqFilesQueue = (MRA_FILES_QUEUE*)hFilesQueueHandle;
	MRA_FILES_QUEUE_ITEM *dat;
	LIST_MT_ITERATOR lmtiIterator;

	mt_lock l(pmrafqFilesQueue);
	ListMTIteratorMoveFirst(pmrafqFilesQueue, &lmtiIterator);
	do {
		if (ListMTIteratorGet(&lmtiIterator, NULL, (LPVOID*)&dat) == NO_ERROR)
		if (dat->dwIDRequest == dwIDRequest) {
			MraFilesQueueItemFree(dat);
			return 0;
		}
	}
		while (ListMTIteratorMoveNext(&lmtiIterator));

	return ERROR_NOT_FOUND;
}

DWORD CMraProto::MraFilesQueueSendMirror(HANDLE hFilesQueueHandle, DWORD dwIDRequest, LPSTR lpszAddreses, size_t dwAddresesSize)
{
	if (!hFilesQueueHandle)
		return ERROR_INVALID_HANDLE;

	MRA_FILES_QUEUE *pmrafqFilesQueue = (MRA_FILES_QUEUE*)hFilesQueueHandle;
	MRA_FILES_QUEUE_ITEM *dat;

	mt_lock l(pmrafqFilesQueue);
	DWORD dwRetErrorCode = MraFilesQueueItemFindByID(hFilesQueueHandle, dwIDRequest, &dat);
	if (dwRetErrorCode == NO_ERROR) {
		MraAddrListGetFromBuff(lpszAddreses, dwAddresesSize, &dat->malAddrList);
		MraAddrListStoreToContact(dat->hContact, &dat->malAddrList);

		dat->hConnection = NULL;
		SetEvent(dat->hWaitHandle);
	}
	return dwRetErrorCode;
}

BOOL CMraProto::MraFilesQueueHandCheck(HANDLE hConnection, MRA_FILES_QUEUE_ITEM *dat)
{
	BOOL bRet = FALSE;

	if (hConnection && dat)
	{
		CHAR szEMail[MAX_EMAIL_LEN] = {0}, szEMailMy[MAX_EMAIL_LEN] = {0};
		BYTE btBuff[((MAX_EMAIL_LEN*2)+(sizeof(MRA_FT_HELLO)*2)+8)] = {0};
		size_t dwEMailSize, dwEMailMySize, dwBuffSize;

		mraGetStaticStringA(NULL, "e-mail", szEMailMy, SIZEOF(szEMailMy), &dwEMailMySize);BuffToLowerCase(szEMailMy, szEMailMy, dwEMailMySize);
		mraGetStaticStringA(dat->hContact, "e-mail", szEMail, SIZEOF(szEMail), &dwEMailSize);BuffToLowerCase(szEMail, szEMail, dwEMailSize);

		if (dat->bSending == FALSE)
		{// receiving
			dwBuffSize = mir_snprintf((LPSTR)btBuff, SIZEOF(btBuff), "%s %s", MRA_FT_HELLO, szEMailMy)+1;
			if (dwBuffSize == Netlib_Send(hConnection, (LPSTR)btBuff, (int)dwBuffSize, 0))
			{// my email sended
				ProtoBroadcastAck(dat->hContact, ACKTYPE_FILE, ACKRESULT_INITIALISING, (HANDLE)dat->dwIDRequest, 0);
				dwBuffSize = Netlib_Recv(hConnection, (LPSTR)btBuff, sizeof(btBuff), 0);
				if ((dwEMailSize+sizeof(MRA_FT_HELLO)+1) == dwBuffSize)
				{// email received
					mir_snprintf(((LPSTR)btBuff+dwBuffSize), (SIZEOF(btBuff)-dwBuffSize), "%s %s", MRA_FT_HELLO, szEMail);
					if (CompareStringA( MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), NORM_IGNORECASE, (LPSTR)btBuff, dwBuffSize, ((LPSTR)btBuff+dwBuffSize), dwBuffSize) == CSTR_EQUAL)
						bRet = TRUE;
				}
			}
		}else {// sending
			dwBuffSize = Netlib_Recv(hConnection, (LPSTR)btBuff, sizeof(btBuff), 0);
			if ((dwEMailSize+sizeof(MRA_FT_HELLO)+1) == dwBuffSize)
			{// email received
				ProtoBroadcastAck(dat->hContact, ACKTYPE_FILE, ACKRESULT_INITIALISING, (HANDLE)dat->dwIDRequest, 0);
				mir_snprintf(((LPSTR)btBuff+dwBuffSize), (SIZEOF(btBuff)-dwBuffSize), "%s %s", MRA_FT_HELLO, szEMail);
				if (CompareStringA( MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), NORM_IGNORECASE, (LPSTR)btBuff, dwBuffSize, ((LPSTR)btBuff+dwBuffSize), dwBuffSize) == CSTR_EQUAL)
				{// email verified
					dwBuffSize = (mir_snprintf((LPSTR)btBuff, SIZEOF(btBuff), "%s %s", MRA_FT_HELLO, szEMailMy)+1);
					if (dwBuffSize == Netlib_Send(hConnection, (LPSTR)btBuff, dwBuffSize, 0))
					{// my email sended
						bRet = TRUE;
					}
				}
			}
		}
	}
	return bRet;
}

HANDLE CMraProto::MraFilesQueueConnectOut(MRA_FILES_QUEUE_ITEM *dat)
{
	if (!dat)
		return NULL;

	if (getByte(NULL, "FileSendEnableDirectConn", MRA_DEF_FS_ENABLE_DIRECT_CONN) && InterlockedExchangeAdd((volatile LONG*)&dat->bIsWorking, 0) && ((dat->bSending == FALSE && getByte(NULL, "FileSendNoOutConnOnRcv", MRA_DEF_FS_NO_OUT_CONN_ON_RCV) == FALSE) || (dat->bSending == TRUE && getByte(NULL, "FileSendNoOutConnOnSend", MRA_DEF_FS_NO_OUT_CONN_ON_SEND) == FALSE))) {
		BOOL bFiltering = FALSE, bIsHTTPSProxyUsed = IsHTTPSProxyUsed(hNetlibUser);
		DWORD dwLocalPort = 0, dwConnectReTryCount, dwCurConnectReTryCount;
		size_t dwAddrCount;
		NETLIBOPENCONNECTION nloc = {0};

		if (getByte(NULL, "FileSendIgnoryAdditionalPorts", MRA_DEF_FS_IGNORY_ADDITIONAL_PORTS) || bIsHTTPSProxyUsed)
		{// ��������� ����� ��� ������ IP, ������ 3 ����� ����������� ������ � ������
			if (bIsHTTPSProxyUsed)
				dwLocalPort = MRA_SERVER_PORT_HTTPS;
			else if ((dwLocalPort = getWord(NULL, "ServerPort", MRA_DEFAULT_SERVER_PORT)) == MRA_SERVER_PORT_STANDART_NLB)
				dwLocalPort = MRA_SERVER_PORT_STANDART;

			dwAddrCount = 0;
			for (size_t i = 0; i < dat->malAddrList.dwAddrCount; i++) {
				if (dwLocalPort == dat->malAddrList.pmaliAddress[i].dwPort) {
					bFiltering = TRUE;
					dwAddrCount++;
				}
			}
		}

		if (bFiltering == FALSE)
			dwAddrCount = dat->malAddrList.dwAddrCount;

		if (dwAddrCount) {
			dat->hConnection = NULL;
			dwConnectReTryCount = getDword(NULL, "ConnectReTryCountFileSend", MRA_DEFAULT_CONN_RETRY_COUNT_FILES);
			nloc.cbSize = sizeof(nloc);
			nloc.flags = NLOCF_V2;
			nloc.timeout = getDword(NULL, "TimeOutConnectFileSend", ((MRA_TIMEOUT_DIRECT_CONN-1)/(dwAddrCount*dwConnectReTryCount)));// -1 ��� ����� ��� �����
			if (nloc.timeout<MRA_TIMEOUT_CONN_MIN) nloc.timeout = MRA_TIMEOUT_CONN_MIN;
			if (nloc.timeout>MRA_TIMEOUT_CONN_���) nloc.timeout = MRA_TIMEOUT_CONN_���;

			// Set up the sockaddr structure
			for (size_t i = 0;i<dat->malAddrList.dwAddrCount;i++) {
				if (dwLocalPort == dat->malAddrList.pmaliAddress[i].dwPort || bFiltering == FALSE) {
					ProtoBroadcastAck(dat->hContact, ACKTYPE_FILE, ACKRESULT_CONNECTING, (HANDLE)dat->dwIDRequest, 0);

					nloc.szHost = inet_ntoa((*((in_addr*)&dat->malAddrList.pmaliAddress[i].dwAddr)));
					nloc.wPort = (WORD)dat->malAddrList.pmaliAddress[i].dwPort;

					dwCurConnectReTryCount = dwConnectReTryCount;
					do {
						dat->hConnection = (HANDLE)CallService(MS_NETLIB_OPENCONNECTION, (WPARAM)hNetlibUser, (LPARAM)&nloc);
					}
						while (--dwCurConnectReTryCount && dat->hConnection == NULL);

					if (dat->hConnection) {
						ProtoBroadcastAck(dat->hContact, ACKTYPE_FILE, ACKRESULT_CONNECTED, (HANDLE)dat->dwIDRequest, 0);
						if (MraFilesQueueHandCheck(dat->hConnection, dat)) {
							// ����� ������������� � ��� ��� �����
							setDword(dat->hContact, "OldIP", getDword(dat->hContact, "IP", 0));
							setDword(dat->hContact, "IP", HTONL(dat->malAddrList.pmaliAddress[i].dwAddr));
							break;
						}
						else {
							// ������� �� ���� ������������ :)
							Netlib_CloseHandle(dat->hConnection);
							dat->hConnection = NULL;
						}
					}
				}
			}
		}
	}
	return dat->hConnection;
}

LPWSTR GetFileNameFromFullPathW(LPWSTR lpwszFullPath, size_t dwFullPathSize)
{
	LPWSTR lpwszFileName = lpwszFullPath, lpwszCurPos;

	lpwszCurPos = (lpwszFullPath+dwFullPathSize);
	for (; lpwszCurPos>lpwszFullPath; lpwszCurPos--) {
		if ((*lpwszCurPos) == '\\') {
			lpwszFileName = (lpwszCurPos+1);
			break;
		}
	}
	return lpwszFileName;
}

HANDLE CMraProto::MraFilesQueueConnectIn(MRA_FILES_QUEUE_ITEM *dat)
{
	if (!dat)
		return NULL;
	if ( !InterlockedExchangeAdd((volatile LONG*)&dat->bIsWorking, 0))
		return NULL;

	CHAR szEMail[MAX_EMAIL_LEN];
	size_t dwEMailSize;

	if (mraGetStaticStringA(dat->hContact, "e-mail", szEMail, SIZEOF(szEMail), &dwEMailSize)) {
		CHAR szAddrList[2048] = {0};
		size_t dwAddrListSize;

		// �������� ������ � ������������ � ��������� � �������� ������� ����
		if (getByte(NULL, "FileSendEnableDirectConn", MRA_DEF_FS_ENABLE_DIRECT_CONN)) {
			NETLIBBIND nlbBind = {0};

			nlbBind.cbSize = sizeof(nlbBind);
			nlbBind.pfnNewConnectionV2 = MraFilesQueueConnectionReceived;
			nlbBind.wPort = 0;
			nlbBind.pExtra = (LPVOID)dat;

			dat->hListen = (HANDLE)CallService(MS_NETLIB_BINDPORT, (WPARAM)hNetlibUser, (LPARAM)&nlbBind);
			if (dat->hListen) {
				ProtoBroadcastAck(dat->hContact, ACKTYPE_FILE, ACKRESULT_LISTENING, (HANDLE)dat->dwIDRequest, 0);
				dwAddrListSize = MraFilesQueueGetLocalAddressesList(szAddrList, sizeof(szAddrList), nlbBind.wPort);
			}
			// �� ������ ������� ����, �� ������.
			else {
				ShowFormattedErrorMessage(L"Files exchange: cant create listen soscket, will try connect to remonte host. Error", GetLastError());

				//dwAddrListSize = 0;
				memmove(szAddrList, MRA_FILES_NULL_ADDRR, sizeof(MRA_FILES_NULL_ADDRR));
				dwAddrListSize = (sizeof(MRA_FILES_NULL_ADDRR)-1);
			}
		}
		// ����������� ������ �����, ����� ����� �� ������������ � �� ������� ����
		else {
			memmove(szAddrList, MRA_FILES_NULL_ADDRR, sizeof(MRA_FILES_NULL_ADDRR));
			dwAddrListSize = (sizeof(MRA_FILES_NULL_ADDRR)-1);
		}

		if (dwAddrListSize) {
			dat->hWaitHandle = CreateEvent(NULL, TRUE, FALSE, NULL);
			if (dat->bSending == FALSE) // ����������� ���������� ����������, �� ���� ����������� �� ������
				MraFileTransferAck(FILE_TRANSFER_MIRROR, szEMail, dwEMailSize, dat->dwIDRequest, (LPBYTE)szAddrList, dwAddrListSize);
			else {  // ����� ���������� ������ �� ��������(������������ ����������)
				// ������ ��������� ������ ������ ��� �������� ������ �������
				LPWSTR lpwszFiles, lpwszCurPos;
				size_t dwFilesSize;

				dwFilesSize = ((MAX_PATH*2)*dat->dwFilesCount);
				lpwszFiles = (LPWSTR)mir_calloc((dwFilesSize*sizeof(WCHAR)));
				if (lpwszFiles) {
					lpwszCurPos = lpwszFiles;
					for (size_t i = 0;i < dat->dwFilesCount;i++) {
						MRA_FILES_QUEUE_FILE &p = dat->pmfqfFiles[i];
						lpwszCurPos += mir_sntprintf(lpwszCurPos, (dwFilesSize-((size_t)lpwszCurPos-(size_t)lpwszFiles)), L"%s;%I64u;",
							GetFileNameFromFullPathW(p.lpwszName, p.dwNameLen), p.dwSize);
					}
					dwFilesSize = (lpwszCurPos-lpwszFiles);// size in WCHARs

					if (dat->hMraMrimProxyData) {
						// ������������� ������ ��� ���������� ������, ���� ��� ���������
						LPSTR lpszFiles;
						size_t dwFilesSizeA;

						dwFilesSizeA = WideCharToMultiByte(MRA_CODE_PAGE, 0, lpwszFiles, dwFilesSize, NULL, 0, NULL, NULL);
						lpszFiles = (LPSTR)mir_calloc((dwFilesSizeA+MAX_PATH));
						if (lpszFiles) {
							dwFilesSizeA = WideCharToMultiByte(MRA_CODE_PAGE, 0, lpwszFiles, dwFilesSize, lpszFiles, (dwFilesSizeA+MAX_PATH), NULL, NULL);
							MraMrimProxySetData(dat->hMraMrimProxyData, szEMail, dwEMailSize, dat->dwIDRequest, MRIM_PROXY_TYPE_FILES, lpszFiles, dwFilesSizeA, NULL, 0, NULL);
							mir_free(lpszFiles);
						}
					}
					MraFileTransfer(szEMail, dwEMailSize, dat->dwIDRequest, dat->dwFilesTotalSize, lpwszFiles, dwFilesSize, szAddrList, dwAddrListSize);

					mir_free(lpwszFiles);
				}
			}
			WaitForSingleObjectEx(dat->hWaitHandle, INFINITE, FALSE);
			CloseHandle(dat->hWaitHandle);
			dat->hWaitHandle = NULL;
		}
	}
	return dat->hConnection;
}

// This function is called from the Netlib when someone is connecting to
// one of our incomming DC ports
void MraFilesQueueConnectionReceived(HANDLE hNewConnection, DWORD dwRemoteIP, void *pExtra)
{
	if (pExtra) {
		MRA_FILES_QUEUE_ITEM *dat = (MRA_FILES_QUEUE_ITEM*)pExtra;

		ProtoBroadcastAck(dat->ppro->m_szModuleName, dat->hContact, ACKTYPE_FILE, ACKRESULT_CONNECTED, (HANDLE)dat->dwIDRequest, 0);
		if ( dat->ppro->MraFilesQueueHandCheck(hNewConnection, dat)) { // ����� ������������� � ��� ��� �����
			dat->hConnection = hNewConnection;
			ProtoBroadcastAck(dat->ppro->m_szModuleName, dat->hContact, ACKTYPE_FILE, ACKRESULT_CONNECTED, (HANDLE)dat->dwIDRequest, 0);
			dat->ppro->setDword(dat->hContact, "OldIP", dat->ppro->getDword(dat->hContact, "IP", 0));
			dat->ppro->setDword(dat->hContact, "IP", dwRemoteIP);
			SetEvent(dat->hWaitHandle);
		}else {// ������� ��� �� �� ���� ������������ :)
			ProtoBroadcastAck(dat->ppro->m_szModuleName, dat->hContact, ACKTYPE_FILE, ACKRESULT_LISTENING, (HANDLE)dat->dwIDRequest, 0);
			Netlib_CloseHandle(hNewConnection);
		}
	}
	else Netlib_CloseHandle(hNewConnection);
}

/////////////////////////////////////////////////////////////////////////////////////////
// Receive files

DWORD CMraProto::MraFilesQueueAddReceive(HANDLE hFilesQueueHandle, DWORD dwFlags, HANDLE hContact, DWORD dwIDRequest, LPWSTR lpwszFiles, size_t dwFilesSize, LPSTR lpszAddreses, size_t dwAddresesSize)
{
	if (!hFilesQueueHandle || !dwIDRequest)
		return ERROR_INVALID_HANDLE;

	MRA_FILES_QUEUE *pmrafqFilesQueue = (MRA_FILES_QUEUE*)hFilesQueueHandle;
	MRA_FILES_QUEUE_ITEM *dat = (MRA_FILES_QUEUE_ITEM*)mir_calloc(sizeof(MRA_FILES_QUEUE_ITEM)+sizeof(LPSTR)+64);
	if ( !dat)
		return GetLastError();

	WCHAR szBuff[MAX_PATH];
	LPWSTR lpwszCurrentItem, lpwszDelimiter, lpwszEndItem;
	size_t dwMemSize, dwAllocatedCount, dwFileNameTotalSize;

	//dat->lmtListMTItem;
	dat->ppro = this;
	dat->bIsWorking = TRUE;
	dat->dwSendTime = GetTickCount();
	dat->dwIDRequest = dwIDRequest;
	dat->dwFlags = dwFlags;
	dat->hContact = hContact;
	if (getByte(NULL, "FileSendEnableMRIMProxyCons", MRA_DEF_FS_ENABLE_MRIM_PROXY_CONS))
		dat->hMraMrimProxyData = MraMrimProxyCreate();

	dwFileNameTotalSize = 0;
	dwAllocatedCount = ALLOCATED_COUNT;
	dat->dwFilesCount = 0;
	dat->dwFilesTotalSize = 0;
	dat->pmfqfFiles = (MRA_FILES_QUEUE_FILE*)mir_calloc((sizeof(MRA_FILES_QUEUE_FILE)*dwAllocatedCount));
	lpwszCurrentItem = lpwszFiles;
	while (TRUE) {
		lpwszDelimiter = (LPWSTR)MemoryFind(((size_t)lpwszCurrentItem-(size_t)lpwszFiles), lpwszFiles, (dwFilesSize*sizeof(WCHAR)), ";", 2);
		if (!lpwszDelimiter)
			break;

		lpwszEndItem = (LPWSTR)MemoryFind((((size_t)lpwszDelimiter+2)-(size_t)lpwszFiles), lpwszFiles, (dwFilesSize*sizeof(WCHAR)), ";", 2);
		if (!lpwszEndItem)
			break;

		if (dat->dwFilesCount == dwAllocatedCount) {
			dwAllocatedCount *= 2;
			dat->pmfqfFiles = (MRA_FILES_QUEUE_FILE*)mir_realloc(dat->pmfqfFiles, (sizeof(MRA_FILES_QUEUE_FILE)*dwAllocatedCount));
		}

		MRA_FILES_QUEUE_FILE &p = dat->pmfqfFiles[dat->dwFilesCount];
		*lpwszDelimiter++ = 0;
		p.lpwszName = mir_wstrdup(lpwszCurrentItem);
		p.dwNameLen = lstrlen(p.lpwszName);
		p.dwSize = StrToUNum64((LPSTR)lpwszDelimiter, (size_t)lpwszEndItem-(size_t)lpwszDelimiter+1);
		dat->dwFilesTotalSize += p.dwSize;
		dwFileNameTotalSize += p.dwNameLen * sizeof(TCHAR);

		dat->dwFilesCount++;
		lpwszCurrentItem = lpwszEndItem+1;
	}
	dat->pmfqfFiles = (MRA_FILES_QUEUE_FILE*)mir_realloc(dat->pmfqfFiles, (sizeof(MRA_FILES_QUEUE_FILE)*(dat->dwFilesCount+4)));

	dwMemSize = (((dat->dwFilesCount+4)*64)+(dwFileNameTotalSize*sizeof(WCHAR))+(dwAddresesSize*sizeof(WCHAR))+128);
	dat->pwszFilesList = (LPWSTR)mir_calloc(dwMemSize);
	dat->pwszDescription = (LPWSTR)mir_calloc(dwMemSize);

	lpwszDelimiter = dat->pwszFilesList;
	lpwszCurrentItem = dat->pwszDescription;
	StrFormatByteSizeW(dat->dwFilesTotalSize, szBuff, SIZEOF(szBuff));
	lpwszCurrentItem += mir_sntprintf(lpwszCurrentItem, ((dwMemSize-((size_t)lpwszCurrentItem-(size_t)dat->pwszDescription))/sizeof(WCHAR)), L"%I64u Files (%s)\r\n", dat->dwFilesCount, szBuff);

	// description + filesnames
	for (size_t i = 0; i < dat->dwFilesCount; i++) {
		lpwszDelimiter += mir_sntprintf(lpwszDelimiter, ((dwMemSize-((size_t)lpwszDelimiter-(size_t)dat->pwszFilesList))/sizeof(WCHAR)), L"%s", dat->pmfqfFiles[i].lpwszName);
		StrFormatByteSizeW(dat->pmfqfFiles[i].dwSize, szBuff, SIZEOF(szBuff));
		lpwszCurrentItem += mir_sntprintf(lpwszCurrentItem, ((dwMemSize-((size_t)lpwszCurrentItem-(size_t)dat->pwszDescription))/sizeof(WCHAR)), L"%s - %s\r\n", dat->pmfqfFiles[i].lpwszName, szBuff);
	}

	lpwszCurrentItem += MultiByteToWideChar(MRA_CODE_PAGE, 0, lpszAddreses, dwAddresesSize, lpwszCurrentItem, ((dwMemSize-((size_t)lpwszCurrentItem-(size_t)dat->pwszDescription))/sizeof(WCHAR)));
	*lpwszCurrentItem = 0;

	MraAddrListGetFromBuff(lpszAddreses, dwAddresesSize, &dat->malAddrList);
	MraAddrListStoreToContact(dat->hContact, &dat->malAddrList);
	{
		mt_lock l(pmrafqFilesQueue);
		ListMTItemAdd(pmrafqFilesQueue, dat, dat);
	}

	// Send chain event
	PROTORECVFILET prf;
	prf.flags = PREF_UNICODE;
	prf.timestamp = _time32(NULL);
	prf.tszDescription = dat->pwszDescription;
	prf.fileCount = 1;//dat->dwFilesCount;
	prf.ptszFiles = &dat->pwszFilesList;
	prf.lParam = dwIDRequest;
	ProtoChainRecvFile(hContact, &prf);
	return NO_ERROR;
}

void CMraProto::MraFilesQueueRecvThreadProc(LPVOID lpParameter)
{
	DWORD dwRetErrorCode = NO_ERROR;

	if (lpParameter)
	{
		MRA_FILES_QUEUE *pmrafqFilesQueue = (MRA_FILES_QUEUE*)((MRA_FILES_THREADPROC_PARAMS*)lpParameter)->hFilesQueueHandle;
		MRA_FILES_QUEUE_ITEM *dat = ((MRA_FILES_THREADPROC_PARAMS*)lpParameter)->dat;

		WCHAR wszFileName[MAX_FILEPATH] = {0};
		WCHAR szErrorText[2048];
		BYTE btBuff[BUFF_SIZE_RCV];
		BOOL bContinue, bFailed, bOK, bConnected;
		DWORD dwReceived, dwUpdateTimeNext, dwUpdateTimeCur;
		HANDLE hFile;
		size_t i, dwBuffSizeUsed;
		LARGE_INTEGER liFileSize;
		NETLIBSELECT nls = {0};
		PROTOFILETRANSFERSTATUS pfts = {0};

		mir_free(lpParameter);

		bFailed = TRUE;
		bConnected = FALSE;
		nls.cbSize = sizeof(nls);
		pfts.cbSize = sizeof(pfts);
		pfts.hContact = dat->hContact;
		pfts.flags = (PFTS_RECEIVING|PFTS_UNICODE);//		pfts.sending = dat->bSending;	//true if sending, false if receiving
		//pfts.files;
		pfts.totalFiles = dat->dwFilesCount;
		//pfts.currentFileNumber = 0;
		pfts.totalBytes = dat->dwFilesTotalSize;
		//pfts.totalProgress = 0;
		pfts.wszWorkingDir = dat->lpwszPath;
		//pfts.currentFile;
		//pfts.currentFileSize;
		//pfts.currentFileProgress;
		//pfts.currentFileTime;  //as seconds since 1970

		if (MraFilesQueueConnectOut(dat))
		{
			bConnected = TRUE;
		}else {
			if (MraFilesQueueConnectIn(dat))
			{
				bConnected = TRUE;
			}else {
				if (InterlockedExchangeAdd((volatile LONG*)&dat->bIsWorking, 0))
				{
					ProtoBroadcastAck(dat->hContact, ACKRESULT_CONNECTPROXY, ACKRESULT_CONNECTED, (HANDLE)dat->dwIDRequest, 0);
					if (MraMrimProxyConnect(dat->hMraMrimProxyData, &dat->hConnection) == NO_ERROR)
					{// ������������ � ������, ��������� �� �� ������ (��� ���, �� ���� ��� ����)
						if (MraFilesQueueHandCheck(dat->hConnection, dat))
						{// ����� ������������� � ��� ��� �����// dat->bSending
							ProtoBroadcastAck(dat->hContact, ACKTYPE_FILE, ACKRESULT_CONNECTED, (HANDLE)dat->dwIDRequest, 0);
							bConnected = TRUE;
						}
					}
				}
			}
		}

		if (bConnected)
		{// email verified
			bFailed = FALSE;
			for (i = 0;i<dat->dwFilesCount;i++)
			{// receiving files
				pfts.currentFileNumber = i;
				pfts.wszCurrentFile = wszFileName;
				pfts.currentFileSize = dat->pmfqfFiles[i].dwSize;
				pfts.currentFileProgress = 0;
				//pfts.currentFileTime;  //as seconds since 1970

				if ((dat->dwPathSize+dat->pmfqfFiles[i].dwNameLen)<SIZEOF(wszFileName))
				{
					memmove(wszFileName, dat->lpwszPath, (dat->dwPathSize*sizeof(WCHAR)));
					memmove((wszFileName+dat->dwPathSize), dat->pmfqfFiles[i].lpwszName, ((dat->pmfqfFiles[i].dwNameLen+1)*sizeof(WCHAR)));
					wszFileName[dat->dwPathSize+dat->pmfqfFiles[i].dwNameLen] = 0;
				}else {
					dwRetErrorCode = ERROR_BAD_PATHNAME;
					ShowFormattedErrorMessage(L"Receive files: error", dwRetErrorCode);
					bFailed = TRUE;
					break;
				}

				//***deb add
				//dwBuffSizeUsed = ProtoBroadcastAck(dat->hContact, ACKTYPE_FILE, ACKRESULT_FILERESUME, (HANDLE)dat->dwIDRequest, (LPARAM)&pfts);

				ProtoBroadcastAck(dat->hContact, ACKTYPE_FILE, ACKRESULT_NEXTFILE, (HANDLE)dat->dwIDRequest, 0);

				//dwBuffSizeUsed = (mir_snprintf((LPSTR)btBuff, SIZEOF(btBuff), "%s %S", MRA_FT_GET_FILE, dat->pmfqfFiles[i].lpwszName)+1);
				memmove(btBuff, MRA_FT_GET_FILE, sizeof(MRA_FT_GET_FILE));
				btBuff[(sizeof(MRA_FT_GET_FILE)-1)] = ' ';
				dwBuffSizeUsed = sizeof(MRA_FT_GET_FILE)+WideCharToMultiByte(MRA_CODE_PAGE, 0, dat->pmfqfFiles[i].lpwszName, dat->pmfqfFiles[i].dwNameLen, (LPSTR)(btBuff+sizeof(MRA_FT_GET_FILE)), (SIZEOF(btBuff)-sizeof(MRA_FT_GET_FILE)), NULL, NULL);
				btBuff[dwBuffSizeUsed] = 0;
				dwBuffSizeUsed++;

				if (dwBuffSizeUsed == Netlib_Send(dat->hConnection, (LPSTR)btBuff, dwBuffSizeUsed, 0))
				{// file request sended
					hFile = CreateFileW(wszFileName, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
					if (hFile != INVALID_HANDLE_VALUE)
					{// file opened/created, pre allocating disk space, for best perfomance
						bOK = FALSE;

						liFileSize.QuadPart = (LONGLONG)dat->pmfqfFiles[i].dwSize;
						if (SetFilePointerEx(hFile, liFileSize, NULL, FILE_BEGIN))
						if (SetEndOfFile(hFile))
						{
							liFileSize.QuadPart = 0;
							bOK = SetFilePointerEx(hFile, liFileSize, NULL, FILE_BEGIN);
						}

						if (bOK)
						{// disk space pre allocated
							bOK = FALSE;
							bContinue = TRUE;
							dwUpdateTimeNext = GetTickCount();
							nls.dwTimeout = (1000*getDword(NULL, "TimeOutReceiveFileData", MRA_DEF_FS_TIMEOUT_RECV));
							nls.hReadConns[0] = dat->hConnection;
							ProtoBroadcastAck(dat->hContact, ACKTYPE_FILE, ACKRESULT_DATA, (HANDLE)dat->dwIDRequest, (LPARAM)&pfts);

							while (bContinue)
							{
								switch (CallService(MS_NETLIB_SELECT, 0, (LPARAM)&nls)) {
								case SOCKET_ERROR:
								case 0:// Time out
									dwRetErrorCode = GetLastError();
									ShowFormattedErrorMessage(L"Receive files: error on receive file data", dwRetErrorCode);
									bContinue = FALSE;
									break;
								case 1:
									dwReceived = Netlib_Recv(dat->hConnection, (LPSTR)&btBuff, SIZEOF(btBuff), 0);
									if (dwReceived == 0 || dwReceived == SOCKET_ERROR)
									{
										dwRetErrorCode = GetLastError();
										ShowFormattedErrorMessage(L"Receive files: error on receive file data", dwRetErrorCode);
										bContinue = FALSE;
									}else {
										if (WriteFile(hFile, (LPVOID)&btBuff, dwReceived, &dwReceived, NULL))
										{
											pfts.currentFileProgress += dwReceived;
											pfts.totalProgress += dwReceived;

											// progress updates
											dwUpdateTimeCur = GetTickCount();
											if (dwUpdateTimeNext <= dwUpdateTimeCur || pfts.currentFileProgress >= dat->pmfqfFiles[i].dwSize)
											{// we update it
												dwUpdateTimeNext = dwUpdateTimeCur+MRA_FILES_QUEUE_PROGRESS_INTERVAL;
												ProtoBroadcastAck(dat->hContact, ACKTYPE_FILE, ACKRESULT_DATA, (HANDLE)dat->dwIDRequest, (LPARAM)&pfts);

												if (pfts.currentFileProgress >= dat->pmfqfFiles[i].dwSize)
												{// file received
													bOK = TRUE;
													bContinue = FALSE;
												}
											}
										}else {// err on write file
											dwRetErrorCode = GetLastError();
											ShowFormattedErrorMessage(L"Receive files: cant write file data, error", dwRetErrorCode);
											bContinue = FALSE;
										}
									}
									break;
								}
							}// end while
						}else {// err allocating file disk space
							dwRetErrorCode = GetLastError();
							mir_sntprintf(szErrorText, SIZEOF(szErrorText), TranslateW(L"Receive files: cant allocate disk space for file, size %lu bytes, error"), dat->pmfqfFiles[i].dwSize);
							ShowFormattedErrorMessage(szErrorText, dwRetErrorCode);
						}
						CloseHandle(hFile);

						if (bOK == FALSE)
						{// file recv failed
							DeleteFileW(wszFileName);
							bFailed = TRUE;
							break;
						}
					}else {// err on open file
						dwRetErrorCode = GetLastError();
						mir_sntprintf(szErrorText, SIZEOF(szErrorText), TranslateW(L"Receive files: cant open file %s, error"), wszFileName);
						ShowFormattedErrorMessage(szErrorText, dwRetErrorCode);
						bFailed = TRUE;
						break;
					}
				}else {// err on send request for file
					dwRetErrorCode = GetLastError();
					mir_sntprintf(szErrorText, SIZEOF(szErrorText), TranslateW(L"Receive files: request for file %s not sended, error"), dat->pmfqfFiles[i].lpwszName);
					ShowFormattedErrorMessage(szErrorText, NO_ERROR);
					bFailed = TRUE;
					break;
				}
			}// end for

			Netlib_CloseHandle(dat->hConnection);
			dat->hConnection = NULL;
		}

		if (bFailed) {
			CHAR szEMail[MAX_EMAIL_LEN];
			size_t dwEMailSize;

			if (mraGetStaticStringA(dat->hContact, "e-mail", szEMail, SIZEOF(szEMail), &dwEMailSize))
				MraFileTransferAck(FILE_TRANSFER_STATUS_ERROR, szEMail, dwEMailSize, dat->dwIDRequest, NULL, 0);

			ProtoBroadcastAck(dat->hContact, ACKTYPE_FILE, ACKRESULT_FAILED, (HANDLE)dat->dwIDRequest, 0);
		}
		else ProtoBroadcastAck(dat->hContact, ACKTYPE_FILE, ACKRESULT_SUCCESS, (HANDLE)dat->dwIDRequest, 0);

		mt_lock l(pmrafqFilesQueue);
		MraFilesQueueItemFree(dat);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
// Send files

DWORD CMraProto::MraFilesQueueAddSend(HANDLE hFilesQueueHandle, DWORD dwFlags, HANDLE hContact, LPWSTR *plpwszFiles, size_t dwFilesCount, DWORD *pdwIDRequest)
{
	if ( !hFilesQueueHandle)
		return ERROR_INVALID_HANDLE;

	MRA_FILES_QUEUE *pmrafqFilesQueue = (MRA_FILES_QUEUE*)hFilesQueueHandle;
	MRA_FILES_QUEUE_ITEM *dat = (MRA_FILES_QUEUE_ITEM*)mir_calloc(sizeof(MRA_FILES_QUEUE_ITEM)+sizeof(LPSTR)+64);
	if ( !dat)
		return GetLastError();

	ULARGE_INTEGER uliFileSize;
	WIN32_FILE_ATTRIBUTE_DATA wfad;

	dat->ppro = this;
	dat->bIsWorking = TRUE;
	dat->dwSendTime = GetTickCount();
	dat->dwIDRequest = InterlockedIncrement((LONG volatile*)&dwCMDNum);// ����������, ��������� �������������
	dat->dwFlags = dwFlags;
	dat->hContact = hContact;
	if (getByte(NULL, "FileSendEnableMRIMProxyCons", MRA_DEF_FS_ENABLE_MRIM_PROXY_CONS)) dat->hMraMrimProxyData = MraMrimProxyCreate();
	dat->dwFilesCount = dwFilesCount;
	dat->pmfqfFiles = (MRA_FILES_QUEUE_FILE*)mir_calloc((sizeof(MRA_FILES_QUEUE_FILE)*(dat->dwFilesCount+1)));
	dat->dwFilesTotalSize = 0;

	for (size_t i = 0; i < dat->dwFilesCount; i++) {
		MRA_FILES_QUEUE_FILE &p = dat->pmfqfFiles[i];
		if (GetFileAttributesExW(plpwszFiles[i], GetFileExInfoStandard, &wfad)) {
			uliFileSize.LowPart = wfad.nFileSizeLow;
			uliFileSize.HighPart = wfad.nFileSizeHigh;
			p.dwSize = uliFileSize.QuadPart;
			dat->dwFilesTotalSize += uliFileSize.QuadPart;
		}
		else p.dwSize = 0;

		p.dwNameLen = lstrlenW(plpwszFiles[i]);
		p.lpwszName = mir_wstrdup(plpwszFiles[i]);
	}

	dat->bSending = TRUE;
	if (pdwIDRequest) *pdwIDRequest = dat->dwIDRequest;

	{
		mt_lock l(pmrafqFilesQueue);
		ListMTItemAdd(pmrafqFilesQueue, dat, dat);
	}
	MRA_FILES_THREADPROC_PARAMS *pmftpp = (MRA_FILES_THREADPROC_PARAMS*)mir_calloc(sizeof(MRA_FILES_THREADPROC_PARAMS));
	pmftpp->hFilesQueueHandle = hFilesQueueHandle;
	pmftpp->dat = dat;
	dat->hThread = ForkThreadEx(&CMraProto::MraFilesQueueSendThreadProc, pmftpp, 0);

	return NO_ERROR;
}

void CMraProto::MraFilesQueueSendThreadProc(LPVOID lpParameter)
{
	DWORD dwRetErrorCode = NO_ERROR;

	if ( !lpParameter)
		return;

	MRA_FILES_QUEUE *pmrafqFilesQueue = (MRA_FILES_QUEUE*)((MRA_FILES_THREADPROC_PARAMS*)lpParameter)->hFilesQueueHandle;
	MRA_FILES_QUEUE_ITEM *dat = ((MRA_FILES_THREADPROC_PARAMS*)lpParameter)->dat;
	mir_free(lpParameter);

	CHAR szFileName[MAX_FILEPATH] = {0};
	WCHAR szErrorText[2048];
	BYTE btBuff[BUFF_SIZE_RCV];
	BOOL bFailed = TRUE, bOK, bConnected = FALSE;
	DWORD dwReceived, dwSendBlockSize, dwUpdateTimeNext, dwUpdateTimeCur;
	size_t i, j, dwBuffSizeUsed = 0;
	LPWSTR lpwszFileName;

	PROTOFILETRANSFERSTATUS pfts = {0};
	pfts.cbSize = sizeof(pfts);
	pfts.hContact = dat->hContact;
	pfts.flags = (PFTS_SENDING|PFTS_UNICODE);// pfts.sending = dat->bSending;	//true if sending, false if receiving
	pfts.totalFiles = dat->dwFilesCount;
	pfts.totalBytes = dat->dwFilesTotalSize;
	pfts.wszWorkingDir = dat->lpwszPath;

	dwSendBlockSize = getDword(NULL, "FileSendBlockSize", MRA_DEFAULT_FILE_SEND_BLOCK_SIZE);
	if (dwSendBlockSize>SIZEOF(btBuff)) dwSendBlockSize = SIZEOF(btBuff);
	if (dwSendBlockSize<512) dwSendBlockSize = MRA_DEFAULT_FILE_SEND_BLOCK_SIZE;

	if (MraFilesQueueConnectIn(dat))
		bConnected = TRUE;
	else if (MraFilesQueueConnectOut(dat))
		bConnected = TRUE;
	else {
		if (InterlockedExchangeAdd((volatile LONG*)&dat->bIsWorking, 0)) {
			ProtoBroadcastAck(dat->hContact, ACKRESULT_CONNECTPROXY, ACKRESULT_CONNECTED, (HANDLE)dat->dwIDRequest, 0);
			if (MraMrimProxyConnect(dat->hMraMrimProxyData, &dat->hConnection) == NO_ERROR) {
				// ������������ � ������, ��������� �� �� ������ (��� ���, �� ���� ��� ����)
				if (MraFilesQueueHandCheck(dat->hConnection, dat)) {
					// ����� ������������� � ��� ��� �����// dat->bSending
					ProtoBroadcastAck(dat->hContact, ACKTYPE_FILE, ACKRESULT_CONNECTED, (HANDLE)dat->dwIDRequest, 0);
					bConnected = TRUE;
				}
			}
		}
	}

	if (bConnected) { // email verified
		bFailed = FALSE;
		for (i = 0; i < dat->dwFilesCount; i++) { // sending files
			ProtoBroadcastAck(dat->hContact, ACKTYPE_FILE, ACKRESULT_NEXTFILE, (HANDLE)dat->dwIDRequest, 0);

			dwBuffSizeUsed = 0;
			while (TRUE) {
				dwReceived = Netlib_Recv(dat->hConnection, ((LPSTR)btBuff+dwBuffSizeUsed), (SIZEOF(btBuff)-dwBuffSizeUsed), 0);
				if (dwReceived == 0 || dwReceived == SOCKET_ERROR) { // err on receive file name to send
					dwRetErrorCode = GetLastError();
					ShowFormattedErrorMessage(L"Send files: file send request not received, error", dwRetErrorCode);
					bFailed = TRUE;
					break;
				}
				else {
					dwBuffSizeUsed += dwReceived;
					if (MemoryFindByte((dwBuffSizeUsed-dwReceived), btBuff, dwBuffSizeUsed, 0))
						break;
				}
			}// end while (file name passible received)*/

			if (bFailed)
				break;

			// ...received
			if (dwBuffSizeUsed > (sizeof(MRA_FT_GET_FILE)+1)) {// file name received
				if (CompareStringA( MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), NORM_IGNORECASE, (LPSTR)btBuff, (sizeof(MRA_FT_GET_FILE)-1), MRA_FT_GET_FILE, (sizeof(MRA_FT_GET_FILE)-1)) == CSTR_EQUAL) {
					// MRA_FT_GET_FILE verified
					bFailed = TRUE;
					for (j = 0;j<dat->dwFilesCount;j++) {
						lpwszFileName = GetFileNameFromFullPathW(dat->pmfqfFiles[j].lpwszName, dat->pmfqfFiles[j].dwNameLen);
						szFileName[WideCharToMultiByte(MRA_CODE_PAGE, 0, lpwszFileName, (dat->pmfqfFiles[j].dwNameLen-(lpwszFileName-dat->pmfqfFiles[j].lpwszName)), szFileName, SIZEOF(szFileName), NULL, NULL)] = 0;

						if (CompareStringA( MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), NORM_IGNORECASE, (((LPSTR)btBuff)+sizeof(MRA_FT_GET_FILE)), (dwBuffSizeUsed-(sizeof(MRA_FT_GET_FILE)+1)), szFileName, -1) == CSTR_EQUAL) {
							bFailed = FALSE;
							break;
						}
					}

					if (bFailed == FALSE) {
						HANDLE hFile = CreateFileW(dat->pmfqfFiles[j].lpwszName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, (FILE_ATTRIBUTE_NORMAL|FILE_FLAG_SEQUENTIAL_SCAN), NULL);
						if (hFile != INVALID_HANDLE_VALUE) {
							bOK = FALSE;
							dwUpdateTimeNext = GetTickCount();
							pfts.currentFileNumber = i;
							pfts.wszCurrentFile = dat->pmfqfFiles[j].lpwszName;
							pfts.currentFileSize = dat->pmfqfFiles[j].dwSize;
							pfts.currentFileProgress = 0;
							//pfts.currentFileTime;  //as seconds since 1970

							WideCharToMultiByte(MRA_CODE_PAGE, 0, dat->pmfqfFiles[j].lpwszName, dat->pmfqfFiles[j].dwNameLen, szFileName, SIZEOF(szFileName), NULL, NULL);
							ProtoBroadcastAck(dat->hContact, ACKTYPE_FILE, ACKRESULT_DATA, (HANDLE)dat->dwIDRequest, (LPARAM)&pfts);

							while (TRUE) { // read and sending
								if (ReadFile(hFile, btBuff, dwSendBlockSize, (DWORD*)&dwBuffSizeUsed, NULL)) {
									dwReceived = Netlib_Send(dat->hConnection, (LPSTR)btBuff, dwBuffSizeUsed, 0);
									if (dwBuffSizeUsed == dwReceived) {
										pfts.currentFileProgress += dwBuffSizeUsed;
										pfts.totalProgress += dwBuffSizeUsed;

										// progress updates
										dwUpdateTimeCur = GetTickCount();
										if (dwUpdateTimeNext <= dwUpdateTimeCur || pfts.currentFileProgress >= dat->pmfqfFiles[j].dwSize) { // we update it
											dwUpdateTimeNext = dwUpdateTimeCur+MRA_FILES_QUEUE_PROGRESS_INTERVAL;

											ProtoBroadcastAck(dat->hContact, ACKTYPE_FILE, ACKRESULT_DATA, (HANDLE)dat->dwIDRequest, (LPARAM)&pfts);

											if (pfts.currentFileProgress >= dat->pmfqfFiles[j].dwSize) { // file received
												bOK = TRUE;
												break;
											}
										}
									}
									else { // err on send file data
										dwRetErrorCode = GetLastError();
										ShowFormattedErrorMessage(L"Send files: error on send file data", dwRetErrorCode);
										break;
									}
								}
								else {// read failure
									dwRetErrorCode = GetLastError();
									ShowFormattedErrorMessage(L"Send files: cant read file data, error", dwRetErrorCode);
									break;
								}
							}// end while
							CloseHandle(hFile);

							if (bOK == FALSE) { // file recv failed
								bFailed = TRUE;
								break;
							}
						}
						else {// err on open file
							dwRetErrorCode = GetLastError();
							mir_sntprintf(szErrorText, SIZEOF(szErrorText), TranslateW(L"Send files: cant open file %s, error"), dat->pmfqfFiles[j].lpwszName);
							ShowFormattedErrorMessage(szErrorText, dwRetErrorCode);
							bFailed = TRUE;
							break;
						}
					}
					else {
						mir_sntprintf(szErrorText, SIZEOF(szErrorText), TranslateW(L"Send files: requested file: %S - not found in send files list."), (((LPSTR)btBuff)+sizeof(MRA_FT_GET_FILE)));
						ShowFormattedErrorMessage(szErrorText, NO_ERROR);
						bFailed = TRUE;
						break;
					}
				}
				else {// err on receive, trash
					ShowFormattedErrorMessage(L"Send files: bad file send request - invalid header", NO_ERROR);
					bFailed = TRUE;
					break;
				}
			}
			else {// bad file name or trash
				ShowFormattedErrorMessage(L"Send files: bad file send request - to small packet", NO_ERROR);
				bFailed = TRUE;
				break;
			}
		}// end for

		Netlib_CloseHandle(dat->hConnection);
		dat->hConnection = NULL;
	}

	if (bFailed) {
		CHAR szEMail[MAX_EMAIL_LEN];
		size_t dwEMailSize;

		if (mraGetStaticStringA(dat->hContact, "e-mail", szEMail, SIZEOF(szEMail), &dwEMailSize))
			MraFileTransferAck(FILE_TRANSFER_STATUS_ERROR, szEMail, dwEMailSize, dat->dwIDRequest, NULL, 0);

		ProtoBroadcastAck(dat->hContact, ACKTYPE_FILE, ACKRESULT_FAILED, (HANDLE)dat->dwIDRequest, 0);
	}
	else ProtoBroadcastAck(dat->hContact, ACKTYPE_FILE, ACKRESULT_SUCCESS, (HANDLE)dat->dwIDRequest, 0);

	mt_lock l(pmrafqFilesQueue);
	MraFilesQueueItemFree(dat);
}
