#pragma once
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "mswsock.lib")

#include "NetworkClient.h"
#include "Define.h"
#include <thread>
#include <vector>

class IOCPServer
{
public:
	IOCPServer(void) {}
	virtual ~IOCPServer(void)
	{
		WSACleanup();
	}

private:
	NetworkClient* GetEmptyClient()
	{
		for (auto& client : mClientList)
		{
			if (client->IsConnected() == false)
			{
				return client;
			}
		}

		return nullptr;

	}

private:
	bool mIsWorkerRunning = true;
	bool mIsAcceptorRunning = true;
	
	int mClientCount = 0;
	UINT32 MaxIOThreadCount = 0;

	std::vector<NetworkClient*> mClientList;
	std::vector<std::thread> mIOThreadList;
	std::thread mAcceptorThread;

	HANDLE mIOCPHandle = INVALID_HANDLE_VALUE;
	SOCKET mListenSocket = INVALID_SOCKET;
};