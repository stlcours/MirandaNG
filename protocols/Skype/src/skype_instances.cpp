#include "skype_proto.h"

LIST<CSkypeProto> CSkypeProto::instanceList(1, CSkypeProto::CompareProtos);

int CSkypeProto::CompareProtos(const CSkypeProto *p1, const CSkypeProto *p2)
{
	return wcscmp(p1->m_tszUserName, p2->m_tszUserName);
}

CSkypeProto* CSkypeProto::InitSkypeProto(const char* protoName, const wchar_t* userName)
{
	if (CSkypeProto::instanceList.getCount() > 0) 
	{
		CSkypeProto::ShowNotification(
			::TranslateT("SkypeKit will only permit you to login to one account at a time.\n"
						 L"Adding multiple instances of SkypeKit is prohibited in the licence "
						 L"agreement and standard distribution terms."),
			MB_ICONWARNING);
		return NULL;
	}

	CSkypeProto *ppro = new CSkypeProto(protoName, userName);
	CSkypeProto::instanceList.insert(ppro);

	return ppro;
}

int CSkypeProto::UninitSkypeProto(CSkypeProto* ppro)
{
	CSkypeProto::instanceList.remove(ppro);
	delete ppro;

	return 0;
}