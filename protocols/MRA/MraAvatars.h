#if !defined(AFX_MRA_AVATARS_H__F58D13FF_F6F2_476C_B8F0_7B9E9357CF48__INCLUDED_)
#define AFX_MRA_AVATARS_H__F58D13FF_F6F2_476C_B8F0_7B9E9357CF48__INCLUDED_


#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define PA_FORMAT_DEFAULT	255 // return file name of def avatar
#define GetContactAvatarFormat(hContact, dwDefaultFormat)	mraGetByte(hContact, "AvatarType", dwDefaultFormat)
#define SetContactAvatarFormat(hContact, dwFormat)			mraSetByte(hContact, "AvatarType", (BYTE)dwFormat)

INT_PTR CALLBACK MraAvatarsQueueDlgProcOpts(HWND hWndDlg, UINT msg, WPARAM wParam, LPARAM lParam);


#endif // !defined(AFX_MRA_AVATARS_H__F58D13FF_F6F2_476C_B8F0_7B9E9357CF48__INCLUDED_)
