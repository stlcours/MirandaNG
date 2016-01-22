enum EXTERNAL_FUNCTIONS
{
	DISPID_EXTERNAL_CALLSERVICE = 600,

	DISPID_EXTERNAL_SET_CONTEXTMENUHANDLER = 630,
	DISPID_EXTERNAL_GET_CURRENTCONTACT,

	DISPID_EXTERNAL_DB_GET = 652,
	DISPID_EXTERNAL_DB_SET,

	DISPID_EXTERNAL_WIN32_SHELL_EXECUTE = 660
};

namespace External
{
	HRESULT mir_CallService(DISPPARAMS *pDispParams, VARIANT *pVarResult);

	HRESULT IEView_SetContextMenuHandler(IEView *self, DISPPARAMS *pDispParams, VARIANT *pVarResult);
	HRESULT IEView_GetCurrentContact(IEView *self, DISPPARAMS *pDispParams, VARIANT *pVarResult);

	HRESULT db_get(DISPPARAMS *pDispParams, VARIANT *pVarResult);
	HRESULT db_set(DISPPARAMS *pDispParams, VARIANT *pVarResult);

	HRESULT win32_ShellExecute(DISPPARAMS *pDispParams, VARIANT *pVarResult);
}