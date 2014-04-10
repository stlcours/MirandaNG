﻿#ifndef _STEAM_POLL_H_
#define _STEAM_POLL_H_

namespace SteamWebApi
{
	class PollApi : public BaseApi
	{
	public:
		enum POOL_TYPE
		{
			POOL_TYPE_UNKNOWN,
			POOL_TYPE_MESSAGE,
			POOL_TYPE_MYMESSAGE,
			POOL_TYPE_TYPING,
			POOL_TYPE_STATE,
			//POOL_TYPE_RELATIONSHIP
		};

		class PoolItem// : public Result
		{
			friend PollApi;

		private:
			std::string steamId;
			DWORD timestamp;
			POOL_TYPE type;

		public:
			PoolItem() : timestamp(0), type(POOL_TYPE_UNKNOWN) { }

			const char *GetSteamId() const { return steamId.c_str(); }
			const DWORD GetTimestamp() const { return timestamp; }
			POOL_TYPE GetType() const { return type; }
		};

		class Typing : public PoolItem { friend PollApi; };

		class Message : public PoolItem
		{
			friend PollApi;

		private:
			std::wstring text;

		public:
			const wchar_t *GetText() const { return text.c_str(); }
		};

		class State : public PoolItem
		{
			friend PollApi;

		private:
			int status;
			std::wstring nickname;

		public:
			int GetStatus() const { return status; }
			const wchar_t *GetNickname() const { return nickname.c_str(); }
		};

		class PollResult : public Result
		{
			friend PollApi;

		private:
			bool need_relogin;
			UINT32 messageId;
			std::vector<PoolItem*> items;

		public:
			PollResult() : messageId(0), need_relogin(false) { }

			UINT32 GetMessageId() const { return messageId; }
			bool IsNeedRelogin() const { return need_relogin; }
			size_t GetItemCount() const { return items.size(); }
			const PoolItem *GetAt(int idx) const { return items.at(idx); }
		};

		static void PollStatus(HANDLE hConnection, const char *token, const char *sessionId, UINT32 messageId, PollResult *pollResult)
		{
			pollResult->success = false;
			pollResult->need_relogin = false;
			pollResult->items.clear();

			char data[512];
			mir_snprintf(data, SIZEOF(data), "access_token=%s&umqid=%s&message=%u", token, sessionId, messageId);

			SecureHttpRequest request(hConnection, REQUEST_POST, STEAM_API_URL "/ISteamWebUserPresenceOAuth/Poll/v0001");
			request.AddHeader("Content-Type", "application/x-www-form-urlencoded");
			request.SetData(data, strlen(data));
			request.SetTimeout(90000); // may need to encrease timeout
			
			mir_ptr<NETLIBHTTPREQUEST> response(request.Send());
			if (!response)
				return;

			if ((pollResult->status = (HTTP_STATUS)response->resultCode) != HTTP_STATUS_OK)
				return;

			JSONNODE *root = json_parse(response->pData), *node, *child;
			node = json_get(root, "error");
			ptrW error(json_as_string(node));

			if (!lstrcmpi(error, L"Not Logged On"))
			{
				pollResult->need_relogin = true;
				//pollResult->success = true;
				return;
			}
			else
			if (!lstrcmpi(error, L"Timeout"))
			{
				pollResult->messageId = messageId;
				pollResult->success = true;
				return;
			}
			else if (lstrcmpi(error, L"OK"))
				return;

			node = json_get(root, "messagelast");
			pollResult->messageId = json_as_int(node);

			node = json_get(root, "messages");
			root = json_as_array(node);
			if (root != NULL)
			{
				for (int i = 0;; i++)
				{
					child = json_at(root, i);
					if (child == NULL)
						break;

					PoolItem *item = NULL;

					node = json_get(child, "type");
					ptrW type(json_as_string(node));
					if (!lstrcmpi(type, L"saytext") || !lstrcmpi(type, L"emote") ||
						!lstrcmpi(type, L"my_saytext") || !lstrcmpi(type, L"my_emote"))
					{
						Message *message = new Message();

						if (_tcsstr(type, L"my_") == NULL)
							message->type = POOL_TYPE_MESSAGE;
						else
							message->type = POOL_TYPE_MYMESSAGE;

						node = json_get(child, "text");
						if (node != NULL) message->text = json_as_string(node);

						node = json_get(child, "utc_timestamp");
						message->timestamp = atol(ptrA(mir_u2a(json_as_string(node))));

						item = message;
					}
					else if(!lstrcmpi(type, L"typing"))
					{
						item = new Typing();
						item->type = POOL_TYPE_TYPING;
					}
					else if (!lstrcmpi(type, L"personastate"))
					{
						State *state = new State();
						state->type = POOL_TYPE_STATE;

						node = json_get(child, "persona_state");
						if (node != NULL) state->status = json_as_int(node);

						node = json_get(child, "persona_name");
						if (node != NULL) state->nickname = json_as_string(node);

						item = state;
					}
					/*else if (!lstrcmpi(type, L"personarelationship"))
					{
						type = POOL_TYPE_RELATIONSHIP;
					}*/
					/*else if (!lstrcmpi(type, L"leftconversation"))
					{
					}*/
					else
					{
						int z = 0;
					}

					node = json_get(child, "steamid_from");
					item->steamId = ptrA(mir_u2a(json_as_string(node)));

					node = json_get(child, "utc_timestamp");
					item->timestamp = atol(ptrA(mir_u2a(json_as_string(node))));

					if (item != NULL)
						pollResult->items.push_back(item);
				}
			}

			pollResult->success = true;
		}
	};
}


#endif //_STEAM_POLL_H_