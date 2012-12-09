#include "skype_proto.h"

BYTE CSkypeProto::GetSettingByte(HANDLE hContact, const char *setting, BYTE errorValue)
{
	return ::DBGetContactSettingByte(hContact, this->m_szModuleName, setting, errorValue);
}

BYTE CSkypeProto::GetSettingByte(const char *setting, BYTE errorValue)
{
	return this->GetSettingByte(NULL, setting, errorValue);
}

WORD CSkypeProto::GetSettingWord(HANDLE hContact, const char *setting, WORD errorValue)
{
	return ::DBGetContactSettingWord(hContact, this->m_szModuleName, setting, errorValue);
}

WORD CSkypeProto::GetSettingWord(const char *setting, WORD errorValue)
{
	return this->GetSettingWord(NULL, setting, errorValue);
}

DWORD CSkypeProto::GetSettingDword(HANDLE hContact, const char *setting, DWORD errorValue)
{
	return ::DBGetContactSettingDword(hContact, this->m_szModuleName, setting, errorValue);
}

DWORD CSkypeProto::GetSettingDword(const char *setting, DWORD errorValue)
{
	return this->GetSettingDword(NULL, setting, errorValue);
}

wchar_t* CSkypeProto::GetSettingString(HANDLE hContact, const char *setting, wchar_t* errorValue)
{
	DBVARIANT dbv = {0};
	wchar_t* result = NULL;

	if ( !::DBGetContactSettingWString(hContact, this->m_szModuleName, setting, &dbv))
	{
		result = ::mir_wstrdup(dbv.pwszVal);
		::DBFreeVariant(&dbv);
	}
	else
	{
		result = ::mir_wstrdup(errorValue);
	}

	return result;
}

wchar_t* CSkypeProto::GetSettingString(const char *setting, wchar_t* errorValue)
{
	return this->GetSettingString(NULL, setting, errorValue);
}

char* CSkypeProto::GetDecodeSettingString(HANDLE hContact, const char *setting, char* errorValue)
{
	DBVARIANT dbv = {0};
	char* result = NULL;

	if ( !::DBGetContactSettingString(hContact, this->m_szModuleName, setting, &dbv))
	{
		result = ::mir_strdup(dbv.pszVal);
		::DBFreeVariant(&dbv);

		::CallService(
			MS_DB_CRYPT_DECODESTRING,
			::strlen(result),
			reinterpret_cast<LPARAM>(result));
	}
	else result = ::mir_strdup(errorValue);

	return result;
}

//

bool CSkypeProto::SetSettingByte(HANDLE hContact, const char *setting, BYTE value)
{
	return !::DBWriteContactSettingByte(hContact, this->m_szModuleName, setting, value);
}

bool CSkypeProto::SetSettingByte(const char *setting, BYTE errorValue)
{
	return this->SetSettingByte(NULL, setting, errorValue);
}

bool CSkypeProto::SetSettingWord(HANDLE hContact, const char *setting, WORD value)
{
	return !::DBWriteContactSettingWord(hContact, this->m_szModuleName, setting, value);
}

bool CSkypeProto::SetSettingWord(const char *setting, WORD value)
{
	return this->SetSettingWord(NULL, setting, value);
}

bool CSkypeProto::SetSettingDword(HANDLE hContact, const char *setting, DWORD value)
{
	return !::DBWriteContactSettingDword(hContact, this->m_szModuleName, setting, value);
}

bool CSkypeProto::SetSettingDword(const char *setting, DWORD value)
{
	return this->SetSettingDword(NULL, setting, value);
}

bool CSkypeProto::SetSettingString(HANDLE hContact, const char *szSetting, const wchar_t* value)
{
	return !::DBWriteContactSettingWString(hContact, this->m_szModuleName, szSetting, value);
}

bool CSkypeProto::SetSettingString(const char *szSetting, const wchar_t* value)
{
	return this->SetSettingString(NULL, szSetting, value);
}

bool CSkypeProto::SetDecodeSettingString(HANDLE hContact, const char *setting, const char* value)
{
	if( ::strcmp(value, ""))
	{
		mir_ptr<char> result (::mir_strdup(value));
		::CallService(MS_DB_CRYPT_ENCODESTRING, strlen(result), LPARAM((char*)result));
		
		return !db_set_s(hContact, m_szModuleName, setting, result);
	}
	
	return !this->SetSettingString(hContact, setting, L"");
}

//

void CSkypeProto::DeleteSetting(const char *setting)
{
	this->DeleteSetting(NULL, setting);
}

void CSkypeProto::DeleteSetting(HANDLE hContact, const char *setting)
{
	::DBDeleteContactSetting(hContact, this->m_szModuleName, setting);
}