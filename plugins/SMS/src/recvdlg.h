#if !defined(AFX_SMS_RECVDLG_H__F58D13FF_F6F2_476C_B8F0_7B9E9357CF48__INCLUDED_)
#define AFX_SMS_RECVDLG_H__F58D13FF_F6F2_476C_B8F0_7B9E9357CF48__INCLUDED_

DWORD	RecvSMSWindowInitialize		();
void	RecvSMSWindowDestroy		();
HWND	RecvSMSWindowAdd			(HCONTACT hContact,DWORD dwEventType,LPWSTR lpwszPhone,SIZE_T dwPhoneSize,LPSTR lpszMessage,SIZE_T dwMessageSize);
void	RecvSMSWindowRemove			(HWND hWndDlg);



#endif // !defined(AFX_SMS_RECVDLG_H__F58D13FF_F6F2_476C_B8F0_7B9E9357CF48__INCLUDED_)
