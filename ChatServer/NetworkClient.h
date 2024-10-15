#pragma once

#include "Define.h"
#include <stdio.h>
#include <mutex>
#include <queue>

class NetworkClient
{
public:
	NetworkClient()
	{
		ZeroMemory(&mRecvOverlappedEx, sizeof(stOverlappedEx));
		mSocket = INVALID_SOCKET;
	}

	~NetworkClient() = default;
	

private:



private:
	INT32 mIndex = 0;
	INT64 mIsConnect = 0;
	UINT64 mLastestClosedTimeSec = 0;

	HANDLE mIOCPHandle = INVALID_HANDLE_VALUE;
	SOCKET mSocket = INVALID_SOCKET;

	stOverlappedEx mAcceptContext;
	stOverlappedEx mRecvOverlappedEx;

	char mAcceptBuf[64];
	char mRecvBuf[MAX_SOCK_RECVBUF];

	std::mutex mSendLock;
	std::queue<stOverlappedEx> mSendQueue;
};