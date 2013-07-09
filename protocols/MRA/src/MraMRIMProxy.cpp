#include "Mra.h"
#include "MraMRIMProxy.h"
#include "proto.h"

struct MRA_MRIMPROXY_DATA
{
	LPSTR			lpszEMail;		// LPS to
	size_t			dwEMailSize;
	DWORD			dwIDRequest;	// DWORD id_request
	DWORD			dwDataType;		// DWORD data_type
	LPSTR			lpszUserData;	// LPS user_data
	size_t			dwUserDataSize;
	MRA_ADDR_LIST	malAddrList;	// LPS lps_ip_port
	MRA_GUID		mguidSessionID;	// DWORD session_id[4]
	HANDLE			hConnection;
	HANDLE			hWaitHandle;	// internal
};

HANDLE MraMrimProxyCreate()
{
	MRA_MRIMPROXY_DATA *pmmpd = (MRA_MRIMPROXY_DATA*)mir_calloc(sizeof(MRA_MRIMPROXY_DATA));
	return (HANDLE)pmmpd;
}

DWORD MraMrimProxySetData(HANDLE hMraMrimProxyData, LPSTR lpszEMail, size_t dwEMailSize, DWORD dwIDRequest, DWORD dwDataType, LPSTR lpszUserData, size_t dwUserDataSize, LPSTR lpszAddreses, size_t dwAddresesSize, MRA_GUID *pmguidSessionID)
{
	if (!hMraMrimProxyData)
		return ERROR_INVALID_HANDLE;

	MRA_MRIMPROXY_DATA *pmmpd = (MRA_MRIMPROXY_DATA*)hMraMrimProxyData;

	if (lpszEMail && dwEMailSize) {
		mir_free(pmmpd->lpszEMail);
		pmmpd->lpszEMail = (LPSTR)mir_calloc(dwEMailSize);
		memmove(pmmpd->lpszEMail, lpszEMail, dwEMailSize);
		pmmpd->dwEMailSize = dwEMailSize;
	}

	if (dwIDRequest) pmmpd->dwIDRequest = dwIDRequest;
	if (dwDataType) pmmpd->dwDataType = dwDataType;

	if (lpszUserData) {
		mir_free(pmmpd->lpszUserData);
		pmmpd->lpszUserData = (LPSTR)mir_calloc(dwUserDataSize);
		memmove(pmmpd->lpszUserData, lpszUserData, dwUserDataSize);
		pmmpd->dwUserDataSize = dwUserDataSize;
	}

	if (lpszAddreses && dwAddresesSize)
		MraAddrListGetFromBuff(lpszAddreses, dwAddresesSize, &pmmpd->malAddrList);
	if (pmguidSessionID)
		memmove(&pmmpd->mguidSessionID, pmguidSessionID, sizeof(MRA_GUID));

	SetEvent(pmmpd->hWaitHandle);
	return 0;
}

void MraMrimProxyFree(HANDLE hMraMrimProxyData)
{
	if (hMraMrimProxyData) {
		MRA_MRIMPROXY_DATA *pmmpd = (MRA_MRIMPROXY_DATA*)hMraMrimProxyData;

		CloseHandle(pmmpd->hWaitHandle);
		Netlib_CloseHandle(pmmpd->hConnection);
		mir_free(pmmpd->lpszEMail);
		mir_free(pmmpd->lpszUserData);
		MraAddrListFree(&pmmpd->malAddrList);
		mir_free(hMraMrimProxyData);
	}
}

void MraMrimProxyCloseConnection(HANDLE hMraMrimProxyData)
{
	if (hMraMrimProxyData) {
		MRA_MRIMPROXY_DATA *pmmpd = (MRA_MRIMPROXY_DATA*)hMraMrimProxyData;
		SetEvent(pmmpd->hWaitHandle);
		Netlib_CloseHandle(pmmpd->hConnection);
	}
}

DWORD CMraProto::MraMrimProxyConnect(HANDLE hMraMrimProxyData, HANDLE *phConnection)
{
	DWORD dwRetErrorCode;

	if (hMraMrimProxyData && phConnection) {
		BOOL bIsHTTPSProxyUsed, bContinue;
		BYTE lpbBufferRcv[BUFF_SIZE_RCV_MIN_FREE];
		DWORD dwBytesReceived, dwConnectReTryCount, dwCurConnectReTryCount;
		size_t dwRcvBuffSize = BUFF_SIZE_RCV_MIN_FREE, dwRcvBuffSizeUsed;
		NETLIBSELECT nls = {0};
		MRA_MRIMPROXY_DATA *pmmpd = (MRA_MRIMPROXY_DATA*)hMraMrimProxyData;
		NETLIBOPENCONNECTION nloc = {0};

		// ������ ����, ������ ���������� �� ��
		if (pmmpd->malAddrList.dwAddrCount) {
			MraAddrListGetToBuff(&pmmpd->malAddrList, (LPSTR)lpbBufferRcv, SIZEOF(lpbBufferRcv), &dwRcvBuffSizeUsed);
			MraProxyAck(PROXY_STATUS_OK, pmmpd->lpszEMail, pmmpd->dwEMailSize, pmmpd->dwIDRequest, pmmpd->dwDataType, pmmpd->lpszUserData, pmmpd->dwUserDataSize, (LPSTR)lpbBufferRcv, dwRcvBuffSizeUsed, pmmpd->mguidSessionID);
		}
		// �� ����������
		else {
			pmmpd->hWaitHandle = CreateEvent(NULL, TRUE, FALSE, NULL);
			if (pmmpd->lpszEMail && pmmpd->dwEMailSize)
			if (MraProxy(pmmpd->lpszEMail, pmmpd->dwEMailSize, pmmpd->dwIDRequest, pmmpd->dwDataType, pmmpd->lpszUserData, pmmpd->dwUserDataSize, NULL, 0, pmmpd->mguidSessionID))
				WaitForSingleObjectEx(pmmpd->hWaitHandle, INFINITE, FALSE);

			CloseHandle(pmmpd->hWaitHandle);
			pmmpd->hWaitHandle = NULL;
		}

		dwRetErrorCode = ERROR_NO_NETWORK;
		if (pmmpd->malAddrList.dwAddrCount) {
			pmmpd->hConnection = NULL;
			bIsHTTPSProxyUsed = IsHTTPSProxyUsed(hNetlibUser);
			dwConnectReTryCount = getDword(NULL, "ConnectReTryCountMRIMProxy", MRA_DEFAULT_CONN_RETRY_COUNT_MRIMPROXY);
			nloc.cbSize = sizeof(nloc);
			nloc.flags = NLOCF_V2;
			nloc.timeout = ((MRA_TIMEOUT_DIRECT_CONN-1)/(pmmpd->malAddrList.dwAddrCount*dwConnectReTryCount));// -1 ��� ����� ��� �����
			if (nloc.timeout<MRA_TIMEOUT_CONN_MIN) nloc.timeout = MRA_TIMEOUT_CONN_MIN;
			if (nloc.timeout>MRA_TIMEOUT_CONN_���) nloc.timeout = MRA_TIMEOUT_CONN_���;

			// Set up the sockaddr structure
			for (size_t i = 0; i < pmmpd->malAddrList.dwAddrCount && dwRetErrorCode != NO_ERROR; i++) {
				// ����� https ������ ������ 443 ����
				if ((pmmpd->malAddrList.pmaliAddress[i].dwPort == MRA_SERVER_PORT_HTTPS && bIsHTTPSProxyUsed) || bIsHTTPSProxyUsed == FALSE) {
					if (pmmpd->dwDataType == MRIM_PROXY_TYPE_FILES)
						ProtoBroadcastAck(MraHContactFromEmail(pmmpd->lpszEMail, pmmpd->dwEMailSize, FALSE, TRUE, NULL), ACKTYPE_FILE, ACKRESULT_CONNECTING, (HANDLE)pmmpd->dwIDRequest, 0);

					nloc.szHost = inet_ntoa((*((in_addr*)&pmmpd->malAddrList.pmaliAddress[i].dwAddr)));
					nloc.wPort = (WORD)pmmpd->malAddrList.pmaliAddress[i].dwPort;

					dwCurConnectReTryCount = dwConnectReTryCount;
					do {
						pmmpd->hConnection = (HANDLE)CallService(MS_NETLIB_OPENCONNECTION, (WPARAM)hNetlibUser, (LPARAM)&nloc);
					}
						while (--dwCurConnectReTryCount && pmmpd->hConnection == NULL);

					if (pmmpd->hConnection) {
						nls.cbSize = sizeof(nls);
						nls.dwTimeout = (MRA_TIMEOUT_DIRECT_CONN*1000*2);
						nls.hReadConns[0] = pmmpd->hConnection;
						bContinue = TRUE;
						dwRcvBuffSizeUsed = 0;

						if (pmmpd->dwDataType == MRIM_PROXY_TYPE_FILES)
							ProtoBroadcastAck(MraHContactFromEmail(pmmpd->lpszEMail, pmmpd->dwEMailSize, FALSE, TRUE, NULL), ACKTYPE_FILE, ACKRESULT_CONNECTED, (HANDLE)pmmpd->dwIDRequest, 0);
						MraSendPacket(nls.hReadConns[0], 0, MRIM_CS_PROXY_HELLO, &pmmpd->mguidSessionID, sizeof(MRA_GUID));

						while (bContinue) {
							switch (CallService(MS_NETLIB_SELECT, 0, (LPARAM)&nls)) {
							case SOCKET_ERROR:
							case 0:// Time out
								dwRetErrorCode = GetLastError();
								ShowFormattedErrorMessage(L"Disconnected, socket error", dwRetErrorCode);
								bContinue = FALSE;
								break;

							case 1:
								if (dwRcvBuffSizeUsed == BUFF_SIZE_RCV_MIN_FREE) { // bad packet
									bContinue = FALSE;
									DebugBreak();
								}
								else {
									dwBytesReceived = Netlib_Recv(nls.hReadConns[0], (LPSTR)(lpbBufferRcv+dwRcvBuffSizeUsed), (dwRcvBuffSize-dwRcvBuffSizeUsed), 0);
									if (dwBytesReceived && dwBytesReceived != SOCKET_ERROR) { // connected
										dwRcvBuffSizeUsed += dwBytesReceived;
										if (dwRcvBuffSizeUsed >= sizeof(mrim_packet_header_t)) { // packet header received
											if (((mrim_packet_header_t*)lpbBufferRcv)->magic == CS_MAGIC) { // packet OK
												if ((dwRcvBuffSizeUsed-sizeof(mrim_packet_header_t)) >= ((mrim_packet_header_t*)lpbBufferRcv)->dlen) { // full packet received, may be more than one
													if (((mrim_packet_header_t*)lpbBufferRcv)->msg == MRIM_CS_PROXY_HELLO_ACK) // connect OK!
														dwRetErrorCode = NO_ERROR;
													else // bad/wrong
														DebugBreak();

													bContinue = FALSE;
												}
												else // not all packet received, continue receiving
													DebugPrintCRLF(L"Not all packet received, continue receiving");
											}
											else  { // bad packet
												DebugPrintCRLF(L"Bad packet");
												DebugBreak();
												bContinue = FALSE;
											}
										}
										else // packet too small, continue receiving
											DebugPrintCRLF(L"Packet to small, continue receiving");
									}
									else { // disconnected
										dwRetErrorCode = GetLastError();
										ShowFormattedErrorMessage(L"Disconnected, socket read error", dwRetErrorCode);
										bContinue = FALSE;
									}
								}
								break;
							}// end switch
						}// end while
					}
					else dwRetErrorCode = GetLastError();
				}// filtered
			}// end for

			if (dwRetErrorCode != NO_ERROR) { // ������� �� ���� ������������ :)
				Netlib_CloseHandle(pmmpd->hConnection);
				pmmpd->hConnection = NULL;
			}
		}
		*phConnection = pmmpd->hConnection;
	}
	else dwRetErrorCode = ERROR_INVALID_HANDLE;
	return dwRetErrorCode;
}
