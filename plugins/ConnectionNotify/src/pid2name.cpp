#include "ConnectionNotify.h"

void pid2name(DWORD procid, TCHAR *buffer, size_t bufLen)
{
	HANDLE hSnap = INVALID_HANDLE_VALUE;
	HANDLE hProcess = INVALID_HANDLE_VALUE;
	PROCESSENTRY32 ProcessStruct;
	ProcessStruct.dwSize = sizeof(PROCESSENTRY32);
	hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnap == INVALID_HANDLE_VALUE)
		return;
	if (Process32First(hSnap, &ProcessStruct) == FALSE)
		return;

	do {
		if (ProcessStruct.th32ProcessID == procid) {
			_tcsncpy_s(buffer, bufLen, ProcessStruct.szExeFile, _TRUNCATE);
			break;
		}
	}
		while( Process32Next( hSnap, &ProcessStruct ));

	CloseHandle( hSnap );
}
