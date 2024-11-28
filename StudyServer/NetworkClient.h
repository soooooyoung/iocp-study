#pragma once
#include <concurrent_queue.h>

#include "NetworkContext.h"

class NetworkPacket;
class NetworkClient : public std::enable_shared_from_this<NetworkClient>
{
public:
	NetworkClient();
	virtual ~NetworkClient() {}

	SOCKET GetSocket() { return mSocket; }
	void SetSocket(SOCKET socket) { mSocket = socket; }

	std::int32_t GetSessionID() { return mSessionID; }
	void SetSessionID(std::int32_t sessionID) { mSessionID = sessionID; }

	UINT64 GetLastCloseTime() { return mLastCloseTimeInSeconds; }

	virtual bool Init();
	virtual void Close(bool bIsForce = false);

	bool Receive();
	bool Send(NetworkContext& context);

	std::shared_ptr<NetworkPacket> GetPacket();
protected:
	std::int32_t mSessionID = 0;
	UINT64 mLastCloseTimeInSeconds = 0;
	bool mIsConnected = false;

	SOCKET mSocket = INVALID_SOCKET;

	std::shared_ptr<NetworkContext> mContext;
	NetworkContext mSendBuffer;
};