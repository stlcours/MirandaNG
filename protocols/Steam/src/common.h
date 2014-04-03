#ifndef _COMMON_H_
#define _COMMON_H_

#include <windows.h>

#include <newpluginapi.h>
#include <m_database.h>
#include <m_langpack.h>
#include <m_message.h>
#include <m_netlib.h>
#include <m_options.h>
#include <m_popup.h>
#include <m_json.h>
#include <m_string.h>
#include <m_freeimage.h>
#include <m_protocols.h>
#include <m_protomod.h>
#include <m_protosvc.h>
#include <m_protoint.h>
#include <win2k.h>

#include "resource.h"
#include "version.h"

#define MODULE "Steam"

class CSteamProto;
extern HINSTANCE g_hInstance;

#include "http_request.h"

#include "Steam\steam.h"

#include "steam_proto.h"

#endif //_COMMON_H_