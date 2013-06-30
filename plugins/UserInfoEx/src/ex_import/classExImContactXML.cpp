/*
UserinfoEx plugin for Miranda IM

Copyright:
� 2006-2010 DeathAxe, Yasnovidyashii, Merlin, K. Romanov, Kreol

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include "..\commonheaders.h"

/***********************************************************************************************************
 * common stuff
 ***********************************************************************************************************/

/**
 * name:	SortProc
 * desc:	used for bsearch in CExImContactXML::IsContactInfo
 * param:	item1	- item to compare
 *			item2	- item to compare
 * return:	the difference
 **/
static int SortProc(const LPDWORD item1, const LPDWORD item2)
{
	return *item1 - *item2;
}

/**
 * name:	CExImContactXML
 * class:	CExImContactXML
 * desc:	the constructor for the contact class
 * param:	pXmlFile	- the owning xml file
 * return:	nothing
 **/
CExImContactXML::CExImContactXML(CFileXml * pXmlFile)
	: CExImContactBase()
{
	_xmlNode	= NULL;
	_pXmlFile	= pXmlFile;
	_hEvent		= NULL;
}

/**
 * name:	IsContactInfo
 * class:	CExImContactXML
 * desc:	this function compares the given setting key to the list of known contact
 *			information keys
 * param:	pszKey	- the settings key to check
 * return:	TRUE if pszKey is a valid contact information
 **/
BYTE CExImContactXML::IsContactInfo(LPCSTR pszKey)
{
	// This is a sorted list of all hashvalues of the contact information.
	// This is the same as the szCiKey[] array below but sorted
	const DWORD dwCiHash[] = {
		0x6576F145,0x65780A70,0x6719120C,0x6776F145,0x67780A70,0x6EDB33D7,0x6F0466B5,
		0x739B6915,0x73B11E48,0x760D8AD5,0x786A70D0,0x8813C350,0x88641AF8,0x8ED5652D,
		0x96D64541,0x97768A14,0x9B786F9C,0x9B7889F9,0x9C26E6ED,0xA6675748,0xA813C350,
		0xA8641AF8,0xAC408FCC,0xAC40AFCC,0xAC40CFCC,0xAEC6EA4C,0xB813C350,0xB8641AF8,
		0xC5227954,0xCC68DE0E,0xCCD62E70,0xCCFBAAF4,0xCD715E13,0xD36182CF,0xD361C2CF,
		0xD361E2CF,0xD42638DE,0xD4263956,0xD426395E,0xD453466E,0xD778D233,0xDB59D87A,
		0xE406F60E,0xE406FA0E,0xE406FA4E,0xECF7E910,0xEF660441,0x00331041,0x0039AB3A,
		0x003D88A6,0x07ABA803,0x113D8227,0x113DC227,0x113DE227,0x2288784F,0x238643D6,
		0x2671C03E,0x275F720B,0x2EBDC0D6,0x3075C8C5,0x32674C9F,0x33EEAE73,0x40239C1C,
		0x44DB75D0,0x44FA69D0,0x4C76989B,0x4FF38979,0x544B2F44,0x55AFAF8C,0x567A6BC5,
		0x5A96C47F,0x6376F145,0x63780A70
	};
	if (pszKey && *pszKey) {
		char buf[MAXSETTING];
		// convert to hash and make bsearch as it is much faster then working with strings
		const DWORD dwHash = hashSetting(_strlwr(mir_strncpy(buf, pszKey, SIZEOF(buf))));
		return bsearch(&dwHash, dwCiHash, SIZEOF(dwCiHash), sizeof(dwCiHash[0]), (int (*)(const void*, const void*))SortProc) != NULL;
	}
	return FALSE;
/*
	WORD i;
	const LPCSTR szCiKey[] = {
		// naming
		SET_CONTACT_TITLE,SET_CONTACT_FIRSTNAME,SET_CONTACT_SECONDNAME,SET_CONTACT_LASTNAME,SET_CONTACT_PREFIX,
		// private address
		SET_CONTACT_STREET,SET_CONTACT_ZIP,SET_CONTACT_CITY,SET_CONTACT_STATE,SET_CONTACT_COUNTRY,
		SET_CONTACT_PHONE,SET_CONTACT_FAX,SET_CONTACT_CELLULAR,
		SET_CONTACT_EMAIL,SET_CONTACT_EMAIL0,SET_CONTACT_EMAIL1,SET_CONTACT_HOMEPAGE,
		// origin
		SET_CONTACT_ORIGIN_STREET,SET_CONTACT_ORIGIN_ZIP,SET_CONTACT_ORIGIN_CITY,SET_CONTACT_ORIGIN_STATE,SET_CONTACT_ORIGIN_COUNTRY,
		// company
		SET_CONTACT_COMPANY_POSITION,SET_CONTACT_COMPANY_OCCUPATION,SET_CONTACT_COMPANY_SUPERIOR,SET_CONTACT_COMPANY_ASSISTENT
		SET_CONTACT_COMPANY,SET_CONTACT_COMPANY_DEPARTMENT,SET_CONTACT_COMPANY_OFFICE,
		SET_CONTACT_COMPANY_STREET,SET_CONTACT_COMPANY_ZIP,SET_CONTACT_COMPANY_CITY,SET_CONTACT_COMPANY_STATE,SET_CONTACT_COMPANY_COUNTRY,
		SET_CONTACT_COMPANY_PHONE,SET_CONTACT_COMPANY_FAX,SET_CONTACT_COMPANY_CELLULAR,
		SET_CONTACT_COMPANY_EMAIL,SET_CONTACT_COMPANY_EMAIL0,SET_CONTACT_COMPANY_EMAIL1,SET_CONTACT_COMPANY_HOMEPAGE,
		// personal information
		SET_CONTACT_ABOUT,SET_CONTACT_MYNOTES,SET_CONTACT_MARITAL,SET_CONTACT_PARTNER,
		SET_CONTACT_LANG1,SET_CONTACT_LANG2,SET_CONTACT_LANG3,SET_CONTACT_TIMEZONE,SET_CONTACT_TIMEZONENAME,SET_CONTACT_TIMEZONEINDEX,
		SET_CONTACT_AGE,SET_CONTACT_GENDER,SET_CONTACT_BIRTHDAY,SET_CONTACT_BIRTHMONTH,SET_CONTACT_BIRTHYEAR,
		"Past0", "Past0Text","Past1", "Past1Text","Past2", "Past2Text",
		"Affiliation0", "Affiliation0Text","Affiliation1", "Affiliation1Text","Affiliation2", "Affiliation2Text",
		"Interest0Cat", "Interest0Text","Interest1Cat", "Interest1Text","Interest2Cat", "Interest2Text"
	};
	DWORD *hash = new DWORD[SIZEOF(szCiKey)];
	char buf[MAX_PATH];

	for (i = 0; i < SIZEOF(szCiKey); i++) {
		strcpy(buf, szCiKey[i]);
		hash[i] = hashSetting(_strlwr((char*)buf));
	}
	qsort(hash, SIZEOF(szCiKey), sizeof(hash[0]), 
	(int (*)(const void*, const void*))SortProc);
	
	FILE* fil = fopen("D:\\temp\\id.txt", "wt");
	for (i = 0; i < SIZEOF(szCiKey); i++) {
		fprintf(fil, "0x%08X,", hash[i]);
	}
	fclose(fil);
	return FALSE;
	*/
}

/***********************************************************************************************************
 * exporting stuff
 ***********************************************************************************************************/

/**
 * name:	CreateXmlNode
 * class:	CExImContactXML
 * desc:	creates a new TiXmlElement representing the contact
 *			whose information are stored in this class
 * param:	none
 * return:	pointer to the newly created TiXmlElement
 **/
TiXmlElement* CExImContactXML::CreateXmlElement()
{
	if (_hContact) {
		if (_pszProto) {
			_xmlNode = new TiXmlElement(XKEY_CONTACT);
			
			if (_xmlNode) {
				LPSTR pszUID = uid2String(TRUE);
				_xmlNode->SetAttribute("ampro", _pszAMPro);
				_xmlNode->SetAttribute("proto", _pszProto);

				if (_pszDisp)  _xmlNode->SetAttribute("disp", _pszDisp);
				if (_pszNick)  _xmlNode->SetAttribute("nick", _pszNick);
				if (_pszGroup) _xmlNode->SetAttribute("group",_pszGroup);
				
				if (pszUID) {

					if (_pszUIDKey) {
						_xmlNode->SetAttribute("uidk", _pszUIDKey);
						_xmlNode->SetAttribute("uidv", pszUID);
					}
					else {
						_xmlNode->SetAttribute("uidk", "#NV");
						_xmlNode->SetAttribute("uidv", "UNLOADED");
					}
					mir_free(pszUID);
				}
			}
		}
		else
			_xmlNode = NULL;
	}
	else {
		_xmlNode = new TiXmlElement(XKEY_OWNER);
	}
	return _xmlNode;
}

/**
 * name:	ExportContact
 * class:	CExImContactXML
 * desc:	exports a contact
 * param:	none
 * return:	ERROR_OK on success or any other on failure
 **/
int CExImContactXML::ExportContact(DB::CEnumList* pModules)
{
	if (_pXmlFile->_wExport & EXPORT_DATA) {
		if (pModules) {
			int i;
			LPSTR p;

			for (i = 0; i < pModules->getCount(); i++) {
				p = (*pModules)[i];
				ExportModule(p);
			}
		}
		else {
			ExportModule(USERINFO);
			ExportModule(MOD_MBIRTHDAY);
		}
	}
	
	// export contact's events
	if (_pXmlFile->_wExport & EXPORT_HISTORY)
		ExportEvents();

	return ERROR_OK;
}

/**
 * name:	ExportSubContact
 * class:	CExImContactXML
 * desc:	exports a meta sub contact
 * param:	none
 * return:	ERROR_OK on success or any other on failure
 **/
int CExImContactXML::ExportSubContact(CExImContactXML *vMetaContact, DB::CEnumList* pModules)
{
	// create xmlNode
	if (!CreateXmlElement()) 
	{
		return ERROR_INVALID_CONTACT;
	}
	if (ExportContact(pModules) == ERROR_OK) 
	{
		if (!_xmlNode->NoChildren() && vMetaContact->_xmlNode->LinkEndChild(_xmlNode)) 
		{
			return ERROR_OK;
		}
	}
	if (_xmlNode) delete _xmlNode;
	return ERROR_NOT_ADDED;
}

/**
 * name:	Export
 * class:	CExImContactXML
 * desc:	exports a contact
 * param:	xmlfile		- handle to the open file to write the contact to
 *			pModules	- list of modules to export for each contact
 * return:	ERROR_OK on success or any other on failure
 **/
int CExImContactXML::Export(FILE *xmlfile, DB::CEnumList* pModules)
{
	if (!xmlfile) 
	{
		return ERROR_INVALID_PARAMS;
	}

	if (_hContact == INVALID_HANDLE_VALUE)
	{
		return ERROR_INVALID_CONTACT;
	}

	if (!CreateXmlElement()) 
	{
		return ERROR_INVALID_CONTACT;
	}

	// export meta
	if (isMeta())
	{
		CExImContactXML vContact(_pXmlFile);

		const int cnt = DB::MetaContact::SubCount(_hContact);
		const int def = DB::MetaContact::SubDefNum(_hContact);
		HANDLE hSubContact = DB::MetaContact::Sub(_hContact, def);
		int i;

		// export default subcontact
		if (hSubContact && vContact.fromDB(hSubContact))
		{
			vContact.ExportSubContact(this, pModules);
		}

		for (i = 0; i < cnt; i++)
		{
			if (i != def)
			{
				hSubContact = DB::MetaContact::Sub(_hContact, i);
				if (hSubContact && vContact.fromDB(hSubContact))
				{
					vContact.ExportSubContact(this, pModules);
				}
			}
		}
	}
	ExportContact(pModules);

	// add xContact to document
	if (_xmlNode->NoChildren())
	{
		delete _xmlNode;
		_xmlNode = NULL;
		return ERROR_NOT_ADDED;
	}
	_xmlNode->Print(xmlfile, 1);
	fputc('\n', xmlfile);

	delete _xmlNode;
	_xmlNode = NULL;

	return ERROR_OK;
}

/**
 * name:	ExportModule
 * class:	CExImContactXML
 * desc:	enumerates all settings of a database module and adds them to the xml tree
 * params:	pszModule	- the module which is to export
 * return:	ERROR_OK on success or any other on failure
 **/
int CExImContactXML::ExportModule(LPCSTR pszModule)
{
	DB::CEnumList	Settings;
	if (!pszModule || !*pszModule) {
		return ERROR_INVALID_PARAMS;
	}
	if (!Settings.EnumSettings(_hContact, pszModule)) {
		int i;
		TiXmlElement *xmod;
		xmod = new TiXmlElement(XKEY_MOD);
		if (!xmod) {
			return ERROR_MEMORY_ALLOC;
		}
		xmod->SetAttribute("key", pszModule);
		for (i = 0; i < Settings.getCount(); i++) {
			ExportSetting(xmod, pszModule, Settings[i]);
		}

		if (!xmod->NoChildren() && _xmlNode->LinkEndChild(xmod)) {
			return ERROR_OK;
		}
		delete xmod;
	}
	return ERROR_EMPTY_MODULE;
}

/**
 * name:	ExportSetting
 * desc:	read a setting from database and add an xmlelement to contact node
 * params:	xmlModule	- xml node to add the setting to
 *			hContact	- handle of the contact whose event chain is to export
 *			pszModule	- the module which is to export
 *			pszSetting	- the setting which is to export
 * return:	pointer to the added element
 **/
int CExImContactXML::ExportSetting(TiXmlElement *xmlModule, LPCSTR pszModule, LPCSTR pszSetting)
{
	DBVARIANT		dbv;
	TiXmlElement	*xmlEntry	= NULL;
	TiXmlText		*xmlValue	= NULL;
	CHAR			buf[32];
	LPSTR			str			= NULL;

	if (DB::Setting::GetAsIs(_hContact, pszModule, pszSetting, &dbv))
		return ERROR_INVALID_VALUE;
	switch (dbv.type) {
		case DBVT_BYTE:		//'b' bVal and cVal are valid
			buf[0] = 'b';
			_ultoa(dbv.bVal, buf + 1, 10);
			xmlValue = new TiXmlText(buf);
			break;
		case DBVT_WORD:		//'w' wVal and sVal are valid
			buf[0] = 'w';
			_ultoa(dbv.wVal, buf + 1, 10);
			xmlValue = new TiXmlText(buf);
			break;
		case DBVT_DWORD:	//'d' dVal and lVal are valid
			buf[0] = 'd';
			_ultoa(dbv.dVal, buf + 1, 10);
			xmlValue = new TiXmlText(buf);
			break;
		case DBVT_ASCIIZ:	//'s' pszVal is valid
		{
			if (mir_IsEmptyA(dbv.pszVal)) break;
			DB::Variant::ConvertString(&dbv, DBVT_UTF8);
			if (str = (LPSTR)mir_alloc(mir_strlen(dbv.pszVal) + 2)) {
				str[0] = 's';
				mir_strcpy(&str[1], dbv.pszVal);
				xmlValue = new TiXmlText(str);
				mir_free(str);
			}
			break;
		}
		case DBVT_UTF8:		//'u' pszVal is valid
		{
			if (mir_IsEmptyA(dbv.pszVal)) break;
			if (str = (LPSTR)mir_alloc(mir_strlen(dbv.pszVal) + 2)) {
				str[0] = 'u';
				mir_strcpy(&str[1], dbv.pszVal);
				xmlValue = new TiXmlText(str);
				mir_free(str);
			}
			break;
		}
		case DBVT_WCHAR:	//'u' pwszVal is valid
		{
			if (mir_IsEmptyW(dbv.pwszVal)) break;
			DB::Variant::ConvertString(&dbv, DBVT_UTF8);
			if (str = (LPSTR)mir_alloc(mir_strlen(dbv.pszVal) + 2)) {
				str[0] = 'u';
				mir_strcpy(&str[1], dbv.pszVal);
				xmlValue = new TiXmlText(str);
				mir_free(str);
			}
			break;
		}
		case DBVT_BLOB:		//'n' cpbVal and pbVal are valid
		{
			// new buffer for base64 encoded data
			INT_PTR baselen = Base64EncodeGetRequiredLength(dbv.cpbVal, BASE64_FLAG_NOCRLF);
			str = (LPSTR)mir_alloc(baselen + 6);
			assert(str != NULL);
			// encode data
			if (Base64Encode(dbv.pbVal, dbv.cpbVal, str+1, &baselen, BASE64_FLAG_NOCRLF)) {
				if (baselen){
					str[baselen+1] = 0;
					str[0] = 'n';
					xmlValue = new TiXmlText(str);
				}
			}
			mir_free(str);
			break;
		}
		case DBVT_DELETED:	//this setting just got deleted, no other values are valid
			#if defined(_DEBUG)
				OutputDebugStringA("DBVT_DELETED\n");
			#endif
			break;
		default:
			#if defined(_DEBUG)
				OutputDebugStringA("DBVT_TYPE unknown\n");
			#endif
			; // nothing
	}
	db_free(&dbv);
	if (xmlValue) {
		xmlEntry = new TiXmlElement(XKEY_SET);
		if (xmlEntry) {
			xmlEntry->SetAttribute("key", pszSetting);
			if (xmlEntry->LinkEndChild(xmlValue) && xmlModule->LinkEndChild(xmlEntry))
				return ERROR_OK;
			delete xmlEntry;
		}
		delete xmlValue;
	}
	return ERROR_MEMORY_ALLOC;
}

/**
 * name:	ExportEvents
 * desc:	adds the event chain for a given contact to the xml tree
 * params:	xContact	- the xml node to add the events as childs to
 *			hContact	- handle of the contact whose event chain is to export
 * return:	TRUE on success, FALSE otherwise
 **/
BYTE CExImContactXML::ExportEvents()
{
	PBYTE		pbEventBuf			= NULL;
	DWORD		cbEventBuf			= 0,
				dwNumEventsAdded	= 0;
	LPSTR		pBase64Data			= NULL;

	int dwNumEvents = db_event_count(_hContact);
	if (dwNumEvents == 0)
		return FALSE;

	INT_PTR cbBase64Data = 0;

	DBEVENTINFO	dbei = { sizeof(DBEVENTINFO) };

	try {
		// read out all events for the current contact
		for (HANDLE hDbEvent = db_event_first(_hContact); hDbEvent != NULL; hDbEvent = db_event_next(hDbEvent)) {
			if (!DB::Event::GetInfoWithData(hDbEvent, &dbei)) {
				// new buffer for base64 encoded data
				INT_PTR cbNewBase64Data = Base64EncodeGetRequiredLength(dbei.cbBlob, BASE64_FLAG_NOCRLF);
				if (cbNewBase64Data > cbBase64Data) {
					pBase64Data = (LPSTR)mir_realloc(pBase64Data, cbNewBase64Data + 5);
					if (pBase64Data == NULL) {
						MessageBoxA(NULL, "mir_realloc(cbNewBase64Data + 5) == NULL", "Error", 0);
						break;
					}
					cbBase64Data = cbNewBase64Data;
				}

				// encode data
				if (Base64Encode(dbei.pBlob, dbei.cbBlob, pBase64Data, &cbNewBase64Data, BASE64_FLAG_NOCRLF)) {
					pBase64Data[cbNewBase64Data] = 0;
					TiXmlElement *xmlEvent = new TiXmlElement("evt");
					if (xmlEvent) {
						xmlEvent->SetAttribute("type", dbei.eventType);
						xmlEvent->SetAttribute("time", dbei.timestamp);
						xmlEvent->SetAttribute("flag", dbei.flags);

						TiXmlText *xmlText = new TiXmlText(pBase64Data);
						xmlEvent->LinkEndChild(xmlText);

						// find module
						TiXmlNode *xmlModule;
						for (xmlModule = _xmlNode->FirstChild(); xmlModule != NULL; xmlModule = xmlModule->NextSibling())
							if (!mir_stricmp(((TiXmlElement*)xmlModule)->Attribute("key"), dbei.szModule))
								break;

						// create new module
						if (!xmlModule) {
							xmlModule = _xmlNode->InsertEndChild(TiXmlElement(XKEY_MOD));
							if (!xmlModule)
								break;
							((TiXmlElement*)xmlModule)->SetAttribute("key", dbei.szModule);
						}

						xmlModule->LinkEndChild(xmlEvent);
						dwNumEventsAdded++;
					}
				}
				MIR_FREE(dbei.pBlob);
			}
		}
	}
	catch(...) {
		// fuck, do nothing
		MIR_FREE(dbei.pBlob);
		dwNumEventsAdded = 0;
	}

	mir_free(pbEventBuf);
	mir_free(pBase64Data);

	return dwNumEventsAdded == dwNumEvents;
}

/***********************************************************************************************************
 * importing stuff
 ***********************************************************************************************************/

/**
 * name:	CountKeys
 * desc:	Counts the number of events and settings stored for a contact
 * params:	xmlContact	- the contact, who is the owner of the keys to count
 * return:	nothing
 **/
void CExImContactXML::CountKeys(DWORD &numSettings, DWORD &numEvents)
{
	TiXmlNode *xmod, *xkey;

	numSettings = numEvents = 0;
	for (xmod = _xmlNode->FirstChild(); 
		xmod != NULL; 
		xmod = xmod->NextSibling(XKEY_MOD)) {
		for (xkey = xmod->FirstChild();
			xkey != NULL;
			xkey = xkey->NextSibling()) {
			if (!mir_stricmp(xkey->Value(), XKEY_SET))
				numSettings++;
			else
				numEvents++;
		}
	}
}

/**
 * name:	LoadXmlElemnt
 * class:	CExImContactXML
 * desc:	get contact information from XML-file
 * param:	xContact	- TiXmlElement representing a contact
 * return:	ERROR_OK if successful or any other error number otherwise
 **/
int CExImContactXML::LoadXmlElemnt(TiXmlElement *xContact)
{
	if (xContact == NULL) return ERROR_INVALID_PARAMS;

	LPSTR pszMetaProto = myGlobals.szMetaProto ? myGlobals.szMetaProto : "MetaContacts";

	// delete last contact
	db_free(&_dbvUID);
	_hContact = INVALID_HANDLE_VALUE;

	_xmlNode = xContact;
	MIR_FREE(_pszAMPro);		ampro(xContact->Attribute("ampro"));
	MIR_FREE(_pszNick);			nick (xContact->Attribute("nick"));
	MIR_FREE(_pszDisp);			disp (xContact->Attribute("disp"));
	MIR_FREE(_pszGroup);		group(xContact->Attribute("group"));
	MIR_FREE(_pszProto);
	MIR_FREE(_pszProtoOld);
	MIR_FREE(_pszUIDKey);

	// is contact a metacontact
	if (_pszAMPro && !strcmp(_pszAMPro, pszMetaProto) /*_xmlNode->FirstChildElement(XKEY_CONTACT)*/) {
		TiXmlElement *xSub;
		proto(pszMetaProto);

		// meta contact must be uniquelly identified by its subcontacts
		// the metaID may change during an export or import call
		for(xSub  = xContact->FirstChildElement(XKEY_CONTACT); 
			xSub != NULL;
			xSub  = xSub->NextSiblingElement(XKEY_CONTACT)) {
			CExImContactXML vSub(_pXmlFile);
			if (vSub = xSub) {
				// identify metacontact by the first valid subcontact in xmlfile
				if (_hContact == INVALID_HANDLE_VALUE && vSub.handle() != INVALID_HANDLE_VALUE) {
					HANDLE hMeta = (HANDLE)CallService(MS_MC_GETMETACONTACT, (WPARAM)vSub.handle(), NULL);
					if (hMeta != NULL) {
						_hContact = hMeta;
						break;
					}
				}
			}
		}
		// if no handle was found, this is a new meta contact
		_isNewContact = _hContact == INVALID_HANDLE_VALUE;
	}
	// entry is a default contact
	else {
		proto(xContact->Attribute("proto"));
		uidk (xContact->Attribute("uidk"));
		if (!_pszProto) {
			// check if this is the owner contact
			if (mir_stricmp(xContact->Value(), XKEY_OWNER))
				return ERROR_INVALID_PARAMS;
			_hContact = NULL;
			_xmlNode  = xContact;
			return ERROR_OK;
		}

		if (_pszUIDKey && mir_strcmp("#NV", _pszUIDKey) !=0) {
			LPCSTR pUID = xContact->Attribute("uidv");

			if (pUID != NULL) {
				size_t	len		= 0;
				INT_PTR	baselen	= NULL;
				PBYTE	pbVal	= NULL;

				switch (*(pUID++)) {
					case 'b':
						uid((BYTE)atoi(pUID));
						break;
					case 'w':
						uid((WORD)atoi(pUID));
						break;
					case 'd':
						uid((DWORD)_atoi64(pUID));
						break;
					case 's':
						// utf8 -> asci
						uida(pUID);
						break;
					case 'u':
						uidu(pUID);
						break;
					case 'n':
						len		= strlen(pUID);
						baselen	= Base64DecodeGetRequiredLength(len);
						pbVal	= (PBYTE)mir_alloc(baselen /*+1*/);
						if (pbVal != NULL){
							if (Base64Decode(pUID, len, pbVal, &baselen)) {
								uidn(pbVal, baselen);
							}
							else {
								assert(pUID != NULL);
							}
						}
						break;
					default:
						uidu((LPCSTR)NULL);
						break;
				}
			}
		}
		// finally try to find contact in contact list
		findHandle();
	}
	return ERROR_OK;
}

/**
 * name:	ImportContact
 * class:	CExImContactXML
 * desc:	create the contact if neccessary and copy
 *			all information from the xmlNode to database
 * param:	none
 * return:	ERROR_OK on success or any other error number otherwise
 **/
int CExImContactXML::ImportContact()
{
	TiXmlNode *xmod;

	// create the contact if not yet exists
	if (toDB() != INVALID_HANDLE_VALUE) {
		DWORD numSettings, numEvents;

		_hEvent = NULL;

		// count settings and events and init progress dialog
		CountKeys(numSettings, numEvents);
		_pXmlFile->_progress.SetSettingsCount(numSettings + numEvents);
		_pXmlFile->_numSettingsTodo += numSettings;
		_pXmlFile->_numEventsTodo += numEvents;

		// import all modules
		for(xmod  = _xmlNode->FirstChild();
			xmod != NULL;
			xmod = xmod->NextSibling(XKEY_MOD)) {

			// import module
			if (ImportModule(xmod) == ERROR_ABORTED) {
				// ask to delete new incomplete contact
				if (_isNewContact && _hContact != NULL) {
					int result = MsgBox(NULL, MB_YESNO|MB_ICONWARNING, 
						LPGENT("Question"), 
						LPGENT("Importing a new contact was aborted!"), 
						LPGENT("You aborted import of a new contact.\nSome information may be missing for this contact.\n\nDo you want to delete the incomplete contact?"));
					if (result == IDYES) {
						DB::Contact::Delete(_hContact);
						_hContact = INVALID_HANDLE_VALUE;
					}
				}
				return ERROR_ABORTED;
			}
		}
		return ERROR_OK;
	}
	return ERROR_NOT_ADDED;
}

/**
 * name:	ImportNormalContact
 * class:	CExImContactXML
 * desc:	create the contact if neccessary and copy
 *			all information from the xmlNode to database.
 *			Remove contact from a metacontact if it is a subcontact
 * param:	none
 * return:	ERROR_OK on success or any other error number otherwise
 **/
int CExImContactXML::ImportNormalContact()
{
	int err = ImportContact();

	// remove contact from a metacontact
	if (err == ERROR_OK && CallService(MS_MC_GETMETACONTACT, (WPARAM)_hContact, NULL)) {
		CallService(MS_MC_REMOVEFROMMETA, NULL, (LPARAM)_hContact);
	}	
	return err;
}

/**
 * name:	Import
 * class:	CExImContactXML
 * desc:	create the contact if neccessary and copy
 *			all information from the xmlNode to database.
 *			Remove contact from a metacontact if it is a subcontact
 * param:	TRUE = keepMetaSubContact
 * return:	ERROR_OK on success or any other error number otherwise
 **/
int CExImContactXML::Import(BYTE keepMetaSubContact)
{
	int result;
	TiXmlElement *xContact = _xmlNode->FirstChildElement("CONTACT");

	// xml contact contains subcontacts?
	if (xContact) {

		// contact is a metacontact and metacontacts plugin is installed?
		if (isMeta()) {
			// create object for first sub contact
			CExImContactXML	vContact(_pXmlFile);
			LPTSTR pszNick;

			// the contact does not yet exist
			if (_isNewContact) {
				// import default contact as normal contact and convert to meta contact
				if (!(vContact = xContact)) {
					return ERROR_CONVERT_METACONTACT;
				}
				// import as normal contact
				result = vContact.ImportContact();
				if (result != ERROR_OK) return result;
				// convert default subcontact to metacontact
				_hContact = (HANDLE)CallService(MS_MC_CONVERTTOMETA, (WPARAM)vContact.handle(), NULL);
				if (_hContact == NULL) {
					_hContact = INVALID_HANDLE_VALUE;
					return ERROR_CONVERT_METACONTACT;
				}

				_pXmlFile->_numContactsDone++;
				// do not load first meta contact twice
				xContact = xContact->NextSiblingElement("CONTACT");
			}
			// xml contact contains more than one subcontacts?
			if (xContact) {
				// load all subcontacts
				do {
					// update progressbar and abort if user clicked cancel
					pszNick = mir_utf8decodeT(xContact->Attribute("nick"));
					result = _pXmlFile->_progress.UpdateContact(_T("Sub Contact: %s (%S)"), pszNick, xContact->Attribute("proto"));
					if (pszNick) mir_free(pszNick);
					// user clicked abort button
					if (!result) break;
					if (vContact = xContact) {
						if (vContact.ImportMetaSubContact(this) == ERROR_ABORTED)
							return ERROR_ABORTED;
						_pXmlFile->_numContactsDone++;
					}
				}
				while (xContact = xContact->NextSiblingElement("CONTACT"));
			}
			// load metacontact information (after subcontact for faster import)
			ImportContact();
			return ERROR_OK;
		}
		// import sub contacts as normal contacts
		return _pXmlFile->ImportContacts(_xmlNode);
	}

	// load contact information
	result = ImportContact();
	if (result == ERROR_OK && !keepMetaSubContact)
	{
		CallService(MS_MC_REMOVEFROMMETA, NULL, (LPARAM)_hContact);
	}

	return result;
}

/**
 * name:	ImportMetaSubContact
 * class:	CExImContactXML
 * desc:	create the contact if neccessary and copy
 *			all information from the xmlNode to database.
 *			Add this contact to an meta contact
 * param:	pMetaContact	- the meta contact to add this one to
 * return:	
 **/
int CExImContactXML::ImportMetaSubContact(CExImContactXML * pMetaContact)
{
	int err = ImportContact();

	// abort here if contact was not imported correctly
	if (err != ERROR_OK) return err;

	// check if contact is subcontact of the desired meta contact
	if ((HANDLE)CallService(MS_MC_GETMETACONTACT, (WPARAM)_hContact, NULL) != pMetaContact->handle()) {
		// add contact to the metacontact (this service returns TRUE if successful)	
		err = CallService(MS_MC_ADDTOMETA, (WPARAM)_hContact, (LPARAM)pMetaContact->handle());
		if (err == FALSE) {
			// ask to delete new contact
			if (_isNewContact && _hContact != NULL) {
				LPTSTR ptszNick = mir_utf8decodeT(_pszNick);
				LPTSTR ptszMetaNick = mir_utf8decodeT(pMetaContact->_pszNick);
				int result = MsgBox(NULL, MB_YESNO|MB_ICONWARNING, 
					LPGENT("Question"), 
					LPGENT("Importing a new meta subcontact failed!"), 
					LPGENT("The newly created MetaSubContact '%s'\ncould not be added to MetaContact '%s'!\n\nDo you want to delete this contact?"),
					ptszNick, ptszMetaNick);
				MIR_FREE(ptszNick);
				MIR_FREE(ptszMetaNick);
				if (result == IDYES) {
					DB::Contact::Delete(_hContact);
					_hContact = INVALID_HANDLE_VALUE;
				}
			}
			return ERROR_ADDTO_METACONTACT;
		}
	}
	return ERROR_OK;
}

/**
 * name:	ImportModule
 * class:	CExImContactXML
 * desc:	interprete an xmlnode as module and add the children to database.
 * params:	hContact	- handle to the contact, who is the owner of the setting to import
 *			xmlModule	- xmlnode representing the module
 *			stat		- structure used to collect some statistics
 * return:	ERROR_OK on success or one other element of ImportError to tell the type of failure
 **/
int CExImContactXML::ImportModule(TiXmlNode* xmlModule)
{
	TiXmlElement *xMod;
	TiXmlElement *xKey;
	LPCSTR pszModule;
	BYTE isProtoModule;
	BYTE isMetaModule;
	
	// check if parent is really a module
	if (!xmlModule || mir_stricmp(xmlModule->Value(), XKEY_MOD))
		return ERROR_INVALID_SIGNATURE;
	// convert to element
	if (!(xMod = xmlModule->ToElement()))
		return ERROR_INVALID_PARAMS;
	// get module name
	pszModule = xMod->Attribute("key");
	if (!pszModule || !*pszModule)
		return ERROR_INVALID_PARAMS;
	// ignore Modul 'Protocol' as it would cause trouble
	if (!mir_stricmp(pszModule, "Protocol"))
		return ERROR_OK;
	
	for (xKey = xmlModule->FirstChildElement(); xKey != NULL; xKey = xKey->NextSiblingElement()) {
		// import setting
		if (!mir_stricmp(xKey->Value(), XKEY_SET)) {
			// check if the module to import is the contact's protocol module
			isProtoModule	= !mir_stricmp(pszModule, _pszProto)/* || DB::Module::IsMeta(pszModule)*/;
			isMetaModule	= DB::Module::IsMeta(pszModule);

			// just ignore MetaModule on normal contact to avoid errors (only keys)
			if (!isProtoModule && isMetaModule) {
				continue;
			}
			// just ignore MetaModule on Meta to avoid errors (only import spetial keys)
			else if (isProtoModule && isMetaModule) {
				if (!mir_stricmp(xKey->Attribute("key"),"Nick") ||
					!mir_stricmp(xKey->Attribute("key"),"TzName") ||
					!mir_stricmp(xKey->Attribute("key"),"Timezone")) {
					if (ImportSetting(pszModule, xKey->ToElement()) == ERROR_OK) {
						_pXmlFile->_numSettingsDone++;
					}
				}
			}
			// just ignore some settings of protocol module to avoid errors (only keys)
			else if (isProtoModule && !isMetaModule) {
				if (!IsContactInfo(xKey->Attribute("key"))) {
					if (ImportSetting(pszModule, xKey->ToElement()) == ERROR_OK) {
						_pXmlFile->_numSettingsDone++;
					}
				}
			}
			// other module
			else if (ImportSetting(pszModule, xKey->ToElement()) == ERROR_OK) {
					_pXmlFile->_numSettingsDone++;
			}
			if (!_pXmlFile->_progress.UpdateSetting(LPGENT("Settings: %S"), pszModule))
				return ERROR_ABORTED;
		}
		// import event
		else if (!mir_stricmp(xKey->Value(), XKEY_EVT)) {
			int error = ImportEvent(pszModule, xKey->ToElement());
			switch (error) {
				case ERROR_OK:
					_pXmlFile->_numEventsDone++;
					break;
				case ERROR_DUPLICATED:
					_pXmlFile->_numEventsDuplicated++;
					break;
			}
			if (!_pXmlFile->_progress.UpdateSetting(LPGENT("Events: %S"), pszModule))
				return ERROR_ABORTED;
		}
	} //*end for
	return ERROR_OK;
}

/**
 * name:	ImportSetting
 * class:	CExImContactXML
 * desc:	interprete an setting representing xmlnode and write the corresponding setting to database.
 * params:	xmlModule	- xmlnode representing the module to write the setting to in the database
 *			xmlEntry	- xmlnode representing the setting to import
 * return:	ERROR_OK on success or one other element of ImportError to tell the type of failure
 **/
int CExImContactXML::ImportSetting(LPCSTR pszModule, TiXmlElement *xmlEntry)
{
	// validate parameter
	if (!xmlEntry || !pszModule || !*pszModule)
		return ERROR_INVALID_PARAMS;

	// validate value
	TiXmlText* xval = (TiXmlText*)xmlEntry->FirstChild();
	if (!xval || xval->Type() != TiXmlText::TEXT)
		return ERROR_INVALID_VALUE;

	LPCSTR value = xval->Value();
	DBVARIANT dbv = { 0 };
	
	// convert data
	size_t	len		= 0;
	INT_PTR	baselen	= NULL;

	switch (value[0]) {
	case 'b':			//'b' bVal and cVal are valid
		dbv.type = DBVT_BYTE;
		dbv.bVal = (BYTE)atoi(value + 1);
		break;
	case 'w':			//'w' wVal and sVal are valid
		dbv.type = DBVT_WORD;
		dbv.wVal = (WORD)atoi(value + 1);
		break;
	case 'd':			//'d' dVal and lVal are valid
		dbv.type = DBVT_DWORD;
		dbv.dVal = (DWORD)_atoi64(value + 1);
		break;
	case 's':			//'s' pszVal is valid
		dbv.type = DBVT_ASCIIZ;
		dbv.pszVal = (LPSTR)mir_utf8decodeA((LPSTR)(value + 1));
		break;
	case 'u':
		dbv.type = DBVT_UTF8;
		dbv.pszVal = (LPSTR)mir_strdup((LPSTR)(value + 1));
		break;
	case 'n':
		len = strlen(value + 1);
		baselen = Base64DecodeGetRequiredLength(len);
		dbv.type = DBVT_BLOB;
		dbv.pbVal = (PBYTE)mir_alloc(baselen +1);
		if (dbv.pbVal != NULL){
			if (Base64Decode((value + 1), len, dbv.pbVal, &baselen))
				dbv.cpbVal = baselen;
			else {
				mir_free(dbv.pbVal);
				return ERROR_NOT_ADDED;
			}
		}
		break;
	default:
		return ERROR_INVALID_TYPE;
	}

	// write value to db
	if (db_set(_hContact, pszModule, xmlEntry->Attribute("key"), &dbv)) {
		//if (cws.value.pbVal>0)
		mir_free(dbv.pbVal);
		if (dbv.type == DBVT_ASCIIZ || dbv.type == DBVT_UTF8) mir_free(dbv.pszVal);
		return ERROR_NOT_ADDED;
	}
	//if (dbv.pbVal>0)
	mir_free(dbv.pbVal);
	if (dbv.type == DBVT_ASCIIZ || dbv.type == DBVT_UTF8) mir_free(dbv.pszVal);
	return ERROR_OK;
}

/**
 * name:	ImportEvent
 * class:	CExImContactXML
 * desc:	interprete an xmlnode and add the corresponding event to database.
 * params:	hContact	- handle to the contact, who is the owner of the setting to import
 *			xmlModule	- xmlnode representing the module to write the setting to in the database
 *			xmlEvent	- xmlnode representing the event to import
 * return:	ERROR_OK on success or one other element of ImportError to tell the type of failure
 **/
int CExImContactXML::ImportEvent(LPCSTR pszModule, TiXmlElement *xmlEvent)
{
	// dont import events from metacontact
	if (isMeta())
		return ERROR_DUPLICATED;

	if (!xmlEvent || !pszModule || !*pszModule)
		return ERROR_INVALID_PARAMS;

	if (_stricmp(xmlEvent->Value(), "evt"))
		return ERROR_NOT_ADDED;

	// timestamp must be valid
	DBEVENTINFO	dbei = { sizeof(dbei) };
	xmlEvent->Attribute("time", (LPINT)&dbei.timestamp);
	if (dbei.timestamp == 0)
		return ERROR_INVALID_TIMESTAMP;

	TiXmlText *xmlValue = (TiXmlText*)xmlEvent->FirstChild();
	if (!xmlValue || xmlValue->Type() != TiXmlText::TEXT)
		return ERROR_INVALID_VALUE;

	LPCSTR tmp = xmlValue->Value();
	if (!tmp || tmp[0] == 0)
		return ERROR_INVALID_VALUE;

	size_t cbSrc = strlen(tmp);
	INT_PTR baselen = Base64DecodeGetRequiredLength(cbSrc);

	dbei.pBlob = (PBYTE)_alloca(baselen+1);
	if (Base64Decode(tmp, cbSrc, dbei.pBlob, &baselen)) {
		// event owning module
		dbei.szModule	= (LPSTR)pszModule;
		dbei.cbBlob		= baselen;

		xmlEvent->Attribute("type", (LPINT)&dbei.eventType);
		xmlEvent->Attribute("flag", (LPINT)&dbei.flags);
		if (dbei.flags == 0)
			dbei.flags = DBEF_READ;

		// search in new and existing contact for existing event to avoid duplicates
		if (/*!_isNewContact && */DB::Event::Exists(_hContact, _hEvent, &dbei)) {
			mir_free(dbei.pBlob);
			return ERROR_DUPLICATED;
		}

		if ((_hEvent = db_event_add(_hContact, &dbei)) != 0)
			return ERROR_OK;
	}

	return ERROR_NOT_ADDED;
}
