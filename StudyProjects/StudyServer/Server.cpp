#include "pch.h"
#include "Server.h"
#include "NetworkManager.h"
#include "Service.h"

Server::Server()
{
}

Server::~Server()
{
}

bool Server::Initialize()
{
	mNetworkManager = std::make_unique<NetworkManager>();
	mNetworkManager->RegisterService(1, std::make_unique<Service>());
	
	if (false == mNetworkManager->Initialize())
	{
		return false;
	}

	return true;
}

void Server::Run()
{
	while (true)
	{
		Sleep(1000);
	}
}

void Server::Shutdown()
{
	mNetworkManager->Shutdown();
}