#include "skype_proto.h"

_tag_iconList CSkypeProto::IconList[] =
{
	{ LPGENT("Protocol icon"),		"main",			IDI_ICON },
	{ LPGENT("Revoke authorization"),	"authRevoke",		IDI_AUTH_REVOKE },
	{ LPGENT("Request authorization"),	"authRequest",		IDI_AUTH_REQUEST },
	{ LPGENT("Grant authorization"),	"authGrant",		IDI_AUTH_GRANT },
};

void CSkypeProto::InitIcons()
{
	wchar_t szFile[MAX_PATH];
	::GetModuleFileName(g_hInstance, szFile, MAX_PATH);

	char szSettingName[100];
	wchar_t szSectionName[100];

	SKINICONDESC sid = {0};
	sid.cbSize = sizeof(SKINICONDESC);
	sid.flags = SIDF_ALL_TCHAR;
	sid.ptszDefaultFile = szFile;
	sid.pszName = szSettingName;
	sid.ptszSection = szSectionName;

	::mir_sntprintf(szSectionName, SIZEOF(szSectionName), _T("%s/%s"), LPGENT("Protocols"), LPGENT(MODULE));
	for (int i = 0; i < SIZEOF(CSkypeProto::IconList); i++) 
	{
		::mir_snprintf(szSettingName, SIZEOF(szSettingName), "%s_%s", MODULE, CSkypeProto::IconList[i].Name);

		sid.ptszDescription = CSkypeProto::IconList[i].Description;
		sid.iDefaultIndex = -CSkypeProto::IconList[i].IconId;
		CSkypeProto::IconList[i].Handle = ::Skin_AddIcon(&sid);
	}	
}

HANDLE CSkypeProto::GetIconHandle(const char* name)
{
	for (size_t i = 0; i < SIZEOF(CSkypeProto::IconList); i++)
	{
		if (::strcmp(CSkypeProto::IconList[i].Name, name) == 0)
			return CSkypeProto::IconList[i].Handle;
	}
	return 0;
}

void CSkypeProto::UninitIcons()
{
	for (size_t i = 0; i < SIZEOF(CSkypeProto::IconList); i++)
	{
		::Skin_RemoveIcon(CSkypeProto::IconList[i].Name);
	}
}