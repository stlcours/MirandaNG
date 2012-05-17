//
// Utils.cpp
//

#include "stdafx.h"
#include "Utils.h"

BOOL IsNt50()
{
	WORD wOsVersion = LOWORD(GetVersion());
	BYTE bMajorVer = LOBYTE(wOsVersion);
	BYTE bMinorVer = HIBYTE(wOsVersion);

	return (bMajorVer>=5 && bMinorVer>=0);
}

void TruncateWithDots(TCHAR* szString, int iNewLen)
{
	assert(iNewLen >= 0);

	int iOrigLen = _tcslen(szString);
	if (iNewLen < iOrigLen)
	{
		TCHAR* p = szString+iNewLen;
		*p = _T('\0');
		for(int i=0; --p>=szString && i<3; i++)
		{
			*p = _T('.');
		}
	}
}

