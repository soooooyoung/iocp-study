#include "NetworkManager.h"
#include "IOCPHandler.h"
#include "NetworkClient.h"
#include "ListenClient.h"
#include "StaticPool.h"


NetworkManager::NetworkManager()
{
	
}

bool NetworkManager::Init()
{
	// TODO: move this and hardcoded variables to server config later
	StaticPool<NetworkClient>::GetInstance().Reserve(10);
	StaticPool<NetworkContext>::GetInstance().Reserve(100);
	mClientList.reserve(11);

	if (false == mIOCPHandler.Init())
	{
		return false;
	}

	if (false == AddListener(9000))
	{
		return false;
	}

	return true;
}

bool NetworkManager::AddListener(int port)
{
	// Listeners don't need to use pooling
	std::shared_ptr<ListenClient> listenClient = std::make_shared<ListenClient>();

	if (false == listenClient->Init())
	{
		return false;
	}

	if (false == listenClient->BindAndListen(port, mIOCPHandler.GetIOCPHandle()))
	{
		return false;
	}

	if (false == _AddClient(std::move(listenClient)))
	{
		return false;
	}

	// Client to use for accepting
	auto pClient = StaticPool<NetworkClient>::GetInstance().Pop();

	if (false == listenClient->Accept(pClient))
	{
		return false;
	}

	if (false == _AddClient(std::move(pClient)))
	{
		return false;
	}

	return true;
}


bool NetworkManager::_AddClient(std::shared_ptr<NetworkClient> client)
{
	if (nullptr == client)
	{
		return false;
	}

	// Register Client
	if (false == mIOCPHandler.Register(client))
	{
		printf("Register Client Error\n");
		return false;
	}

	client->SetSessionID(static_cast<UINT32>(mClientList.size()));

	mClientList.emplace_back(std::move(client));

	return true;
}

std::weak_ptr<NetworkClient> NetworkManager::_GetClient(UINT32 index)
{
	if (index >= mClientList.size())
	{
		return std::weak_ptr<NetworkClient>();
	}

	return mClientList[index]->GetWeakPtr();
}

