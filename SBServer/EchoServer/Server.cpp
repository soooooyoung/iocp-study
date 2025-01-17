#include "../ServerLib/IOCPHandler.h"
#include "Server.h"
#include "ConfigLoader.h"

Server::Server()
{
	mIOCPHandler = std::make_unique<NetworkLib::IOCPHandler>();
}

Server::~Server()
{
}

bool Server::StartServer()
{
	WSADATA wsaData;
	int initResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (initResult != 0)
	{
		return false;
	}

	ConfigLoader configLoader;
	if (false == configLoader.LoadConfig("config.json"))
	{
		return false;
	}

	if (false == mIOCPHandler->Initialize(configLoader.GetNetworkConfig()))
	{
		return false;
	}

	return true;
}

void Server::Shutdown()
{
}

void Server::Run()
{
	while (true)
	{
		std::this_thread::sleep_for(std::chrono::seconds(1));

		if (false == mIOCPHandler->HasPacket())
		{
			continue;
		}

		auto& packet = mIOCPHandler->PopPacket();

		if (packet.mPacketID == 0)
		{
			continue;
		}

		if (packet.mPacketID == (int)PacketProtocol::ECHO)
		{
			printf_s("Echo Packet Received Data : %s\n", packet.mData.data());
		}
	}
}