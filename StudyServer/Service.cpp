#include "pch.h"
#include "Service.h"

Service::Service()
{
	mPacketHandler = std::unordered_map<int, PacketFunction>();
	RegisterPacketHandler(ServiceProtocol::ECHO, &Service::Echo);
}

Service::~Service()
{
}

void Service::RegisterPacketHandler(ServiceProtocol packetID, const PacketFunction& handler)
{
	mPacketHandler.emplace(static_cast<int>(packetID), handler);
}

void Service::ProcessPacket(std::shared_ptr<NetworkPacket> packet)
{
	auto packetID = packet->Header.PacketID;
	auto iter = mPacketHandler.find(packetID);
	if (iter == mPacketHandler.end())
	{
		printf_s("Unknown PacketID: %d\n", packetID);
		return;
	}

	auto func = iter->second;
	(this->*func)(*packet);

	packet.reset();
}

void Service::Echo(const NetworkPacket& packet)
{
	printf_s("Echo: %s\n", packet.Body.data());

	NetworkPacket responsePacket;
	responsePacket.Header.BodyLength = packet.Header.BodyLength;
	responsePacket.Header.PacketID = static_cast<int>(ServiceProtocol::ECHO);
	responsePacket.Header.SessionID = packet.Header.SessionID;
	memcpy(responsePacket.Body.data(), packet.Body.data(), packet.Header.BodyLength);

	printf_s("Echo Response: %s  To Session: %d\n", responsePacket.Body.data(), responsePacket.Header.SessionID);

	mSendFunction(packet.Header.SessionID, std::make_shared<NetworkPacket>(responsePacket));
}

