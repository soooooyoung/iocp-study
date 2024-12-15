#pragma once
#include <memory>
#include <concurrent_queue.h>
#include <unordered_map>
#include <functional>

#include "SharedEnum.h"
#include "MemoryPool.h"

struct Packet;
struct NetworkPacket;
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

	virtual void ProcessPacket(MemoryPool<Packet>::UniquePtr packet);

	std::function<void(int, std::unique_ptr<Packet>)> mSendFunction;
};