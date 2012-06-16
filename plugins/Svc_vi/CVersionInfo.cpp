/*
Version information plugin for Miranda IM

Copyright � 2002-2006 Luca Santarelli, Cristian Libotean

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


#include "CVersionInfo.h"
//#include "AggressiveOptimize.h"

#include "common.h"
#include "resource.h"

//using namespace std;

BOOL (WINAPI *MyGetDiskFreeSpaceEx)(LPCTSTR, PULARGE_INTEGER, PULARGE_INTEGER, PULARGE_INTEGER);
BOOL (WINAPI *MyIsWow64Process)(HANDLE, PBOOL);
void (WINAPI *MyGetSystemInfo)(LPSYSTEM_INFO);
BOOL (WINAPI *MyGlobalMemoryStatusEx)(LPMEMORYSTATUSEX lpBuffer) = NULL;

LANGID (WINAPI *MyGetUserDefaultUILanguage)() = NULL;
LANGID (WINAPI *MyGetSystemDefaultUILanguage)() = NULL;

static int ValidExtension(TCHAR *fileName, TCHAR *extension)
{
	TCHAR *dot = _tcschr(fileName, '.');
	if ( dot != NULL && !lstrcmpi(dot + 1, extension))
		if (dot[lstrlen(extension) + 1] == 0)
			return 1;

	return 0;
}

void FillLocalTime(std::tstring &output, FILETIME *fileTime)
{
	TIME_ZONE_INFORMATION tzInfo = {0};
	FILETIME local = {0};
	SYSTEMTIME sysTime;
	TCHAR date[1024];
	TCHAR time[256];

	FileTimeToLocalFileTime(fileTime, &local);
	FileTimeToSystemTime(&local, &sysTime);

	GetDateFormat(EnglishLocale, 0, &sysTime, _T("dd' 'MMM' 'yyyy"), date, SIZEOF(date));
	GetTimeFormat(NULL, TIME_FORCE24HOURFORMAT, &sysTime, _T("HH':'mm':'ss"), time, SIZEOF(time)); //americans love 24hour format ;)
	output = std::tstring(date) + _T(" at ") + std::tstring(time);

	int res = GetTimeZoneInformation(&tzInfo);
	char tzName[32] = {0};
	TCHAR tzOffset[64] = {0};
	int offset = 0;
	switch (res) {
	case TIME_ZONE_ID_DAYLIGHT:
		offset = -(tzInfo.Bias + tzInfo.DaylightBias);
		WideCharToMultiByte(CP_ACP, 0, tzInfo.DaylightName, -1, tzName, SIZEOF(tzName), NULL, NULL);
		break;

	case TIME_ZONE_ID_STANDARD:
		WideCharToMultiByte(CP_ACP, 0, tzInfo.StandardName, -1, tzName, SIZEOF(tzName), NULL, NULL);
		offset = -(tzInfo.Bias + tzInfo.StandardBias);
		break;

	case TIME_ZONE_ID_UNKNOWN:
		WideCharToMultiByte(CP_ACP, 0, tzInfo.StandardName, -1, tzName, SIZEOF(tzName), NULL, NULL);
		offset = -tzInfo.Bias;
		break;
	}

	mir_sntprintf(tzOffset, SIZEOF(tzOffset), _T("UTC %+02d:%02d"), offset / 60, offset % 60);
	output += _T(" (") + std::tstring(tzOffset) + _T(")");
}

CVersionInfo::CVersionInfo()
{
	luiFreeDiskSpace = 0;
	bDEPEnabled = 0;
}

CVersionInfo::~CVersionInfo()
{
	listInactivePlugins.clear();
	listActivePlugins.clear();
	listUnloadablePlugins.clear();
	
	lpzMirandaVersion.~basic_string();
	lpzNightly.~basic_string();
	lpzUnicodeBuild.~basic_string();
	lpzBuildTime.~basic_string();
	lpzOSVersion.~basic_string();
	lpzMirandaPath.~basic_string();
	lpzCPUName.~basic_string();
	lpzCPUIdentifier.~basic_string();
};

void CVersionInfo::Initialize()
{
#ifdef _DEBUG
		if (verbose) PUShowMessage("Before GetMirandaVersion().", SM_NOTIFY);
#endif
	GetMirandaVersion();

#ifdef _DEBUG
		if (verbose) PUShowMessage("Before GetProfileSettings().", SM_NOTIFY);
#endif
	GetProfileSettings();
	
#ifdef _DEBUG
		if (verbose) PUShowMessage("Before GetLangpackInfo().", SM_NOTIFY);
#endif
	GetOSLanguages();
	GetLangpackInfo();

#ifdef _DEBUG
	if (verbose) PUShowMessage("Before GetPluginLists().", SM_NOTIFY);
#endif
	GetPluginLists();

#ifdef _DEBUG
	if (verbose) PUShowMessage("Before GetOSVersion().", SM_NOTIFY);
#endif
	GetOSVersion();

#ifdef _DEBUG
	if (verbose) PUShowMessage("Before GetHWSettings().", SM_NOTIFY);
#endif
	GetHWSettings();

#ifdef _DEBUG
	if (verbose) PUShowMessage("Done with GetHWSettings().", SM_NOTIFY);
#endif
}

bool CVersionInfo::GetMirandaVersion()
{
	//Miranda version
	const BYTE str_size = 64;
	char str[str_size];
	CallService(MS_SYSTEM_GETVERSIONTEXT, (WPARAM)str_size, (LPARAM)str);
	this->lpzMirandaVersion = _A2T(str);
	//Is it a nightly?
	if (lpzMirandaVersion.find( _T("alpha"),0) != std::string::npos)
		lpzNightly = _T("Yes");
	else
		lpzNightly = _T("No");

	if (lpzMirandaVersion.find( _T("Unicode"), 0) != std::string::npos)
		lpzUnicodeBuild = _T("Yes");
	else
		lpzUnicodeBuild = _T("No");

	TCHAR time[128], date[128];
	GetModuleTimeStamp(date, time);
	lpzBuildTime = std::tstring(time) + _T(" (UTC) on ") + std::tstring(date);
	return TRUE;
}

bool CVersionInfo::GetOSVersion()
{
	//Operating system informations
	OSVERSIONINFO osvi = { 0 };
	osvi.dwOSVersionInfoSize = sizeof(osvi);
	GetVersionEx(&osvi);

	//Now fill the private members.
	TCHAR aux[256];
	wsprintf(aux, _T("%d.%d.%d %s"), osvi.dwMajorVersion, osvi.dwMinorVersion, LOWORD(osvi.dwBuildNumber), osvi.szCSDVersion);
	lpzOSVersion = aux;

	//OSName
	//Let's read the registry.
	HKEY hKey;
	TCHAR szKey[MAX_PATH], szValue[MAX_PATH];
	lstrcpyn(szKey, _T("Hardware\\Description\\System\\CentralProcessor\\0"), MAX_PATH);
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, szKey, 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS) {
		DWORD type, size, result;

		lstrcpyn(szValue, _T("Identifier"), MAX_PATH);
		result = RegQueryValueEx(hKey, szValue, 0, &type, NULL, &size);
		if (result == ERROR_SUCCESS) {
			TCHAR *aux = new TCHAR[size+1];
			result = RegQueryValueEx(hKey, szValue, 0, &type, (LPBYTE) aux, &size);
			lpzCPUIdentifier = aux;
			delete[] aux;
		}
		else {
			NotifyError(GetLastError(), _T("RegQueryValueEx()"), __LINE__);
			lpzCPUIdentifier = _T("<Error in RegQueryValueEx()>");
		}

		lstrcpyn(szValue, _T("ProcessorNameString"), MAX_PATH);
		result = RegQueryValueEx(hKey, szValue, 0, &type, NULL, &size); //get the size
		if (result == ERROR_SUCCESS) {
			TCHAR *aux = new TCHAR[size+1];
			result = RegQueryValueEx(hKey, szValue, 0, &type, (LPBYTE) aux, &size);
			lpzCPUName = aux;
			delete[] aux;
		}
		else { //else try to use cpuid instruction to get the proc name
			char szName[50];
			#if (!defined(WIN64) && !defined(_WIN64))
			__asm
			{
				push eax //save the registers
				push ebx
				push ecx
				push edx

				xor eax, eax //get simple name
				cpuid
				mov DWORD PTR szName[0], ebx
				mov DWORD PTR szName[4], edx
				mov DWORD PTR szName[8], ecx
				mov DWORD PTR szName[12], 0

				mov eax, 0x80000000 //try to get pretty name
				cpuid

				cmp eax, 0x80000004
				jb end //if we don't have the extension end the check

				mov DWORD PTR szName[0], 0 //make the string null

				mov eax, 0x80000002 //first part of the string
				cpuid
				mov DWORD PTR szName[0], eax
				mov DWORD PTR szName[4], ebx
				mov DWORD PTR szName[8], ecx
				mov DWORD PTR szName[12], edx

				mov eax, 0x80000003 //second part of the string
				cpuid
				mov DWORD PTR szName[16], eax
				mov DWORD PTR szName[20], ebx
				mov DWORD PTR szName[24], ecx
				mov DWORD PTR szName[28], edx

				mov eax, 0x80000004 //third part of the string
				cpuid
				mov DWORD PTR szName[32], eax
				mov DWORD PTR szName[36], ebx
				mov DWORD PTR szName[40], ecx
				mov DWORD PTR szName[44], edx

end:
				pop edx //load them back
				pop ecx
				pop ebx
				pop eax
			}
			szName[SIZEOF(szName) - 1] = '\0';
			#else
				szName[0] = 0;
			#endif

			if ( !szName[0] )
				lpzCPUName = _T("<name N/A>");
			else
				lpzCPUName = _A2T(szName);
		}
	}

	bDEPEnabled = IsProcessorFeaturePresent(PF_NX_ENABLED);

	switch (osvi.dwPlatformId) {
	case VER_PLATFORM_WIN32_WINDOWS:
		lstrcpyn(szKey, _T("Software\\Microsoft\\Windows\\CurrentVersion"), MAX_PATH);
		lstrcpyn(szValue, _T("Version"), MAX_PATH);
		break;
	case VER_PLATFORM_WIN32_NT:

		lstrcpyn(szKey, _T("Software\\Microsoft\\Windows NT\\CurrentVersion"), MAX_PATH);
		lstrcpyn(szValue, _T("ProductName"), MAX_PATH);
		break;
	}

	RegCloseKey(hKey);
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,szKey,0,KEY_QUERY_VALUE,&hKey) == ERROR_SUCCESS) {
		DWORD type, size, result;
		//Get the size of the value we'll read.
		result = RegQueryValueEx((HKEY)hKey,szValue,(LPDWORD)NULL, (LPDWORD)&type,(LPBYTE)NULL,
				(LPDWORD)&size);
		if (result == ERROR_SUCCESS) {
			//Read it.
			TCHAR *aux = new TCHAR[size+1];
			result = RegQueryValueEx((HKEY)hKey,szValue,(LPDWORD)NULL, (LPDWORD)&type,(LPBYTE)aux,(LPDWORD)&size);
			lpzOSName = aux;
			delete[] aux;
		}
		else {
			NotifyError(GetLastError(), _T("RegQueryValueEx()"), __LINE__);
			lpzOSName = _T("<Error in RegQueryValueEx()>");
		}
	}
	else {
		NotifyError(GetLastError(), _T("RegOpenKeyEx()"), __LINE__);
		lpzOSName = _T("<Error in RegOpenKeyEx()>");
	}
	
	//Now we can improve it if we can.
	switch (LOWORD(osvi.dwBuildNumber)) {
	case 950: lpzOSName = _T("Microsoft Windows 95"); break;
	case 1111: lpzOSName = _T("Microsoft Windows 95 OSR2"); break;
	case 1998: lpzOSName = _T("Microsoft Windows 98"); break;
	case 2222: lpzOSName = _T("Microsoft Windows 98 SE"); break;
	case 3000: lpzOSName = _T("Microsoft Windows ME"); break; //Even if this is wrong, we have already read it in the registry.
	case 1381: lpzOSName = _T("Microsoft Windows NT"); break; //What about service packs?
	case 2195: lpzOSName = _T("Microsoft Windows 2000"); break; //What about service packs?
	case 2600: lpzOSName = _T("Microsoft Windows XP"); break;
	case 3790:
		if ( GetSystemMetrics( 89 )) //R2 ?
			lpzOSName = _T("Microsoft Windows 2003 R2");
		else
			lpzOSName = _T("Microsoft Windows 2003");
				
		break; //added windows 2003 info
	}

	return TRUE;
}

bool CVersionInfo::GetHWSettings() {
	//Free space on Miranda Partition.
	TCHAR szMirandaPath[MAX_PATH] = { 0 };
	{ 
		GetModuleFileName(GetModuleHandle(NULL), szMirandaPath, SIZEOF(szMirandaPath));
		TCHAR* str2 = _tcsrchr(szMirandaPath,'\\');
		if ( str2 != NULL) *str2=0;
	}
	HMODULE hKernel32;
	hKernel32 = LoadLibraryA("kernel32.dll");
	if (hKernel32) {
		
			MyGetDiskFreeSpaceEx = (BOOL (WINAPI *)(LPCTSTR,PULARGE_INTEGER, PULARGE_INTEGER, PULARGE_INTEGER))GetProcAddress(hKernel32, "GetDiskFreeSpaceExW");
		

		MyIsWow64Process = (BOOL (WINAPI *) (HANDLE, PBOOL)) GetProcAddress(hKernel32, "IsWow64Process");
		MyGetSystemInfo = (void (WINAPI *) (LPSYSTEM_INFO)) GetProcAddress(hKernel32, "GetNativeSystemInfo");
		MyGlobalMemoryStatusEx = (BOOL (WINAPI *) (LPMEMORYSTATUSEX)) GetProcAddress(hKernel32, "GlobalMemoryStatusEx");
		if ( !MyGetSystemInfo )
			MyGetSystemInfo = GetSystemInfo;
		
		FreeLibrary(hKernel32);
	}
	if ( MyGetDiskFreeSpaceEx ) {
		ULARGE_INTEGER FreeBytes, a, b;
		MyGetDiskFreeSpaceEx(szMirandaPath, &FreeBytes, &a, &b);
		//Now we need to convert it.
		__int64 aux = FreeBytes.QuadPart;
		aux /= (1024*1024);
		luiFreeDiskSpace = (unsigned long int)aux;
	}
	else luiFreeDiskSpace = 0;
	
	TCHAR szInfo[1024];
	GetWindowsShell(szInfo, SIZEOF(szInfo));
	lpzShell = szInfo;
	GetInternetExplorerVersion(szInfo, SIZEOF(szInfo));
	lpzIEVersion = szInfo;
	
	
	lpzAdministratorPrivileges = (IsCurrentUserLocalAdministrator()) ? _T("Yes") : _T("No");
	
	bIsWOW64 = 0;
	if (MyIsWow64Process)
		if (!MyIsWow64Process(GetCurrentProcess(), &bIsWOW64))
			bIsWOW64 = 0;
	
	SYSTEM_INFO sysInfo = {0};
	GetSystemInfo(&sysInfo);
	luiProcessors = sysInfo.dwNumberOfProcessors;
	
	//Installed RAM
	if (MyGlobalMemoryStatusEx) { //windows 2000+
		MEMORYSTATUSEX ms = {0};
		ms.dwLength = sizeof(ms);
		MyGlobalMemoryStatusEx(&ms);
		luiRAM = (unsigned int) ((ms.ullTotalPhys / (1024 * 1024)) + 1);
	}
	else {
		MEMORYSTATUS ms = {0};
		ms.dwLength = sizeof(ms);
		GlobalMemoryStatus(&ms);
		luiRAM = (ms.dwTotalPhys/(1024*1024))+1; //Ugly hack!!!!
	}

	return TRUE;
}

bool CVersionInfo::GetProfileSettings()
{
	TCHAR* tszProfileName = Utils_ReplaceVarsT(_T("%miranda_userdata%\\%miranda_profilename%.dat"));
	lpzProfilePath = tszProfileName;

	WIN32_FIND_DATA fd;
	if ( FindFirstFile(tszProfileName, &fd) != INVALID_HANDLE_VALUE ) {
		TCHAR number[40];
		mir_sntprintf( number, SIZEOF(number), _T("%.2f KBytes"), double(fd.nFileSizeLow) / 1024 );
		lpzProfileSize = number;

		FillLocalTime(lpzProfileCreationDate, &fd.ftCreationTime);
	}
	else {
		DWORD error = GetLastError();
		TCHAR tmp[1024];
		wsprintf(tmp, _T("%d"), error);
		lpzProfileCreationDate = _T("<error ") + std::tstring(tmp) + _T(" at FileOpen>") + std::tstring(tszProfileName);
		lpzProfileSize = _T("<error ") + std::tstring(tmp) + _T(" at FileOpen>") + std::tstring(tszProfileName);
	}

	mir_free( tszProfileName );
	return true;
}

static TCHAR szSystemLocales[4096] = {0};
static WORD systemLangID;
#define US_LANG_ID 0x00000409

BOOL CALLBACK EnumSystemLocalesProc(TCHAR *szLocale)
{
	DWORD locale = _ttoi(szLocale);
	TCHAR *name = GetLanguageName(locale);
	if ( !_tcsstr(szSystemLocales, name)) {
		_tcscat(szSystemLocales, name);
		_tcscat(szSystemLocales, _T(", "));
	}
	
	return TRUE;
}

BOOL CALLBACK EnumResLangProc(HMODULE hModule, LPCTSTR lpszType, LPCTSTR lpszName, WORD wIDLanguage, LONG_PTR lParam)
{
	if (!lpszName)
	return FALSE;

	if (wIDLanguage !=  US_LANG_ID)
	systemLangID = wIDLanguage;

	return TRUE;
}

bool CVersionInfo::GetOSLanguages()
{
	lpzOSLanguages = _T("(UI | Locale (User/System)) : ");
	
	LANGID UILang;
	
	OSVERSIONINFO os = {0};
	os.dwOSVersionInfoSize = sizeof(os);
	GetVersionEx(&os);
	if (os.dwMajorVersion == 4) {
		if (os.dwPlatformId == VER_PLATFORM_WIN32_NT) { //Win NT 
			HMODULE hLib = LoadLibraryA("ntdll.dll");

			if (hLib) {
				EnumResourceLanguages(hLib, RT_VERSION, MAKEINTRESOURCE(1), EnumResLangProc, NULL);

				FreeLibrary(hLib);

				if (systemLangID == US_LANG_ID) {
					UINT uiACP;

					uiACP = GetACP();
					switch (uiACP)
					{
						case 874:     // Thai code page activated, it's a Thai enabled system
							systemLangID = MAKELANGID(LANG_THAI, SUBLANG_DEFAULT);
							break;

						case 1255:    // Hebrew code page activated, it's a Hebrew enabled system
							systemLangID = MAKELANGID(LANG_HEBREW, SUBLANG_DEFAULT);
							break;

						case 1256:    // Arabic code page activated, it's a Arabic enabled system
							systemLangID = MAKELANGID(LANG_ARABIC, SUBLANG_ARABIC_SAUDI_ARABIA);
							break;
								
						default:
							break;
					}
				}
			}
		}
		else { //Win 95-Me
			HKEY hKey = NULL;
			TCHAR szLangID[128] = _T("0x");
			DWORD size = SIZEOF(szLangID) - 2;
			TCHAR err[512];
				
			if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("Control Panel\\Desktop\\ResourceLocale"), 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS) {
				if (RegQueryValueEx(hKey, _T(""), 0, NULL, (LPBYTE) &szLangID + 2, &size) == ERROR_SUCCESS)
					_tscanf(szLangID, _T("%lx"), &systemLangID);
				else {
					FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, GetLastError(), LANG_SYSTEM_DEFAULT, err, size, NULL);
					MessageBox(0, err, _T("Error at RegQueryValueEx()"), MB_OK);
				}
				RegCloseKey(hKey);
			}
			else {
				FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, GetLastError(), LANG_SYSTEM_DEFAULT, err, size, NULL);
				MessageBox(0, err, _T("Error at RegOpenKeyEx()"), MB_OK);
			}
		}

		lpzOSLanguages += GetLanguageName(systemLangID);
	}
	else {
		HMODULE hKernel32 = LoadLibraryA("kernel32.dll");
		if (hKernel32) {
			MyGetUserDefaultUILanguage = (LANGID (WINAPI *)()) GetProcAddress(hKernel32, "GetUserDefaultUILanguage");
			MyGetSystemDefaultUILanguage = (LANGID (WINAPI *)()) GetProcAddress(hKernel32, "GetSystemDefaultUILanguage");
				
			FreeLibrary(hKernel32);
		}
		
		if ((MyGetUserDefaultUILanguage) && (MyGetSystemDefaultUILanguage)) {
			UILang = MyGetUserDefaultUILanguage();
			lpzOSLanguages += GetLanguageName(UILang);
			lpzOSLanguages += _T("/");
			UILang = MyGetSystemDefaultUILanguage();
			lpzOSLanguages += GetLanguageName(UILang);
		}
		else lpzOSLanguages += _T("Missing functions in kernel32.dll (GetUserDefaultUILanguage, GetSystemDefaultUILanguage)");
	}
	
	lpzOSLanguages += _T(" | ");
	lpzOSLanguages += GetLanguageName(LOCALE_USER_DEFAULT);
	lpzOSLanguages += _T("/");
	lpzOSLanguages += GetLanguageName(LOCALE_SYSTEM_DEFAULT);

	if (DBGetContactSettingByte(NULL, ModuleName, "ShowInstalledLanguages", 0)) {
		szSystemLocales[0] = '\0';
		lpzOSLanguages += _T(" [");
		EnumSystemLocales(EnumSystemLocalesProc, LCID_INSTALLED);
		if (_tcslen(szSystemLocales) > 2)
			szSystemLocales[ _tcslen(szSystemLocales) - 2] = '\0';

		lpzOSLanguages += szSystemLocales;
		lpzOSLanguages += _T("]");
	}
	
	return true;
}

int SaveInfo(const char *data, const char *lwrData, const char *search, TCHAR *dest, int size)
{
	const char *pos = strstr(lwrData, search);
	int res = 1;
	if (pos == lwrData) {
		_tcsncpy(dest, _A2T(&data[strlen(search)]), size);
		res = 0;
	}
	
	return res;
}

bool CVersionInfo::GetLangpackInfo()
{
	TCHAR langpackPath[MAX_PATH] = {0};
	TCHAR search[MAX_PATH] = {0};

	lpzLangpackModifiedDate = _T("");	
	GetModuleFileName(GetModuleHandle(NULL), langpackPath, SIZEOF(langpackPath));
	TCHAR* p = _tcsrchr(langpackPath, '\\');
	if (p) {
		WIN32_FIND_DATA data = {0};
		HANDLE hLangpack;
		
		p[1] = '\0';
		_tcscpy(search, langpackPath);
		_tcscat(search, _T("langpack_*.txt"));
		hLangpack = FindFirstFile(search, &data);
		if (hLangpack != INVALID_HANDLE_VALUE) {
			char buffer[1024];
			char temp[1024];
			FillLocalTime(lpzLangpackModifiedDate, &data.ftLastWriteTime);
					
			TCHAR locale[128] = {0};
			TCHAR language[128] = {0};
			TCHAR version[128] = {0};
			_tcscpy(version, _T("N/A"));
			
			_tcsncpy(language, data.cFileName, SIZEOF(language));
			p = _tcsrchr(language, '.');
			p[0] = '\0';
			
			_tcscat(langpackPath, data.cFileName);
			FILE *fin = _tfopen(langpackPath, _T("rt"));
			if (fin) {
				size_t len;
				while (!feof(fin)) {
					fgets(buffer, SIZEOF(buffer), fin);
					len = strlen(buffer);
					if (buffer[len - 1] == '\n') buffer[len - 1] = '\0';
					strncpy(temp, buffer, SIZEOF(temp));
					_strlwr(temp);
					if (SaveInfo(buffer, temp, "language: ", language, SIZEOF(language))) {
						if (SaveInfo(buffer, temp, "locale: ", locale, SIZEOF(locale))) {
							char* p = strstr(buffer, "; FLID: ");
							if (p) {
								int ok = 1;
								int i;
								for (i = 0; ((i < 3) && (ok)); i++) {
									p = strrchr(temp, '.');
									if (p)
										p[0] = '\0';
									else
										ok = 0;
								}
								p = strrchr(temp, ' ');
								if ((ok) && (p))
									_tcsncpy(version, _A2T(&buffer[p - temp + 1]), SIZEOF(version));
								else
									_tcsncpy(version, _T("<unknown>"), SIZEOF(version));
				}	}	}	}
				
				lpzLangpackInfo = std::tstring(language) + _T(" [") + std::tstring(locale) + _T("]");
				if ( version[0] )
					lpzLangpackInfo += _T(" v. ") + std::tstring(version);

				fclose(fin);
			}
			else {
				int err = GetLastError();
				lpzLangpackInfo = _T("<error> Could not open file " + std::tstring(data.cFileName));
			}
			FindClose(hLangpack);
		}
		else lpzLangpackInfo = _T("No language pack installed");
	}	
	
	return true;
}

std::tstring GetPluginTimestamp(FILETIME *fileTime)
{
	SYSTEMTIME sysTime;
	FileTimeToSystemTime(fileTime, &sysTime); //convert the file tyme to system time

	//char time[256];
	TCHAR date[256]; //lovely
	GetDateFormat(EnglishLocale, 0, &sysTime, _T("dd' 'MMM' 'yyyy"), date, SIZEOF(date));
	return date;
}

bool CVersionInfo::GetPluginLists()
{
	HANDLE hFind;
	TCHAR szMirandaPath[MAX_PATH] = { 0 }, szSearchPath[MAX_PATH] = { 0 }; //For search purpose
	WIN32_FIND_DATA fd;
	TCHAR szMirandaPluginsPath[MAX_PATH] = { 0 }, szPluginPath[MAX_PATH] =  { 0 }; //For info reading purpose
	BYTE PluginIsEnabled = 0;
	HINSTANCE hInstPlugin = NULL;
	PLUGININFOEX *(*MirandaPluginInfo)(DWORD); //These two are used to get informations from the plugin.
	PLUGININFOEX *pluginInfo = NULL; //Read above.
	DWORD mirandaVersion = 0;
	BOOL asmCheckOK = FALSE;
	DWORD loadError;
	//	SYSTEMTIME sysTime; //for timestamp

	mirandaVersion=(DWORD)CallService(MS_SYSTEM_GETVERSION,0,0);
	{	
		GetModuleFileName(GetModuleHandle(NULL), szMirandaPath, SIZEOF(szMirandaPath));
		TCHAR* str2 = _tcsrchr(szMirandaPath,'\\');
		if(str2!=NULL) *str2=0;
	}
	lpzMirandaPath = szMirandaPath;

	//We got Miranda path, now we'll use it for two different purposes.
	//1) finding plugins.
	//2) Reading plugin infos
	lstrcpyn(szSearchPath,szMirandaPath, MAX_PATH); //We got the path, now we copy it into szSearchPath. We'll use szSearchPath as am auxiliary variable, while szMirandaPath will keep a "fixed" value.
	lstrcat(szSearchPath, _T("\\Plugins\\*.dll"));

	lstrcpyn(szMirandaPluginsPath, szMirandaPath, MAX_PATH);
	lstrcat(szMirandaPluginsPath, _T("\\Plugins\\"));

	hFind=FindFirstFile(szSearchPath,&fd);
	if ( hFind != INVALID_HANDLE_VALUE) {
		do {
			if (verbose) PUShowMessageT(fd.cFileName, SM_NOTIFY);
			if (!ValidExtension(fd.cFileName, _T("dll")))
				continue; //do not report plugins that do not have extension .dll

			hInstPlugin = GetModuleHandle(fd.cFileName); //try to get the handle of the module

			if (hInstPlugin) //if we got it then the dll is loaded (enabled)
				PluginIsEnabled = 1;
			else {
				PluginIsEnabled = 0;
				lstrcpyn(szPluginPath, szMirandaPluginsPath, MAX_PATH); // szPluginPath becomes "drive:\path\Miranda\Plugins\"
				lstrcat(szPluginPath, fd.cFileName); // szPluginPath becomes "drive:\path\Miranda\Plugins\popup.dll"
				hInstPlugin = LoadLibrary(szPluginPath);
			}
			if (!hInstPlugin) { //It wasn't loaded.
				loadError = GetLastError();
				int bUnknownError = 1; //assume plugin didn't load because of unknown error
				//Some error messages.
				//get the dlls the plugin statically links to 
				if (DBGetContactSettingByte(NULL, ModuleName, "CheckForDependencies", TRUE))
				{
					std::tstring linkedModules;

					lstrcpyn(szPluginPath, szMirandaPluginsPath, MAX_PATH); // szPluginPath becomes "drive:\path\Miranda\Plugins\"
					lstrcat(szPluginPath, fd.cFileName); // szPluginPath becomes "drive:\path\Miranda\Plugins\popup.dll"
					if (GetLinkedModulesInfo(szPluginPath, linkedModules)) {
						std::tstring time = GetPluginTimestamp(&fd.ftLastWriteTime);
						CPlugin thePlugin(fd.cFileName, _T("<unknown>"), UUID_NULL, _T(""), 0, time.c_str(), linkedModules.c_str());
						AddPlugin(thePlugin, listUnloadablePlugins);
						bUnknownError = 0; //we know why the plugin didn't load
					}
				}
				if (bUnknownError) { //if cause is unknown then report it
					std::tstring time = GetPluginTimestamp(&fd.ftLastWriteTime);
					TCHAR buffer[4096];
					TCHAR error[2048];
					//DWORD_PTR arguments[2] = {loadError, 0};
					FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ARGUMENT_ARRAY, NULL, loadError, 0, error, SIZEOF(error), NULL);
					wsprintf(buffer, _T("    Error %ld - %s"), loadError, error);
					CPlugin thePlugin( fd.cFileName, _T("<unknown>"), UUID_NULL, _T(""), 0, time.c_str(), buffer);
					AddPlugin(thePlugin, listUnloadablePlugins);
				}
			}
			else { //It was successfully loaded.
				MirandaPluginInfo = (PLUGININFOEX *(*)(DWORD))GetProcAddress(hInstPlugin, "MirandaPluginInfoEx");
				if (!MirandaPluginInfo)
					MirandaPluginInfo = (PLUGININFOEX *(*)(DWORD))GetProcAddress(hInstPlugin, "MirandaPluginInfo"); 

				if (!MirandaPluginInfo) //There is no function: it's not a valid plugin. Let's move on to the next file.
					continue;

				//It's a valid plugin, since we could find MirandaPluginInfo
				#if (!defined(WIN64) && !defined(_WIN64))
					asmCheckOK = FALSE;
					__asm {
						push mirandaVersion
						push mirandaVersion
						call MirandaPluginInfo
						pop eax
						pop eax
						cmp eax, mirandaVersion
						jne a1
						mov asmCheckOK, 0xffffffff
					a1:
					}
				#else
					asmCheckOK = TRUE;
				#endif
				if (asmCheckOK)
					pluginInfo = CopyPluginInfo(MirandaPluginInfo(mirandaVersion));
				else {
					ZeroMemory(&pluginInfo, sizeof(pluginInfo));
					MessageBox(NULL, fd.cFileName, _T("Invalid plugin"), MB_OK);
			}	}

			//Let's get the info.
			if (MirandaPluginInfo != NULL) {//a valid miranda plugin
				if (pluginInfo != NULL) {
					//We have loaded the informations into pluginInfo.
					std::tstring timedate = GetPluginTimestamp(&fd.ftLastWriteTime);
					CPlugin thePlugin(fd.cFileName, _A2T(pluginInfo->shortName), pluginInfo->uuid, (pluginInfo->flags & 1) ? _T("Unicode aware") : _T(""), (DWORD) pluginInfo->version, timedate.c_str(), _T(""));

					if (PluginIsEnabled)
						AddPlugin(thePlugin, listActivePlugins);
					else {
						if ((IsUUIDNull(pluginInfo->uuid)) && (mirandaVersion >= PLUGIN_MAKE_VERSION(0,8,0,9))) {
							thePlugin.SetErrorMessage( _T("    Plugin does not have an UUID and will not work with Miranda 0.8.\r\n"));
							AddPlugin(thePlugin, listUnloadablePlugins);
						}
						else AddPlugin(thePlugin, listInactivePlugins);

						FreeLibrary(hInstPlugin); //We don't need it anymore.
					}
					FreePluginInfo(pluginInfo);
					MirandaPluginInfo = NULL;
					#ifdef _DEBUG
						if (verbose) {
							TCHAR szMsg[4096] = { 0 };
							wsprintf(szMsg, _T("Done with: %s"), fd.cFileName);
							PUShowMessageT(szMsg, SM_NOTIFY);
						}
					#endif
				}
				else { //pluginINFO == NULL
					pluginInfo = CopyPluginInfo(MirandaPluginInfo(PLUGIN_MAKE_VERSION(9, 9, 9, 9))); //let's see if the plugin likes this miranda version
					char *szShortName = "<unknown>";
					std::tstring time = GetPluginTimestamp(&fd.ftLastWriteTime); //get the plugin timestamp;
					DWORD version = 0;
					if (pluginInfo) {
						szShortName = pluginInfo->shortName;
						version = pluginInfo->version;
					}

					CPlugin thePlugin(fd.cFileName, _A2T(szShortName), (pluginInfo) ? pluginInfo->uuid : UUID_NULL, (((pluginInfo) && (pluginInfo->flags & 1)) ? _T("Unicode aware") : _T("")), version, time.c_str(), _T("    Plugin refuses to load. Miranda version too old."));

					AddPlugin(thePlugin, listUnloadablePlugins);
					if (pluginInfo)
						FreePluginInfo(pluginInfo);
			}	}
		}
			while (FindNextFile(hFind,&fd));
		FindClose(hFind);
	}
	return TRUE;
}

bool CVersionInfo::AddPlugin(CPlugin &aPlugin, std::list<CPlugin> &aList) {
	std::list<CPlugin>::iterator pos = aList.begin();
	bool inserted = FALSE;

	if (aList.begin() == aList.end()) { //It's empty
		aList.push_back(aPlugin);
		return TRUE;
	}
	else { //It's not empty
		while (pos != aList.end()) {
			//It can be either < or >, not equal.
			if (aPlugin < (*pos)) {
				aList.insert(pos, aPlugin);
				return TRUE;
			}

			//It's greater: we need to insert it.
			pos++;
	}	}

	if (inserted == FALSE) {
		aList.push_back(aPlugin);
		return TRUE;
	}
	return TRUE;
};

static char *GetStringFromRVA(DWORD RVA, const LOADED_IMAGE *image)
{
	char *moduleName;
	moduleName = (char *) ImageRvaToVa(image->FileHeader, image->MappedAddress, RVA, NULL);
	return moduleName;
}

bool CVersionInfo::GetLinkedModulesInfo(TCHAR *moduleName, std::tstring &linkedModules)
{
	LOADED_IMAGE image;
	ULONG importTableSize;
	IMAGE_IMPORT_DESCRIPTOR *importData;
	//HMODULE dllModule;
	linkedModules = _T("");
	bool result = false;
	TCHAR szError[20];
	char* szModuleName = mir_t2a(moduleName);
	if (MapAndLoad(szModuleName, NULL, &image, TRUE, TRUE) == FALSE) {
		wsprintf(szError, _T("%d"), GetLastError());
		mir_free( szModuleName );
		linkedModules = _T("<error ") + std::tstring(szError) + _T(" at MapAndLoad()>\r\n");
		return result;
	}
	mir_free( szModuleName );
	importData = (IMAGE_IMPORT_DESCRIPTOR *) ImageDirectoryEntryToData(image.MappedAddress, FALSE, IMAGE_DIRECTORY_ENTRY_IMPORT, &importTableSize);
	if (!importData) {
		wsprintf(szError, _T("%d"), GetLastError());
		linkedModules = _T("<error ") + std::tstring(szError) + _T(" at ImageDirectoryEntryToDataEx()>\r\n");
	}
	else {
		while (importData->Name) {
			char *moduleName;
			moduleName = GetStringFromRVA(importData->Name, &image);
			if (!DoesDllExist(moduleName)) {
				linkedModules.append( _T("    Plugin statically links to missing dll file: ") + std::tstring(_A2T(moduleName)) + _T("\r\n"));
				result = true;
			}

			importData++; //go to next record
	}	}

	//	FreeLibrary(dllModule);
	UnMapAndLoad(&image); //unload the image
	return result;
}

std::tstring CVersionInfo::GetListAsString(std::list<CPlugin> &aList, DWORD flags, int beautify) {
	std::list<CPlugin>::iterator pos = aList.begin();
	std::tstring out = _T("");
#ifdef _DEBUG
	if (verbose) PUShowMessage("CVersionInfo::GetListAsString, begin.", SM_NOTIFY);
#endif

	TCHAR szHeader[32] = {0};
	TCHAR szFooter[32] = {0};
	if ((((flags & VISF_FORUMSTYLE) == VISF_FORUMSTYLE) || beautify) && (DBGetContactSettingByte(NULL, ModuleName, "BoldVersionNumber", TRUE))) {
		GetStringFromDatabase("BoldBegin", _T("[b]"), szHeader, SIZEOF(szHeader));
		GetStringFromDatabase("BoldEnd", _T("[/b]"), szFooter, SIZEOF(szFooter));
	}
	
	while (pos != aList.end()) {
		out.append(std::tstring((*pos).getInformations(flags, szHeader, szFooter)));
		pos++;
	}
	#ifdef _DEBUG
		if (verbose) PUShowMessage("CVersionInfo::GetListAsString, end.", SM_NOTIFY);
	#endif
	return out;
};

void CVersionInfo::BeautifyReport(int beautify, LPCTSTR szBeautifyText, LPCTSTR szNonBeautifyText, std::tstring &out)
{
	if (beautify)
		out.append(szBeautifyText);
	else
		out.append(szNonBeautifyText);
}

void CVersionInfo::AddInfoHeader(int suppressHeader, int forumStyle, int beautify, std::tstring &out)
{
	if (forumStyle) { //forum style
		TCHAR szSize[256], szQuote[256];

		GetStringFromDatabase("SizeBegin", _T("[size=1]"), szSize, SIZEOF(szSize));
		GetStringFromDatabase("QuoteBegin", _T("[quote]"), szQuote, SIZEOF(szQuote));
		out.append(szQuote);
		out.append(szSize);
	}
	else out = _T("");

	if (!suppressHeader) {
		out.append( _T("Miranda IM - VersionInformation plugin by Hrk, modified by Eblis\r\n"));
		if (!forumStyle) {
			out.append( _T("Miranda's homepage: http://www.miranda-im.org/\r\n")); //changed homepage
			out.append( _T("Miranda tools: http://miranda-im.org/download/\r\n\r\n")); //was missing a / before download
	}	}

	TCHAR buffer[1024]; //for beautification
	GetStringFromDatabase("BeautifyHorizLine", _T("<hr />"), buffer, SIZEOF(buffer));	
	BeautifyReport(beautify, buffer, _T(""), out);
	GetStringFromDatabase("BeautifyBlockStart", _T("<blockquote>"), buffer, SIZEOF(buffer));
	BeautifyReport(beautify, buffer, _T(""), out);
	if (!suppressHeader) {
		//Time of report:
		TCHAR lpzTime[12]; 	GetTimeFormat(LOCALE_USER_DEFAULT, 0, NULL, _T("HH':'mm':'ss"), lpzTime, SIZEOF(lpzTime));
		TCHAR lpzDate[32]; 	GetDateFormat(EnglishLocale, 0, NULL, _T("dd' 'MMMM' 'yyyy"), lpzDate, SIZEOF(lpzDate));
		out.append( _T("Report generated at: ") + std::tstring(lpzTime) + _T(" on ") + std::tstring(lpzDate) + _T("\r\n\r\n"));
	}

	//Operating system
	out.append(_T("CPU: ") + lpzCPUName + _T(" [") + lpzCPUIdentifier + _T("]"));
	if (bDEPEnabled)
		out.append(_T(" [DEP enabled]"));

	if (luiProcessors > 1) {
		TCHAR noProcs[128];
		wsprintf(noProcs, _T(" [%d CPUs]"), luiProcessors);
		out.append(noProcs);
	}
	out.append( _T("\r\n"));
	
	//RAM
	TCHAR szRAM[64]; wsprintf(szRAM, _T("%d"), luiRAM);
	out.append( _T("Installed RAM: ") + std::tstring(szRAM) + _T(" MBytes\r\n"));
	
	//operating system
	out.append( _T("Operating System: ") + lpzOSName + _T(" [version: ") + lpzOSVersion + _T("]\r\n"));
	
	//shell, IE, administrator
	out.append( _T("Shell: ") + lpzShell + _T(", Internet Explorer ") + lpzIEVersion + _T("\r\n"));
	out.append( _T("Administrator privileges: ") + lpzAdministratorPrivileges + _T("\r\n"));
	
	//languages
	out.append( _T("OS Languages: ") + lpzOSLanguages + _T("\r\n"));
	
	//FreeDiskSpace
	if (luiFreeDiskSpace) {
		TCHAR szDiskSpace[64]; wsprintf(szDiskSpace, _T("%d"), luiFreeDiskSpace);
		out.append( _T("Free disk space on Miranda partition: ") + std::tstring(szDiskSpace) + _T(" MBytes\r\n"));
	}
	
	//Miranda
	out.append( _T("Miranda path: ")	+ lpzMirandaPath + _T("\r\n"));
	out.append( _T("Miranda IM version: ") + lpzMirandaVersion);
	if (bIsWOW64)
		out.append( _T(" [running inside WOW64]"));
	if (bServiceMode)
		out.append( _T(" [service mode]"));

	out.append( _T("\r\nBuild time: ") + lpzBuildTime + _T("\r\n"));
	out.append( _T("Profile path: ") + lpzProfilePath + _T("\r\n"));
	out.append( _T("Profile size: ") + lpzProfileSize + _T("\r\n"));
	out.append( _T("Profile creation date: ") + lpzProfileCreationDate + _T("\r\n"));
	out.append( _T("Language pack: ") + lpzLangpackInfo);
	out.append((lpzLangpackModifiedDate.size() > 0) ? _T(", modified: ") + lpzLangpackModifiedDate : _T(""));
	out.append( _T("\r\n"));
		
	out.append( _T("Nightly: ") + lpzNightly + _T("\r\n"));
	out.append( _T("Unicode core: ") + lpzUnicodeBuild);
	
	GetStringFromDatabase("BeautifyBlockEnd", _T("</blockquote>"), buffer, SIZEOF(buffer));
	BeautifyReport(beautify, buffer, _T("\r\n"), out);
}

void CVersionInfo::AddInfoFooter(int suppressFooter, int forumStyle, int beautify, std::tstring &out)
{
	//End of report
	TCHAR buffer[1024]; //for beautification purposes
	GetStringFromDatabase("BeautifyHorizLine", _T("<hr />"), buffer, SIZEOF(buffer));
	if (!suppressFooter) {
		BeautifyReport(beautify, buffer, _T("\r\n"), out);
		out.append( _T("\r\nEnd of report.\r\n"));
	}

	if (!forumStyle) {
		if (!suppressFooter)
			out.append( TranslateT("If you are going to use this report to submit a bug, remember to check the website for questions or help the developers may need.\r\nIf you don't check your bug report and give feedback, it will not be fixed!"));
	}
	else {
		TCHAR szSize[256], szQuote[256];
		GetStringFromDatabase("SizeEnd", _T("[/size]"), szSize, SIZEOF(szSize));
		GetStringFromDatabase("QuoteEnd", _T("[/quote]"), szQuote, SIZEOF(szQuote));
		out.append(szSize);
		out.append(szQuote);
	}
}

static void AddSectionAndCount(std::list<CPlugin> list, LPCTSTR listText, std::tstring &out)
{
	TCHAR tmp[64];
	wsprintf(tmp, _T(" (%u)"), list.size());
	out.append(listText);
	out.append( tmp );
	out.append( _T(":"));
}

std::tstring CVersionInfo::GetInformationsAsString(int bDisableForumStyle) {
	//Begin of report
	std::tstring out;
	int forumStyle = (bDisableForumStyle) ? 0 : DBGetContactSettingByte(NULL, ModuleName, "ForumStyle", TRUE);
	int showUUID = DBGetContactSettingByte(NULL, ModuleName, "ShowUUIDs", FALSE);
	int beautify = DBGetContactSettingByte(NULL, ModuleName, "Beautify", 0) & (!forumStyle);
	int suppressHeader = DBGetContactSettingByte(NULL, ModuleName, "SuppressHeader", TRUE);

	DWORD flags = (forumStyle) | (showUUID << 1);

	AddInfoHeader(suppressHeader, forumStyle, beautify, out);
	TCHAR normalPluginsStart[1024]; //for beautification purposes, for normal plugins text (start)
	TCHAR normalPluginsEnd[1024]; //for beautification purposes, for normal plugins text (end)
	TCHAR horizLine[1024]; //for beautification purposes
	TCHAR buffer[1024]; //for beautification purposes
	
	TCHAR headerHighlightStart[10] = _T("");
	TCHAR headerHighlightEnd[10] = _T("");
	if (forumStyle) {
		TCHAR start[128], end[128];
		GetStringFromDatabase("BoldBegin", _T("[b]"), start, SIZEOF(start));
		GetStringFromDatabase("BoldEnd", _T("[/b]"), end, SIZEOF(end));
		_tcsncpy(headerHighlightStart, start, SIZEOF(headerHighlightStart));
		_tcsncpy(headerHighlightEnd, end, SIZEOF(headerHighlightEnd));
	}

	//Plugins: list of active (enabled) plugins.
	GetStringFromDatabase("BeautifyHorizLine", _T("<hr />"), horizLine, SIZEOF(horizLine));
	BeautifyReport(beautify, horizLine, _T("\r\n"), out);
	GetStringFromDatabase("BeautifyActiveHeaderBegin", _T("<b><font size=\"-1\" color=\"DarkGreen\">"), buffer, SIZEOF(buffer));
	BeautifyReport(beautify, buffer, headerHighlightStart, out);
	AddSectionAndCount(listActivePlugins, _T("Active Plugins"), out);
	GetStringFromDatabase("BeautifyActiveHeaderEnd", _T("</font></b>"), buffer, SIZEOF(buffer));
	BeautifyReport(beautify, buffer, headerHighlightEnd, out);
	out.append( _T("\r\n"));

	GetStringFromDatabase("BeautifyPluginsBegin", _T("<font size=\"-2\" color=\"black\">"), normalPluginsStart, SIZEOF(normalPluginsStart));
	BeautifyReport(beautify, normalPluginsStart, _T(""), out);
	out.append(GetListAsString(listActivePlugins, flags, beautify));
	GetStringFromDatabase("BeautifyPluginsEnd", _T("</font>"), normalPluginsEnd, SIZEOF(normalPluginsEnd));
	BeautifyReport(beautify, normalPluginsEnd, _T(""), out);
	//Plugins: list of inactive (disabled) plugins.
	if ((!forumStyle) && ((DBGetContactSettingByte(NULL, ModuleName, "ShowInactive", TRUE)) || (bServiceMode))) {
		BeautifyReport(beautify, horizLine, _T("\r\n"), out);
		GetStringFromDatabase("BeautifyInactiveHeaderBegin", _T("<b><font size=\"-1\" color=\"DarkRed\">"), buffer, SIZEOF(buffer));
		BeautifyReport(beautify, buffer, headerHighlightStart, out);
		AddSectionAndCount(listInactivePlugins, _T("Inactive Plugins"), out);
		GetStringFromDatabase("BeautifyInactiveHeaderEnd", _T("</font></b>"), buffer, SIZEOF(buffer));
		BeautifyReport(beautify, buffer, headerHighlightEnd, out);
		out.append( _T("\r\n"));
		BeautifyReport(beautify, normalPluginsStart, _T(""), out);
		out.append(GetListAsString(listInactivePlugins, flags, beautify));
		BeautifyReport(beautify, normalPluginsEnd, _T(""), out);
	}
	if (listUnloadablePlugins.size() > 0) {
		BeautifyReport(beautify, horizLine, _T("\r\n"), out);
		GetStringFromDatabase("BeautifyUnloadableHeaderBegin", _T("<b><font size=\"-1\"><font color=\"Red\">"), buffer, SIZEOF(buffer));
		BeautifyReport(beautify, buffer, headerHighlightStart, out);
		AddSectionAndCount(listUnloadablePlugins, _T("Unloadable Plugins"), out);
		GetStringFromDatabase("BeautifyUnloadableHeaderEnd", _T("</font></b>"), buffer, SIZEOF(buffer));
		BeautifyReport(beautify, buffer, headerHighlightEnd, out);
		out.append( _T("\r\n"));
		BeautifyReport(beautify, normalPluginsStart, _T(""), out);
		out.append(GetListAsString(listUnloadablePlugins, flags, beautify));
		BeautifyReport(beautify, normalPluginsEnd, _T(""), out);			
	}
	AddInfoFooter(suppressHeader, forumStyle, beautify, out);
	return out;
}

//========== Print functions =====

void CVersionInfo::PrintInformationsToFile(const TCHAR *info)
{
	TCHAR buffer[MAX_PATH], outputFileName[MAX_PATH];
	if (bFoldersAvailable) {
		FoldersGetCustomPathT(hOutputLocation,  buffer, SIZEOF(buffer), _T("%miranda_path%"));
		_tcscat(buffer, _T("\\VersionInfo.txt"));
	}
	else GetStringFromDatabase("OutputFile", _T("VersionInfo.txt"), buffer, SIZEOF(buffer));

	RelativePathToAbsolute(buffer, outputFileName, SIZEOF(buffer));
	
	FILE *fp = _tfopen(outputFileName, _T("wb"));
	if ( fp != NULL ) {
		char* utf = mir_utf8encodeT( info );
		fputs( "\xEF\xBB\xBF", fp);
		fputs( utf, fp );
		fclose(fp);

		TCHAR mex[512];
		mir_sntprintf(mex, SIZEOF(mex), TranslateT("Information successfully written to file: \"%s\"."), outputFileName);
		Log(mex);
	}
	else {
		TCHAR mex[512];
		mir_sntprintf(mex, SIZEOF(mex), TranslateT("Error during the creation of file \"%s\". Disk may be full or write protected."), outputFileName);
		Log(mex);
	}
}

void CVersionInfo::PrintInformationsToFile()
{
	PrintInformationsToFile( GetInformationsAsString().c_str());
}

void CVersionInfo::PrintInformationsToMessageBox()
{
	MessageBox(NULL, GetInformationsAsString().c_str(), _T("VersionInfo"), MB_OK);
}

void CVersionInfo::PrintInformationsToOutputDebugString()
{
	OutputDebugString( GetInformationsAsString().c_str());
}


void CVersionInfo::PrintInformationsToDialogBox()
{
	HWND DialogBox = CreateDialogParam(hInst,
			MAKEINTRESOURCE(IDD_DIALOGINFO),
			NULL, DialogBoxProc, (LPARAM) this);
	
	SetDlgItemText(DialogBox, IDC_TEXT, GetInformationsAsString().c_str());
}

void CVersionInfo::PrintInformationsToClipboard(bool showLog)
{
	if (GetOpenClipboardWindow()) {
		Log( TranslateT("The clipboard is not available, retry."));
		return;
	}
	
	OpenClipboard(NULL);
	//Ok, let's begin, then.
	EmptyClipboard();
	//Storage data we'll use.
	LPTSTR lptstrCopy;
	std::tstring aux = GetInformationsAsString();
	size_t length = aux.length() + 1;
	HANDLE hData = GlobalAlloc(GMEM_MOVEABLE, length*sizeof(TCHAR) + 5);
	//Lock memory, copy it, release it.
	lptstrCopy = (LPTSTR)GlobalLock(hData);
	lstrcpy(lptstrCopy, aux.c_str());
	lptstrCopy[length] = '\0';
	GlobalUnlock(hData);
	//Now set the clipboard data.
	
		SetClipboardData(CF_UNICODETEXT, hData);

	//Remove the lock on the clipboard.
	CloseClipboard();
	if (showLog)
		Log( TranslateT("Information successfully copied into clipboard."));
}
