#include "headers.h"

#pragma comment(lib,"ws2_32.lib")

#include "csocket.h"
#include "cserver.h"

#define MS_FAVCONTACTS_OPEN_CONTACT			"FavContacts/OpenContact"

class CHttpProcessor: public IConnectionProcessor
{
private:
	CSocket *m_socket;

	char *FetchURL(char *s)
	{
		char *p;
		if (p = strstr(s, "\r\n")) *p = 0;
		if (p = strrchr(s, ' ')) *p = 0;
		if (p = strchr(s, ' ')) while (*p && *p == ' ') p++;
		return mir_strdup(p);
	}

public:
	CHttpProcessor(CSocket *s): m_socket(s) {}

	void ProcessConnection()
	{
		char buf[1024];
		int n = m_socket->Recv(buf, sizeof(buf));
		buf[n] = 0;

		char *s = FetchURL(buf);

		if (!strncmp(s, "/fav/list/", 10))
		{
			SendList();
		} else
		if (!strncmp(s, "/fav/open/", 10))
		{
			OpenContact(s);
		}

		mir_free(s);
		m_socket->Close();
	}

	void OpenContact(char *s)
	{
		m_socket->Send("HTTP 200 OK\r\n\r\n");

		int hContact;
		sscanf(s, "/fav/open/%d", &hContact);
		if (CallService(MS_DB_CONTACT_IS, hContact, 0))
			CallServiceSync(MS_FAVCONTACTS_OPEN_CONTACT, (WPARAM)hContact, 0);
	}

	void SendList()
	{
		TFavContacts favList;
		favList.build();

		m_socket->Send(
			"HTTP 200 OK\r\n"
			"Content-Type: text/javascript\r\n"
			"\r\n");

		Send("try {\r\n");
		Send("SetContactCount(");
		Send(favList.getCount());
		Send(");\r\n");

		for (int i = 0; i < favList.getCount(); ++i)
		{
			HANDLE hContact = favList[i]->getHandle();
			TCHAR *name = (TCHAR *)CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM)hContact, GCDNF_TCHAR);
			AVATARCACHEENTRY *avatar = (AVATARCACHEENTRY *)CallService(MS_AV_GETAVATARBITMAP, (WPARAM)hContact, 0);
			int status = DBGetContactSettingWord(hContact, (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0), "Status", ID_STATUS_OFFLINE);

			Send("SetContact(");
			Send(i);
			Send(", ");
			Send((int)hContact);
			Send(", '");
			SendQuoted(name);
			Send("', ");
			Send(status);
			Send(", '");
			SendQuoted(avatar ? avatar->szFilename : _T(""));
			Send("');\r\n");
		}
		Send("} catch(e) {}\r\n");
	}

	void Send(char *s)
	{
		m_socket->Send(s);
	}

#ifdef UNICODE
	void Send(WCHAR *ws)
	{
		char *s = mir_utf8encodeW(ws);
		m_socket->Send(s);
		mir_free(s);
	}
#endif

	void Send(int i)
	{
		char buf[32];
		wsprintfA(buf, "%d", i);
		Send(buf);
	}

	template<class XCHAR>
	void SendQuoted(const XCHAR *s)
	{
		int length = 0;
		const XCHAR *p;
		for (p = s; *p;  p++)
		{
			if (*p == '\'' || *p == '\\' || *p == '\"')
				length++;
			length++;
		}
		XCHAR *buf = (XCHAR *)mir_alloc(sizeof(XCHAR) * (length + 1));
		XCHAR *q = buf;
		for (p = s; *p;  p++)
		{
			if (*p == '\'' || *p == '\\' || *p == '\"')
			{
				*q = '\\';
				q++;
			}
			*q = *p;
			q++;
		}
		*q = 0;
		Send(buf);
		mir_free(buf);
	}
};

class CHttpProcessorFactory: public IConnectionProcessorFactory
{
public:
	IConnectionProcessor *Create(CSocket *s)
	{
		return new CHttpProcessor(s);
	}
};

static CHttpProcessorFactory g_httpProcessorFactory;
static CServer g_httpServer;

void LoadHttpApi()
{
	g_httpServer.Start(60888, &g_httpProcessorFactory, true);
}

void UnloadHttpApi()
{
	g_httpServer.Stop();
}
