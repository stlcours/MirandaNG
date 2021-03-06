// Copyright � 2010-2012 sss
// 
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#include "stdafx.h"

wstring new_key;
MCONTACT new_key_hcnt = NULL;
boost::mutex new_key_hcnt_mutex;
bool _terminate = false;
int returnNoError(MCONTACT hContact);

std::list<HANDLE> sent_msgs;

void RecvMsgSvc_func(MCONTACT hContact, std::wstring str, char *msg, DWORD, DWORD timestamp)
{
	DWORD dbflags = DBEF_UTF;
	{
		// check for gpg related data
		wstring::size_type s1 = str.find(L"-----BEGIN PGP MESSAGE-----");
		wstring::size_type s2 = str.find(L"-----END PGP MESSAGE-----");
		if (s2 != wstring::npos && s1 != wstring::npos)
		{ //this is generic encrypted data block
			if (!isContactSecured(hContact))
			{
				if (bDebugLog)
					debuglog << std::string(time_str() + ": info: received encrypted message from: " + toUTF8(pcli->pfnGetContactDisplayName(hContact, 0)) + " with turned off encryption");
				if (MessageBox(0, TranslateT("We received encrypted message from contact with encryption turned off.\nDo you want to turn on encryption for this contact?"), TranslateT("Warning"), MB_YESNO) == IDYES)
				{
					if (!isContactHaveKey(hContact))
					{
						void ShowLoadPublicKeyDialog();
						item_num = 0;		 //black magic here
						user_data[1] = hContact;
						ShowLoadPublicKeyDialog();
					}
					else
					{
						db_set_b(db_mc_isMeta(hContact) ? metaGetMostOnline(hContact) : hContact, szGPGModuleName, "GPGEncryption", 1);
						setSrmmIcon(hContact);
						setClistIcon(hContact);
					}
					if (isContactHaveKey(hContact))
					{
						db_set_b(db_mc_isMeta(hContact) ? metaGetMostOnline(hContact) : hContact, szGPGModuleName, "GPGEncryption", 1);
						setSrmmIcon(hContact);
						setClistIcon(hContact);
					}
				}
				else if (MessageBox(0, TranslateT("Do you want to try to decrypt encrypted message?"), TranslateT("Warning"), MB_YESNO) == IDNO)
				{
					HistoryLog(hContact, db_event(msg, timestamp, 0, dbflags));
					return;
				}
			}
			else if (bDebugLog)
				debuglog << std::string(time_str() + ": info: received encrypted message from: " + toUTF8(pcli->pfnGetContactDisplayName(hContact, 0)));
			boost::algorithm::erase_all(str, "\r");
			s2 += mir_wstrlen(L"-----END PGP MESSAGE-----");

			ptrW ptszHomePath(UniGetContactSettingUtf(NULL, szGPGModuleName, "szHomePath", L""));
			wstring encfile = toUTF16(get_random(10));
			wstring decfile = toUTF16(get_random(10));
			{
				wstring path = wstring(ptszHomePath) + L"\\tmp\\" + encfile;
				if(!bDebugLog)
				{
					boost::system::error_code e;
					boost::filesystem::remove(path, e);
				}

				{
					const int timeout = 5000, step = 100;
					int count = 0;

					fstream f(path.c_str(), std::ios::out);
					while (!f.is_open())
					{
						boost::this_thread::sleep(boost::posix_time::milliseconds(step));
						count += step;
						if(count >= timeout)
						{
							db_set_b(hContact, szGPGModuleName, "GPGEncryption", 0);
							setSrmmIcon(hContact);
							setClistIcon(hContact);
							debuglog<<std::string(time_str() + "info: failed to create temporary file for decryption, disabling gpg for contact to avoid deadlock");
							return;
						}
						f.open(path.c_str(), std::ios::out);
					}
					char *tmp = mir_u2a(str.substr(s1, s2 - s1).c_str());
					f << tmp;
					mir_free(tmp);
					f.close();
				}
				extern wchar_t *password;
				string out;
				DWORD code;
				std::vector<wstring> cmd;
				cmd.push_back(L"--batch");
				{
					char *inkeyid = UniGetContactSettingUtf(db_mc_isMeta(hContact) ? metaGetMostOnline(hContact) : hContact, szGPGModuleName, "InKeyID", "");
					wchar_t *pass = NULL;
					if (inkeyid[0]) {
						string dbsetting = "szKey_";
						dbsetting += inkeyid;
						dbsetting += "_Password";
						pass = UniGetContactSettingUtf(NULL, szGPGModuleName, dbsetting.c_str(), L"");
						if (pass[0] && bDebugLog)
							debuglog << std::string(time_str() + ": info: found password in database for key ID: " + inkeyid + ", trying to decrypt message from " + toUTF8(pcli->pfnGetContactDisplayName(hContact, 0)) + " with password");
					}
					else
					{
						pass = UniGetContactSettingUtf(NULL, szGPGModuleName, "szKeyPassword", L"");
						if (pass[0] && bDebugLog)
							debuglog << std::string(time_str() + ": info: found password for all keys in database, trying to decrypt message from " + toUTF8(pcli->pfnGetContactDisplayName(hContact, 0)) + " with password");
					}
					if (pass && pass[0]) {
						cmd.push_back(L"--passphrase");
						cmd.push_back(pass);
					}
					else if (password && password[0])
					{
						if (bDebugLog)
							debuglog << std::string(time_str() + ": info: found password in memory, trying to decrypt message from " + toUTF8(pcli->pfnGetContactDisplayName(hContact, 0)) + " with password");
						cmd.push_back(L"--passphrase");
						cmd.push_back(password);
					}
					else if (bDebugLog)
						debuglog << std::string(time_str() + ": info: passwords not found in database or memory, trying to decrypt message from " + toUTF8(pcli->pfnGetContactDisplayName(hContact, 0)) + " with out password");
					mir_free(pass);
					mir_free(inkeyid);
				}

				if(!bDebugLog)
				{
					boost::system::error_code e;
					boost::filesystem::remove(wstring(ptszHomePath) + L"\\tmp\\" + decfile, e);
				}

				cmd.push_back(L"--output");
				cmd.push_back(std::wstring(ptszHomePath) + L"\\tmp\\" + decfile);
				cmd.push_back(L"-d");
				cmd.push_back(L"-a");
				cmd.push_back(path);
				gpg_execution_params params(cmd);
				pxResult result;
				params.out = &out;
				params.code = &code;
				params.result = &result;
				if (!gpg_launcher(params))
				{
					if(!bDebugLog)
					{
						boost::system::error_code e;
						boost::filesystem::remove(path, e);
					}
					HistoryLog(hContact, db_event(msg, timestamp, 0, dbflags));
					BYTE enc = db_get_b(hContact, szGPGModuleName, "GPGEncryption", 0);
					db_set_b(hContact, szGPGModuleName, "GPGEncryption", 0);
					ProtoChainSend(hContact, PSS_MESSAGE, 0, (LPARAM)"Unable to decrypt PGP encrypted message");
					HistoryLog(hContact, db_event("Error message sent", 0, 0, DBEF_SENT));
					db_set_b(hContact, szGPGModuleName, "GPGEncryption", enc);
					return;
				}
				if (result == pxNotFound)
				{
					if(!bDebugLog)
					{
						boost::system::error_code e;
						boost::filesystem::remove(path, e);
					}

					HistoryLog(hContact, db_event(msg, timestamp, 0, dbflags));
					return;
				}
/*				if (result == pxSuccessExitCodeInvalid) //sometime we have invalid return code after succesful decryption, this should be non-fatal at  least
				{
					if(!bDebugLog)
					{
						boost::system::error_code e;
						boost::filesystem::remove(path, e);
					}

					HistoryLog(hContact, db_event(msg, timestamp, 0, dbflags));
					HistoryLog(hContact, db_event(Translate("failed to decrypt message, GPG returned error, turn on debug log for more details"), timestamp, 0, 0));
					return;
				} */

				//TODO: check gpg output for errors
				_terminate = false;
				while (out.find("public key decryption failed: bad passphrase") != string::npos)
				{
					if (bDebugLog)
						debuglog << std::string(time_str() + ": info: failed to decrypt messaage from " + toUTF8(pcli->pfnGetContactDisplayName(hContact, 0)) + " password needed, trying to get one");
					if (_terminate) {
						BYTE enc = db_get_b(hContact, szGPGModuleName, "GPGEncryption", 0);
						db_set_b(hContact, szGPGModuleName, "GPGEncryption", 0);
						ProtoChainSend(hContact, PSS_MESSAGE, 0, (LPARAM)"Unable to decrypt PGP encrypted message");
						HistoryLog(hContact, db_event("Error message sent", 0, 0, DBEF_SENT));
						db_set_b(hContact, szGPGModuleName, "GPGEncryption", enc);
						break;
					}
					{ //save inkey id
						string::size_type s = out.find(" encrypted with ");
						s = out.find(" ID ", s);
						s += mir_strlen(" ID ");
						db_set_s(db_mc_isMeta(hContact) ? metaGetMostOnline(hContact) : hContact, szGPGModuleName, "InKeyID", out.substr(s, out.find(",", s) - s).c_str());
					}
					void ShowLoadKeyPasswordWindow();
					new_key_hcnt_mutex.lock();
					new_key_hcnt = hContact;
					ShowLoadKeyPasswordWindow();
					std::vector<wstring> cmd2 = cmd;
					if (password)
					{
						if (bDebugLog)
							debuglog << std::string(time_str() + ": info: found password in memory, trying to decrypt message from " + toUTF8(pcli->pfnGetContactDisplayName(hContact, 0)));
						std::vector<wstring> tmp3;
						tmp3.push_back(L"--passphrase");
						tmp3.push_back(password);
						cmd2.insert(cmd2.begin(), tmp3.begin(), tmp3.end());
					}
					out.clear();
					gpg_execution_params params2(cmd2);
					pxResult result2;
					params2.out = &out;
					params2.code = &code;
					params2.result = &result2;
					if (!gpg_launcher(params2)) {
						if(!bDebugLog)
						{
							boost::system::error_code e;
							boost::filesystem::remove(path, e);
						}

						HistoryLog(hContact, db_event(msg, timestamp, 0, dbflags));
						BYTE enc = db_get_b(hContact, szGPGModuleName, "GPGEncryption", 0);
						db_set_b(hContact, szGPGModuleName, "GPGEncryption", 0);
						ProtoChainSend(hContact, PSS_MESSAGE, 0, (LPARAM)"Unable to decrypt PGP encrypted message");
						HistoryLog(hContact, db_event("Error message sent", 0, 0, DBEF_SENT));
						db_set_b(hContact, szGPGModuleName, "GPGEncryption", enc);
						return;
					}
					if (result2 == pxNotFound)
					{
						if(!bDebugLog)
						{
							boost::system::error_code e;
							boost::filesystem::remove(path, e);
						}

						HistoryLog(hContact, db_event(msg, timestamp, 0, dbflags));
						return;
					}
				}
				out.clear();
				if (!gpg_launcher(params))
				{
					if(!bDebugLog)
					{
						boost::system::error_code e;
						boost::filesystem::remove(path, e);
					}

					HistoryLog(hContact, db_event(msg, timestamp, 0, dbflags));
					BYTE enc = db_get_b(hContact, szGPGModuleName, "GPGEncryption", 0);
					db_set_b(hContact, szGPGModuleName, "GPGEncryption", 0);
					ProtoChainSend(hContact, PSS_MESSAGE, 0, (LPARAM)"Unable to decrypt PGP encrypted message");
					HistoryLog(hContact, db_event("Error message sent", 0, 0, DBEF_SENT));
					db_set_b(hContact, szGPGModuleName, "GPGEncryption", enc);
					return;
				}
				if (result == pxNotFound)
				{
					if(!bDebugLog)
					{
						boost::system::error_code e;
						boost::filesystem::remove(path, e);
					}

					HistoryLog(hContact, db_event(msg, timestamp, 0, dbflags));
				}
				if(!bDebugLog)
				{
					boost::system::error_code e;
					boost::filesystem::remove(wstring(ptszHomePath) + L"\\tmp\\" + encfile, e);
				}

				if (!boost::filesystem::exists(wstring(ptszHomePath) + L"\\tmp\\" + decfile))
				{
					string str1 = msg;
					str1.insert(0, "Received unencrypted message:\n");
					if (bDebugLog)
						debuglog << std::string(time_str() + ": info: Failed to decrypt GPG encrypted message.");

					ptrA tmp4((char*)mir_alloc(sizeof(char)*(str1.length() + 1)));
					mir_strcpy(tmp4, str1.c_str());
					HistoryLog(hContact, db_event(msg, timestamp, 0, dbflags));
					BYTE enc = db_get_b(hContact, szGPGModuleName, "GPGEncryption", 0);
					db_set_b(hContact, szGPGModuleName, "GPGEncryption", 0);
					ProtoChainSend(hContact, PSS_MESSAGE, 0, (LPARAM)"Unable to decrypt PGP encrypted message");
					HistoryLog(hContact, db_event("Error message sent", 0, 0, DBEF_SENT));
					db_set_b(hContact, szGPGModuleName, "GPGEncryption", enc);
					return;
				}

				str.clear();

				wstring tszDecPath = wstring(ptszHomePath) + L"\\tmp\\" + decfile;
				{
					fstream f(tszDecPath.c_str(), std::ios::in | std::ios::ate | std::ios::binary);
					if (f.is_open())
					{
						size_t size = f.tellg();
						char *tmp = new char[size + 1];
						f.seekg(0, std::ios::beg);
						f.read(tmp, size);
						tmp[size] = '\0';
						toUTF16(tmp);
						str.append(toUTF16(tmp));
						delete[] tmp;
						f.close();
						if(!bDebugLog)
						{
							boost::system::error_code ec;
							boost::filesystem::remove(tszDecPath, ec);
							if(ec)
							{
								//TODO: handle error
							}
						}
					}
				}
				if (str.empty())
				{
					string szMsg = msg;
					szMsg.insert(0, "Failed to decrypt GPG encrypted message.\nMessage body for manual decryption:\n");
					if (bDebugLog)
						debuglog << std::string(time_str() + ": info: Failed to decrypt GPG encrypted message.");

					HistoryLog(hContact, db_event(msg, timestamp, 0, dbflags));
					BYTE enc = db_get_b(hContact, szGPGModuleName, "GPGEncryption", 0);
					db_set_b(hContact, szGPGModuleName, "GPGEncryption", 0);
					ProtoChainSend(hContact, PSS_MESSAGE, 0, (LPARAM)"Unable to decrypt PGP encrypted message");
					HistoryLog(hContact, db_event("Error message sent", 0, 0, DBEF_SENT));
					db_set_b(hContact, szGPGModuleName, "GPGEncryption", enc);
					return;
				}

				fix_line_term(str);
				if (bAppendTags)
				{
					str.insert(0, inopentag);
					str.append(inclosetag);
				}

				char *tmp = mir_strdup(toUTF8(str).c_str());
				HistoryLog(hContact, db_event(tmp, timestamp, 0, dbflags));
				mir_free(tmp);
				return;
			}
		}
	}
	if (db_get_b(db_mc_isMeta(hContact) ? metaGetMostOnline(hContact) : hContact, szGPGModuleName, "GPGEncryption", 0))
	{
		HistoryLog(hContact, db_event(msg, timestamp, 0, dbflags | DBEF_READ));
		return;
	}
	HistoryLog(hContact, db_event(msg, timestamp, 0, dbflags));
	return;
}

INT_PTR RecvMsgSvc(WPARAM w, LPARAM l)
{
	CCSDATA *ccs = (CCSDATA*)l;
	if (!ccs)
		return Proto_ChainRecv(w, ccs);
	PROTORECVEVENT *pre = (PROTORECVEVENT*)(ccs->lParam);
	if (!pre)
		return Proto_ChainRecv(w, ccs);
	char *msg = pre->szMessage;
	if (!msg)
		return Proto_ChainRecv(w, ccs);
	DWORD dbflags = DBEF_UTF;
	if (db_mc_isMeta(ccs->hContact))
	{
		if (!strstr(msg, "-----BEGIN PGP MESSAGE-----"))
			return Proto_ChainRecv(w, ccs);
		else
		{
			if (bDebugLog)
				debuglog << std::string(time_str() + ": info: blocked pgp message to metacontact:" + toUTF8(pcli->pfnGetContactDisplayName(ccs->hContact, 0)));
			return 0;
		}
	}
	wstring str = toUTF16(msg);
	size_t s1, s2;
	if (bAutoExchange && (str.find(L"-----PGP KEY RESPONSE-----") != wstring::npos))
	{
		if (bDebugLog)
			debuglog << std::string(time_str() + ": info(autoexchange): parsing key response:" + toUTF8(pcli->pfnGetContactDisplayName(ccs->hContact, 0)));
		s2 = str.find(L"-----END PGP PUBLIC KEY BLOCK-----");
		s1 = str.find(L"-----BEGIN PGP PUBLIC KEY BLOCK-----");
		if (s1 != wstring::npos && s2 != wstring::npos)
		{
			if (bDebugLog)
				debuglog << std::string(time_str() + ": info(autoexchange): found pubkey block:" + toUTF8(pcli->pfnGetContactDisplayName(ccs->hContact, 0)));
			s2 += mir_wstrlen(L"-----END PGP PUBLIC KEY BLOCK-----");
			db_set_ws(ccs->hContact, szGPGModuleName, "GPGPubKey", str.substr(s1, s2 - s1).c_str());
			{ //gpg execute block
				std::vector<wstring> cmd;
				wchar_t tmp2[MAX_PATH] = { 0 };
				string output;
				DWORD exitcode;
				{
					ptrW ptmp(UniGetContactSettingUtf(NULL, szGPGModuleName, "szHomePath", L""));
					mir_wstrcpy(tmp2, ptmp);
					mir_free(ptmp);
					mir_wstrcat(tmp2, L"\\");
					wchar_t *tmp3 = mir_a2u(get_random(5).c_str());
					mir_wstrcat(tmp2, tmp3);
					mir_wstrcat(tmp2, L".asc");
					mir_free(tmp3);
					//mir_wstrcat(tmp2, L"temporary_exported.asc");
					if(!bDebugLog)
					{
						boost::system::error_code e;
						boost::filesystem::remove(tmp2, e);
					}
					wfstream f(tmp2, std::ios::out);
					{
						const int timeout = 5000, step = 100;
						int count = 0;
						while (!f.is_open())
						{
							boost::this_thread::sleep(boost::posix_time::milliseconds(step));
							count += step;
							if(count >= timeout)
							{
								db_set_b(ccs->hContact, szGPGModuleName, "GPGEncryption", 0);
								setSrmmIcon(ccs->hContact);
								setClistIcon(ccs->hContact);
								debuglog<<std::string(time_str() + "info: failed to create temporary file for decryption, disabling gpg for contact to avoid deadlock");
								return 1;
							}
							f.open(tmp2, std::ios::out);
						}
					}
					ptmp = UniGetContactSettingUtf(ccs->hContact, szGPGModuleName, "GPGPubKey", L"");
					f << (wchar_t*)ptmp;
					f.close();
					cmd.push_back(L"--batch");
					cmd.push_back(L"--import");
					cmd.push_back(tmp2);
				}
				gpg_execution_params params(cmd);
				pxResult result;
				params.out = &output;
				params.code = &exitcode;
				params.result = &result;
				if (!gpg_launcher(params))
					return 1;
				if(!bDebugLog)
				{
					boost::system::error_code e;
					boost::filesystem::remove(tmp2, e);
				}
				if (result == pxNotFound)
					return 1;
/*				if (result == pxSuccessExitCodeInvalid) //sometime we have invalid return code after succesful decryption, this should be non-fatal at  least
				{
					HistoryLog(ccs->hContact, db_event(Translate("failed to decrypt message, GPG returned error, turn on debug log for more details")));
					return 1;
				} */
				{
					char *tmp = NULL;
					s1 = output.find("gpg: key ") + mir_strlen("gpg: key ");
					s2 = output.find(":", s1);
					db_set_s(ccs->hContact, szGPGModuleName, "KeyID", output.substr(s1, s2 - s1).c_str());
					s2 += 2;
					s1 = output.find("“", s2);
					if (s1 == string::npos)
					{
						s1 = output.find("\"", s2);
						s1 += 1;
					}
					else s1 += 3;

					if ((s2 = output.find("(", s1)) == string::npos)
						s2 = output.find("<", s1);
					else if (s2 > output.find("<", s1))
						s2 = output.find("<", s1);
					tmp = (char*)mir_alloc(output.substr(s1, s2 - s1 - 1).length() + 1);
					mir_strcpy(tmp, output.substr(s1, s2 - s1 - 1).c_str());
					mir_utf8decode(tmp, 0);
					db_set_s(ccs->hContact, szGPGModuleName, "KeyMainName", tmp);
					mir_free(tmp);
					if ((s1 = output.find(")", s2)) == string::npos)
						s1 = output.find(">", s2);
					else if (s1 > output.find(">", s2))
						s1 = output.find(">", s2);
					s2++;
					if (output[s1] == ')')
					{
						tmp = (char*)mir_alloc(output.substr(s2, s1 - s2).length() + 1);
						mir_strcpy(tmp, output.substr(s2, s1 - s2).c_str());
						mir_utf8decode(tmp, 0);
						db_set_s(ccs->hContact, szGPGModuleName, "KeyComment", tmp);
						mir_free(tmp);
						s1 += 3;
						s2 = output.find(">", s1);
						tmp = (char*)mir_alloc(output.substr(s1, s2 - s1).length() + 1);
						mir_strcpy(tmp, output.substr(s1, s2 - s1).c_str());
						mir_utf8decode(tmp, 0);
						db_set_s(ccs->hContact, szGPGModuleName, "KeyMainEmail", tmp);
						mir_free(tmp);
					}
					else
					{
						tmp = (char*)mir_alloc(output.substr(s2, s1 - s2).length() + 1);
						mir_strcpy(tmp, output.substr(s2, s1 - s2).c_str());
						mir_utf8decode(tmp, 0);
						db_set_s(ccs->hContact, szGPGModuleName, "KeyMainEmail", output.substr(s2, s1 - s2).c_str());
						mir_free(tmp);
					}
					db_set_b(ccs->hContact, szGPGModuleName, "GPGEncryption", 1);
					db_set_b(ccs->hContact, szGPGModuleName, "bAlwatsTrust", 1);
					setSrmmIcon(ccs->hContact);
					setClistIcon(ccs->hContact);
					if (db_mc_isSub(ccs->hContact))
					{
						setSrmmIcon(db_mc_getMeta(ccs->hContact));
						setClistIcon(db_mc_getMeta(ccs->hContact));
					}
					HistoryLog(ccs->hContact, "PGP Encryption turned on by key autoexchange feature");
				}
			}
			return 1;
		}
	}
	if (((s2 = str.find(L"-----END PGP PUBLIC KEY BLOCK-----")) == wstring::npos) || ((s1 = str.find(L"-----BEGIN PGP PUBLIC KEY BLOCK-----")) == wstring::npos))
	{
		s2 = str.find(L"-----END PGP PRIVATE KEY BLOCK-----");
		s1 = str.find(L"-----BEGIN PGP PRIVATE KEY BLOCK-----");
	}
	if ((s2 != wstring::npos) && (s1 != wstring::npos))
	{  //this is public key
		if (bDebugLog)
			debuglog << std::string(time_str() + ": info: received key from: " + toUTF8(pcli->pfnGetContactDisplayName(ccs->hContact, 0)));
		s1 = 0;
		while ((s1 = str.find(L"\r", s1)) != wstring::npos)
			str.erase(s1, 1);
		void ShowNewKeyDialog();
		if (((s2 = str.find(L"-----END PGP PUBLIC KEY BLOCK-----")) != wstring::npos) && ((s1 = str.find(L"-----BEGIN PGP PUBLIC KEY BLOCK-----")) != wstring::npos))
			s2 += mir_wstrlen(L"-----END PGP PUBLIC KEY BLOCK-----");
		else if (((s2 = str.find(L"-----BEGIN PGP PRIVATE KEY BLOCK-----")) != wstring::npos) && ((s1 = str.find(L"-----END PGP PRIVATE KEY BLOCK-----")) != wstring::npos))
			s2 += mir_wstrlen(L"-----END PGP PRIVATE KEY BLOCK-----");
		new_key.append(str.substr(s1, s2 - s1));
		//new_key_hcnt_mutex.lock();
		new_key_hcnt = ccs->hContact;
		ShowNewKeyDialog();
		HistoryLog(ccs->hContact, db_event(msg, 0, 0, dbflags));
		return 0;
	}
	if (bAutoExchange && strstr(msg, "-----PGP KEY REQUEST-----") && gpg_valid && gpg_keyexist)
	{
		if (bDebugLog)
			debuglog << std::string(time_str() + ": info(autoexchange): received key request from: " + toUTF8(pcli->pfnGetContactDisplayName(ccs->hContact, 0)));

		ptrA tmp(UniGetContactSettingUtf(NULL, szGPGModuleName, "GPGPubKey", ""));
		if (tmp[0]) {
			int enc_state = db_get_b(ccs->hContact, szGPGModuleName, "GPGEncryption", 0);
			if (enc_state)
				db_set_b(ccs->hContact, szGPGModuleName, "GPGEncryption", 0);

			string str1 = "-----PGP KEY RESPONSE-----";
			str1.append(tmp);
			ProtoChainSend(ccs->hContact, PSS_MESSAGE, 0, (LPARAM)str1.c_str());
			if (enc_state)
				db_set_b(ccs->hContact, szGPGModuleName, "GPGEncryption", 1);
		}
		return 0;
	}
	else if (!isContactHaveKey(ccs->hContact) && bAutoExchange && gpg_valid && gpg_keyexist)
	{
		char *proto = GetContactProto(ccs->hContact);
		DWORD uin = db_get_dw(ccs->hContact, proto, "UIN", 0);
		if (uin)
		{
			if (ProtoServiceExists(proto, PS_ICQ_CHECKCAPABILITY))
			{
				ICQ_CUSTOMCAP cap = { 0 };
				strncpy(cap.caps, "GPGAutoExchange", sizeof(cap.caps));
				if (CallProtoService(proto, PS_ICQ_CHECKCAPABILITY, (WPARAM)ccs->hContact, (LPARAM)&cap))
				{
					ProtoChainSend(ccs->hContact, PSS_MESSAGE, 0, (LPARAM)"-----PGP KEY REQUEST-----");
					return 0;
				}
			}
		}
		else
		{
			wchar_t *jid = UniGetContactSettingUtf(ccs->hContact, proto, "jid", L"");
			if (jid[0])
			{
				extern list <JabberAccount*> Accounts;
				list<JabberAccount*>::iterator end = Accounts.end();
				for (list<JabberAccount*>::iterator p = Accounts.begin(); p != end; p++)
				{
					wchar_t *caps = (*p)->getJabberInterface()->GetResourceFeatures(jid);
					if (caps)
					{
						wstring str1;
						for (int i = 0;; i++)
						{
							str1.push_back(caps[i]);
							if (caps[i] == '\0')
								if (caps[i + 1] == '\0')
									break;
						}
						mir_free(caps);
						if (str1.find(L"GPG_Key_Auto_Exchange:0") != string::npos)
						{
							ProtoChainSend(ccs->hContact, PSS_MESSAGE, 0, (LPARAM)"-----PGP KEY REQUEST-----");
							return 0;
						}
					}
				}
			}
			mir_free(jid);
		}
	}
	if (!strstr(msg, "-----BEGIN PGP MESSAGE-----"))
		return Proto_ChainRecv(w, ccs);

	new boost::thread(boost::bind(RecvMsgSvc_func, ccs->hContact, str, msg, (DWORD)ccs->wParam, pre->timestamp));
	return 0;
}

void SendMsgSvc_func(MCONTACT hContact, char *msg, DWORD flags)
{
	wstring str = toUTF16(msg);
	if (bStripTags && bAppendTags) {
		if (bDebugLog)
			debuglog << std::string(time_str() + ": info: stripping tags in outgoing message, name: " + toUTF8(pcli->pfnGetContactDisplayName(hContact, 0)));
		strip_tags(str);
	}
	/*	for(std::wstring::size_type i = str.find(L"\r\n"); i != std::wstring::npos; i = str.find(L"\r\n", i+1))
			str.replace(i, 2, L"\n"); */
	string out;
	DWORD code;
	wstring file = toUTF16(get_random(10)), path;
	std::vector<std::wstring> cmd;
	extern bool bJabberAPI, bIsMiranda09;
	{
		wchar_t *tmp2;
		{
			char *tmp = UniGetContactSettingUtf(hContact, szGPGModuleName, "KeyID", "");
			if (!tmp[0]) {
				mir_free(tmp);
				HistoryLog(hContact, db_event("Failed to encrypt message with GPG (not found key for encryption in db)", 0, 0, DBEF_SENT));
				ProtoChainSend(hContact, PSS_MESSAGE, flags, (LPARAM)msg);
				return;
			}
			if (!bJabberAPI || !bIsMiranda09) //force jabber to handle encrypted message by itself
			{
				cmd.push_back(L"--comment");
				cmd.push_back(L"\"\"");
				cmd.push_back(L"--no-version");
			}
			if (db_get_b(hContact, szGPGModuleName, "bAlwaysTrust", 0)) {
				cmd.push_back(L"--trust-model");
				cmd.push_back(L"always");
			}
			cmd.push_back(L"--batch");
			cmd.push_back(L"--yes");
			cmd.push_back(L"-eatr");
			tmp2 = mir_a2u(tmp);
			mir_free(tmp);
		}
		cmd.push_back(tmp2);
		mir_free(tmp2);
	}
	{
		wchar_t *tmp2 = UniGetContactSettingUtf(NULL, szGPGModuleName, "szHomePath", L"");
		path = tmp2;
		cmd.push_back(std::wstring(tmp2) + L"\\tmp\\" + file);
		mir_free(tmp2);
	}
	path += L"\\tmp\\";
	path += file;
	const int timeout = 5000, step = 100;
	int count = 0;
	{
		fstream f(path.c_str(), std::ios::out);
		while (!f.is_open()) {
			boost::this_thread::sleep(boost::posix_time::milliseconds(step));
			count += step;
			if(count >= timeout)
			{
				db_set_b(hContact, szGPGModuleName, "GPGEncryption", 0); //disable encryption
				setSrmmIcon(hContact);
				setClistIcon(hContact);
				debuglog << std::string(time_str() + ": info: failed to create temporary file for encryption, disabling encryption to avoid deadlock");
				break;
			}
			f.open(path.c_str(), std::ios::out);
		}
		if(count < timeout)
		{
			std::string tmp = toUTF8(str);
			f.write(tmp.c_str(), tmp.size());
			f.close();
		}
	}
	pxResult result;
	{
		gpg_execution_params params(cmd);
		params.out = &out;
		params.code = &code;
		params.result = &result;
		if (!gpg_launcher(params)) {
			//mir_free(msg);
			ProtoChainSend(hContact, PSS_MESSAGE, flags, (LPARAM)msg);
			return;
		}
		if (result == pxNotFound) {
			//mir_free(msg);
			ProtoChainSend(hContact, PSS_MESSAGE, flags, (LPARAM)msg);
			return;
		}
	}

	if (out.find("There is no assurance this key belongs to the named user") != string::npos) {
		out.clear();
		if (MessageBox(0, TranslateT("We're trying to encrypt with untrusted key. Do you want to trust this key permanently?"), TranslateT("Warning"), MB_YESNO) == IDYES) {
			db_set_b(hContact, szGPGModuleName, "bAlwaysTrust", 1);
			std::vector<std::wstring> tmp;
			tmp.push_back(L"--trust-model");
			tmp.push_back(L"always");
			cmd.insert(cmd.begin(), tmp.begin(), tmp.end());
			gpg_execution_params params(cmd);
			params.out = &out;
			params.code = &code;
			params.result = &result;
			if (!gpg_launcher(params)) {
				ProtoChainSend(hContact, PSS_MESSAGE, flags, (LPARAM)msg);
				return;
			}
			if (result == pxNotFound) {
				ProtoChainSend(hContact, PSS_MESSAGE, flags, (LPARAM)msg);
				return;
			}
			//TODO: check gpg output for errors
		}
		else return;
	}
//	if (result == pxSuccessExitCodeInvalid) { //sometims gpg return error after succesful operation, this should be non-fatal at  least
		//mir_free(msg);
//		HistoryLog(hContact, db_event(Translate("failed to encrypt message, GPG returned error, turn on debug log for more details"), 0, 0, DBEF_SENT));
//		if(!bDebugLog)
//		{
//			boost::system::error_code e;
//			boost::filesystem::remove(path, e);
//		}
//		return;
//	}
	if (out.find("usage: ") != string::npos) {
		MessageBox(0, TranslateT("Something is wrong, GPG does not understand us, aborting encryption."), TranslateT("Warning"), MB_OK);
		//mir_free(msg);
		ProtoChainSend(hContact, PSS_MESSAGE, flags, (LPARAM)msg);
		if(!bDebugLog)
		{
			boost::system::error_code e;
			boost::filesystem::remove(path, e);
		}
		return;
	}
	if(!bDebugLog)
	{
		boost::system::error_code e;
		boost::filesystem::remove(path, e);
	}
	path.append(L".asc");
	wfstream f(path.c_str(), std::ios::in | std::ios::ate | std::ios::binary);
	count = 0;
	while (!f.is_open()) {
		boost::this_thread::sleep(boost::posix_time::milliseconds(step));
		f.open(path.c_str(), std::ios::in | std::ios::ate | std::ios::binary);
		count += step;
		if(count >= timeout)
		{
			db_set_b(hContact, szGPGModuleName, "GPGEncryption", 0); //disable encryption
			setSrmmIcon(hContact);
			setClistIcon(hContact);
			debuglog << std::string(time_str() + ": info: gpg failed to encrypt message, disabling encryption to avoid deadlock");
			break;
		}
	}
	str.clear();
	if (f.is_open()) {
		std::wifstream::pos_type size = f.tellg();
		wchar_t *tmp = new wchar_t[(std::ifstream::pos_type)size + (std::ifstream::pos_type)1];
		f.seekg(0, std::ios::beg);
		f.read(tmp, size);
		tmp[size] = '\0';
		str.append(tmp);
		delete[] tmp;
		f.close();
		if(!bDebugLog)
		{
			boost::system::error_code e;
			boost::filesystem::remove(path, e);
		}
	}
	if (str.empty()) {
		HistoryLog(hContact, db_event("Failed to encrypt message with GPG", 0, 0, DBEF_SENT));
		if (bDebugLog)
			debuglog << std::string(time_str() + ": info: Failed to encrypt message with GPG");
		ProtoChainSend(hContact, PSS_MESSAGE, flags, (LPARAM)msg);
		return;
	}
	string str_event = msg;
	if (bAppendTags) {
		str_event.insert(0, toUTF8(outopentag));
		str_event.append(toUTF8(outclosetag));
	}

	if (bDebugLog)
		debuglog << std::string(time_str() + ": adding event to contact: " + toUTF8(pcli->pfnGetContactDisplayName(hContact, 0)) + " on send message.");

	fix_line_term(str);
	sent_msgs.push_back((HANDLE)ProtoChainSend(hContact, PSS_MESSAGE, flags, (LPARAM)toUTF8(str).c_str()));
}

INT_PTR SendMsgSvc(WPARAM w, LPARAM l)
{
	CCSDATA *ccs = (CCSDATA*)l;
	if (!ccs)
		return Proto_ChainSend(w, ccs);
	if (!ccs->lParam)
		return Proto_ChainSend(w, ccs);
	char *msg = (char*)ccs->lParam;
	if (!msg) {
		if (bDebugLog)
			debuglog << std::string(time_str() + ": info: failed to get message data, name: " + toUTF8(pcli->pfnGetContactDisplayName(ccs->hContact, 0)));
		return Proto_ChainSend(w, ccs);
	}
	if (strstr(msg, "-----BEGIN PGP MESSAGE-----")) {
		if (bDebugLog)
			debuglog << std::string(time_str() + ": info: encrypted messge, let it go, name: " + toUTF8(pcli->pfnGetContactDisplayName(ccs->hContact, 0)));
		return Proto_ChainSend(w, ccs);
	}
	if (bDebugLog)
		debuglog << std::string(time_str() + ": info: contact have key, name: " + toUTF8(pcli->pfnGetContactDisplayName(ccs->hContact, 0)));
	if (bDebugLog && db_mc_isMeta(ccs->hContact))
		debuglog << std::string(time_str() + ": info: protocol is metacontacts, name: " + toUTF8(pcli->pfnGetContactDisplayName(ccs->hContact, 0)));
	if (!isContactSecured(ccs->hContact) || db_mc_isMeta(ccs->hContact)) {
		if (bDebugLog)
			debuglog << std::string(time_str() + ": info: contact not secured, name: " + toUTF8(pcli->pfnGetContactDisplayName(ccs->hContact, 0)));
		return Proto_ChainSend(w, ccs);
	}
	return returnNoError(ccs->hContact);
}

boost::mutex event_processing_mutex;

int HookSendMsg(WPARAM w, LPARAM l)
{
	if (!l)
		return 0;
	DBEVENTINFO * dbei = (DBEVENTINFO*)l;
	if (dbei->eventType != EVENTTYPE_MESSAGE)
		return 0;
	MCONTACT hContact = (MCONTACT)w;
	if (dbei->flags & DBEF_SENT) {
		if (isContactSecured(hContact) && strstr((char*)dbei->pBlob, "-----BEGIN PGP MESSAGE-----")) //our service data, can be double added by metacontacts e.w.c.
		{
			if (bDebugLog)
				debuglog << std::string(time_str() + ": info(send handler): block pgp message event, name: " + toUTF8(pcli->pfnGetContactDisplayName(hContact, 0)));
			return 1;
		}
		if (bAutoExchange && (strstr((char*)dbei->pBlob, "-----PGP KEY RESPONSE-----") || strstr((char*)dbei->pBlob, "-----PGP KEY REQUEST-----"))) ///do not show service data in history
		{
			if (bDebugLog)
				debuglog << std::string(time_str() + ": info(send handler): block pgp key request/response event, name: " + toUTF8(pcli->pfnGetContactDisplayName(hContact, 0)));
			return 1;
		}
	}
	if (db_mc_isMeta(hContact))
		return 0;

	if (!isContactHaveKey(hContact)) {
		if (bDebugLog)
			debuglog << std::string(time_str() + ": info: contact have not key, name: " + toUTF8(pcli->pfnGetContactDisplayName(hContact, 0)));
		if (bAutoExchange && !strstr((char*)dbei->pBlob, "-----PGP KEY REQUEST-----") && !strstr((char*)dbei->pBlob, "-----BEGIN PGP PUBLIC KEY BLOCK-----") && gpg_valid) {
			if (bDebugLog)
				debuglog << std::string(time_str() + ": info: checking for autoexchange possibility, name: " + toUTF8(pcli->pfnGetContactDisplayName(hContact, 0)));
			
			LPSTR proto = GetContactProto(hContact);
			DWORD uin = db_get_dw(hContact, proto, "UIN", 0);
			if (uin) {
				if (bDebugLog)
					debuglog << std::string(time_str() + ": info(autoexchange): protocol looks like icq, name: " + toUTF8(pcli->pfnGetContactDisplayName(hContact, 0)));

				char *proto2 = GetContactProto(hContact);
				if (ProtoServiceExists(proto2, PS_ICQ_CHECKCAPABILITY)) {
					if (bDebugLog)
						debuglog << std::string(time_str() + ": info(autoexchange, icq): checking for autoexchange icq capability, name: " + toUTF8(pcli->pfnGetContactDisplayName(hContact, 0)));
					ICQ_CUSTOMCAP cap = { 0 };
					strncpy(cap.caps, "GPGAutoExchange", sizeof(cap.caps));
					if (CallProtoService(proto2, PS_ICQ_CHECKCAPABILITY, hContact, (LPARAM)&cap)) {
						if (bDebugLog)
							debuglog << std::string(time_str() + ": info(autoexchange, icq): sending key requiest, name: " + toUTF8(pcli->pfnGetContactDisplayName(hContact, 0)));
						ProtoChainSend(hContact, PSS_MESSAGE, 0, (LPARAM)"-----PGP KEY REQUEST-----");
						hcontact_data[hContact].msgs_to_send.push_back((char*)dbei->pBlob);
						new boost::thread(boost::bind(send_encrypted_msgs_thread, (void*)hContact));
						return 0;
					}
				}
			}
			else {
				wchar_t *jid = UniGetContactSettingUtf(hContact, proto, "jid", L"");
				if (jid[0]) {
					if (bDebugLog)
						debuglog << std::string(time_str() + ": info(autoexchange): protocol looks like jabber, name: " + toUTF8(pcli->pfnGetContactDisplayName(hContact, 0)));
					extern list <JabberAccount*> Accounts;
					list<JabberAccount*>::iterator end = Accounts.end();
					for (list<JabberAccount*>::iterator p = Accounts.begin(); p != end; p++) {
						wchar_t *caps = (*p)->getJabberInterface()->GetResourceFeatures(jid);
						if (caps) {
							wstring str;
							for (int i = 0;; i++) {
								str.push_back(caps[i]);
								if (caps[i] == '\0')
									if (caps[i + 1] == '\0')
										break;
							}
							mir_free(caps);
							if (str.find(L"GPG_Key_Auto_Exchange:0") != string::npos) {
								if (bDebugLog)
									debuglog << std::string(time_str() + ": info(autoexchange, jabber): autoexchange capability found, sending key request, name: " + toUTF8(pcli->pfnGetContactDisplayName(hContact, 0)));
								ProtoChainSend(hContact, PSS_MESSAGE, 0, (LPARAM)"-----PGP KEY REQUEST-----");
								hcontact_data[hContact].msgs_to_send.push_back((char*)dbei->pBlob);
								new boost::thread(boost::bind(send_encrypted_msgs_thread, (void*)hContact));
								return 0;
							}
						}
					}
				}
				mir_free(jid);
			}
		}
		else {
			return 0;
		}
	}
	if (isContactSecured(hContact) && (dbei->flags & DBEF_SENT)) //aggressive outgoing events filtering
	{
		SendMsgSvc_func(hContact, (char*)dbei->pBlob, 0);
		//TODO: handle errors somehow ...
		if (bAppendTags) {
			string str_event = (char*)dbei->pBlob;
			//mir_free(dbei->pBlob);
			str_event.insert(0, toUTF8(outopentag));
			str_event.append(toUTF8(outclosetag));
			dbei->pBlob = (PBYTE)mir_strdup(str_event.c_str());
			dbei->cbBlob = (DWORD)str_event.length() + 1;
		}

		return 0;
	}
	if (!isContactSecured(hContact)) {
		if (bDebugLog)
			debuglog << std::string(time_str() + ": event message: \"" + (char*)dbei->pBlob + "\" passed event filter, contact " + toUTF8(pcli->pfnGetContactDisplayName(hContact, 0)) + " is unsecured");
		return 0;
	}
	if (!(dbei->flags & DBEF_SENT) && db_mc_isMeta((MCONTACT)w)) {
		char tmp[29];
		strncpy(tmp, (char*)dbei->pBlob, 27);
		tmp[28] = '\0';
		if (strstr(tmp, "-----BEGIN PGP MESSAGE-----")) {
			if (bDebugLog)
				debuglog << std::string(time_str() + ": info(send handler): block pgp message event, name: " + toUTF8(pcli->pfnGetContactDisplayName(hContact, 0)));
			return 1;
		}
	}
	return 0;
}

static INT_PTR CALLBACK DlgProcKeyPassword(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM)
{
	char *inkeyid = NULL;
	switch (msg) {
	case WM_INITDIALOG:
		inkeyid = UniGetContactSettingUtf(new_key_hcnt, szGPGModuleName, "InKeyID", "");
		new_key_hcnt_mutex.unlock();

		SetWindowPos(hwndDlg, 0, key_password_rect.left, key_password_rect.top, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW);
		TranslateDialogDefault(hwndDlg);
		{
			string questionstr = "Please enter password for key with ID: ";
			questionstr += inkeyid;
			mir_free(inkeyid);
			SetDlgItemTextA(hwndDlg, IDC_KEYID, questionstr.c_str());
			EnableWindow(GetDlgItem(hwndDlg, IDC_DEFAULT_PASSWORD), 0);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			{
				wchar_t tmp[64];
				GetDlgItemText(hwndDlg, IDC_KEY_PASSWORD, tmp, _countof(tmp));
				if (tmp[0]) {
					extern wchar_t *password;
					if (IsDlgButtonChecked(hwndDlg, IDC_SAVE_PASSWORD)) {
						inkeyid = UniGetContactSettingUtf(new_key_hcnt, szGPGModuleName, "InKeyID", "");
						if (inkeyid && inkeyid[0] && BST_UNCHECKED == IsDlgButtonChecked(hwndDlg, IDC_DEFAULT_PASSWORD)) {
							string dbsetting = "szKey_";
							dbsetting += inkeyid;
							dbsetting += "_Password";
							db_set_ws(NULL, szGPGModuleName, dbsetting.c_str(), tmp);
						}
						else db_set_ws(NULL, szGPGModuleName, "szKeyPassword", tmp);
					}
					if (password)
						mir_free(password);
					password = (wchar_t*)mir_alloc(sizeof(wchar_t)*(mir_wstrlen(tmp) + 1));
					mir_wstrcpy(password, tmp);
				}
				mir_free(inkeyid);
				DestroyWindow(hwndDlg);
			}
			break;

		case IDCANCEL:
			mir_free(inkeyid);
			_terminate = true;
			DestroyWindow(hwndDlg);
			break;
		}
		break;

	case WM_CLOSE:
		mir_free(inkeyid);
		DestroyWindow(hwndDlg);
		break;

	case WM_DESTROY:
		GetWindowRect(hwndDlg, &key_password_rect);
		db_set_dw(NULL, szGPGModuleName, "PasswordWindowX", key_password_rect.left);
		db_set_dw(NULL, szGPGModuleName, "PasswordWindowY", key_password_rect.top);
		break;
	}
	return FALSE;
}

void ShowLoadKeyPasswordWindow()
{
	extern HINSTANCE hInst;
	DialogBox(hInst, MAKEINTRESOURCE(IDD_KEY_PASSWD), NULL, DlgProcKeyPassword);
}
