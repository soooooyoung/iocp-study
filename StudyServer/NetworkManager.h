#pragma once
#include <concurrent_vector.h>
#include <concurrent_queue.h>
#include <unordered_map>
#include <functional>

struct NetworkPacket;
class Service;
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
	void RemoveClient(NetworkClient& client);

	bool RegisterService(int serviceID, std::unique_ptr<Service> service);
private:
	void WorkerThread();

	void _HandleAccept(ListenClient& host, NetworkContext& context);
	void _HandleReceive(NetworkClient& client, NetworkContext& context, int transferred);
	void _HandleSend(NetworkClient& client, NetworkContext& context, int transferred);

	bool mIsRunning = false;
	HANDLE mIOCPHandle;

	std::vector<std::thread> mIOThreadPool;
	std::array<std::shared_ptr<ListenClient>, MAX_LISTEN_COUNT> mListenClientList;

	// Connected Clients, Uses Index as SessionID
	concurrency::concurrent_vector<std::shared_ptr<NetworkClient>> mClientList;
	// Vacant SessionID Pool
	concurrency::concurrent_queue<std::shared_ptr<NetworkClient>> mClientPool;

	std::unordered_map<int, std::shared_ptr<NetworkDispatcher>> mServiceList;
};