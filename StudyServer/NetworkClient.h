#pragma once
#include "pch.h"

struct NetworkPacket;
class NetworkContext;
class NetworkClient : public std::enable_shared_from_this<NetworkClient>
{
public:
	NetworkClient();
	virtual ~NetworkClient() {}

	SOCKET GetSocket() const { return mSocket; }
	void SetSocket(SOCKET socket) { mSocket = socket; }

	uint32_t GetSessionID() const { return mSessionID; }
	void SetSessionID(std::int32_t sessionID) { mSessionID = sessionID; }

	uint64_t GetLastCloseTime() const { return mLastCloseTimeInSeconds; }
	bool IsConnected() const { return mIsConnected; }

	virtual bool Init();
	virtual void Close(bool bIsForce = false);
	virtual void Reset();

	bool Receive();
	bool Send(NetworkContext& context);
	void SendComplete();

	std::unique_ptr<NetworkPacket> GetPacket();
protected:
	uint64_t mLastCloseTimeInSeconds = 0;
	SOCKET mSocket = INVALID_SOCKET;
	
	uint32_t mSessionID = 0;
	bool mIsConnected = false;

	std::unique_ptr<NetworkContext> mReceiveContext;
	std::shared_ptr<NetworkContext> mSendContext;
};