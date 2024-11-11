#pragma once
#include <concurrent_vector.h>

const UINT64 MAX_LISTEN_COUNT = 1;

class NetworkClient;
class ListenClient;
class IOCPHandler;
class NetworkManager
{
public:
	NetworkManager() {}
	virtual ~NetworkManager() {}

	bool Init();

	bool AddClient(std::shared_ptr<NetworkClient> client);
	bool AddListener(int port);

	std::weak_ptr<NetworkClient> GetClient(UINT32 index);
private:
	IOCPHandler* mIOCPHandler;
	std::array<std::shared_ptr<ListenClient>, MAX_LISTEN_COUNT> mListenClientList;

	// Connected Clients, Uses Index as SessionID
	concurrency::concurrent_vector<std::shared_ptr<NetworkClient>> mClientList;
};