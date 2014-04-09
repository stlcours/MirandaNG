﻿#ifndef _STEAM_AUTHORIZATION_H_
#define _STEAM_AUTHORIZATION_H_

namespace SteamWebApi
{
	class AuthorizationApi : public BaseApi
	{
	public:

		class AuthResult : public Result
		{
			friend AuthorizationApi;

		private:
			std::string steamid;
			std::string token;
			std::string cookie;

			std::string emailauth;
			std::string emaildomain;
			std::string emailsteamid;

			std::string captchagid;
			std::string captcha_text;

			std::wstring message;

			bool captcha_needed;
			bool emailauth_needed;

		public:
			AuthResult()
			{
				captcha_needed = false;
				emailauth_needed = false;
				captchagid = "-1";
			}

			bool IsCaptchaNeeded() const { return captcha_needed; }
			bool IsEmailAuthNeeded() const { return emailauth_needed; }
			const char *GetSteamid() const { return steamid.c_str(); }
			const char *GetToken() const { return token.c_str(); }
			const char *GetCookie() const { return cookie.c_str(); }
			const char *GetAuthId() const { return emailauth.c_str(); }
			const char *GetAuthCode() const { return emailsteamid.c_str(); }
			const char *GetEmailDomain() const { return emaildomain.c_str(); }
			const char *GetCaptchaId() const { return captchagid.c_str(); }
			const wchar_t *GetMessage() const { return message.c_str(); }

			void SetAuthCode(char *code)
			{
				emailauth = code;
			}

			void SetCaptchaText(char *text)
			{
				captcha_text = text;
			}
		};

		static void Authorize(HANDLE hConnection, const wchar_t *username, const char *password, const char *timestamp, AuthResult *authResult)
		{
			authResult->success = false;
			authResult->captcha_needed = false;
			authResult->emailauth_needed = false;

			ptrA base64Username(mir_urlEncode(ptrA(mir_utf8encodeW(username))));

			CMStringA data;
			data.AppendFormat("username=%s", base64Username);
			data.AppendFormat("&password=%s", ptrA(mir_urlEncode(password)));
			data.AppendFormat("&emailauth=%s", ptrA(mir_urlEncode(authResult->emailauth.c_str())));
			data.AppendFormat("&emailsteamid=%s", authResult->emailsteamid.c_str());
			data.AppendFormat("&captchagid=%s", authResult->captchagid.c_str());
			data.AppendFormat("&captcha_text=%s", ptrA(mir_urlEncode(authResult->captcha_text.c_str())));
			data.AppendFormat("&rsatimestamp=%s", timestamp);
			data.AppendFormat("&oauth_scope=%s", "read_profile write_profile read_client write_client");
			data.Append("&oauth_client_id=DE45CD61");

			HttpRequest request(hConnection, REQUEST_POST, STEAM_COM_URL "/mobilelogin/dologin");
			request.AddHeader("Content-Type", "application/x-www-form-urlencoded");
			request.SetData(data.GetBuffer(), data.GetLength());

			mir_ptr<NETLIBHTTPREQUEST> response(request.Send());
			if (!response)
				return;

			if ((authResult->status = (HTTP_STATUS)response->resultCode) != HTTP_STATUS_OK)
				return;

			JSONNODE *root = json_parse(response->pData), *node;

			node = json_get(root, "success");
			authResult->success = json_as_bool(node) > 0;
			if (!authResult->success)
			{
				node = json_get(root, "emailauth_needed");
				authResult->emailauth_needed = json_as_bool(node) > 0;
				if (authResult->emailauth_needed)
				{
					node = json_get(root, "emailsteamid");
					authResult->emailsteamid = ptrA(mir_u2a(json_as_string(node)));

					node = json_get(root, "emaildomain");
					authResult->emaildomain = ptrA(mir_utf8encodeW(json_as_string(node)));
				}

				node = json_get(root, "captcha_needed");
				authResult->captcha_needed = json_as_bool(node) > 0;
				if (authResult->captcha_needed)
				{
					node = json_get(root, "captcha_gid");
					authResult->captchagid = ptrA(mir_u2a(json_as_string(node)));
				}

				if (!authResult->emailauth_needed && !authResult->captcha_needed)
				{
					node = json_get(root, "message");
					authResult->message = json_as_string(node);
				}
			}
			else
			{
				node = json_get(root, "login_complete");
				if (!json_as_bool(node))
					return;

				node = json_get(root, "oauth");
				root = json_parse(ptrA(mir_u2a(json_as_string(node))));

				node = json_get(root, "steamid");
				authResult->steamid = ptrA(mir_u2a(json_as_string(node)));

				node = json_get(root, "oauth_token");
				authResult->token = ptrA(mir_u2a(json_as_string(node)));

				/*node = json_get(root, "webcookie");
				authResult->cookie = ptrA(mir_u2a(json_as_string(node)));*/

				authResult->success = true;
				authResult->captcha_needed = false;
				authResult->emailauth_needed = false;
			}
		}
	};
}


#endif //_STEAM_AUTHORIZATION_H_