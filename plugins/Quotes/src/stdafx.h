// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include <windows.h>
#include <mshtml.h>
#include <comdef.h>
#include <commctrl.h>
#include <ShellAPI.h>
#include <sys/stat.h>
#include <CommDlg.h>
#include <fstream>
#include <msapi/comptr.h>

#include <newpluginapi.h>
#include <m_database.h>
#include <win2k.h>
#include <m_xml.h>
#include <m_clist.h>
#include <m_langpack.h>
#include <m_options.h>
#include <m_protosvc.h>
#include <m_extraicons.h>
#include <m_icolib.h>
#include <m_genmenu.h>
#include <m_netlib.h>
#include <m_popup.h>
#include <m_userinfo.h>

#include <m_variables.h>
#include <m_Quotes.h>
#include <m_toptoolbar.h>

#include <boost\bind.hpp>
#include <boost\scoped_ptr.hpp>
#include <boost\foreach.hpp>
#include <boost\date_time\posix_time\posix_time.hpp>
#include <boost\date_time\c_local_time_adjustor.hpp>

typedef std::wstring tstring;
typedef std::wostringstream tostringstream;
typedef std::wistringstream tistringstream;
typedef std::wofstream tofstream;
typedef std::wifstream tifstream;
typedef std::wostream tostream;
typedef std::wistream tistream;
typedef boost::posix_time::wtime_input_facet ttime_input_facet;
typedef boost::posix_time::wtime_facet ttime_facet;

inline std::string quotes_t2a(const wchar_t* t)
{
	std::string s;
	char* p = mir_u2a(t);
	if (p) {
		s = p;
		mir_free(p);
	}
	return s;
}

inline tstring quotes_a2t(const char* s)
{
	tstring t;
	wchar_t* p = mir_a2u(s);
	if (p) {
		t = p;
		mir_free(p);
	}
	return t;
}

#include "resource.h"
#include "version.h"
#include "IconLib.h"
#include "QuoteInfoDlg.h"
#include "ModuleInfo.h"
#include "DBUtils.h"
#include "HTTPSession.h"
#include "CurrencyConverter.h"
#include "WinCtrlHelper.h"
#include "ImportExport.h"
#include "ComHelper.h"
#include "Log.h"
#include "CommonOptionDlg.h"
#include "EconomicRateInfo.h"
#include "SettingsDlg.h"
#include "CreateFilePath.h"
#include "Locale.h"
#include "ExtraImages.h"
#include "IsWithinAccuracy.h"
#include "OptionDukasCopy.h"
#include "IQuotesProvider.h"
#include "QuotesProviders.h"
#include "QuotesProviderBase.h"
#include "QuotesProviderFinance.h"
#include "QuotesProviderGoogle.h"
#include "QuotesProviderYahoo.h"
#include "QuotesProviderDukasCopy.h"
#include "QuotesProviderGoogleFinance.h"
#include "QuotesProviderVisitor.h"
#include "QuotesProviderVisitorDbSettings.h"
#include "QuotesProviderVisitorFormater.h"
#include "QuotesProviderVisitorTendency.h"
#include "QuotesProviderVisitorFormatSpecificator.h"
#ifdef CHART_IMPLEMENT
#include "QuoteChart.h"
#include "Chart.h"
#endif
#include "IHTMLParser.h"
#include "IHTMLEngine.h"
#include "HTMLParserMS.h"
#include "IXMLEngine.h"
#include "XMLEngineMI.h"

extern HINSTANCE g_hInstance;
