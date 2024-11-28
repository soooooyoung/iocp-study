#pragma once
#include <concurrent_queue.h>

class NetworkContext;
class NetworkPacket;
class NetworkDispatcher
{
public:
	NetworkDispatcher();
	virtual ~NetworkDispatcher();

	bool Initialize();

	void PushPacket(std::unique_ptr<NetworkPacket> packet);
	void DispatchThread();

private:
	concurrency::concurrent_queue<std::shared_ptr<NetworkPacket>> mPacketQueue;

	std::thread mDispatchThread;

	bool mIsRunning = false;
};