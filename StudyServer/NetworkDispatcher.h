#pragma once
#include <concurrent_queue.h>
#include <concurrent_vector.h>
#include <memory>
#include <thread>
#include <vector>

struct NetworkPacket;
class NetworkContext;
class Service;
class NetworkDispatcher
{
public:
	NetworkDispatcher();
	virtual ~NetworkDispatcher();

	bool Initialize(std::unique_ptr<Service> service, int nRemainThread = 1);
	void PushPacket(std::unique_ptr<NetworkPacket> packet);
private:
	void _DispatchThread();
	std::thread mDispatchThread;

	std::unique_ptr<Service> mService;
	concurrency::concurrent_queue<std::shared_ptr<NetworkPacket>> mPacketQueue;

	bool mIsRunning = false;
};