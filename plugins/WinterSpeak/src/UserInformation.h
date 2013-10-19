#pragma once

#include <map>
#include <string>

class UserInformation
{
  public:
	UserInformation();
	~UserInformation();

	//--------------------------------------------------------------------------
	// Description : update the users status
	// Parameters  : user - the current user
	//               status - the users status
	// Returns     : true - the status changed
	//               false - the status stayed the same
	//--------------------------------------------------------------------------
	bool updateStatus(HANDLE user, int status);

	//--------------------------------------------------------------------------
	// Description : get a string containing the users current status string
	// Parameters  : user - the current user
	// Returns     : the string containing the users status
	//--------------------------------------------------------------------------
	std::wstring statusString(HANDLE user);

	//--------------------------------------------------------------------------
	// Description : return the status mode of the user
	// Parameters  : user - the current user
	// Returns     : the string containing the users status mode
	//--------------------------------------------------------------------------
    std::wstring statusModeString(HANDLE user);

	//--------------------------------------------------------------------------
	// Description : insert the name into the string at the %u location
	// Parameters  : str  - the string to have the username inserted into
	//               user - the current user
	//--------------------------------------------------------------------------
	void insertName(std::wstring &str, HANDLE user) const;

	//--------------------------------------------------------------------------
	// Description : get the name string for the user
	// Parameters  : user - the current user
	// Returns     : a string containing the user's name
	//--------------------------------------------------------------------------
	std::wstring nameString(HANDLE user) const;

  private:
	std::map<HANDLE, int>      m_status_info;
	std::map<int, std::wstring> m_status_strings;
};

