#include "common.h"

void WhatsAppProto::ChangeStatus(void*)
{
	if (m_iDesiredStatus != ID_STATUS_OFFLINE && m_iStatus == ID_STATUS_OFFLINE)
	{
		ResetEvent(update_loop_lock_);
		ForkThread(&WhatsAppProto::sentinelLoop, this);
		ForkThread(&WhatsAppProto::stayConnectedLoop, this);
	}
	else if (m_iStatus == ID_STATUS_INVISIBLE && m_iDesiredStatus == ID_STATUS_ONLINE)
	{
		if (this->connection != NULL)
		{
			this->connection->sendAvailableForChat();
			m_iStatus = ID_STATUS_ONLINE;
			ProtoBroadcastAck(0, ACKTYPE_STATUS, ACKRESULT_SUCCESS, (HANDLE) m_iStatus, ID_STATUS_INVISIBLE);
		}
	}
	else if (m_iStatus == ID_STATUS_ONLINE && m_iDesiredStatus == ID_STATUS_INVISIBLE)
	{
		if (this->connection != NULL)
		{
			this->connection->sendClose();
			m_iStatus = ID_STATUS_INVISIBLE;
			SetAllContactStatuses( ID_STATUS_OFFLINE, true );
			ProtoBroadcastAck(0, ACKTYPE_STATUS, ACKRESULT_SUCCESS, (HANDLE) m_iStatus, ID_STATUS_ONLINE);
		}
	}
	else if (m_iDesiredStatus == ID_STATUS_OFFLINE)
	{
		if (this->conn != NULL)
		{
			SetEvent(update_loop_lock_);
			this->conn->forceShutdown();
			LOG("Forced shutdown");
		}
	}
}

void WhatsAppProto::stayConnectedLoop(void*)
{
	bool error = true;
	std::string cc, in, pass;
	DBVARIANT dbv = {0};

	if ( !db_get_s(NULL,m_szModuleName,WHATSAPP_KEY_CC,&dbv,DBVT_ASCIIZ))
	{
		cc = dbv.pszVal;
		db_free(&dbv);
		error = cc.empty();
	}
	if (error)
	{
		NotifyEvent(m_tszUserName,TranslateT("Please enter a country-code."),NULL,WHATSAPP_EVENT_CLIENT);
		return;
	}

	error = true;
	if ( !db_get_s(NULL,m_szModuleName,WHATSAPP_KEY_LOGIN,&dbv,DBVT_ASCIIZ))
	{
		in = dbv.pszVal;
		db_free(&dbv);
		error = in.empty();
	}
	if (error)
	{
		NotifyEvent(m_tszUserName,TranslateT("Please enter a phone-number without country code."),NULL,WHATSAPP_EVENT_CLIENT);
		return;
	}
	this->phoneNumber = cc + in;
	this->jid = this->phoneNumber + "@s.whatsapp.net";

	error = true;
	if ( !db_get_s(NULL, m_szModuleName, WHATSAPP_KEY_NICK, &dbv, DBVT_ASCIIZ))
	{
		this->nick = dbv.pszVal;
		db_free(&dbv);
		error = nick.empty();
	}
	if (error)
	{
		NotifyEvent(m_tszUserName, TranslateT("Please enter a nickname."), NULL, WHATSAPP_EVENT_CLIENT);
		return;
	}

	error = true;
	if ( !db_get_s(NULL,m_szModuleName,WHATSAPP_KEY_PASS,&dbv, DBVT_ASCIIZ))
	{
		CallService(MS_DB_CRYPT_DECODESTRING,strlen(dbv.pszVal)+1,
			reinterpret_cast<LPARAM>(dbv.pszVal));
		pass = dbv.pszVal;
		db_free(&dbv);
		error = pass.empty();
	}
	if (error)
	{
		NotifyEvent(m_tszUserName,TranslateT("Please enter a password."),NULL,WHATSAPP_EVENT_CLIENT);
		return;
	}

	// -----------------------------

	Mutex writerMutex;
	WALogin* login = NULL;
	int desiredStatus;

	this->conn = NULL;

	while (true)
	{
		if (connection != NULL)
		{
			if (connection->getLogin() == NULL && login != NULL)
			{
				delete login;
				login = NULL;
			}
			delete connection;
			connection = NULL;
		}
		if (this->conn != NULL)
		{
			delete this->conn;
			this->conn = NULL;
		}

		desiredStatus = this->m_iDesiredStatus;
		if (desiredStatus == ID_STATUS_OFFLINE || error)
		{
			LOG("Set status to offline");
			SetAllContactStatuses( ID_STATUS_OFFLINE, true );
			this->ToggleStatusMenuItems(false);
			int prevStatus = this->m_iStatus;
			this->m_iStatus = ID_STATUS_OFFLINE;
			ProtoBroadcastAck(0, ACKTYPE_STATUS, ACKRESULT_SUCCESS, (HANDLE) m_iStatus, prevStatus);
			break;
		}

		LOG("Connecting...");
		this->m_iStatus = ID_STATUS_CONNECTING;
		ProtoBroadcastAck(0, ACKTYPE_STATUS, ACKRESULT_SUCCESS, (HANDLE) ID_STATUS_OFFLINE, m_iStatus);

		CODE_BLOCK_TRY

			BYTE UseSSL = db_get_b(NULL, this->ModuleName(), WHATSAPP_KEY_SSL, 0);
			if (UseSSL) {
				this->conn = new WASocketConnection("c.whatsapp.net", 443);

				connection = new WAConnection(&this->connMutex, this, this);
				login = new WALogin(connection, new BinTreeNodeReader(connection, conn, WAConnection::dictionary, WAConnection::DICTIONARY_LEN),
						new BinTreeNodeWriter(connection, conn, WAConnection::dictionary, WAConnection::DICTIONARY_LEN, &writerMutex),
						"s.whatsapp.net", this->phoneNumber, std::string(ACCOUNT_RESOURCE) +"-443", base64_decode(pass), nick);
			} else {
				this->conn = new WASocketConnection("c.whatsapp.net", 5222);

				connection = new WAConnection(&this->connMutex, this, this);
				login = new WALogin(connection, new BinTreeNodeReader(connection, conn, WAConnection::dictionary, WAConnection::DICTIONARY_LEN),
						new BinTreeNodeWriter(connection, conn, WAConnection::dictionary, WAConnection::DICTIONARY_LEN, &writerMutex),
						"s.whatsapp.net", this->phoneNumber, std::string(ACCOUNT_RESOURCE) +"-5222", base64_decode(pass), nick);
			}

			std::vector<unsigned char>* nextChallenge = login->login(*this->challenge);
			delete this->challenge;
			this->challenge = nextChallenge;
			connection->setLogin(login);
			connection->setVerboseId(true); // ?
			if (desiredStatus != ID_STATUS_INVISIBLE) {
				connection->sendAvailableForChat();
			}

			LOG("Set status to online");
			this->m_iStatus = desiredStatus;
			ProtoBroadcastAck(0, ACKTYPE_STATUS, ACKRESULT_SUCCESS, (HANDLE )m_iStatus, ID_STATUS_CONNECTING);
			this->ToggleStatusMenuItems(true);

			ForkThread(&WhatsAppProto::ProcessBuddyList, this);

			// #TODO Move out of try block. Exception is expected on disconnect
			bool cont = true;
			while (cont == true)
			{
				this->lastPongTime = time(NULL);
				cont = connection->read();
			}
			LOG("Exit from read-loop");

		CODE_BLOCK_CATCH(WAException)
			error = true;
		CODE_BLOCK_CATCH(exception)
			error = true;
		CODE_BLOCK_CATCH_UNKNOWN
			error = true;
		CODE_BLOCK_END
	}
	LOG("Break out from loop");
}

void WhatsAppProto::sentinelLoop(void*)
{
	int delay = MAX_SILENT_INTERVAL;
	int quietInterval;
	while (WaitForSingleObjectEx(update_loop_lock_, delay * 1000, true) == WAIT_TIMEOUT)
	{
		if (this->m_iStatus != ID_STATUS_OFFLINE && this->connection != NULL && this->m_iDesiredStatus == this->m_iStatus)
		{
			// #TODO Quiet after pong or tree read?
			quietInterval = difftime(time(NULL), this->lastPongTime);
			if (quietInterval >= MAX_SILENT_INTERVAL)
			{
				CODE_BLOCK_TRY
					LOG("send ping");
					this->lastPongTime = time(NULL);
					this->connection->sendPing();
				CODE_BLOCK_CATCH(exception)
				CODE_BLOCK_END
			}
			else
			{
				delay = MAX_SILENT_INTERVAL - quietInterval;
			}
		}
		else
		{
			delay = MAX_SILENT_INTERVAL;
		}
	}
	ResetEvent(update_loop_lock_);
	LOG("Exiting sentinel loop");
}

void WhatsAppProto::onPing(const std::string& id)
{
	if (this->isOnline()) {
		CODE_BLOCK_TRY
			LOG("Sending pong with id %s", id.c_str());
			this->connection->sendPong(id);
		CODE_BLOCK_CATCH_ALL
	}
}
