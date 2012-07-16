/*

Omegle plugin for Miranda Instant Messenger
_____________________________________________

Copyright © 2011-12 Robert Pösel

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "common.h"

// TODO: Make following as "globals" structure?

CLIST_INTERFACE* pcli;
int hLangpack;

HINSTANCE g_hInstance;
std::string g_strUserAgent;
DWORD g_mirandaVersion;

PLUGININFOEX pluginInfo = {
	sizeof(PLUGININFOEX),
	"Omegle Protocol",
	__VERSION_DWORD,
	"Provides basic support for Omegle Chat protocol. [Built: "__DATE__" "__TIME__"]",
	"Robert Posel",
	"robyer@seznam.cz",
	"(c) 2011-12 Robert Posel",
	"http://code.google.com/p/robyer/",
	UNICODE_AWARE,
	// {9E1D9244-606C-4ef4-99A0-1D7D23CB7601}
	{ 0x9e1d9244, 0x606c, 0x4ef4, { 0x99, 0xa0, 0x1d, 0x7d, 0x23, 0xcb, 0x76, 0x1 } }
};

/////////////////////////////////////////////////////////////////////////////
// Protocol instances
static int compare_protos(const OmegleProto *p1, const OmegleProto *p2)
{
	return _tcscmp(p1->m_tszUserName, p2->m_tszUserName);
}

OBJLIST<OmegleProto> g_Instances(1, compare_protos);

DWORD WINAPI DllMain(HINSTANCE hInstance,DWORD,LPVOID)
{
	g_hInstance = hInstance;
	return TRUE;
}

extern "C" __declspec(dllexport) PLUGININFOEX* MirandaPluginInfoEx(DWORD mirandaVersion)
{
	g_mirandaVersion = mirandaVersion;
	return &pluginInfo;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Interface information

extern "C" __declspec(dllexport) const MUUID MirandaInterfaces[] = {MIID_PROTOCOL, MIID_LAST};

/////////////////////////////////////////////////////////////////////////////////////////
// Load

static PROTO_INTERFACE* protoInit(const char *proto_name,const TCHAR *username )
{
	OmegleProto *proto = new OmegleProto(proto_name, username);
	g_Instances.insert(proto);
	return proto;
}

static int protoUninit(PROTO_INTERFACE* proto)
{
	g_Instances.remove(( OmegleProto* )proto);
	return EXIT_SUCCESS;
}

static HANDLE g_hEvents[1];

extern "C" int __declspec(dllexport) Load(void)
{

	mir_getLP(&pluginInfo);

	pcli = reinterpret_cast<CLIST_INTERFACE*>( CallService(
	    MS_CLIST_RETRIEVE_INTERFACE,0,reinterpret_cast<LPARAM>(g_hInstance)));

	PROTOCOLDESCRIPTOR pd = { 0 };
	pd.cbSize = sizeof(pd);
	pd.szName = "Omegle";
	pd.type = PROTOTYPE_PROTOCOL;
	pd.fnInit = protoInit;
	pd.fnUninit = protoUninit;
	CallService(MS_PROTO_REGISTERMODULE,0,reinterpret_cast<LPARAM>(&pd));

	InitIcons();
	//InitContactMenus();

	// Init native User-Agent
	{
		std::stringstream agent;
		agent << "Miranda IM/";
		agent << (( g_mirandaVersion >> 24) & 0xFF);
		agent << ".";
		agent << (( g_mirandaVersion >> 16) & 0xFF);
		agent << ".";
		agent << (( g_mirandaVersion >>  8) & 0xFF);
		agent << ".";
		agent << (( g_mirandaVersion      ) & 0xFF);
	#ifdef _WIN64
		agent << " Omegle Protocol x64/";
	#else
		agent << " Omegle Protocol/";
	#endif
		agent << __VERSION_STRING;
		g_strUserAgent = agent.str( );
	}

  return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Unload

extern "C" int __declspec(dllexport) Unload(void)
{
	//UninitContactMenus();
	for(size_t i=0; i<SIZEOF(g_hEvents); i++)
		UnhookEvent(g_hEvents[i]);

	g_Instances.destroy();

	return 0;
}
