#pragma once
#include <concurrent_vector.h>

const UINT64 MAX_LISTEN_COUNT = 1;

class NetworkClient;
class ListenClient;
class IOCPHandler;

class NetworkManager : public Singleton<NetworkManager>
{
public:
	NetworkManager() {}
	virtual ~NetworkManager() {}

	bool Init();

	bool AddClient(std::shared_ptr<NetworkClient> client);
	bool AddListener(int port);

	bool SendPacket(UINT32 clientIndex, std::uint8_t* pData, size_t size);

	std::weak_ptr<NetworkClient> GetClient(UINT32 clientIndex);
private:
	std::unique_ptr<IOCPHandler> mIOCPHandler;
	std::array<std::shared_ptr<ListenClient>, MAX_LISTEN_COUNT> mListenClientList;

	// Connected Clients, Uses Index as SessionID
	concurrency::concurrent_vector<std::shared_ptr<NetworkClient>> mClientList;
};