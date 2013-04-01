#include "commonheaders.h"

int SendBroadcast(HANDLE hContact, int type, int result, HANDLE hProcess, LPARAM lParam)
{
	ACKDATA ack = { sizeof(ack) };
	ack.szModule = GetContactProto(hContact);
	ack.hContact = hContact;
	ack.type = type;
	ack.result = result;
	ack.hProcess = hProcess;
	ack.lParam = lParam;
	return CallService(MS_PROTO_BROADCASTACK, 0, (LPARAM)&ack);
}

static void sttWaitForExchange(LPVOID param)
{
	HANDLE hContact = (HANDLE)param;
	pUinKey ptr = getUinKey(hContact);
	if (!ptr)
		return;

	for (int i=0; i < db_get_w(0, MODULENAME, "ket", 10)*10; i++) {
		Sleep(100);
		if (ptr->waitForExchange != 1)
			break;
	}

	Sent_NetLog("sttWaitForExchange: %d",ptr->waitForExchange);

	// if keyexchange failed or timeout
	if (ptr->waitForExchange == 1 || ptr->waitForExchange == 3) { // �������� - ���������� ��������������, ���� ����
		if (ptr->msgQueue && msgbox1(0,sim104,MODULENAME,MB_YESNO|MB_ICONQUESTION) == IDYES) {
			EnterCriticalSection(&localQueueMutex);
			ptr->sendQueue = true;
			pWM ptrMessage = ptr->msgQueue;
			while(ptrMessage) {
				Sent_NetLog("Sent (unencrypted) message from queue: %s",ptrMessage->Message);

				// send unencrypted messages
				CallContactService(ptr->hContact,PSS_MESSAGE,(WPARAM)ptrMessage->wParam|PREF_METANODB,(LPARAM)ptrMessage->Message);
				mir_free(ptrMessage->Message);
				pWM tmp = ptrMessage;
				ptrMessage = ptrMessage->nextMessage;
				mir_free(tmp);
			}
			ptr->msgQueue = NULL;
			ptr->sendQueue = false;
			LeaveCriticalSection(&localQueueMutex);
		}
		ptr->waitForExchange = 0;
		ShowStatusIconNotify(ptr->hContact);
	}
	else if (ptr->waitForExchange == 2) { // ������� ������� ����� ������������� ����������
		EnterCriticalSection(&localQueueMutex);
		// we need to resend last send back message with new crypto Key
		pWM ptrMessage = ptr->msgQueue;
		while (ptrMessage) {
			Sent_NetLog("Sent (encrypted) message from queue: %s",ptrMessage->Message);

			// send unencrypted messages
			CallContactService(ptr->hContact,PSS_MESSAGE,(WPARAM)ptrMessage->wParam|PREF_METANODB,(LPARAM)ptrMessage->Message);
			mir_free(ptrMessage->Message);
			pWM tmp = ptrMessage;
			ptrMessage = ptrMessage->nextMessage;
			mir_free(tmp);
		}
		ptr->msgQueue = NULL;
		ptr->waitForExchange = 0;
		LeaveCriticalSection(&localQueueMutex);
	}
	else if (ptr->waitForExchange == 0) { // �������� �������
		EnterCriticalSection(&localQueueMutex);
		// we need to resend last send back message with new crypto Key
		pWM ptrMessage = ptr->msgQueue;
		while (ptrMessage) {
			mir_free(ptrMessage->Message);
			pWM tmp = ptrMessage;
			ptrMessage = ptrMessage->nextMessage;
			mir_free(tmp);
		}
		ptr->msgQueue = NULL;
		LeaveCriticalSection(&localQueueMutex);
	}
}

// set wait flag and run thread
void waitForExchange(pUinKey ptr, int flag)
{
	switch(flag) {
	case 0: // reset
	case 2: // send secure
	case 3: // send unsecure
		if (ptr->waitForExchange)
			ptr->waitForExchange = flag;
		break;
	case 1: // launch
		if (ptr->waitForExchange)
			break;

		ptr->waitForExchange = 1;
		mir_forkthread(sttWaitForExchange, ptr->hContact);
		break;
	}
}


// EOF
