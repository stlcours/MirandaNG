/*

Facebook plugin for Miranda Instant Messenger
_____________________________________________

Copyright � 2011-16 Robert P�sel

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#ifndef _FACEBOOK_REQUEST_SEARCH_H_
#define _FACEBOOK_REQUEST_SEARCH_H_

// searching
class SearchRequest : public HttpRequest
{
public:
	SearchRequest(bool mobileBasicWorks, const char *query, int s, const char *ssid) :
		HttpRequest(REQUEST_GET, FORMAT, "%s/search/", mobileBasicWorks ? FACEBOOK_SERVER_MBASIC : FACEBOOK_SERVER_MOBILE)
	{
		flags |= NLHRF_REDIRECT;

		Url
			<< "search=people"
			<< CHAR_VALUE("query", query)
			<< INT_VALUE("s", s);

		if (mir_strlen(ssid) > 0) {
			Url << CHAR_VALUE("ssid", ssid);
		}
	}
};

#endif //_FACEBOOK_REQUEST_SEARCH_H_
