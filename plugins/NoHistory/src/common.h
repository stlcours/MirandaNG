#ifndef _COMMON_INC
#define _COMMON_INC

#define _WIN32_WINNT 0x0501
#define _WIN32_IE 0x0500

#include <windows.h>
#include <commctrl.h>

#include <win2k.h>
#include <newpluginapi.h>
#include <m_database.h>
#include <m_clist.h>
#include <m_clc.h>
#include <m_langpack.h>
#include <m_protocols.h>
#include <m_options.h>
#include <m_message.h>
#include <m_icolib.h>
#include <m_extraicons.h>

#include "resource.h"
#include "icons.h"
#include "options.h"
#include "Version.h"

#define MODULE						"NoHistory"
#define DBSETTING_REMOVE			"RemoveHistory"

extern HINSTANCE hInst;

#endif


void SrmmMenu_Load();
void SrmmMenu_Unload();


