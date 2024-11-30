#pragma once
#include <concurrent_queue.h>
#include <concurrent_vector.h>
#include <memory>
#include <thread>
#include <vector>

class NetworkClient;
class NetworkPacket;
class NetworkContext;
class NetworkDispatcher
{
public:
	NetworkDispatcher();
	virtual ~NetworkDispatcher();

	bool Initialize(int nRemainThread = 1);

	void AddSession(std::weak_ptr<NetworkClient> session);

	void PushPacket(std::unique_ptr<NetworkPacket> packet);
	void PushSend(int sessionID, void* data, int size);

	void DispatchThread();
	void SendThread();

private:
	concurrency::concurrent_queue<std::shared_ptr<NetworkPacket>> mPacketQueue;
	concurrency::concurrent_queue<std::shared_ptr<NetworkContext>> mSendQueue;

	concurrency::concurrent_vector<std::weak_ptr<NetworkClient>> mSessionList;

	std::thread mDispatchThread;
	std::vector<std::thread> mSendThreadPool;

	bool mIsRunning = false;
};