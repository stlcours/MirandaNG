/*
Copyright (c) 2015-16 Miranda NG project (http://miranda-ng.org)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation version 2
of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include "stdafx.h"

HANDLE CSkypeProto::SearchBasic(const TCHAR* id)
{
	ForkThread(&CSkypeProto::SearchBasicThread, (void *)id);
	return (HANDLE)1;
}

void CSkypeProto::SearchBasicThread(void* id)
{
	debugLogA("CSkypeProto::OnSearchBasicThread");
	if (IsOnline())
	{
		ptrA szString(mir_urlEncode(T2Utf((TCHAR*)id)));
		SendRequest(new GetSearchRequest(szString, li), &CSkypeProto::OnSearch);
	}
}

void CSkypeProto::OnSearch(const NETLIBHTTPREQUEST *response)
{
	debugLogA(__FUNCTION__);
	if (response == NULL) {
		ProtoBroadcastAck(0, ACKTYPE_SEARCH, ACKRESULT_SUCCESS, (HANDLE)1, 0);
		return;
	}

	debugLogA("CSkypeProto::OnSearch %d", response->resultCode);
	if (response->resultCode != 200) {
		ProtoBroadcastAck(0, ACKTYPE_SEARCH, ACKRESULT_SUCCESS, (HANDLE)1, 0);
		return;
	}

	JSONNode root = JSONNode::parse(response->pData);
	if (!root) {
		ProtoBroadcastAck(0, ACKTYPE_SEARCH, ACKRESULT_SUCCESS, (HANDLE)1, 0);
		return;
	}

	const JSONNode &items = root["results"].as_array();
	for (auto it = items.begin(); it != items.end(); ++it)
	{
		const JSONNode &item = (*it)["nodeProfileData"];

		std::string skypeId = item["skypeId"].as_string();
		std::string name = item["name"].as_string();

		PROTOSEARCHRESULT psr = { sizeof(psr) };
		psr.flags = PSR_UTF8;
		psr.id.a = const_cast<char*>(skypeId.c_str());
		psr.nick.a = const_cast<char*>(name.c_str());
		ProtoBroadcastAck(0, ACKTYPE_SEARCH, ACKRESULT_DATA, (HANDLE)1, (LPARAM)&psr);
	}
	
	ProtoBroadcastAck(0, ACKTYPE_SEARCH, ACKRESULT_SUCCESS, (HANDLE)1, 0);
}