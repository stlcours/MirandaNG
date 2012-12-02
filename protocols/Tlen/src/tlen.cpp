/*

Tlen Protocol Plugin for Miranda NG
Copyright (C) 2002-2004  Santithorn Bunchua
Copyright (C) 2004-2007  Piotr Piastucki

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

#include "commons.h"
#include "jabber.h"
#include "tlen_muc.h"
#include "tlen_file.h"
#include "tlen_voice.h"
#include "jabber_list.h"
#include "jabber_iq.h"
#include "resource.h"
#include "tlen_picture.h"
#include <m_file.h>
#include <richedit.h>
#include <ctype.h>
#include <m_icolib.h>
#include <m_genmenu.h>
#include "m_mucc.h"


int hLangpack;
HINSTANCE hInst;
HANDLE hMainThread;

PLUGININFOEX pluginInfoEx = {
	sizeof(PLUGININFOEX),
	"Tlen Protocol",
	PLUGIN_MAKE_VERSION(TLEN_MAJOR_VERSION,TLEN_MINOR_VERSION,TLEN_RELEASE_NUM,TLEN_BUILD_NUM),
	"Tlen protocol plugin for Miranda NG.",
	"Santithorn Bunchua, Adam Strzelecki, Piotr Piastucki",
	"the_leech@users.berlios.de",
	"(c) 2002-2012 Santithorn Bunchua, Piotr Piastucki",
	"http://miranda-ng.org/",
	UNICODE_AWARE,
	{0x748f8934, 0x781a, 0x528d, { 0x52, 0x08, 0x00, 0x12, 0x65, 0x40, 0x4a, 0xb3 }}
};

// Main jabber server connection thread global variables

int TlenUserInfoInit(void *ptr, WPARAM wParam, LPARAM lParam);
int TlenSystemModulesLoaded(void *ptr, WPARAM wParam, LPARAM lParam);

BOOL WINAPI DllMain(HINSTANCE hModule, DWORD dwReason, LPVOID lpvReserved)
{
#ifdef _DEBUG
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	hInst = hModule;
	return TRUE;
}

extern "C" __declspec(dllexport) PLUGININFOEX *MirandaPluginInfoEx(DWORD mirandaVersion)
{
	return &pluginInfoEx;
}

extern "C" __declspec(dllexport) const MUUID interfaces[] = {MIID_PROTOCOL, MIID_LAST};
extern "C" __declspec(dllexport) const MUUID* MirandaPluginInterfaces(void)
{
	return interfaces;
}

static IconItem iconList[] =
{
	{ LPGEN("Protocol icon"),          "PROTO",        IDI_TLEN       },
	{ LPGEN("Tlen inbox"),             "MAIL",         IDI_MAIL       },
	{ LPGEN("Group chats"),            "MUC",          IDI_MUC        },
	{ LPGEN("Tlen chats"),             "CHATS",        IDI_CHATS      },
	{ LPGEN("Grant authorization"),    "GRANT",        IDI_GRANT      },
	{ LPGEN("Request authorization"),  "REQUEST",      IDI_REQUEST    },
	{ LPGEN("Voice chat"),             "VOICE",        IDI_VOICE      },
	{ LPGEN("Microphone"),             "MICROPHONE",   IDI_MICROPHONE },
	{ LPGEN("Speaker"),                "SPEAKER",      IDI_SPEAKER    },
	{ LPGEN("Send image"),             "IMAGE",        IDI_IMAGE      }
};

static HANDLE GetIconHandle(int iconId)
{
	for (int i = 0; i < SIZEOF(iconList); i++)
		if (iconList[i].defIconID == iconId)
			return iconList[i].hIcolib;

	return NULL;
}

HICON GetIcolibIcon(int iconId)
{
	HANDLE handle = GetIconHandle(iconId);
	if (handle != NULL)
		return Skin_GetIconByHandle(handle);

	return NULL;
}

void ReleaseIcolibIcon(HICON hIcon) {
	Skin_ReleaseIcon(hIcon);
}

static void TlenRegisterIcons()
{
	Icon_Register(hInst, "Protocols/Tlen", iconList, SIZEOF(iconList), "TLEN");
}

int TlenPrebuildContactMenu(void *ptr, WPARAM wParam, LPARAM lParam)
{
	HANDLE hContact;
	DBVARIANT dbv;
	CLISTMENUITEM clmi = { sizeof(clmi) };
	JABBER_LIST_ITEM *item;
	TlenProtocol *proto = (TlenProtocol *)ptr;
	if ((hContact=(HANDLE) wParam) != NULL && proto->isOnline) {
		if (!DBGetContactSetting(hContact, proto->m_szModuleName, "jid", &dbv)) {
			if ((item=JabberListGetItemPtr(proto, LIST_ROSTER, dbv.pszVal)) != NULL) {
				if (item->subscription == SUB_NONE || item->subscription == SUB_FROM)
					clmi.flags = CMIM_FLAGS;
				else
					clmi.flags = CMIM_FLAGS|CMIF_HIDDEN;
				CallService(MS_CLIST_MODIFYMENUITEM, (WPARAM) proto->hMenuContactRequestAuth, (LPARAM) &clmi);

				if (item->subscription == SUB_NONE || item->subscription == SUB_TO)
					clmi.flags = CMIM_FLAGS;
				else
					clmi.flags = CMIM_FLAGS|CMIF_HIDDEN;
				CallService(MS_CLIST_MODIFYMENUITEM, (WPARAM) proto->hMenuContactGrantAuth, (LPARAM) &clmi);

				if (item->status != ID_STATUS_OFFLINE)
					clmi.flags = CMIM_FLAGS;
				else
					clmi.flags = CMIM_FLAGS|CMIF_HIDDEN;
				CallService(MS_CLIST_MODIFYMENUITEM, (WPARAM) proto->hMenuContactMUC, (LPARAM) &clmi);

				if (item->status != ID_STATUS_OFFLINE && !TlenVoiceIsInUse(proto))
					clmi.flags = CMIM_FLAGS;
				else
					clmi.flags = CMIM_FLAGS|CMIF_HIDDEN;
				CallService(MS_CLIST_MODIFYMENUITEM, (WPARAM) proto->hMenuContactVoice, (LPARAM) &clmi);

				if (item->status != ID_STATUS_OFFLINE)
					clmi.flags = CMIM_FLAGS;
				else
					clmi.flags = CMIM_FLAGS|CMIF_HIDDEN;
				CallService(MS_CLIST_MODIFYMENUITEM, (WPARAM) proto->hMenuPicture, (LPARAM) &clmi);

				DBFreeVariant(&dbv);
				return 0;
			}
			DBFreeVariant(&dbv);
		}
	}
	clmi.flags = CMIM_FLAGS|CMIF_HIDDEN;
	CallService(MS_CLIST_MODIFYMENUITEM, (WPARAM) proto->hMenuContactMUC, (LPARAM) &clmi);
	CallService(MS_CLIST_MODIFYMENUITEM, (WPARAM) proto->hMenuContactVoice, (LPARAM) &clmi);
	CallService(MS_CLIST_MODIFYMENUITEM, (WPARAM) proto->hMenuContactRequestAuth, (LPARAM) &clmi);
	CallService(MS_CLIST_MODIFYMENUITEM, (WPARAM) proto->hMenuContactGrantAuth, (LPARAM) &clmi);
	return 0;
}

INT_PTR TlenContactMenuHandleRequestAuth(void *ptr, LPARAM wParam, LPARAM lParam)
{
	HANDLE hContact;
	DBVARIANT dbv;
	TlenProtocol *proto = (TlenProtocol *)ptr;
	if ((hContact=(HANDLE) wParam) != NULL && proto->isOnline) {
		if (!DBGetContactSetting(hContact, proto->m_szModuleName, "jid", &dbv)) {
			JabberSend(proto, "<presence to='%s' type='subscribe'/>", dbv.pszVal);
			DBFreeVariant(&dbv);
		}
	}
	return 0;
}

INT_PTR TlenContactMenuHandleGrantAuth(void *ptr, LPARAM wParam, LPARAM lParam)
{
	HANDLE hContact;
	DBVARIANT dbv;
	TlenProtocol *proto = (TlenProtocol *)ptr;
	if ((hContact=(HANDLE) wParam) != NULL && proto->isOnline) {
		if (!DBGetContactSetting(hContact, proto->m_szModuleName, "jid", &dbv)) {
			JabberSend(proto, "<presence to='%s' type='subscribed'/>", dbv.pszVal);
			DBFreeVariant(&dbv);
		}
	}
	return 0;
}

INT_PTR TlenContactMenuHandleSendPicture(void *ptr, LPARAM wParam, LPARAM lParam)
{
	HANDLE hContact;
	TlenProtocol *proto = (TlenProtocol *)ptr;
	if ((hContact=(HANDLE) wParam) != NULL && proto->isOnline) {
		SendPicture(proto, hContact);
	}
	return 0;
}

INT_PTR TlenMenuHandleInbox(void *ptr, LPARAM wParam, LPARAM lParam)
{
	char szFileName[ MAX_PATH ];
	DBVARIANT dbv;
	NETLIBHTTPREQUEST req;
	NETLIBHTTPHEADER headers[2];
	NETLIBHTTPREQUEST *resp;
	char *login = NULL, *password = NULL;
	char form[1024];
	char cookie[1024];
	TlenProtocol *proto = (TlenProtocol *)ptr;
	if (!DBGetContactSetting(NULL, proto->m_szModuleName, "LoginName", &dbv)) {
		login = mir_strdup(dbv.pszVal);
		DBFreeVariant(&dbv);
	}
	if (DBGetContactSettingByte(NULL, proto->m_szModuleName, "SavePassword", TRUE) == TRUE) {
		if (!DBGetContactSetting(NULL, proto->m_szModuleName, "Password", &dbv)) {
			CallService(MS_DB_CRYPT_DECODESTRING, strlen(dbv.pszVal)+1, (LPARAM) dbv.pszVal);
			password = mir_strdup(dbv.pszVal);
			DBFreeVariant(&dbv);
		}
	} else if (proto->threadData != NULL && strlen(proto->threadData->password) > 0) {
		password = mir_strdup(proto->threadData->password);
	}

	ZeroMemory(&cookie, sizeof(cookie));
	if (login != NULL && password != NULL) {
		mir_snprintf( form, SIZEOF(form), "username=%s&password=%s", login, password);
		headers[0].szName = "Content-Type";
		headers[0].szValue = "application/x-www-form-urlencoded";
		ZeroMemory(&req, sizeof(req));
		req.cbSize = sizeof(req);
		req.requestType = REQUEST_POST;
		req.flags = 0;
		req.headersCount = 1;
		req.headers = headers;
		req.pData = form;
		req.dataLength = (int)strlen(form);
		req.szUrl = "http://poczta.o2.pl/login.html";
		resp = (NETLIBHTTPREQUEST *)CallService(MS_NETLIB_HTTPTRANSACTION, (WPARAM)proto->hNetlibUser, (LPARAM)&req);
		if (resp != NULL) {
			if (resp->resultCode/100 == 2 || resp->resultCode == 302) {
				int i;
				for (i=0; i<resp->headersCount; i++ ) {
					if (strcmpi(resp->headers[i].szName, "Set-Cookie") == 0) {
						char *start = strstr(resp->headers[i].szValue, "ssid=");
						if (start != NULL) {
							char *end = strstr(resp->headers[i].szValue, ";");
							start = start + 5;
							if (end == NULL) {
								end = resp->headers[i].szValue + strlen(resp->headers[i].szValue);
							}
							strncpy(cookie, start, (end - start));
							break;
						}
					}
				}
			}
			CallService(MS_NETLIB_FREEHTTPREQUESTSTRUCT, 0, (LPARAM)resp);
		}
	}
	mir_free(login);
	mir_free(password);
	_snprintf(szFileName, sizeof(szFileName), "http://poczta.o2.pl/login.html?sid=%s", cookie);
	CallService(MS_UTILS_OPENURL, (WPARAM) 1, (LPARAM) szFileName);
	return 0;
}

int TlenOnModulesLoaded(void *ptr, WPARAM wParam, LPARAM lParam)
{

	char str[128];
	TlenProtocol *proto = (TlenProtocol *)ptr;
	/* Set all contacts to offline */
	HANDLE hContact = db_find_first();
	while (hContact != NULL) {
		char *szProto = GetContactProto(hContact);
		if (szProto != NULL && !strcmp(szProto, proto->m_szModuleName)) {
			if (DBGetContactSettingWord(hContact, proto->m_szModuleName, "Status", ID_STATUS_OFFLINE) != ID_STATUS_OFFLINE) {
				DBWriteContactSettingWord(hContact, proto->m_szModuleName, "Status", ID_STATUS_OFFLINE);
			}
		}
		hContact = db_find_next(hContact);
	}
	TlenMUCInit(proto);
	sprintf(str, "%s", LPGEN("Incoming mail"));
	SkinAddNewSoundEx("TlenMailNotify", proto->m_szProtoName, str);
	sprintf(str, "%s", LPGEN("Alert"));
	SkinAddNewSoundEx("TlenAlertNotify", proto->m_szProtoName, str);
	sprintf(str, "%s", LPGEN("Voice chat"));
	SkinAddNewSoundEx("TlenVoiceNotify", proto->m_szProtoName, str);

	HookEventObj_Ex(ME_USERINFO_INITIALISE, proto, TlenUserInfoInit);

	return 0;
}


int TlenPreShutdown(void *ptr, WPARAM wParam, LPARAM lParam)
{
	TlenProtocol *proto = (TlenProtocol *)ptr;
	JabberLog(proto, "TLEN TlenPreShutdown");
	return 0;
}


static void initMenuItems(TlenProtocol *proto)
{
	char text[_MAX_PATH];

	CLISTMENUITEM mi = { sizeof(mi) }, clmi = { sizeof(clmi) };
	clmi.flags = CMIM_FLAGS | CMIF_GRAYED;

	mi.pszContactOwner = proto->m_szModuleName;
	mi.popupPosition = 500090000;

	strcpy(text, proto->m_szModuleName);
	mi.pszService = text;
	mi.ptszName = proto->m_tszUserName;
	mi.position = -1999901009;
	mi.pszPopupName = (char *)-1;
	mi.flags = CMIF_ROOTPOPUP | CMIF_TCHAR | CMIF_ICONFROMICOLIB;
	mi.icolibItem = GetIconHandle(IDI_TLEN);
	proto->hMenuRoot = Menu_AddMainMenuItem(&mi);

	mi.flags = CMIF_CHILDPOPUP | CMIF_ICONFROMICOLIB;
	//mi.pszPopupName = (char *)proto->hMenuRoot;
	mi.hParentMenu = (HGENMENU)proto->hMenuRoot;

	proto->hMenuChats = NULL;
	if (ServiceExists(MS_MUCC_NEW_WINDOW))
	{
		sprintf(text, "%s/MainMenuChats", proto->m_szModuleName);
		CreateServiceFunction_Ex(text, proto, TlenMUCMenuHandleChats);
		mi.pszName = "Tlen Chats";
		mi.position = 2000050001;
		mi.icolibItem = GetIconHandle(IDI_CHATS);
		mi.pszService = text;
		proto->hMenuChats = Menu_AddMainMenuItem(&mi);
		CallService(MS_CLIST_MODIFYMENUITEM, (WPARAM) proto->hMenuChats, (LPARAM) &clmi);
	}

	// "Multi-User Conference"
	sprintf(text, "%s/MainMenuMUC", proto->m_szModuleName);
	CreateServiceFunction_Ex(text, proto, TlenMUCMenuHandleMUC);
	mi.pszName = "Multi-User Conference";
	mi.position = 2000050002;
	mi.icolibItem = GetIconHandle(IDI_MUC);
	mi.pszService = text;
	proto->hMenuMUC = Menu_AddMainMenuItem(&mi);
	CallService(MS_CLIST_MODIFYMENUITEM, (WPARAM) proto->hMenuMUC, (LPARAM) &clmi);

	sprintf(text, "%s/MainMenuInbox", proto->m_szModuleName);
	CreateServiceFunction_Ex(text, proto, TlenMenuHandleInbox);
	mi.pszName = "Tlen Mail";
	mi.position = 2000050003;
	mi.icolibItem = GetIconHandle(IDI_MAIL);
	mi.pszService = text;
	proto->hMenuInbox = Menu_AddMainMenuItem(&mi);

	mi.hParentMenu = NULL;

	// "Send picture"
	sprintf(text, "%s/SendPicture", proto->m_szModuleName);
	CreateServiceFunction_Ex(text, proto, TlenContactMenuHandleSendPicture);
	mi.flags = CMIF_ICONFROMICOLIB;
	mi.pszName = "Send picture";
	mi.position = -2000019030;
	mi.icolibItem = GetIconHandle(IDI_IMAGE);
	mi.pszService = text;
	proto->hMenuPicture = Menu_AddContactMenuItem(&mi);

	// "Invite to MUC"
	sprintf(text, "%s/ContactMenuMUC", proto->m_szModuleName);
	CreateServiceFunction_Ex(text, proto, TlenMUCContactMenuHandleMUC);
	mi.pszName = "Multi-User Conference";
	mi.position = -2000019020;
	mi.icolibItem = GetIconHandle(IDI_MUC);
	mi.pszService = text;
	proto->hMenuContactMUC = Menu_AddContactMenuItem(&mi);

	// "Invite to voice chat"
	sprintf(text, "%s/ContactMenuVoice", proto->m_szModuleName);
	CreateServiceFunction_Ex(text, proto, TlenVoiceContactMenuHandleVoice);
	mi.pszName = "Voice Chat";
	mi.position = -2000019010;
	mi.icolibItem = GetIconHandle(IDI_VOICE);
	mi.pszService = text;
	proto->hMenuContactVoice = Menu_AddContactMenuItem(&mi);

	// "Request authorization"
	sprintf(text, "%s/RequestAuth", proto->m_szModuleName);
	CreateServiceFunction_Ex(text, proto, TlenContactMenuHandleRequestAuth);
	mi.pszName = "Request authorization";
	mi.position = -2000001001;
	mi.icolibItem = GetIconHandle(IDI_REQUEST);
	mi.pszService = text;
	proto->hMenuContactRequestAuth = Menu_AddContactMenuItem(&mi);

	// "Grant authorization"
	sprintf(text, "%s/GrantAuth", proto->m_szModuleName);
	CreateServiceFunction_Ex(text, proto, TlenContactMenuHandleGrantAuth);
	mi.pszName = "Grant authorization";
	mi.position = -2000001000;
	mi.icolibItem = GetIconHandle(IDI_GRANT);
	mi.pszService = text;
	proto->hMenuContactGrantAuth = Menu_AddContactMenuItem(&mi);
}

void uninitMenuItems(TlenProtocol *proto) {
	CallService(MS_CLIST_REMOVEMAINMENUITEM, (WPARAM)proto->hMenuChats, (LPARAM) 0);
	CallService(MS_CLIST_REMOVEMAINMENUITEM, (WPARAM)proto->hMenuMUC, (LPARAM) 0);
	CallService(MS_CLIST_REMOVEMAINMENUITEM, (WPARAM)proto->hMenuInbox, (LPARAM) 0);
	CallService(MS_CLIST_REMOVEMAINMENUITEM, (WPARAM)proto->hMenuRoot, (LPARAM) 0);
	CallService(MS_CLIST_REMOVECONTACTMENUITEM, (WPARAM)proto->hMenuContactMUC, (LPARAM) 0);
	CallService(MS_CLIST_REMOVECONTACTMENUITEM, (WPARAM)proto->hMenuPicture, (LPARAM) 0);
	CallService(MS_CLIST_REMOVECONTACTMENUITEM, (WPARAM)proto->hMenuContactVoice, (LPARAM) 0);
	CallService(MS_CLIST_REMOVECONTACTMENUITEM, (WPARAM)proto->hMenuContactRequestAuth, (LPARAM) 0);
	CallService(MS_CLIST_REMOVECONTACTMENUITEM, (WPARAM)proto->hMenuContactGrantAuth, (LPARAM) 0);
}

TlenProtocol* tlenProtoInit( const char* pszProtoName, const TCHAR* tszUserName )
{
	TlenProtocol* ppro = new TlenProtocol( pszProtoName, tszUserName );

	return ppro;
}

int TlenSystemModulesLoaded(void *ptr, WPARAM wParam, LPARAM lParam)
{

	TlenProtocol *proto = (TlenProtocol *)ptr;
	initMenuItems(proto);

	return 0;
}

static int tlenProtoUninit( TlenProtocol* ppro )
{
	delete ppro;
	return 0;
}

extern "C" int __declspec(dllexport) Load(void)
{
	mir_getLP( &pluginInfoEx );

	DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &hMainThread, THREAD_SET_CONTEXT, FALSE, 0);

	srand((unsigned) time(NULL));

	TlenRegisterIcons();

	// Register protocol module
	PROTOCOLDESCRIPTOR pd = { sizeof(pd) };
	pd.szName = "TLEN";
	pd.fnInit = ( pfnInitProto )tlenProtoInit;
	pd.fnUninit = ( pfnUninitProto )tlenProtoUninit;
	pd.type = PROTOTYPE_PROTOCOL;
	CallService(MS_PROTO_REGISTERMODULE, 0, (LPARAM) &pd);

	return 0;
}

extern "C" int __declspec(dllexport) Unload(void)
{
	return 0;
}
