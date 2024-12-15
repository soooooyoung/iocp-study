#pragma once
#include <memory>
#include <thread>
#include <queue>
#include <mutex>

#include "MemoryPool.h"

struct Packet;
struct NetworkPacket;
class NetworkContext;
class Service;
class NetworkDispatcher
{
public:
	NetworkDispatcher();
	virtual ~NetworkDispatcher();

	bool Initialize(std::unique_ptr<Service> service, int nRemainThread = 1);
	void PushPacket(MemoryPool<Packet>::UniquePtr packet);
private:
	void _DispatchThread();
	std::thread mDispatchThread;

	std::unique_ptr<Service> mService;

	std::mutex mMutex;
	std::queue<MemoryPool<Packet>::UniquePtr> mPacketQueue;

	bool mIsRunning = false;
};