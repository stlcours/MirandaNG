// Copyright Scott Ellis (mail@scottellis.com.au) 2005
// This software is licenced under the GPL (General Public Licence)
// available at http://www.gnu.org/copyleft/gpl.html

#ifndef _FONT_SERVICE_API_INC
#define _FONT_SERVICE_API_INC

//////////////////////////////////////////////////////////////////////////
//
//  FONTS
//

// style flags
#define DBFONTF_BOLD       1
#define DBFONTF_ITALIC     2
#define DBFONTF_UNDERLINE  4
#define DBFONTF_STRIKEOUT  8

// flags for compatibility
#define FIDF_APPENDNAME       0x0001  // append 'Name' to the setting used to store font face (as CLC settings require)
#define FIDF_NOAS             0x0002  // disable the <prefix>As setting to prevent 'same as' behaviour
#define FIDF_SAVEACTUALHEIGHT 0x0004  // write the actual height of a test string to the db
#define FIDF_SAVEPOINTSIZE    0x0008  // write the font point size to the db

// additional flags
#define FIDF_DEFAULTVALID		0x0020  // the default font settings are valid - else, just use generic default
#define FIDF_NEEDRESTART		0x0040  // setting changes will not take effect until miranda is restarted
#define FIDF_ALLOWREREGISTER	0x0080  // allow plugins to register this font again (i.e. override already registered settings such as flags)
#define FIDF_ALLOWEFFECTS		0x0100  // allow setting of font effects (i.e. underline and strikeout)
#define FIDF_DISABLESTYLES		0x0200  // don't allow to select font attributes (bold/underline/italics)
                                      // FIDF_ALLOWEFFECTS has priority and will override this flag!

// font class
#define FIDF_CLASSMASK			0x70000000
#define FIDF_CLASSHEADER		0x10000000
#define FIDF_CLASSGENERAL		0x20000000
#define FIDF_CLASSSMALL			0x30000000

// settings to be used for the value of 'deffontsettings' in the FontID structure below - i.e. defaults

struct FontSettings
{
	COLORREF colour;
	char     size;
	BYTE     style;					// see the DBFONTF_* flags above
	BYTE     charset;
	char     szFace[LF_FACESIZE];
};

struct FontSettingsW
{
	COLORREF colour;
	char     size;
	BYTE     style;					// see the DBFONTF_* flags above
	BYTE     charset;
	wchar_t  szFace[LF_FACESIZE];
};

// a font identifier structure - used for registering a font, and getting one out again

// WARNING: do not use Translate(TS) for name or group as they
// are translated by the core, which may lead to double translation.
// Use LPGEN instead which are just dummy wrappers/markers for "lpgen.pl".

struct FontID
{
	int   cbSize;
	char  group[64];               // [TRANSLATED-BY-CORE] group the font belongs to - this is the 'Font Group' list in the options page
	char  name[64];                // [TRANSLATED-BY-CORE] this is the name of the font setting - e.g. 'contacts' in the 'contact list' group
	char  dbSettingsGroup[32];     // the 'module' in the database where the font data is stored
	char  prefix[32];              // this is prepended to the settings used to store this font's data in the db
	DWORD flags;                   // bitwise OR of the FIDF_* flags above
	FontSettings deffontsettings;  // defaults, valid if flags & FIDF_DEFAULTVALID
	int   order;                   // controls the order in the font group in which the fonts are listed in the UI (if order fields are equal,
											 // they will be ordered alphabetically by name)
	char  backgroundGroup[64];
	char  backgroundName[64];
};

struct FontIDW
{
	int     cbSize;
	wchar_t group[64];             // [TRANSLATED-BY-CORE] group the font belongs to - this is the 'Font Group' list in the options page
	wchar_t name[64];              // [TRANSLATED-BY-CORE] this is the name of the font setting - e.g. 'contacts' in the 'contact list' group
	char    dbSettingsGroup[32];   // the 'module' in the database where the font data is stored
	char    prefix[32];            // this is prepended to the settings used to store this font's data in the db
	DWORD   flags;                 // bitwise OR of the FIDF_* flags above
	FontSettingsW deffontsettings; // defaults, valid if flags & FIDF_DEFAULTVALID
	int     order;                 // controls the order in the font group in which the fonts are listed in the UI (if order fields are equal,
											 // they will be ordered alphabetically by name)
	wchar_t backgroundGroup[64];
	wchar_t backgroundName[64];
};

// register a font
extern int hLangpack;

EXTERN_C MIR_APP_DLL(int) Font_Register(FontID *pFont, int = hLangpack);
EXTERN_C MIR_APP_DLL(int) Font_RegisterW(FontIDW *pFont, int = hLangpack);

// get a font
// will fill the logfont structure passed in with the user's choices, or the default if it was set and the user has not chosen a font yet,
// or the global default font settings otherwise (i.e. no user choice and default set, or font not registered)
// global default font is gotten using SPI_GETICONTITLELOGFONT, color COLOR_WINDOWTEXT, size 8.
// returns the font's colour

EXTERN_C MIR_APP_DLL(COLORREF) Font_Get(const char *szGroup, const char *szName, LOGFONTA *pFont);
EXTERN_C MIR_APP_DLL(COLORREF) Font_GetW(const wchar_t *wszGroup, const wchar_t *wszName, LOGFONTW *pFont);

__forceinline COLORREF Font_Get(FontID &p, LOGFONTA *pFont)
{	return Font_Get(p.group, p.name, pFont);
}
__forceinline COLORREF Font_GetW(FontIDW &p, LOGFONTW *pFont)
{	return Font_GetW(p.group, p.name, pFont);
}

// fired when a user modifies font settings, so reload your fonts
// wparam = lparam = 0
#define ME_FONT_RELOAD        "Font/Reload"

// service for reload fonts
#define MS_FONT_RELOAD		"Font/ReloadSvc"

//////////////////////////////////////////////////////////////////////////
//
//  COLOURS
//

// WARNING: do not use Translate(TS) for name or group as they
// are translated by the core, which may lead to double translation.
// Use LPGEN instead which are just dummy wrappers/markers for "lpgen.pl".

struct ColourID
{
	int      cbSize;
	char     group[64];	// [TRANSLATED-BY-CORE]
	char     name[64];	// [TRANSLATED-BY-CORE]
	char     dbSettingsGroup[32];
	char     setting[32];
	DWORD    flags;		// not used
	COLORREF defcolour; // default value
	int      order;
};

// a font identifier structure - used for registering a font, and getting one out again
struct ColourIDW
{
	int      cbSize;
	wchar_t  group[64];	// [TRANSLATED-BY-CORE]
	wchar_t  name[64];	// [TRANSLATED-BY-CORE]
	char     dbSettingsGroup[32];
	char     setting[32];
	DWORD    flags;      // not used
	COLORREF defcolour;  // default value
	int      order;
};

// register a colour (this should be used for everything except actual text colour for registered fonts)
// [note - a colour with name 'Background' [translated!] has special meaning and will be used as the background colour of
// the font list box in the options, for the given group]

EXTERN_C MIR_APP_DLL(int) Colour_Register(ColourID *pFont, int = hLangpack);
EXTERN_C MIR_APP_DLL(int) Colour_RegisterW(ColourIDW *pFont, int = hLangpack);

// get a colour
EXTERN_C MIR_APP_DLL(COLORREF) Colour_Get(const char *szGroup, const char *szName);
EXTERN_C MIR_APP_DLL(COLORREF) Colour_GetW(const wchar_t *wszGroup, const wchar_t *wszName);

__forceinline COLORREF Colour_GetW(ColourIDW &p)
{	return Colour_GetW(p.group, p.name);
}

// fired when a user modifies font settings, so reget your fonts and colours
// wparam = lparam = 0
#define ME_COLOUR_RELOAD      "Colour/Reload"

// service for colors reload
#define MS_COLOUR_RELOAD      "Colour/ReloadSvc"

//////////////////////////////////////////////////////////////////////////
//
//  EFFECTS
//

struct FONTEFFECT
{
	BYTE     effectIndex;
	DWORD    baseColour;        // ARGB
	DWORD    secondaryColour;   // ARGB
};

struct EffectID
{
	int        cbSize;
	char       group[64];
	char       name[64];
	char       dbSettingsGroup[32];
	char       setting[32];
	DWORD      flags;
	FONTEFFECT defeffect;
	int        order;
	FONTEFFECT value;
};

struct EffectIDW
{
	int        cbSize;
	wchar_t    group[64];
	wchar_t    name[64];
	char       dbSettingsGroup[32];
	char       setting[32];
	DWORD      flags;
	FONTEFFECT defeffect;
	int        order;
	FONTEFFECT value;
};

// register an effect
// wparam = (EffectID *)&effect_id
// lparam = 0

EXTERN_C MIR_APP_DLL(int) Effect_Register(EffectID *pEffect, int = hLangpack);
EXTERN_C MIR_APP_DLL(int) Effect_RegisterW(EffectIDW *pEffect, int = hLangpack);

// get a effect
// wparam = (EffectID *)&effect_id (only name and group matter)
// lparam = (FONTEFFECT *)&effect
// rerturns 0, or -1 if not found

EXTERN_C MIR_APP_DLL(int) Effect_Get(const char *szGroup, const char *wszName, FONTEFFECT *pEffect);
EXTERN_C MIR_APP_DLL(int) Effect_GetW(const wchar_t *wszGroup, const wchar_t *szName, FONTEFFECT *pEffect);

// fired when a user modifies font settings, so reget your fonts and colours
// wparam = lparam = 0
#define ME_EFFECT_RELOAD      "Effect/Reload"

EXTERN_C MIR_APP_DLL(void) KillModuleFonts(int hLangpack);
EXTERN_C MIR_APP_DLL(void) KillModuleColours(int hLangpack);
EXTERN_C MIR_APP_DLL(void) KillModuleEffects(int hLangpack);

#endif // _FONT_SERVICE_API_INC
