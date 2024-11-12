#include "SimpleCore.h"
#include "NetworkManager.h"
#include "IOCPHandler.h"
#include "NetworkClient.h"
#include "ListenClient.h"


bool NetworkManager::Init()
{
	// TODO: move this and hardcoded variables to server config later
	StaticPool<NetworkClient>::GetInstance().Reserve(10);
	StaticPool<NetworkContext>::GetInstance().Reserve(100);
	mClientList.reserve(11);

	// Lambda for Pushing Packets to Clients
	auto pushSendPacket = [&](UINT32 clientIndex, std::uint8_t* pData, size_t size)
	{
		SendPacket(clientIndex, pData, size);
	};

	mIOCPHandler = std::make_unique<IOCPHandler>();
	mIOCPHandler->PushSendPacket = pushSendPacket;

	// Initialize Winsock and Create IOCP Handle, Start Worker Threads
	if (false == mIOCPHandler->Init())
	{
		return false;
	}

	// Listeners for Incoming Connections
	for (int i = 0; i < MAX_LISTEN_COUNT; ++i)
	{
		// FIXME: hardcoded port
		if (false == AddListener(9000 + i))
		{
			return false;
		}
	}

	return true;
}

bool NetworkManager::AddListener(int port)
{
	// Listeners don't need to use pooling
	std::shared_ptr<ListenClient> listenClient = std::make_shared<ListenClient>();

	// Initialize Listen Socket
	if (false == listenClient->Init())
	{
		return false;
	}

	// Listen for Incoming Connections
	if (false == listenClient->BindAndListen(port))
	{
		return false;
	}
	
	// Register Listener
	if (false == mIOCPHandler->Register(listenClient))
	{
		printf("Register Client Error\n");
		return false;
	}

	// Post Accept
	listenClient->PostAccept();

	// Add Listener to List
	listenClient->SetSessionID(static_cast<UINT32>(mListenClientList.size()));
	mListenClientList[0] = std::move(listenClient);

	return true;
}

bool NetworkManager::SendPacket(UINT32 clientIndex, std::uint8_t* pData, size_t size)
{
	auto pClient = GetClient(clientIndex).lock();
	if (nullptr == pClient)
	{
		return false;
	}

	pClient->PushSend(pData, size);

	return true;
}

bool NetworkManager::AddClient(std::shared_ptr<NetworkClient> client)
{
	if (nullptr == client)
	{
		return false;
	}

	// Register Client
	if (false == mIOCPHandler->Register(client))
	{
		printf("Register Client Error\n");
		return false;
	}

	// For real world scenario, should use a session manager to assign session id
	client->SetSessionID(static_cast<UINT32>(mClientList.size()));
	mClientList.push_back(std::move(client));

	return true;
}

std::weak_ptr<NetworkClient> NetworkManager::GetClient(UINT32 index)
{
	if (index >= mClientList.size())
	{
		return std::weak_ptr<NetworkClient>();
	}

	// at() is concurrency-safe for read operations, and also while growing the vector, as long as you have ensured that the value _Index is less than the size of the concurrent vector.
	return mClientList.at(index)->GetWeakPtr();
}

