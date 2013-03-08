// Forex.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"

int hLangpack;
HINSTANCE g_hInstance = NULL;
HANDLE g_hEventWorkThreadStop;
int g_nStatus = ID_STATUS_OFFLINE;
HGENMENU g_hMenuEditSettings = NULL;
HGENMENU g_hMenuOpenLogFile = NULL;
#ifdef CHART_IMPLEMENT
HGENMENU g_hMenuChart = NULL;
#endif
HGENMENU g_hMenuRefresh = NULL;

namespace
{
	typedef std::vector<HANDLE> THandles;
	THandles g_ahEvents;
	THandles g_ahServices;
	THandles g_ahThreads;
	std::vector<HGENMENU> g_ahMenus;

	PLUGININFOEX Global_pluginInfo =
	{
		sizeof(PLUGININFOEX),
		__PLUGIN_NAME,
		PLUGIN_MAKE_VERSION(__MAJOR_VERSION, __MINOR_VERSION, __RELEASE_NUM, __BUILD_NUM),
		__DESCRIPTION,
		__AUTHOR,
		__AUTHOREMAIL,
		__COPYRIGHT,
		__AUTHORWEB,
		UNICODE_AWARE,
		// {E882056D-0D1D-4131-9A98-404CBAEA6A9C}
		{0xe882056d, 0xd1d, 0x4131, {0x9a, 0x98, 0x40, 0x4c, 0xba, 0xea, 0x6a, 0x9c}}
	};

	INT_PTR QuotesMenu_RefreshAll(WPARAM wp,LPARAM lp)
	{
		const CQuotesProviders::TQuotesProviders& apProviders = CModuleInfo::GetQuoteProvidersPtr()->GetProviders();
		std::for_each(apProviders.begin(),apProviders.end(),boost::bind(&IQuotesProvider::RefreshAll,_1));
		return 0;
	}

	void InitMenu()
	{
		CLISTMENUITEM mi = { sizeof(mi) };
		mi.ptszName = LPGENT("Quotes");
		mi.flags = CMIF_TCHAR|CMIF_ICONFROMICOLIB|CMIF_ROOTPOPUP;
		mi.icolibItem = Quotes_GetIconHandle(IDI_ICON_MAIN);
		HGENMENU hMenuRoot = Menu_AddMainMenuItem(&mi);
		g_ahMenus.push_back(hMenuRoot);

		mi.ptszName = LPGENT("Refresh All Quotes\\Rates");
		mi.flags = CMIF_TCHAR|CMIF_ICONFROMICOLIB|CMIF_ROOTHANDLE;
		//mi.position = 0x0FFFFFFF;
		mi.icolibItem = Quotes_GetIconHandle(IDI_ICON_MAIN);
		mi.pszService = "Quotes/RefreshAll";
		mi.hParentMenu = hMenuRoot;
		HGENMENU hMenu = Menu_AddMainMenuItem(&mi);
		g_ahMenus.push_back(hMenu);
		HANDLE h = CreateServiceFunction(mi.pszService, QuotesMenu_RefreshAll);
		g_ahServices.push_back(h);

		mi.ptszName = LPGENT("Currency Converter...");
		//mi.flags = CMIF_TCHAR|CMIF_ICONFROMICOLIB|CMIF_ROOTHANDLE;
		//mi.position = 0x0FFFFFFF;
		mi.icolibItem = Quotes_GetIconHandle(IDI_ICON_CURRENCY_CONVERTER);
		mi.pszService = "Quotes/CurrencyConverter";
		hMenu = Menu_AddMainMenuItem(&mi);
		g_ahMenus.push_back(hMenu);
		h = CreateServiceFunction(mi.pszService, QuotesMenu_CurrencyConverter);
		g_ahServices.push_back(h);

#ifdef TEST_IMPORT_EXPORT
		mi.ptszName = LPGENT("Export All Quotes");
		//mi.flags = CMIF_TCHAR|CMIF_ICONFROMICOLIB|CMIF_ROOTHANDLE;
		mi.icolibItem = Quotes_GetIconHandle(IDI_ICON_EXPORT);
		mi.pszService = "Quotes/ExportAll";
		hMenu = Menu_AddMainMenuItem(&mi);
		g_ahMenus.push_back(hMenu);
		h = CreateServiceFunction(mi.pszService, QuotesMenu_ExportAll);
		g_ahServices.push_back(h);

		mi.ptszName = LPGENT("Import All Quotes");
		//mi.flags = CMIF_TCHAR|CMIF_ICONFROMICOLIB|CMIF_ROOTHANDLE;
		mi.icolibItem = Quotes_GetIconHandle(IDI_ICON_IMPORT);
		mi.pszService = "Quotes/ImportAll";
		hMenu = Menu_AddMainMenuItem(&mi);
		g_ahMenus.push_back(hMenu);
		h = CreateServiceFunction(mi.pszService, QuotesMenu_ImportAll);
		g_ahServices.push_back(h);
#endif

		bool bSubGroups = 1 == ServiceExists(MS_CLIST_MENUBUILDSUBGROUP);

		h = HookEvent(ME_CLIST_PREBUILDCONTACTMENU,Quotes_PrebuildContactMenu);
		g_ahEvents.push_back(h);

		ZeroMemory(&mi,sizeof(mi));
		mi.cbSize = sizeof(mi);
		mi.pszContactOwner = QUOTES_PROTOCOL_NAME;
		hMenuRoot = NULL;
		if(bSubGroups)
		{
			mi.pszPopupName=(char *)-1;
			mi.icolibItem = Quotes_GetIconHandle(IDI_ICON_MAIN);
			mi.flags = CMIF_ICONFROMICOLIB|CMIF_TCHAR|CMIF_ROOTPOPUP;
			tstring sProtocolName = quotes_a2t(QUOTES_PROTOCOL_NAME);
			mi.ptszName = const_cast<TCHAR*>(sProtocolName.c_str());//A2T(QUOTES_PROTOCOL_NAME);
			mi.position = 0;

			hMenuRoot = Menu_AddContactMenuItem(&mi);
		}

		mi.flags = CMIF_TCHAR;
		if(bSubGroups)
		{
			mi.flags |= CMIF_CHILDPOPUP;
			mi.pszPopupName = (char*)hMenuRoot;
		}

		mi.ptszName = LPGENT("Refresh");
		mi.popupPosition = 0;
		mi.flags |= CMIF_ICONFROMICOLIB;
		mi.icolibItem =  Quotes_GetIconHandle(IDI_ICON_REFRESH);
		mi.pszService = "Quotes/RefreshContact";
		hMenu = Menu_AddContactMenuItem(&mi);
		g_hMenuRefresh = hMenu;
		g_ahMenus.push_back(hMenu);
		h = CreateServiceFunction(mi.pszService, QuotesMenu_RefreshContact);
		g_ahServices.push_back(h);

		mi.ptszName = LPGENT("Open Log File...");
		mi.popupPosition = 1;
		mi.icolibItem = NULL;
		mi.pszService = "Quotes/OpenLogFile";
		hMenu = Menu_AddContactMenuItem(&mi);
		g_hMenuOpenLogFile = hMenu;
		g_ahMenus.push_back(hMenu);
		h = CreateServiceFunction(mi.pszService, QuotesMenu_OpenLogFile);
		g_ahServices.push_back(h);

#ifdef CHART_IMPLEMENT
		mi.ptszName = LPGENT("Chart...");
		mi.popupPosition = 2;
		mi.icolibItem = NULL;
		mi.pszService = "Quotes/Chart";
		hMenu = Menu_AddContactMenuItem(&mi);
		g_hMenuChart = hMenu;
		g_ahMenus.push_back(hMenu);
		h = CreateServiceFunction(mi.pszService, QuotesMenu_Chart);
		g_ahServices.push_back(h);
#endif

		mi.ptszName = LPGENT("Edit Settings...");
#ifdef CHART_IMPLEMENT
		mi.popupPosition = 3;
#else
		mi.popupPosition = 2;
#endif
		mi.icolibItem = NULL;
		mi.pszService = "Quotes/EditSettings";
		hMenu = Menu_AddContactMenuItem(&mi);
		g_hMenuEditSettings = hMenu;
		g_ahMenus.push_back(hMenu);
		h = CreateServiceFunction(mi.pszService, QuotesMenu_EditSettings);
		g_ahServices.push_back(h);
	}


	int QuotesEventFunc_OnModulesLoaded(WPARAM, LPARAM)
	{
		CHTTPSession::Init();

		HANDLE h = HookEvent(ME_CLIST_EXTRA_IMAGE_APPLY,QuotesEventFunc_onExtraImageApply);
		g_ahEvents.push_back(h);

		g_hEventWorkThreadStop = ::CreateEvent(NULL,TRUE,FALSE,NULL);
		h = (ME_USERINFO_INITIALISE,QuotesEventFunc_OnUserInfoInit);
		g_ahEvents.push_back(h);

		h = HookEvent(ME_CLIST_DOUBLECLICKED,Quotes_OnContactDoubleClick);
		g_ahEvents.push_back(h);

		InitMenu();

		return 0;
	}

	int QuotesEventFunc_OnContactDeleted(WPARAM wParam, LPARAM)
	{
		HANDLE hContact = reinterpret_cast<HANDLE>(wParam);

		const CModuleInfo::TQuotesProvidersPtr& pProviders = CModuleInfo::GetQuoteProvidersPtr();
		CQuotesProviders::TQuotesProviderPtr pProvider = pProviders->GetContactProviderPtr(hContact);
		if(pProvider)
		{
			pProvider->DeleteContact(hContact);
		}

		return 0;
	}

	INT_PTR QuoteProtoFunc_GetStatus(WPARAM/* wp*/,LPARAM/* lp*/)
	{
		return g_nStatus;
	}

	void WaitForWorkingThreads()
	{
		size_t cThreads = g_ahThreads.size();
		if(cThreads > 0)
		{
			HANDLE* paHandles = &*(g_ahThreads.begin());
			::WaitForMultipleObjects((DWORD)cThreads,paHandles,TRUE,INFINITE);
		}
	}

	INT_PTR QuoteProtoFunc_SetStatus(WPARAM wp,LPARAM /*lp*/)
	{
		int nStatus = wp;
		if ((ID_STATUS_ONLINE == nStatus) || (ID_STATUS_OFFLINE == nStatus))
		{
			int nOldStatus = g_nStatus;
			if(nStatus != g_nStatus)
			{
				g_nStatus = nStatus;
				if ((ID_STATUS_ONLINE == nOldStatus) && (ID_STATUS_OFFLINE == g_nStatus))
				{
					BOOL b = ::SetEvent(g_hEventWorkThreadStop);
					assert(b);
				}
				else if ((ID_STATUS_ONLINE == g_nStatus) && (ID_STATUS_OFFLINE == nOldStatus))
				{
					BOOL b = ::ResetEvent(g_hEventWorkThreadStop);
					assert(b && "Failed to reset event");

					const CModuleInfo::TQuotesProvidersPtr& pProviders = CModuleInfo::GetQuoteProvidersPtr();
					const CQuotesProviders::TQuotesProviders& rapProviders = pProviders->GetProviders();
					for(CQuotesProviders::TQuotesProviders::const_iterator i = rapProviders.begin();i != rapProviders.end();++i)
					{
						const CQuotesProviders::TQuotesProviderPtr& pProvider = *i;
						HANDLE hThread = reinterpret_cast<HANDLE>(mir_forkthread(WorkingThread,pProvider.get()));
						g_ahThreads.push_back(hThread);
					}
				}

				ProtoBroadcastAck(QUOTES_PROTOCOL_NAME,NULL,ACKTYPE_STATUS,ACKRESULT_SUCCESS,reinterpret_cast<HANDLE>(nOldStatus),g_nStatus);
			}

		}

		return 0;
	}

	int QuotesEventFunc_PreShutdown(WPARAM wParam, LPARAM lParam)
	{
		QuoteProtoFunc_SetStatus(ID_STATUS_OFFLINE,0);
		//WindowList_Broadcast(g_hWindowListEditSettings,WM_CLOSE,0,0);
		CModuleInfo::GetInstance().OnMirandaShutdown();
		return 0;
	}

	INT_PTR QuoteProtoFunc_GetName(WPARAM wParam, LPARAM lParam)
	{
		if(lParam)
		{
			lstrcpynA(reinterpret_cast<char*>(lParam),QUOTES_PROTOCOL_NAME,wParam);
			return 0;
		}
		else
		{
			return 1;
		}
	}

	INT_PTR QuoteProtoFunc_GetCaps(WPARAM wp,LPARAM lp)
	{
		int ret = 0;
		switch(wp)
		{
		case PFLAGNUM_1:
			ret = PF1_PEER2PEER;
			break;
		case PFLAGNUM_3:
		case PFLAGNUM_2:
			ret = PF2_ONLINE|PF2_LONGAWAY;
			if(CModuleInfo::GetInstance().GetExtendedStatusFlag())
			{
				ret |= PF2_LIGHTDND;
			}
			break;
		}

		return ret;
	}

	INT_PTR QuoteProtoFunc_LoadIcon(WPARAM wp,LPARAM /*lp*/)
	{
		if ((wp & 0xffff) == PLI_PROTOCOL)
		{
			return reinterpret_cast<int>(::CopyIcon(Quotes_LoadIconEx(ICON_STR_MAIN)));
		}

		return 0;
	}

	int QuotesEventFunc_OptInitialise(WPARAM wp,LPARAM/* lp*/)
	{
		const CModuleInfo::TQuotesProvidersPtr& pProviders = CModuleInfo::GetQuoteProvidersPtr();
		const CQuotesProviders::TQuotesProviders& rapProviders = pProviders->GetProviders();

		OPTIONSDIALOGPAGE odp = { 0 };
		odp.cbSize = sizeof(odp);
		odp.position = 910000000;
		odp.hInstance = g_hInstance;
		odp.ptszTitle = _T(QUOTES_PROTOCOL_NAME);
		odp.ptszGroup = LPGENT("Network");
		odp.hIcon = Quotes_LoadIconEx(ICON_STR_MAIN);
		odp.flags = ODPF_USERINFOTAB | ODPF_TCHAR;

		std::for_each(rapProviders.begin(),rapProviders.end(),boost::bind(&IQuotesProvider::ShowPropertyPage,_1,wp,boost::ref(odp)));
		return 0;
	}

	inline int Quotes_DestroyServiceFunction(HANDLE h)
	{
		return DestroyServiceFunction(h);
	}

	inline int Quotes_UnhookEvent(HANDLE h)
	{
		return UnhookEvent(h);
	}

	inline int Quotes_RemoveMenuItem(HGENMENU h)
	{
		return CallService(MS_CLIST_REMOVECONTACTMENUITEM,reinterpret_cast<WPARAM>(h),0);
	}

// 	PROTO_INTERFACE* protoInit(const char* pszProtoName, const TCHAR* tszUserName)
// 	{
// 		CAimProto *ppro = new CAimProto(pszProtoName, tszUserName);
// 		g_Instances.insert(ppro);
// 		return ppro;
// 	}
//
// 	int protoUninit(PROTO_INTERFACE* ppro)
// 	{
// 		g_Instances.remove((CAimProto*)ppro);
// 		return 0;
// 	}

}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	g_hInstance = hinstDLL;
	return TRUE;
}

extern "C"
{
	__declspec(dllexport) PLUGININFOEX* MirandaPluginInfoEx(DWORD mirandaVersion)
	{
		return &Global_pluginInfo;
	}

	int __declspec(dllexport) Load(void)
	{

		mir_getLP(&Global_pluginInfo);
 
		if(false == CModuleInfo::Verify())
		{
			return 1;
		}

		Quotes_IconsInit();

		PROTOCOLDESCRIPTOR pd = { PROTOCOLDESCRIPTOR_V3_SIZE };
		pd.szName = QUOTES_PROTOCOL_NAME;
		pd.type = PROTOTYPE_VIRTUAL;
		CallService( MS_PROTO_REGISTERMODULE, 0, ( LPARAM )&pd );

		HANDLE h = CreateProtoServiceFunction(QUOTES_PROTOCOL_NAME, PS_GETNAME, QuoteProtoFunc_GetName);
		g_ahServices.push_back(h);
		h = CreateProtoServiceFunction(QUOTES_PROTOCOL_NAME, PS_GETCAPS, QuoteProtoFunc_GetCaps);
		g_ahServices.push_back(h);
		h = CreateProtoServiceFunction(QUOTES_PROTOCOL_NAME, PS_SETSTATUS, QuoteProtoFunc_SetStatus);
		g_ahServices.push_back(h);
		h = CreateProtoServiceFunction(QUOTES_PROTOCOL_NAME, PS_GETSTATUS, QuoteProtoFunc_GetStatus);
		g_ahServices.push_back(h);
		h = CreateProtoServiceFunction(QUOTES_PROTOCOL_NAME, PS_LOADICON, QuoteProtoFunc_LoadIcon);
		g_ahServices.push_back(h);

		h = HookEvent(ME_SYSTEM_MODULESLOADED,QuotesEventFunc_OnModulesLoaded);
		g_ahEvents.push_back(h);
		h = HookEvent(ME_DB_CONTACT_DELETED,QuotesEventFunc_OnContactDeleted);
		g_ahEvents.push_back(h);
		h = HookEvent(ME_SYSTEM_PRESHUTDOWN,QuotesEventFunc_PreShutdown);
		g_ahEvents.push_back(h);
		h = HookEvent(ME_OPT_INITIALISE,QuotesEventFunc_OptInitialise);
		g_ahEvents.push_back(h);

		h = CreateServiceFunction(MS_QUOTES_EXPORT, Quotes_Export);
		g_ahServices.push_back(h);
		h = CreateServiceFunction(MS_QUOTES_IMPORT, Quotes_Import);
		g_ahServices.push_back(h);

		return 0;
	}

	__declspec(dllexport) int Unload(void)
	{
		std::for_each(g_ahServices.begin(),g_ahServices.end(),boost::bind(Quotes_DestroyServiceFunction,_1));
		std::for_each(g_ahEvents.begin(),g_ahEvents.end(),boost::bind(Quotes_UnhookEvent,_1));
		std::for_each(g_ahMenus.begin(),g_ahMenus.end(),boost::bind(Quotes_RemoveMenuItem,_1));

		WaitForWorkingThreads();

		::CloseHandle(g_hEventWorkThreadStop);

		return 0;
	}
}
