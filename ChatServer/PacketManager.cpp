#include "PacketManager.h"
#include "UserManager.h"

void PacketManager::Init(const UINT32 maxClient)
{
	mRecvPacketHandlerMap = std::unordered_map<int, PROCESS_RECV_PACKET_FUNCTION>();

	mRecvPacketHandlerMap[(int)PACKET_ID::SYS_USER_CONNECT] = &PacketManager::ProcessUserConnect;
	mRecvPacketHandlerMap[(int)PACKET_ID::SYS_USER_DISCONNECT] = &PacketManager::ProcessUserDisconnect;
	mRecvPacketHandlerMap[(int)PACKET_ID::LOGIN_REQUEST] = &PacketManager::ProcessLogin;

	CreateComponent(maxClient);
}

bool PacketManager::Run()
{
	mIsProcessThreadRunning = true;
	mProcessThread = std::thread(&PacketManager::ProcessPacket, this);

	return true;
}

void PacketManager::End()
{
	mIsProcessThreadRunning = false;
	
	if (mProcessThread.joinable())
	{
		mProcessThread.join();
	}
}

void PacketManager::ReceivePacket(const UINT32 clientIndex, UINT32 size, char* pData)
{
	auto pUser = mUserManager->GetUserByConnectionIndex(clientIndex);
	
	if (nullptr == pUser)
	{
		return;
	}

	pUser->SetPacketData(size, pData);
	EnqueuePacket(clientIndex);
}

void PacketManager::EnqueueSystemPacket(PacketInfo packet)
{
	std::lock_guard<std::mutex> lock(mLock);
	mSystemPacketQueue.push_back(packet);
}

void PacketManager::CreateComponent(const UINT32 maxClient)
{
	mUserManager = new UserManager();
	mUserManager->Init(maxClient);
}

void PacketManager::ClearConnectionInfo(INT32 clientIndex)
{
	auto pUser = mUserManager->GetUserByConnectionIndex(clientIndex);

	if (nullptr == pUser)
	{
		return;
	}

	if (User::DOMAIN_STATE::NONE != pUser->GetDomainState())
	{
		mUserManager->DeleteUserInfo(pUser);
	}
}

void PacketManager::EnqueuePacket(const UINT32 clientIndex)
{
	std::lock_guard<std::mutex> lock(mLock);
	mIncomingPacketUserIndex.push_back(clientIndex);
}

PacketInfo PacketManager::DequeuePacket()
{
	UINT32 userIndex = 0;

	{
		std::lock_guard<std::mutex> lock(mLock);
		if (mIncomingPacketUserIndex.empty())
		{
			return PacketInfo();
		}

		userIndex = mIncomingPacketUserIndex.front();
		mIncomingPacketUserIndex.pop_front();
	}

	auto pUser = mUserManager->GetUserByConnectionIndex(userIndex);

	if (nullptr == pUser)
	{
		return PacketInfo();
	}

	auto packetData = pUser->GetPacket();
	packetData.ClientIndex = userIndex;

	return packetData;
}

PacketInfo PacketManager::DequeueSystemPacket()
{

	std::lock_guard<std::mutex> lock(mLock);

	if (mSystemPacketQueue.empty())
	{
		return PacketInfo();
	}

	auto packetData = mSystemPacketQueue.front();
	mSystemPacketQueue.pop_front();

	return packetData;
}

void PacketManager::ProcessPacket()
{
	while (mIsProcessThreadRunning)
	{
		bool isIdle = true;

		if (auto packetData = DequeuePacket();
			packetData.PacketID > (UINT16)PACKET_ID::SYS_END)
		{
			isIdle = false;
			ProcessRecvPacket(packetData.ClientIndex, packetData.PacketID, packetData.DataSize, packetData.pPacketData);
		}

		if (auto packetData = DequeueSystemPacket();
			packetData.PacketID <= (UINT16)PACKET_ID::SYS_END)
		{
			isIdle = false;
			ProcessRecvPacket(packetData.ClientIndex, packetData.PacketID, packetData.DataSize, packetData.pPacketData);
		}

		if (isIdle)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}
}

void PacketManager::ProcessRecvPacket(const UINT32 clientIndex, const UINT32 packetId, const UINT16 packetSize, char* pPacketData)
{
	auto iter = mRecvPacketHandlerMap.find(packetId);

	if (iter != mRecvPacketHandlerMap.end())
	{
		(this->*(iter->second))(clientIndex, packetSize, pPacketData);
	}
}

void PacketManager::ProcessUserConnect(UINT32 clientIndex, UINT16 packetSize, char* pPacketData)
{
	printf("ProcessUserConnect client index : %d\n", clientIndex);

	auto pUser = mUserManager->GetUserByConnectionIndex(clientIndex);

	if (nullptr == pUser)
	{
		return;
	}

	pUser->Clear();
}

void PacketManager::ProcessUserDisconnect(UINT32 clientIndex, UINT16 packetSize, char* pPacketData)
{
	printf("ProcessUserDisconnect client index : %d\n", clientIndex);	

	auto pUser = mUserManager->GetUserByConnectionIndex(clientIndex);
	pUser->Clear();
}

void PacketManager::ProcessLogin(UINT32 clientIndex, UINT16 packetSize, char* pPacketData)
{
	if (LOGIN_REQUEST_PACKET_SIZE != packetSize || 
		nullptr == pPacketData)
	{
		return;
	}

	auto pLoginRequestPacket = reinterpret_cast<LOGIN_REQUEST_PACKET*>(pPacketData);

	auto pUserID = pLoginRequestPacket->UserID;
	printf("ProcessLogin client index : %d, user id : %s\n", clientIndex, pUserID);

	LOGIN_RESPONSE_PACKET loginResponsePacket;
	loginResponsePacket.PacketID = (UINT16)PACKET_ID::LOGIN_REQUEST;
	loginResponsePacket.PacketLength = sizeof(LOGIN_REQUEST_PACKET);

	if (mUserManager->GetCurrentUserCount() >= mUserManager->GetMaxUserCount())
	{
		loginResponsePacket.Result = (UINT16)ERROR_CODE::LOGIN_USER_USED_ALL_OBJ;
		SendPacketFunc(clientIndex, sizeof(LOGIN_RESPONSE_PACKET), (char*)&loginResponsePacket);
	}
	else if (mUserManager->FindUserIndexByID(pUserID) != -1)
	{
		loginResponsePacket.Result = (UINT16)ERROR_CODE::NONE;
		SendPacketFunc(clientIndex, sizeof(LOGIN_RESPONSE_PACKET), (char*)&loginResponsePacket);
	}
	else 
	{
		loginResponsePacket.Result = (UINT16)ERROR_CODE::LOGIN_USER_ALREADY;
		SendPacketFunc(clientIndex, sizeof(LOGIN_RESPONSE_PACKET), (char*)&loginResponsePacket);
	}
}
