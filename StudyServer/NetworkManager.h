#pragma once
#include <concurrent_vector.h>
#include <concurrent_queue.h>

class ListenClient;
class NetworkClient;
class NetworkContext;
class NetworkDispatcher;
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
	NetworkDispatcher* mDispatcher;

	std::vector<std::thread> mIOThreadPool;
	std::vector<std::thread> mPacketPool;

	std::array<std::shared_ptr<ListenClient>, MAX_LISTEN_COUNT> mListenClientList;

	// Connected Clients, Uses Index as SessionID
	concurrency::concurrent_vector<std::shared_ptr<NetworkClient>> mClientList;
	// Vacant SessionID Pool
	concurrency::concurrent_queue<std::shared_ptr<NetworkClient>> mClientPool;

	concurrency::concurrent_queue<std::shared_ptr<NetworkContext>> mSendQueue;
};