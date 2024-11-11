#pragma once

#include <memory>
#include <basetsd.h>
#include <vector>

const UINT64 MAX_LISTEN_COUNT = 1;

class NetworkClient;
class IOCPHandler;
class NetworkManager
{
public:
	NetworkManager() {}
	virtual ~NetworkManager() {}

	bool Init();
	bool AddListener(int port);
private:
	bool _AddClient(std::shared_ptr<NetworkClient> client);
	std::weak_ptr<NetworkClient> _GetClient(UINT32 index);

	IOCPHandler mIOCPHandler;
	std::vector<std::shared_ptr<NetworkClient>> mClientList;
};