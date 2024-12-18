#pragma once

#include "Packet.h"

#include <unordered_map>
#include <deque>
#include <functional>
#include <thread>
#include <mutex>

class UserManager;
class RoomManager;
class RedisManager;

class PacketManager {
public:
	PacketManager() = default;
	~PacketManager() = default;

	void Init(const UINT32 maxClient);

	bool Run();

	void End();

	void ReceivePacket(const UINT32 clientIndex, UINT32 size, char* pData);
	void EnqueueSystemPacket(PacketInfo packet);

	std::function<void(UINT32, UINT32, char*)> SendPacketFunc;

private:
	void CreateComponent(const UINT32 maxClient);
	void ClearConnectionInfo(INT32 clientIndex);
	void EnqueuePacket(const UINT32 clientIndex);

	PacketInfo DequeuePacket();
	PacketInfo DequeueSystemPacket();

	void ProcessPacket();
	void ProcessRecvPacket(const UINT32 clientIndex, const UINT32 packetId, const UINT16 packetSize, char* pPacketData);

	void ProcessUserConnect(UINT32 clientIndex, UINT16 packetSize, char* pPacketData);
	void ProcessUserDisconnect(UINT32 clientIndex, UINT16 packetSize, char* pPacketData);
	void ProcessLogin(UINT32 clientIndex, UINT16 packetSize, char* pPacketData);

private:
	typedef void(PacketManager::* PROCESS_RECV_PACKET_FUNCTION)(UINT32, UINT16, char*);
	std::unordered_map<int, PROCESS_RECV_PACKET_FUNCTION> mRecvPacketHandlerMap;

	UserManager* mUserManager;

	bool mIsProcessThreadRunning = false;

	std::thread mProcessThread;
	std::mutex mLock;
	std::deque<UINT32> mIncomingPacketUserIndex;
	std::deque<PacketInfo> mSystemPacketQueue;
	std::function<void(int, char*)> mSendMQDataFunc;
};