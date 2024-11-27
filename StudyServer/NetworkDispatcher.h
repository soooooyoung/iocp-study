#pragma once
#include <concurrent_queue.h>
#include <thread>
#include "Define.h"

class NetworkContext;
class NetworkPacket;
class NetworkDispatcher
{
public:
	NetworkDispatcher();
	virtual ~NetworkDispatcher();

	bool Initialize();

	void PushPacket(std::shared_ptr<NetworkPacket> packet);
	void EnqueueClientPacket(std::weak_ptr<NetworkContext> context);

	void DispatchThread();
	void PacketThread();

private:
	concurrency::concurrent_queue<std::weak_ptr<NetworkContext>> mIncomingPacketQueue;
	concurrency::concurrent_queue<std::shared_ptr<NetworkPacket>> mPacketQueue;

	std::thread mPacketThread;
	std::thread mDispatchThread;

	bool mIsRunning = false;
};