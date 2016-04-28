#ifndef _HTTP_REQUEST_H_
#define _HTTP_REQUEST_H_

class HttpRequestException
{
	CMStringA message;

public:
	HttpRequestException(const char *message) :
		message(message)
	{
	}

	const char* what() const throw()
	{
		return message.c_str();
	}
};

class HttpRequest : protected NETLIBHTTPREQUEST
{
private:
	CMStringA m_szUrl;

	void Init(int type)
	{
		cbSize = sizeof(NETLIBHTTPREQUEST);
		requestType = type;
		flags = NLHRF_HTTP11 | NLHRF_SSL | NLHRF_NODUMPSEND | NLHRF_DUMPASTEXT;
		szUrl = NULL;
		headers = NULL;
		headersCount = 0;
		pData = NULL;
		dataLength = 0;
		resultCode = 0;
		szResultDescr = NULL;
		nlc = NULL;
		timeout = 0;
	}

protected:
	enum HttpRequestUrlFormat { FORMAT };

	void AddHeader(LPCSTR szName, LPCSTR szValue)
	{
		headers = (NETLIBHTTPHEADER*)mir_realloc(headers, sizeof(NETLIBHTTPHEADER) * (headersCount + 1));
		headers[headersCount].szName = mir_strdup(szName);
		headers[headersCount].szValue = mir_strdup(szValue);
		headersCount++;
	}

	void AddBasicAuthHeader(LPCSTR szLogin, LPCSTR szPassword)
	{
		char cPair[128];
		mir_snprintf(
			cPair,
			_countof(cPair),
			"%s:%s",
			szLogin,
			szPassword);

		char *ePair = (char *)mir_base64_encode((BYTE*)cPair, (UINT)mir_strlen(cPair));

		char value[128];
		mir_snprintf(
			value,
			_countof(value),
			"Basic %s",
			ePair);

		mir_free(ePair);

		headers = (NETLIBHTTPHEADER*)mir_realloc(headers, sizeof(NETLIBHTTPHEADER)*(headersCount + 1));
		headers[headersCount].szName = mir_strdup("Authorization");
		headers[headersCount].szValue = mir_strdup(value);
		headersCount++;
	}

	void AddBearerAuthHeader(LPCSTR szValue)
	{
		char value[128];
		mir_snprintf(
			value,
			_countof(value),
			"Bearer %s",
			szValue);

		headers = (NETLIBHTTPHEADER*)mir_realloc(headers, sizeof(NETLIBHTTPHEADER)*(headersCount + 1));
		headers[headersCount].szName = mir_strdup("Authorization");
		headers[headersCount].szValue = mir_strdup(value);
		headersCount++;
	}

	void AddUrlParameter(const char *urlFormat, ...)
	{
		va_list urlArgs;
		va_start(urlArgs, urlFormat);
		m_szUrl += m_szUrl.Find('?') == -1 ? '?' : '&';
		m_szUrl.AppendFormatV(urlFormat, urlArgs);
		va_end(urlArgs);
	}

	void SetData(const char *data, size_t size)
	{
		if (pData != NULL)
			mir_free(pData);

		dataLength = (int)size;
		pData = (char*)mir_alloc(size);
		memcpy(pData, data, size);
	}

public:
	HttpRequest(int type, LPCSTR url)
	{
		Init(type);

		m_szUrl = url;
	}

	HttpRequest(int type, HttpRequestUrlFormat, LPCSTR urlFormat, ...)
	{
		Init(type);

		va_list formatArgs;
		va_start(formatArgs, urlFormat);
		m_szUrl.AppendFormatV(urlFormat, formatArgs);
		va_end(formatArgs);
	}

	~HttpRequest()
	{
		for (int i = 0; i < headersCount; i++)
		{
			mir_free(headers[i].szName);
			mir_free(headers[i].szValue);
		}
		mir_free(headers);
		if (pData)
			mir_free(pData);
	}

	NETLIBHTTPREQUEST* Send(HANDLE hNetlibConnection)
	{
		m_szUrl.Replace('\\', '/');
		szUrl = m_szUrl.GetBuffer();
		return (NETLIBHTTPREQUEST*)CallService(MS_NETLIB_HTTPTRANSACTION, (WPARAM)hNetlibConnection, (LPARAM)this);
	}
};

#endif //_HTTP_REQUEST_H_