#pragma once
#include <unordered_map>

#include "User.h"
#include "ErrorCode.h"

class UserManager
{
public:
	UserManager() = default;
	~UserManager() = default;

	void Init(const INT32 maxUserCount)
	{
		mUserIDMap.clear();
		mUserPool.clear();

		mMaxUserCount = maxUserCount;
		mUserPool = std::vector<User*>(mMaxUserCount);

		for (INT32 i = 0; i < mMaxUserCount; ++i)
		{
			mUserPool[i] = new User();
			mUserPool[i]->Init(i);
		}
	}

	INT32 GetCurrentUserCount() const
	{
		return mCurrentUserCount;
	}

	INT32 GetMaxUserCount() const
	{
		return mMaxUserCount;
	}

	void IncreaseUserCount()
	{
		++mCurrentUserCount;
	}

	void DecreaseUserCount()
	{
		if (mCurrentUserCount <= 0)
		{
			return;
		}

		--mCurrentUserCount;
	}

	ERROR_CODE AddUser(char* userID, int clientIndex)
	{
		auto userIndex = clientIndex;

		mUserPool[userIndex]->SetLogin(userID);
		mUserIDMap.insert(std::make_pair(userID, userIndex));

		return ERROR_CODE::NONE;
	}

	INT32 FindUserIndexByID(char* userID)
	{
		if (auto iter = mUserIDMap.find(userID); iter != mUserIDMap.end())
		{
			return iter->second;
		}

		return -1;
	}

	void DeleteUserInfo(User* user)
	{
		mUserIDMap.erase(user->GetUserID());
		
		if (nullptr == user)
		{
			return;
		}
		
		user->Clear();
	}

	User* GetUserByConnectionIndex(INT32 connectionIndex)
	{
		return mUserPool[connectionIndex];
	}

private:
	INT32 mMaxUserCount = 0;
	INT32 mCurrentUserCount = 0;

	std::vector<User*> mUserPool;
	std::unordered_map<std::string, int> mUserIDMap;
};