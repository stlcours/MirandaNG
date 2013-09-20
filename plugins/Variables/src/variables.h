/*
    Variables Plugin for Miranda-IM (www.miranda-im.org)
    Copyright 2003-2006 P. Boon

    This program is mir_free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#define _CRT_NON_CONFORMING_SWPRINTFS
#define _CRT_SECURE_NO_WARNINGS
#define PCRE_STATIC

#include <malloc.h>
#include <windows.h>
#include <uxtheme.h>
#include <time.h>
#include <tlhelp32.h>
#include <vdmdbg.h>
#include <lmcons.h>

#include <win2k.h>
#include <newpluginapi.h>
#include <m_langpack.h>
#include <m_database.h>
#include <m_protosvc.h>
#include <m_clist.h>
#include <m_contacts.h>
#include <m_options.h>
#include <m_icolib.h>
#include <m_clui.h>
#include <m_clc.h>

#include <m_variables.h>
#include <m_metacontacts.h>

#include "..\helpers\gen_helpers.h"

#include "ac\ac.h"
#include "pcre\include\pcre.h"
#include "libxml\xmlmemory.h"
#include "libxml\debugXML.h"
#include "libxml\HTMLtree.h"
#include "libxml\xmlIO.h"
#include "libxml\DOCBparser.h"
#include "libxml\xinclude.h"
#include "libxml\catalog.h"
#include "libxslt\xslt.h"
#include "libxslt\xsltInternals.h"
#include "libxslt\transform.h"
#include "libxslt\xsltutils.h"

#include "resource.h"
#include "version.h"
#include "contact.h"
#include "enumprocs.h"
#include "parse_alias.h"
#include "parse_external.h"
#include "parse_inet.h"
#include "parse_logic.h"
#include "parse_math.h"
#include "parse_metacontacts.h"
#include "parse_miranda.h"
#include "parse_regexp.h"
#include "parse_str.h"
#include "parse_system.h"
#include "parse_variables.h"
#include "parse_xml.h"

#define MODULENAME "Variables"

#define SETTING_STARTUPTEXT    "StartupText"
#define SETTING_STRIPCRLF      "StripCRLF"
#define SETTING_STRIPWS        "StripWS"
#define SETTING_STRIPALL       "StripAll"
#define SETTING_PARSEATSTARTUP "ParseAtStartup"
#define SETTING_SPLITTERPOS    "SplitterPos"
#define SETTING_SUBJECT        "LastSubject"

#define FIELD_CHAR          '%'
#define FUNC_CHAR           '?'
#define FUNC_ONCE_CHAR      '!'
#define DONTPARSE_CHAR      '`'
#define TRYPARSE_CHAR_OPEN	 '['
#define TRYPARSE_CHAR_CLOSE ']'
#define COMMENT_STRING      "#"

// special tokens
#define SUBJECT       "subject"
#define MIR_EXTRATEXT "extratext"

// options
#define IDT_PARSE 1

#define DM_SPLITTERMOVED     (WM_USER+15)

// Messages you can send to the help window:
#define VARM_PARSE              (WM_APP+11) // wParam=lParam=0
#define VARM_SETINPUTTEXT       (WM_APP+12)
#define VARM_GETINPUTTEXT	     (WM_APP+13)
#define VARM_GETINPUTTEXTLENGTH (WM_APP+14)
#define VARM_SETSUBJECT         (WM_APP+15)
#define VARM_GETSUBJECT         (WM_APP+16) // wParam=HANDLE hContact
#define VARM_SETEXTRATEXT       (WM_APP+17)
#define VARM_GETEXTRATEXT       (WM_APP+18)
#define VARM_GETEXTRATEXTLENGTH (WM_APP+19)
#define VARM_GETDIALOG          (WM_APP+20)

// if a different struct internally is used, we can use TOKENREGISTEREX
#define TOKENREGISTEREX TOKENREGISTER

// old struct
typedef struct {
	int cbSize;
	char *szFormat;
	char *szSource;
	HANDLE hContact;
	int pCount;  // number of succesful parses
	int eCount;	 // number of failures
} FORMATINFOV1;

struct ParseOptions {
	BOOL bStripEOL;
	BOOL bStripWS;
	BOOL bStripAll;
};

extern HINSTANCE hInst;
extern struct ParseOptions gParseOpts;
extern int hLangpack;

// variables.c
//TCHAR *getArguments(char *string, char ***aargv, int *aargc);
//int isValidTokenChar(char c);
TCHAR *formatString(FORMATINFO *fi);
int  setParseOptions(struct ParseOptions *po);
int  LoadVarModule();
int  UnloadVarModule();
// tokenregister.c
int  registerIntToken(TCHAR *szToken, TCHAR *(*parseFunction)(ARGUMENTSINFO *ai), int extraFlags, char* szHelpText);
INT_PTR registerToken(WPARAM wParam, LPARAM lParam);
int  deRegisterToken(TCHAR *var);
TOKENREGISTEREX *searchRegister(TCHAR *var, int type);
TCHAR *parseFromRegister(ARGUMENTSINFO *ai);
TOKENREGISTEREX *getTokenRegister(int i);
int  getTokenRegisterCount();
TOKENREGISTER *getTokenRegisterByIndex(int i);
void deRegisterTemporaryVariables();
int  initTokenRegister();
int  deinitTokenRegister();
// contact.c
BYTE getContactInfoType(TCHAR* type);
TCHAR* getContactInfoT(BYTE type, HANDLE hContact);
int  getContactFromString( CONTACTSINFO* );
int  initContactModule();
int  deinitContactModule();
// alias
int  registerAliasTokens();
void unregisterAliasTokens();
// system
int  registerSystemTokens();
// external
int  registerExternalTokens();
int  deInitExternal();
// miranda
int  registerMirandaTokens();
// str
int  registerStrTokens();
// variables
int  registerVariablesTokens();
void unregisterVariablesTokens();
int  clearVariableRegister();
// logic
int  registerLogicTokens();
// math
int  registerMathTokens();
// metacontacts
int registerMetaContactsTokens();
// options
int  OptionsInit(WPARAM wParam, LPARAM lParam);
// reg exp
int  registerRegExpTokens();
// inet
int  registerInetTokens();
// xml
int  registerXsltTokens();
// help
INT_PTR  showHelpService(WPARAM wParam, LPARAM lParam);
INT_PTR  showHelpExService(WPARAM wParam, LPARAM lParam);
INT_PTR  getSkinItemService(WPARAM wParam, LPARAM lParam);
int  iconsChanged(WPARAM wParam, LPARAM lParam);

int ttoi(TCHAR *string);
TCHAR *itot(int num);

extern DWORD g_mirandaVersion;
