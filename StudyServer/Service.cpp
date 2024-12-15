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

void Service::ProcessPacket(MemoryPool<Packet>::UniquePtr packet)
{
	auto packetID = packet->PacketID;

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

void Service::Echo(const Packet& packet)
{
	printf_s("Echo: %s\n", packet.Body.data());

	Packet response;
	response.PacketID = static_cast<int>(ServiceProtocol::ECHO);
	response.BodyLength = packet.BodyLength;
	memcpy(response.Body.data(), packet.Body.data(), packet.BodyLength);

	mSendFunction(packet.SessionID, std::make_unique<Packet>(response));
}
