#pragma once
#include "Define.h"
#include <memory>
#include <array>
#include <mutex>
#include <concurrent_queue.h>

#include "NetworkContext.h"


class NetworkClient : public std::enable_shared_from_this<NetworkClient>
{
public:
	NetworkClient();
	virtual ~NetworkClient() {}

	SOCKET GetSocket() { return mSocket; }
	void SetSocket(SOCKET socket) { mSocket = socket; }

	std::int32_t GetSessionID() { return mSessionID; }
	void SetSessionID(std::int32_t sessionID) { mSessionID = sessionID; }

	virtual bool Init();
	virtual void Close(bool bIsForce = false);
	virtual void Update();

	bool Receive();
	bool Send(NetworkContext& context);

	std::weak_ptr<NetworkContext> GetContext() { return mContext; }
protected:
	std::int32_t mSessionID = 0;
	SOCKET mSocket = INVALID_SOCKET;

	std::shared_ptr<NetworkContext> mContext;
	NetworkContext mSendBuffer;
};