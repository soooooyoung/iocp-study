#pragma once
#include <unordered_map>
#include <memory>
#include "UserSession.h"

class UserSessionManager
{
public:
	UserSessionManager();
	virtual ~UserSessionManager();

	void AddUserSession(int sessionID);
	void RemoveUserSession(int sessionID);
private:
	std::unordered_map<int, std::unique_ptr<UserSession>> mUserSessions;
};