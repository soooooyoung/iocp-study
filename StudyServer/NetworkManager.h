#pragma once
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "mswsock.lib")

#include "Define.h"
#include "SecurePool.h"

#include <memory>
#include <vector>
#include <thread>
#include <array>
#include <concurrent_vector.h>

const UINT64 MAX_LISTEN_COUNT = 1;

class ListenClient;
class NetworkClient;
class NetworkContext;
class NetworkManager
{
public:
	NetworkManager();
	~NetworkManager();

	bool Initialize();
	void Shutdown();

	bool AddListener(int index, int port);
	bool AddClient(std::shared_ptr<NetworkClient> client);
	void RemoveClient(NetworkClient* client);

	bool PushSend(int sessionID, void* data, int transferred);
private:
	void WorkerThread();
	void SendThread();

	void _HandleAccept(ListenClient& host, NetworkContext& context);
	void _HandleReceive(NetworkClient& client, NetworkContext& context, int transferred);
	void _HandleSend(NetworkClient& client, NetworkContext& context, int transferred);

	bool mIsRunning = false;

	HANDLE mIOCPHandle;

	// using cores/2 for each thread pool
	std::vector<std::thread> mIOThreadPool;
	std::vector<std::thread> mPacketPool;

	std::array<std::shared_ptr<ListenClient>, MAX_LISTEN_COUNT> mListenClientList;

	// Connected Clients, Uses Index as SessionID
	concurrency::concurrent_vector<std::shared_ptr<NetworkClient>> mClientList;
	concurrency::concurrent_queue<std::shared_ptr<NetworkContext>> mSendQueue;
};