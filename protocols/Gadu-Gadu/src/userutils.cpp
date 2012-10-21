////////////////////////////////////////////////////////////////////////////////
// Gadu-Gadu Plugin for Miranda IM
//
// Copyright (c) 2003-2006 Adam Strzelecki <ono+miranda@java.pl>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
////////////////////////////////////////////////////////////////////////////////

#include "gg.h"

////////////////////////////////////////////////////////////////////////////////
// Create New Account : Proc

void *gg_doregister(GGPROTO *gg, char *newPass, char *newEmail)
{
	// Connection handles
	struct gg_http *h = NULL;
	struct gg_pubdir *s = NULL;
	GGTOKEN token;

#ifdef DEBUGMODE
	gg->netlog("gg_doregister(): Starting.");
#endif
	if (!newPass || !newEmail) return NULL;

	// Load token
	if (!gg->gettoken(&token)) return NULL;

	if (!(h = gg_register3(newEmail, newPass, token.id, token.val, 0)) || !(s = (gg_pubdir*)h->data) || !s->success || !s->uin)
	{
		TCHAR error[128];
		mir_sntprintf(error, SIZEOF(error), TranslateT("Cannot register new account because of error:\n\t%S"),
			(h && !s) ? http_error_string(h ? h->error : 0) :
			(s ? TranslateT("Registration rejected") : _tcserror(errno)));
		MessageBox(
			NULL,
			error,
			gg->m_tszUserName,
			MB_OK | MB_ICONSTOP
		);
		gg->netlog("gg_doregister(): Cannot register because of \"%s\".", strerror(errno));
	}
	else
	{
		db_set_dw(NULL, gg->m_szModuleName, GG_KEY_UIN, s->uin);
		CallService(MS_DB_CRYPT_ENCODESTRING, strlen(newPass) + 1, (LPARAM) newPass);
		gg->checknewuser(s->uin, newPass);
		db_set_s(NULL, gg->m_szModuleName, GG_KEY_PASSWORD, newPass);
		db_set_s(NULL, gg->m_szModuleName, GG_KEY_EMAIL, newEmail);
		gg_pubdir_free(h);
		gg->netlog("gg_doregister(): Account registration succesful.");
		MessageBox( NULL, 
			TranslateT("You have registered new account.\nPlease fill up your personal details in \"M->View/Change My Details...\""),
			gg->m_tszUserName, MB_OK | MB_ICONINFORMATION);
	}

#ifdef DEBUGMODE
	gg->netlog("gg_doregister(): End.");
#endif

	return NULL;
}

////////////////////////////////////////////////////////////////////////////////
// Remove Account : Proc
void *gg_dounregister(GGPROTO *gg, uin_t uin, char *password)
{
	// Connection handles
	struct gg_http *h;
	struct gg_pubdir *s;
	GGTOKEN token;

#ifdef DEBUGMODE
	gg->netlog("gg_dounregister(): Starting.");
#endif
	if (!uin || !password) return NULL;

	// Load token
	if (!gg->gettoken(&token)) return NULL;

	if (!(h = gg_unregister3(uin, password, token.id, token.val, 0)) || !(s = (gg_pubdir*)h->data) || !s->success || s->uin != uin)
	{
		TCHAR error[128];
		mir_sntprintf(error, SIZEOF(error), TranslateT("Your account cannot be removed because of error:\n\t%S"),
			(h && !s) ? http_error_string(h ? h->error : 0) :
			(s ? TranslateT("Bad number or password") : _tcserror(errno)));
		MessageBox(NULL, error, gg->m_tszUserName, MB_OK | MB_ICONSTOP);
		gg->netlog("gg_dounregister(): Cannot remove account because of \"%s\".", strerror(errno));
	}
	else
	{
		gg_pubdir_free(h);
		db_unset(NULL, gg->m_szModuleName, GG_KEY_PASSWORD);
		db_unset(NULL, gg->m_szModuleName, GG_KEY_UIN);
		gg->netlog("gg_dounregister(): Account %d has been removed.", uin);
		MessageBox(NULL, TranslateT("Your account has been removed."), gg->m_tszUserName, MB_OK | MB_ICONINFORMATION);
	}

#ifdef DEBUGMODE
	gg->netlog("gg_dounregister(): End.");
#endif

	return NULL;
}

////////////////////////////////////////////////////////////////////////////////
// Change Password Page : Proc

void *gg_dochpass(GGPROTO *gg, uin_t uin, char *password, char *newPass)
{
	// Readup email
	char email[255] = "\0"; DBVARIANT dbv_email;
	// Connection handles
	struct gg_http *h;
	struct gg_pubdir *s;
	GGTOKEN token;

#ifdef DEBUGMODE
	gg->netlog("gg_dochpass(): Starting.");
#endif
	if (!uin || !password || !newPass) return NULL;

	if (!db_get_s(NULL, gg->m_szModuleName, GG_KEY_EMAIL, &dbv_email, DBVT_ASCIIZ)) 
	{
		strncpy(email, dbv_email.pszVal, sizeof(email));
		DBFreeVariant(&dbv_email);
	}

	// Load token
	if (!gg->gettoken(&token))
		return NULL;

	if (!(h = gg_change_passwd4(uin, email, password, newPass, token.id, token.val, 0)) || !(s = (gg_pubdir*)h->data) || !s->success)
	{
		TCHAR error[128];
		mir_sntprintf(error, SIZEOF(error), TranslateT("Your password cannot be changed because of error:\n\t%S"),
			(h && !s) ? http_error_string(h ? h->error : 0) :
			(s ? TranslateT("Invalid data entered") : _tcserror(errno)));
		MessageBox(NULL, error, gg->m_tszUserName, MB_OK | MB_ICONSTOP);
		gg->netlog("gg_dochpass(): Cannot change password because of \"%s\".", strerror(errno));
	}
	else
	{
		gg_pubdir_free(h);
		CallService(MS_DB_CRYPT_ENCODESTRING, strlen(newPass) + 1, (LPARAM) newPass);
		db_set_s(NULL, gg->m_szModuleName, GG_KEY_PASSWORD, newPass);
		gg->netlog("gg_dochpass(): Password change succesful.");
		MessageBox(NULL, TranslateT("Your password has been changed."), gg->m_tszUserName, MB_OK | MB_ICONINFORMATION);
	}

#ifdef DEBUGMODE
	gg->netlog("gg_dochpass(): End.");
#endif

	return NULL;
}

////////////////////////////////////////////////////////////////////////////////
// Change E-mail Page : Proc

void *gg_dochemail(GGPROTO *gg, uin_t uin, char *password, char *email, char *newEmail)
{
	// Connection handles
	struct gg_http *h;
	struct gg_pubdir *s;
	GGTOKEN token;

#ifdef DEBUGMODE
	gg->netlog("gg_doemail(): Starting.");
#endif
	if (!uin || !email || !newEmail) return NULL;

	// Load token
	if (!gg->gettoken(&token)) return NULL;

	if (!(h = gg_change_passwd4(uin, newEmail, password, password, token.id, token.val, 0)) || !(s = (gg_pubdir*)h->data) || !s->success)
	{
		TCHAR error[128];
		mir_sntprintf(error, SIZEOF(error), TranslateT("Your e-mail cannot be changed because of error:\n\t%s"),
			(h && !s) ? http_error_string(h ? h->error : 0) : (s ? TranslateT("Bad old e-mail or password") : _tcserror(errno)));
		MessageBox(NULL, error, gg->m_tszUserName, MB_OK | MB_ICONSTOP);
		gg->netlog("gg_dochpass(): Cannot change e-mail because of \"%s\".", strerror(errno));
	}
	else
	{
		gg_pubdir_free(h);
		db_set_s(NULL, gg->m_szModuleName, GG_KEY_EMAIL, newEmail);
		gg->netlog("gg_doemail(): E-mail change succesful.");
		MessageBox(NULL, TranslateT("Your e-mail has been changed."), gg->m_tszUserName, MB_OK | MB_ICONINFORMATION);
	}

#ifdef DEBUGMODE
	gg->netlog("gg_doemail(): End.");
#endif

	return NULL;
}

////////////////////////////////////////////////////////////////////////////////
// User Util Dlg Page : Data
INT_PTR CALLBACK gg_userutildlgproc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	GGUSERUTILDLGDATA *dat = (GGUSERUTILDLGDATA *)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);

	switch (msg)
	{
		case WM_INITDIALOG:
			TranslateDialogDefault(hwndDlg);
			WindowSetIcon(hwndDlg, "settings");
			dat = (GGUSERUTILDLGDATA *)lParam;
			SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR)lParam);
			if (dat) SetDlgItemTextA(hwndDlg, IDC_EMAIL, dat->email); // Readup email
			return TRUE;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDC_PASSWORD:
				case IDC_CPASSWORD:
				case IDC_CONFIRM:
				{
					char pass[128], cpass[128];
					BOOL enable;
					GetDlgItemTextA(hwndDlg, IDC_PASSWORD, pass, sizeof(pass));
					GetDlgItemTextA(hwndDlg, IDC_CPASSWORD, cpass, sizeof(cpass));
					enable = strlen(pass) && strlen(cpass) && !strcmp(cpass, pass);
					if (dat && dat->mode == GG_USERUTIL_REMOVE)
						EnableWindow(GetDlgItem(hwndDlg, IDOK), IsDlgButtonChecked(hwndDlg, IDC_CONFIRM) ? enable : FALSE);
					else
						EnableWindow(GetDlgItem(hwndDlg, IDOK), enable);
					break;
				}

				case IDOK:
				{
					char pass[128], cpass[128], email[128];
					GetDlgItemTextA(hwndDlg, IDC_PASSWORD, pass, sizeof(pass));
					GetDlgItemTextA(hwndDlg, IDC_CPASSWORD, cpass, sizeof(cpass));
					GetDlgItemTextA(hwndDlg, IDC_EMAIL, email, sizeof(email));
					EndDialog(hwndDlg, IDOK);

					// Check dialog box mode
					if (!dat) break;
					switch (dat->mode)
					{
						case GG_USERUTIL_CREATE: gg_doregister(dat->gg, pass, email);							break;
						case GG_USERUTIL_REMOVE: gg_dounregister(dat->gg, dat->uin, pass);						break;
						case GG_USERUTIL_PASS:   gg_dochpass(dat->gg, dat->uin, dat->pass, pass);				break;
						case GG_USERUTIL_EMAIL:  gg_dochemail(dat->gg, dat->uin, dat->pass, dat->email, email);	break;
					}
					break;
				}

				case IDCANCEL:
					EndDialog(hwndDlg, IDCANCEL);
					break;
			}
			break;

		case WM_DESTROY:
			WindowFreeIcon(hwndDlg);
			break;
	}
	return FALSE;
}

//////////////////////////////////////////////////////////
// Hooks protocol event

HANDLE GGPROTO::hookProtoEvent(const char* szEvent, GGEventFunc handler)
{
	return ::HookEventObj(szEvent, ( MIRANDAHOOKOBJ )*( void** )&handler, this);
}

//////////////////////////////////////////////////////////
// Adds a new protocol specific service function

void GGPROTO::createObjService(const char* szService, GGServiceFunc serviceProc)
{
	CreateServiceFunctionObj(szService, (MIRANDASERVICEOBJ)*( void** )&serviceProc, this);
}

void GGPROTO::createProtoService(const char* szService, GGServiceFunc serviceProc)
{
	char str[MAXMODULELABELLENGTH];
	mir_snprintf(str, sizeof(str), "%s%s", m_szModuleName, szService);
	CreateServiceFunctionObj(str, (MIRANDASERVICEOBJ)*( void** )&serviceProc, this);
}

//////////////////////////////////////////////////////////
// Forks a thread

void GGPROTO::forkthread(GGThreadFunc pFunc, void *param)
{
	UINT threadId;
	CloseHandle( mir_forkthreadowner((pThreadFuncOwner)*(void**)&pFunc, this, param, &threadId));
}

//////////////////////////////////////////////////////////
// Forks a thread and returns a pseudo handle for it

HANDLE GGPROTO::forkthreadex(GGThreadFunc pFunc, void *param, UINT *threadId)
{
	return mir_forkthreadowner((pThreadFuncOwner)*(void**)&pFunc, this, param, threadId);
}

//////////////////////////////////////////////////////////
// Wait for thread to stop

void GGPROTO::threadwait(GGTHREAD *thread)
{
	if (!thread->hThread) return;
	while (WaitForSingleObjectEx(thread->hThread, INFINITE, TRUE) != WAIT_OBJECT_0);
	CloseHandle(thread->hThread);
	ZeroMemory(thread, sizeof(GGTHREAD));
}

