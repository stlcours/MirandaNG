/*

Jabber Protocol Plugin for Miranda NG

Copyright (c) 2002-04  Santithorn Bunchua
Copyright (c) 2005-12  George Hazan
Copyright (c) 2007     Maxim Mluhov
Copyright (�) 2012-16 Miranda NG project

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#ifndef _JABBER_XML_H_
#define _JABBER_XML_H_

#include <m_xml.h>

void    __fastcall XmlAddChild(HXML, HXML);
HXML    __fastcall XmlAddChild(HXML, LPCTSTR pszName);
HXML    __fastcall XmlAddChild(HXML, LPCTSTR pszName, LPCTSTR ptszValue);
HXML    __fastcall XmlAddChild(HXML, LPCTSTR pszName, int iValue);

LPCTSTR __fastcall XmlGetAttrValue(HXML, LPCTSTR key);
HXML    __fastcall XmlGetChild(HXML, int n = 0);
HXML    __fastcall XmlGetChild(HXML, LPCSTR key);
HXML    __fastcall XmlGetChild(HXML, LPCTSTR key);
int     __fastcall XmlGetChildCount(HXML);
HXML    __fastcall XmlGetChildByTag(HXML, LPCTSTR key, LPCTSTR attrName, LPCTSTR attrValue);
HXML    __fastcall XmlGetChildByTag(HXML, LPCSTR key, LPCSTR attrName, LPCTSTR attrValue);
HXML    __fastcall XmlGetNthChild(HXML, LPCTSTR key, int n = 0);

LPCTSTR __fastcall XmlGetName(HXML);
LPCTSTR __fastcall XmlGetText(HXML);

void    __fastcall XmlAddAttr(HXML, LPCTSTR pszName, LPCTSTR ptszValue);
void    __fastcall XmlAddAttr(HXML, LPCTSTR pszName, int value);
void    __fastcall XmlAddAttr(HXML hXml, LPCTSTR pszName, unsigned __int64 value);
void    __fastcall XmlAddAttrID(HXML, int id);

int     __fastcall XmlGetAttrCount(HXML);
LPCTSTR __fastcall XmlGetAttr(HXML, int n);
LPCTSTR __fastcall XmlGetAttrName(HXML, int n);
LPCTSTR __fastcall XmlGetAttrValue(HXML, LPCTSTR key);

struct XmlNode
{
	__forceinline XmlNode() { m_hXml = NULL; }

	__forceinline XmlNode(LPCTSTR pszString, int* numBytes, LPCTSTR ptszTag)
	{
		m_hXml = xmlParseString(pszString, numBytes, ptszTag);
	}

	XmlNode(const XmlNode& n);
	XmlNode(LPCTSTR name);
	XmlNode(LPCTSTR pszName, LPCTSTR ptszText);
	~XmlNode();

	XmlNode& operator =(const XmlNode& n);

	__forceinline operator HXML() const
	{	return m_hXml;
	}

private:
	HXML m_hXml;
};

class CJabberIqInfo;

struct XmlNodeIq : public XmlNode
{
	XmlNodeIq(const wchar_t *type, int id = -1, const wchar_t *to = NULL);
	XmlNodeIq(const wchar_t *type, const wchar_t *idStr, const wchar_t *to);
	XmlNodeIq(const wchar_t *type, HXML node, const wchar_t *to);
	// new request
	XmlNodeIq(CJabberIqInfo *pInfo);
	// answer to request
	XmlNodeIq(const wchar_t *type, CJabberIqInfo *pInfo);
};

typedef void (*JABBER_XML_CALLBACK)(HXML, void*);

/////////////////////////////////////////////////////////////////////////////////////////

struct XATTR
{
	LPCTSTR name, value;

	__forceinline XATTR(LPCTSTR _name, LPCTSTR _value) :
		name(_name),
		value(_value)
		{}
};

HXML __forceinline operator<<(HXML node, const XATTR& attr)
{	XmlAddAttr(node, attr.name, attr.value);
	return node;
}

/////////////////////////////////////////////////////////////////////////////////////////

struct XATTRI
{
	LPCTSTR name;
	int value;

	__forceinline XATTRI(LPCTSTR _name, int _value) :
		name(_name),
		value(_value)
		{}
};

HXML __forceinline operator<<(HXML node, const XATTRI& attr)
{	XmlAddAttr(node, attr.name, attr.value);
	return node;
}

/////////////////////////////////////////////////////////////////////////////////////////

struct XATTRI64
{
	LPCTSTR name;
	unsigned __int64 value;

	__forceinline XATTRI64(LPCTSTR _name, unsigned __int64 _value) :
		name(_name),
		value(_value)
		{}
};

HXML __forceinline operator<<(HXML node, const XATTRI64& attr)
{	XmlAddAttr(node, attr.name, attr.value);
	return node;
}

/////////////////////////////////////////////////////////////////////////////////////////

struct XATTRID
{
	int id;

	__forceinline XATTRID(int _value) :
		id(_value)
		{}
};

HXML __forceinline operator<<(HXML node, const XATTRID& attr)
{	XmlAddAttrID(node, attr.id);
	return node;
}

/////////////////////////////////////////////////////////////////////////////////////////

struct XCHILD
{
	LPCTSTR name, value;

	__forceinline XCHILD(LPCTSTR _name, LPCTSTR _value = NULL) :
		name(_name),
		value(_value)
		{}
};

HXML __forceinline operator<<(HXML node, const XCHILD& child)
{	return XmlAddChild(node, child.name, child.value);
}

/////////////////////////////////////////////////////////////////////////////////////////

struct XCHILDNS
{
	LPCTSTR name, ns;

	__forceinline XCHILDNS(LPCTSTR _name, LPCTSTR _ns = NULL) :
		name(_name),
		ns(_ns)
		{}
};

HXML __fastcall operator<<(HXML node, const XCHILDNS& child);

/////////////////////////////////////////////////////////////////////////////////////////

struct XQUERY
{
	LPCTSTR ns;

	__forceinline XQUERY(LPCTSTR _ns) :
		ns(_ns)
		{}
};

HXML __fastcall operator<<(HXML node, const XQUERY& child);

/////////////////////////////////////////////////////////////////////////////////////////
// Limited XPath support
//     path should look like: "node-spec/node-spec/.../result-spec"
//     where "node-spec" can be "node-name", "node-name[@attr-name='attr-value']" or "node-name[node-index]"
//     result may be either "node-spec", or "@attr-name"
//
// Samples:
//    LPCTSTR s = XPathT(node, "child/subchild[@attr='value']");          // get node text
//    LPCTSTR s = XPathT(node, "child/subchild[2]/@attr");                // get attribute value
//    XPathT(node, "child/subchild[@name='test']/@attr") = L"Hello";   // create path if needed and set attribute value
//
//    XPathT(node, "child/subchild[@name='test']") = L"Hello";         // TODO: create path if needed and set node text

#define XPathT(a,b) XPath(a, _T(b))

class XPath
{
public:
	__forceinline XPath(HXML hXml, wchar_t *path):
		m_type(T_UNKNOWN),
		m_hXml(hXml),
		m_szPath(path),
		m_szParam(NULL)
		{}

	// Read data
	operator HXML()
	{
		switch (Lookup())
		{
			case T_NODE: return m_hXml;
			case T_NODESET: return XmlGetNthChild(m_hXml, m_szParam, 1);
		}
		return NULL;
	}
	operator LPTSTR()
	{
		switch (Lookup())
		{
			case T_ATTRIBUTE: return (wchar_t *)XmlGetAttrValue(m_hXml, m_szParam);
			case T_NODE: return (wchar_t *)XmlGetText(m_hXml);
			case T_NODESET: return (wchar_t *)XmlGetText(XmlGetNthChild(m_hXml, m_szParam, 1));
		}
		return NULL;
	}
	operator int()
	{
		if (wchar_t *s = *this) return _wtoi(s);
		return 0;
	}
	__forceinline bool operator== (wchar_t *str)
	{
		return !mir_wstrcmp((LPCTSTR)*this, str);
	}
	__forceinline bool operator!= (wchar_t *str)
	{
		return mir_wstrcmp((LPCTSTR)*this, str) ? true : false;
	}
	HXML operator[] (int idx)
	{
		return (Lookup() == T_NODESET) ? XmlGetNthChild(m_hXml, m_szParam, idx) : NULL;
	}

	// Write data
	void operator= (LPCTSTR value)
	{
		switch (Lookup(true))
		{
			case T_ATTRIBUTE: XmlAddAttr(m_hXml, m_szParam, value); break;
			case T_NODE: break; // TODO: set node text
		}
	}
	void operator= (int value)
	{
		wchar_t buf[16];
		_itow(value, buf, 10);
		*this = buf;
	}

private:
	enum PathType
	{
		T_UNKNOWN,
		T_ERROR,
		T_NODE,
		T_ATTRIBUTE,
		T_NODESET
	};

	__forceinline PathType Lookup(bool bCreate=false)
	{
		return (m_type == T_UNKNOWN) ? LookupImpl(bCreate) : m_type;
	}

	enum LookupState
	{
		S_START,
		S_ATTR_STEP,
		S_NODE_NAME,
		S_NODE_OPENBRACKET,
		S_NODE_INDEX,
		S_NODE_ATTRNAME,
		S_NODE_ATTREQUALS,
		S_NODE_ATTRVALUE,
		S_NODE_ATTRCLOSEVALUE,
		S_NODE_CLOSEBRACKET,

		S_FINAL,
		S_FINAL_ERROR,
		S_FINAL_ATTR,
		S_FINAL_NODESET,
		S_FINAL_NODE
	};

	struct LookupString
	{
		void Begin(const wchar_t *p_) { p = p_; }
		void End(const wchar_t *p_) { length = p_ - p; }
		operator bool() { return p ? true : false; }

		const wchar_t *p;
		int length;

	};

	struct LookupInfo
	{
		void Reset() { memset(this, 0, sizeof(*this)); }
		LookupString nodeName;
		LookupString attrName;
		LookupString attrValue;
		LookupString nodeIndex;
	};

	void ProcessPath(LookupInfo &info, bool bCreate);
	PathType LookupImpl(bool bCreate);

	PathType m_type;
	HXML m_hXml;
	LPCTSTR m_szPath;
	LPCTSTR m_szParam;
};

class XPathFmt: public XPath
{
public:
	enum { BUFSIZE = 512 };
	XPathFmt(HXML hXml, wchar_t *path, ...): XPath(hXml, m_buf)
	{
		*m_buf = 0;

		va_list args;
		va_start(args, path);
		mir_vsnwprintf(m_buf, BUFSIZE, path, args);
		m_buf[BUFSIZE-1] = 0;
		va_end(args);
	}

	XPathFmt(HXML hXml, char *path, ...): XPath(hXml, m_buf)
	{
		*m_buf = 0;
		char buf[BUFSIZE];

		va_list args;
		va_start(args, path);
		mir_vsnprintf(buf, BUFSIZE, path, args);
		buf[BUFSIZE-1] = 0;
		MultiByteToWideChar(CP_ACP, 0, buf, -1, m_buf, BUFSIZE);
		va_end(args);
	}

private:
	wchar_t m_buf[BUFSIZE];
};

#endif
