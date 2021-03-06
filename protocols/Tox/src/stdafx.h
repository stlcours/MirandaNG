#ifndef _COMMON_H_
#define _COMMON_H_

#include <windows.h>
#include <windns.h>
#include <time.h>
#include <commctrl.h>
#include <msapi/comptr.h>

#include <mmreg.h>
#include <MMDeviceAPI.h>

#define EXIT_ON_ERROR(hres) if (FAILED(hres)) { goto Exit; }

DEFINE_PROPERTYKEY(PKEY_Device_FriendlyName, 0xa45c254e, 0xdf1c, 0x4efd, 0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0, 14);

#include <vector>
#include <regex>
#include <map>

#include <newpluginapi.h>

#include <m_protoint.h>
#include <m_protosvc.h>

#include <m_database.h>
#include <m_langpack.h>
#include <m_options.h>
#include <m_netlib.h>
#include <m_popup.h>
#include <m_icolib.h>
#include <m_userinfo.h>
#include <m_addcontact.h>
#include <m_message.h>
#include <m_avatars.h>
#include <m_skin.h>
#include <m_chat_int.h>
#include <m_genmenu.h>
#include <m_clc.h>
#include <m_clist.h>
#include <m_gui.h>
#include <m_folders.h>
#include <m_assocmgr.h>
#include <m_json.h>
#include <m_http.h>

#include <tox.h>
#include <ToxAV.h>
#include <toxdns.h>
#include <toxencryptsave.h>

struct CToxProto;

#include "version.h"
#include "resource.h"
#include "tox_menus.h"
#include "tox_thread.h"
#include "tox_address.h"
#include "tox_dialogs.h"
#include "tox_profile.h"
#include "tox_options.h"
#include "tox_transfer.h"
#include "tox_multimedia.h"
#include "tox_chatrooms.h"
#include "tox_proto.h"

#include "http_request.h"

extern HINSTANCE g_hInstance;

#define MODULE "Tox"
#define MODULEW L"Tox"

#define TOX_ERROR -1

#define TOX_MAX_CONNECT_RETRIES 300
#define TOX_MAX_DISCONNECT_RETRIES 300

#define TOX_MAX_CALLS 1

#define TOX_INI_PATH "%miranda_path%\\Plugins\\tox.ini"
#define TOX_JSON_PATH "%miranda_userdata%\\tox.json"

#define TOX_SETTINGS_ID "ToxID"
#define TOX_SETTINGS_DNS "DnsID"
#define TOX_SETTINGS_CHAT_ID "ChatID"
#define TOX_SETTINGS_GROUP "DefaultGroup"
#define TOX_SETTINGS_AVATAR_HASH "AvatarHash"

#define TOX_SETTINGS_NODE_PREFIX "Node_"
#define TOX_SETTINGS_NODE_IPV4 TOX_SETTINGS_NODE_PREFIX"%d_IPv4"
#define TOX_SETTINGS_NODE_IPV6 TOX_SETTINGS_NODE_PREFIX"%d_IPv6"
#define TOX_SETTINGS_NODE_PORT TOX_SETTINGS_NODE_PREFIX"%d_Port"
#define TOX_SETTINGS_NODE_PKEY TOX_SETTINGS_NODE_PREFIX"%d_PubKey"
#define TOX_SETTINGS_NODE_COUNT TOX_SETTINGS_NODE_PREFIX"Count"

enum TOX_DB_EVENT
{
	DB_EVENT_ACTION = 10001,
	DB_EVENT_CALL = 20001
};

#define PSR_AUDIO "/RecvAudio"

#define TOX_MAX_AVATAR_SIZE 1 << 16 // 2 ^ 16 bytes

extern HMODULE g_hToxLibrary;

template<typename T>
T CreateFunction(LPCSTR functionName)
{
	return reinterpret_cast<T>(GetProcAddress(g_hToxLibrary, functionName));
}

#endif //_COMMON_H_