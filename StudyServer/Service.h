#pragma once
#include <memory>
#include <concurrent_vector.h>
#include <unordered_map>
#include <functional>


#include "SharedEnum.h"
#include "MemoryPool.h"

struct Packet;
class Service
{
private:
	typedef void(Service::* PacketFunction)(const Packet&);
	std::unordered_map<int, PacketFunction> mPacketHandler;
protected:
	virtual void Echo(const Packet& packet);

public:
	Service();
	virtual ~Service();

	void RegisterPacketHandler(ServiceProtocol packetID, const PacketFunction& handler);

	virtual void ProcessPacket(const Packet& packet);

	std::function<void(int, const Packet&)> mSendFunction;
};