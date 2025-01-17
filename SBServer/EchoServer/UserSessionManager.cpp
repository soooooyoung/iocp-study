#include "UserSessionManager.h"

UserSessionManager::UserSessionManager()
{
}

UserSessionManager::~UserSessionManager()
{
	mUserSessions.clear();
}

void UserSessionManager::AddUserSession(int sessionID)
{
	mUserSessions.insert(std::make_pair(sessionID, std::make_unique<UserSession>(sessionID)));
}

void UserSessionManager::RemoveUserSession(int sessionID)
{
	mUserSessions.erase(sessionID);
}
