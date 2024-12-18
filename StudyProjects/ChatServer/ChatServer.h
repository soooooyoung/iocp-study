#pragma once

#include "IOCPServer.h"
#include "PacketManager.h"
#include "Packet.h"

#include <vector>
#include <deque>
#include <memory>
#include <thread>
#include <mutex>

class ChatServer : public IOCPServer
{
public:
	ChatServer() = default;
	virtual ~ChatServer() = default;

	virtual void OnConnect(const UINT32 clientIndex) override
	{
		printf("OnConnect : %d\n", clientIndex);

		PacketInfo packet{ clientIndex, (UINT16)PACKET_ID::SYS_USER_CONNECT, 0 };
		mPacketManager->EnqueueSystemPacket(packet);
	}

	virtual void OnClose(const UINT32 clientIndex) override
	{
		printf("OnClose : %d\n", clientIndex);

		PacketInfo packet{ clientIndex, (UINT16)PACKET_ID::SYS_USER_DISCONNECT, 0 };
		mPacketManager->EnqueueSystemPacket(packet);
	}

	virtual void OnReceive(const UINT32 clientIndex, UINT32 size, char* pData) override
	{
		printf("OnReceive Index:%d, Size:%d\n", clientIndex, size);

		mPacketManager->ReceivePacket(clientIndex, size, pData);
	}

	void Run(const UINT16 maxClient)
	{
		auto sendPacketFunc = [&](UINT32 clientIndex, UINT32 size, char* pData)
		{
			SendMsg(clientIndex, size, pData);
		};

		mPacketManager = std::make_unique<PacketManager>();
		mPacketManager->SendPacketFunc = sendPacketFunc;
		mPacketManager->Init(maxClient);
		mPacketManager->Run();

		StartServer(maxClient);
	}

	void End()
	{
		mPacketManager->End();
		DestroyThread();
	}

private:
	std::unique_ptr<PacketManager> mPacketManager;
};
