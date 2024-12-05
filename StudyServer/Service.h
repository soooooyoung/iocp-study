#pragma once
#include <memory>
#include <concurrent_queue.h>
#include <unordered_map>
#include <functional>

#include "SharedEnum.h"

struct NetworkPacket;
class Service
{
private:
	typedef void(Service::* PacketFunction)(const NetworkPacket&);
	std::unordered_map<int, PacketFunction> mPacketHandler;

protected:
	virtual void Echo(const NetworkPacket& packet);

public:
	Service();
	virtual ~Service();

	void RegisterPacketHandler(ServiceProtocol packetID, const PacketFunction& handler);

	virtual void ProcessPacket(std::shared_ptr<NetworkPacket> packet);

	std::function<void(int, std::shared_ptr<NetworkPacket>)> mSendFunction;
};