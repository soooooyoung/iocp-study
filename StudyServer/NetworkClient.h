#pragma once
#include <queue>
#include <atomic>
#include <memory>

#include "NetworkPacket.h"

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

	void EnqueuePacket(MemoryPool<Packet>::UniquePtr packet);

	bool Receive();
	bool Send();

	MemoryPool<Packet>::UniquePtr GetPacket(std::shared_ptr<MemoryPool<Packet>> packetPool);
	NetworkPacket SetPacket(const Packet& packet);

	std::atomic<bool> mSending = true;
protected:
	uint64_t mLastCloseTimeInSeconds = 0;
	SOCKET mSocket = INVALID_SOCKET;

	uint32_t mSessionID = 0;
	bool mIsConnected = false;

	std::unique_ptr<NetworkContext> mReceiveContext;
	std::unique_ptr<NetworkContext> mSendContext;

	std::queue<MemoryPool<Packet>::UniquePtr> mSendQueue;
};