#pragma once
#include <Winsock2.h>
#include <mutex>
#include "SecureQueue.h"
#include "StaticPool.h"

struct NetworkContext;
class NetworkClient : public PoolObject<NetworkClient>
{
public:
	virtual ~NetworkClient();

	UINT32 GetSessionID() const { return mSessionID; }
	SOCKET GetSocket() const { return mSocket; }
	UINT64 GetLatestClosedTimeSec() { return mLatestClosedTimeSec; }

	void SetSessionID(UINT32 sessionID) { mSessionID = sessionID; }

	bool Init();
	void Close(bool bIsForce = false);
	void Reset();
protected:
	UINT32 mSessionID = 0;
	SOCKET mSocket = INVALID_SOCKET;
	UINT64 mLatestClosedTimeSec = 0;

	std::shared_ptr<NetworkContext> mContext;
	SecureQueue<std::shared_ptr<NetworkContext>> mSendQueue;
};

