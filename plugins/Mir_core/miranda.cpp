/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2012 Miranda ICQ/IM project, 
all portions of this codebase are copyrighted to the people
listed in contributors.txt.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "commonheaders.h"

HWND hAPCWindow = NULL;

int  InitPathUtils(void);
void (*RecalculateTime)(void);

HANDLE hStackMutex, hThreadQueueEmpty;
int hLangpack = 0;
HINSTANCE hInst = 0;

/////////////////////////////////////////////////////////////////////////////////////////
// exception handling

static DWORD __cdecl sttDefaultFilter(DWORD, EXCEPTION_POINTERS*)
{
	return EXCEPTION_EXECUTE_HANDLER;
}

pfnExceptionFilter pMirandaExceptFilter = sttDefaultFilter;

MIR_CORE_DLL(pfnExceptionFilter) GetExceptionFilter()
{
	return pMirandaExceptFilter;
}

MIR_CORE_DLL(pfnExceptionFilter) SetExceptionFilter(pfnExceptionFilter pMirandaExceptFilter)
{
	pfnExceptionFilter oldOne = pMirandaExceptFilter;
	if (pMirandaExceptFilter != 0)
		pMirandaExceptFilter = pMirandaExceptFilter;
	return oldOne;
}

/////////////////////////////////////////////////////////////////////////////////////////
// thread support functions

struct THREAD_WAIT_ENTRY
{
	DWORD dwThreadId;	// valid if hThread isn't signalled
	HANDLE hThread;
	HINSTANCE hOwner;
	void* pObject;
	//PVOID addr;
};

static LIST<THREAD_WAIT_ENTRY> threads(10, NumericKeySortT);

struct FORK_ARG {
	HANDLE hEvent;
	pThreadFunc threadcode;
	pThreadFuncEx threadcodeex;
	void *arg, *owner;
};

/////////////////////////////////////////////////////////////////////////////////////////
// forkthread - starts a new thread

void __cdecl forkthread_r(void * arg)
{
	struct FORK_ARG * fa = (struct FORK_ARG *) arg;
	void (*callercode)(void*)=fa->threadcode;
	void * cookie=fa->arg;
	CallService(MS_SYSTEM_THREAD_PUSH, 0, (LPARAM)callercode);
	SetEvent(fa->hEvent);
	__try
	{
		callercode(cookie);
	}
	__except(pMirandaExceptFilter(GetExceptionCode(), GetExceptionInformation()))
	{
	}

	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
	CallService(MS_SYSTEM_THREAD_POP, 0, 0);
	return;
}

MIR_CORE_DLL(UINT_PTR) forkthread( void (__cdecl *threadcode)(void*), unsigned long stacksize, void *arg)
{
	UINT_PTR rc;
	struct FORK_ARG fa;
	fa.hEvent=CreateEvent(NULL, FALSE, FALSE, NULL);
	fa.threadcode=threadcode;
	fa.arg=arg;
	rc=_beginthread(forkthread_r, stacksize, &fa);
	if ((UINT_PTR)-1L != rc)
		WaitForSingleObject(fa.hEvent, INFINITE);

	CloseHandle(fa.hEvent);
	return rc;
}

/////////////////////////////////////////////////////////////////////////////////////////
// forkthreadex - starts a new thread with the extended info and returns the thread id

unsigned __stdcall forkthreadex_r(void * arg)
{
	struct FORK_ARG *fa = (struct FORK_ARG *)arg;
	pThreadFuncEx threadcode = fa->threadcodeex;
	pThreadFuncOwner threadcodeex = (pThreadFuncOwner)fa->threadcodeex;
	void *cookie = fa->arg;
	void *owner = fa->owner;
	unsigned long rc = 0;

	CallService(MS_SYSTEM_THREAD_PUSH, (WPARAM)fa->owner, (LPARAM)threadcode);
	SetEvent(fa->hEvent);
	__try
	{
		if (owner)
			rc = threadcodeex(owner, cookie);
		else
			rc = threadcode(cookie);
	}
	__except(pMirandaExceptFilter(GetExceptionCode(), GetExceptionInformation()))
	{
	}

	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
	CallService(MS_SYSTEM_THREAD_POP, 0, 0);
	return rc;
}

MIR_CORE_DLL(UINT_PTR) forkthreadex(
	void *sec, 
	unsigned stacksize, 
	unsigned (__stdcall *threadcode)(void*), 
	void* owner, 
	void *arg, 
	unsigned *thraddr)
{
	UINT_PTR rc;
	struct FORK_ARG fa = { 0 };
	fa.threadcodeex = threadcode;
	fa.arg = arg;
	fa.owner = owner;
	fa.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	rc = _beginthreadex(sec, stacksize, forkthreadex_r, (void *)&fa, 0, thraddr);
	if (rc)
		WaitForSingleObject(fa.hEvent, INFINITE);

	CloseHandle(fa.hEvent);
	return rc;
}

/////////////////////////////////////////////////////////////////////////////////////////
// APC and mutex functions

static void __stdcall DummyAPCFunc(ULONG_PTR)
{
	/* called in the context of thread that cleared it's APC queue */
	return;
}

static int MirandaWaitForMutex(HANDLE hEvent)
{
	for (;;) {
		// will get WAIT_IO_COMPLETE for QueueUserAPC() which isnt a result
		DWORD rc = MsgWaitForMultipleObjectsEx(1, &hEvent, INFINITE, QS_ALLINPUT, MWMO_ALERTABLE);
		if (rc == WAIT_OBJECT_0 + 1) {
			MSG msg;
			while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
				if (IsDialogMessage(msg.hwnd, &msg)) continue;
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		else if (rc == WAIT_OBJECT_0) { // got object
			return 1;
		}
		else if (rc == WAIT_ABANDONED_0 || rc == WAIT_FAILED)
			return 0;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////

static void CALLBACK KillAllThreads(HWND, UINT, UINT_PTR, DWORD)
{
	if ( MirandaWaitForMutex(hStackMutex)) {
		for (int j=0; j < threads.getCount(); j++) {
			THREAD_WAIT_ENTRY* p = threads[j];
			char szModuleName[ MAX_PATH ];
			GetModuleFileNameA(p->hOwner, szModuleName, sizeof(szModuleName));
			TerminateThread(p->hThread, 9999);
			CloseHandle(p->hThread);
			mir_free(p);
		}

		threads.destroy();

		ReleaseMutex(hStackMutex);
		SetEvent(hThreadQueueEmpty);
	}	
}

MIR_CORE_DLL(void) KillObjectThreads(void* owner)
{
	if (owner == NULL)
		return;

	WaitForSingleObject(hStackMutex, INFINITE);

	HANDLE* threadPool = (HANDLE*)alloca(threads.getCount()*sizeof(HANDLE));
	int threadCount = 0;

	for (int j = threads.getCount(); j--;) {
		THREAD_WAIT_ENTRY* p = threads[j];
		if (p->pObject == owner)
			threadPool[ threadCount++ ] = p->hThread;
	}
	ReleaseMutex(hStackMutex); 

	// is there anything to kill?
	if (threadCount > 0) {
		if ( WaitForMultipleObjects(threadCount, threadPool, TRUE, 5000) == WAIT_TIMEOUT) {
			// forcibly kill all remaining threads after 5 secs
			WaitForSingleObject(hStackMutex, INFINITE);
			for (int j = threads.getCount()-1; j >= 0; j--) {
				THREAD_WAIT_ENTRY* p = threads[j];
				if (p->pObject == owner) {
					TerminateThread(p->hThread, 9999);
					CloseHandle(p->hThread);
					threads.remove(j);
					mir_free(p);
				}
			}
			ReleaseMutex(hStackMutex);
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////

MIR_CORE_DLL(void) Thread_Wait(void)
{
	// acquire the list and wake up any alertable threads
	if ( MirandaWaitForMutex(hStackMutex)) {
		int j;
		for (j=0; j < threads.getCount(); j++)
			QueueUserAPC(DummyAPCFunc, threads[j]->hThread, 0);
		ReleaseMutex(hStackMutex);
	}

	// give all unclosed threads 5 seconds to close
	SetTimer(NULL, 0, 5000, KillAllThreads);

	// wait til the thread list is empty
	MirandaWaitForMutex(hThreadQueueEmpty);
}

/////////////////////////////////////////////////////////////////////////////////////////

typedef LONG (WINAPI *pNtQIT)(HANDLE, LONG, PVOID, ULONG, PULONG);
#define ThreadQuerySetWin32StartAddress 9

MIR_CORE_DLL(INT_PTR) Thread_Push(HINSTANCE hInst, void* pOwner)
{
	ResetEvent(hThreadQueueEmpty); // thread list is not empty
	if ( WaitForSingleObject(hStackMutex, INFINITE) == WAIT_OBJECT_0) {
		THREAD_WAIT_ENTRY* p = (THREAD_WAIT_ENTRY*)mir_calloc(sizeof(THREAD_WAIT_ENTRY));

		DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &p->hThread, 0, FALSE, DUPLICATE_SAME_ACCESS);
		p->dwThreadId = GetCurrentThreadId();
		p->pObject = pOwner;
		p->hOwner = hInst;
		threads.insert(p);

		ReleaseMutex(hStackMutex);
	}
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////

MIR_CORE_DLL(INT_PTR) Thread_Pop()
{
	if ( WaitForSingleObject(hStackMutex, INFINITE) == WAIT_OBJECT_0) {
		DWORD dwThreadId = GetCurrentThreadId();
		for (int j=0; j < threads.getCount(); j++) {
			THREAD_WAIT_ENTRY* p = threads[j];
			if (p->dwThreadId == dwThreadId) {
				SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
				CloseHandle(p->hThread);
				threads.remove(j);
				mir_free(p);

				if ( !threads.getCount()) {
					threads.destroy();
					ReleaseMutex(hStackMutex);
					SetEvent(hThreadQueueEmpty); // thread list is empty now
					return 0;
				} 

				ReleaseMutex(hStackMutex);
				return 0;
			}
		}
		ReleaseMutex(hStackMutex);
	}
	return 1;
}

static LRESULT CALLBACK APCWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg == WM_NULL) SleepEx(0, TRUE);
	if (msg == WM_TIMECHANGE && RecalculateTime)
		RecalculateTime();
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

/////////////////////////////////////////////////////////////////////////////////////////
// module init

static void LoadSystemModule(void)
{
	INITCOMMONCONTROLSEX icce = {0};
	icce.dwSize = sizeof(icce);
	icce.dwICC = ICC_WIN95_CLASSES | ICC_USEREX_CLASSES;
	InitCommonControlsEx(&icce);

	if ( IsWinVerXPPlus()) {
		hAPCWindow=CreateWindowEx(0, _T("ComboLBox"), NULL, 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL);
		SetClassLongPtr(hAPCWindow, GCL_STYLE, GetClassLongPtr(hAPCWindow, GCL_STYLE) | CS_DROPSHADOW);
		DestroyWindow(hAPCWindow);
		hAPCWindow = NULL;
	}

	hAPCWindow = CreateWindowEx(0, _T("STATIC"), NULL, 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL);
	SetWindowLongPtr(hAPCWindow, GWLP_WNDPROC, (LONG_PTR)APCWndProc);
	hStackMutex = CreateMutex(NULL, FALSE, NULL);

	#ifdef WIN64
		HMODULE mirInst = GetModuleHandleA("miranda64.exe");
	#else
		HMODULE mirInst = GetModuleHandleA("miranda32.exe");
	#endif
	RecalculateTime = (void (*)()) GetProcAddress(mirInst, "RecalculateTime");

	InitPathUtils();
	InitialiseModularEngine();
}

static void UnloadSystemModule(void)
{
	DestroyModularEngine();
	UnloadLangPackModule();
}

/////////////////////////////////////////////////////////////////////////////////////////
// entry point

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	if (fdwReason == DLL_PROCESS_ATTACH) {
		hInst = hinstDLL;
		LoadSystemModule();
	}
	else if(fdwReason == DLL_PROCESS_DETACH)
		UnloadSystemModule();
	return TRUE;
}
