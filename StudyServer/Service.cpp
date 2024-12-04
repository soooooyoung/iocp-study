#include "Service.h"
#include "NetworkPacket.h"

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
}

