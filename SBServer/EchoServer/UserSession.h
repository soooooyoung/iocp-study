#pragma once
#include <memory>

class UserSession : public std::enable_shared_from_this<UserSession>
{
public:
	UserSession(int sessionID) : mSessionID(sessionID) {}
	virtual ~UserSession() = default;

	void SetSessionID(int sessionID) { mSessionID = sessionID; }
	int GetSessionID() const { return mSessionID; }

private:

	int mSessionID{ 0 };

};