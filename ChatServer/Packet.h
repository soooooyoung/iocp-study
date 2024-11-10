#pragma once 

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

struct RawPacketData
{
	UINT32 ClientIndex = 0;
	UINT32 DataSize = 0;
	char* pPacketData = nullptr;

	void Set(RawPacketData& packetData)
	{
		ClientIndex = packetData.ClientIndex;
		DataSize = packetData.DataSize;
		pPacketData = new char[packetData.DataSize];
		CopyMemory(pPacketData, packetData.pPacketData, packetData.DataSize);
	}

	void Set(UINT32 clientIndex, UINT32 dataSize, char* pData)
	{
		ClientIndex = clientIndex;
		DataSize = dataSize;
		pPacketData = new char[dataSize];
		CopyMemory(pPacketData, pData, dataSize);
	}

	void Release()
	{
		if (pPacketData != nullptr)
		{
			delete[] pPacketData;
			pPacketData = nullptr;
		}
	}
};

struct PacketInfo
{
	UINT32 ClientIndex = 0;
	UINT16 PacketID = 0;
	UINT16 DataSize = 0;
	char* pPacketData = nullptr;
};

enum class  PACKET_ID : UINT16
{
	//SYSTEM
	SYS_USER_CONNECT = 11,
	SYS_USER_DISCONNECT = 12,
	SYS_END = 30,

	//DB
	DB_END = 199,

	//Client
	LOGIN_REQUEST = 201,
	LOGIN_RESPONSE = 202,

	ROOM_ENTER_REQUEST = 206,
	ROOM_ENTER_RESPONSE = 207,

	ROOM_LEAVE_REQUEST = 215,
	ROOM_LEAVE_RESPONSE = 216,

	ROOM_CHAT_REQUEST = 221,
	ROOM_CHAT_RESPONSE = 222,
	ROOM_CHAT_NOTIFY = 223,
};

#pragma pack(push,1)
struct PACKET_HEADER
{
	UINT16 PacketLength;
	UINT16 PacketID;
	UINT8 Type; // 압축여부 암호화여부 등
};

const UINT32 PACKET_HEADAER_LENGTH = sizeof(PACKET_HEADER);



////////////////////////////////////////////////////////////////
// 로그인 
const int MAX_USER_ID_LENGTH = 32;
const int MAX_USER_PW_LENGTH = 32;

struct LOGIN_REQUEST_PACKET : public PACKET_HEADER
{
	char UserID[MAX_USER_ID_LENGTH + 1];
	char UserPW[MAX_USER_PW_LENGTH + 1];
};

const size_t LOGIN_REQUEST_PACKET_SIZE = sizeof(LOGIN_REQUEST_PACKET);

struct LOGIN_RESPONSE_PACKET : public PACKET_HEADER
{
	UINT32 Result;
};

////////////////////////////////////////////////////////////////
// 룸 나가기
struct ROOM_LEAVE_REQUEST_PACKET : public PACKET_HEADER
{
};

struct ROOM_LEAVE_RESPONSE_PACKET : public PACKET_HEADER
{
	UINT32 Result;
};

////////////////////////////////////////////////////////////////
// 채팅
const int MAX_CHAT_SIZE = 256;
struct ROOM_CHAT_REQUEST_PACKET : public PACKET_HEADER
{
	char Message[MAX_CHAT_SIZE + 1] = { 0, };
};

struct ROOM_CHAT_RESPONSE_PACKET : public PACKET_HEADER
{
	UINT32 Result;
};

struct ROOM_CHAT_NOTIFY_PACKET : public PACKET_HEADER
{
	char UserID[MAX_USER_ID_LENGTH + 1] = { 0, };
	char Message[MAX_CHAT_SIZE + 1] = { 0, };
};
#pragma pack(pop)