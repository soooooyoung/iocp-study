#pragma once

#include <string>

#include "Packet.h"

class User
{
	const UINT32 PACKET_DATA_BUFFER_SIZE = 8096;

public:
	enum class DOMAIN_STATE
	{
		NONE = 0,
		LOGIN = 1,
		ROOM = 2
	};

	User() = default;
	~User() = default;

	void Init(const INT32 index)
	{
		mIndex = index;
		mPacketDataBuffer = new char[PACKET_DATA_BUFFER_SIZE];
	}

	void Clear()
	{
		mRoomIndex = -1;
		mUserID = "";
		mIsConfirm = false;
		mDomainState = DOMAIN_STATE::NONE;
		mPacketDataBufferWritePos = 0;
		mPacketDataBufferReadPos = 0;
	}

	int SetLogin(char* userID)
	{
		mDomainState = DOMAIN_STATE::LOGIN;
		mUserID = userID;

		return 0;
	}

	void EnterRoom(INT32 roomIndex)
	{
		mDomainState = DOMAIN_STATE::ROOM;
		mRoomIndex = roomIndex;
	}

	void SetDomainState(DOMAIN_STATE domainState)
	{
		mDomainState = domainState;
	}

	INT32 GetCurrentRoomIndex()
	{
		return mRoomIndex;
	}

	INT32 GetConnectionIndex()
	{
		return mIndex;
	}

	std::string GetUserID()
	{
		return mUserID;
	}

	DOMAIN_STATE GetDomainState()
	{
		return mDomainState;
	}

	void SetPacketData(const UINT32 dataSize, char* pData)
	{
		if (PACKET_DATA_BUFFER_SIZE <= mPacketDataBufferWritePos + dataSize)
		{

			auto remainSize = PACKET_DATA_BUFFER_SIZE - mPacketDataBufferWritePos;

			if (remainSize > 0)
			{
				CopyMemory(&mPacketDataBuffer[0], &mPacketDataBuffer[mPacketDataBufferWritePos], remainSize);
				mPacketDataBufferWritePos = 0;

			}
			else
			{
				mPacketDataBufferWritePos = 0;
			}

			mPacketDataBufferReadPos = 0;
		}

		CopyMemory(&mPacketDataBuffer[mPacketDataBufferWritePos], pData, dataSize);
		mPacketDataBufferWritePos += dataSize;
	}

	PacketInfo GetPacket()
	{
		const int PACKET_SIZE_LENGTH = 2;
		const int PACKET_ID_LENGTH = 2;

		short packetSize = 0;

		UINT32 remainSize = mPacketDataBufferWritePos - mPacketDataBufferReadPos;

		if (remainSize < PACKET_SIZE_LENGTH)
		{
			return PacketInfo();
		}

		auto pHeader = (PACKET_HEADER*)&mPacketDataBuffer[mPacketDataBufferReadPos];

		if (remainSize < pHeader->PacketLength)
		{
			return PacketInfo();
		}

		PacketInfo packetInfo;
		packetInfo.PacketID = pHeader->PacketID;
		packetInfo.DataSize = pHeader->PacketLength;
		packetInfo.pPacketData = &mPacketDataBuffer[mPacketDataBufferReadPos];

		mPacketDataBufferReadPos += pHeader->PacketLength;

		return packetInfo;
	}

private:
	INT32 mIndex = -1;
	INT32 mRoomIndex = -1;
	DOMAIN_STATE mDomainState = DOMAIN_STATE::NONE;

	std::string mUserID;
	std::string mAuthToken;

	bool mIsConfirm = false;

	char* mPacketDataBuffer = nullptr;

	UINT32 mPacketDataBufferWritePos = 0;
	UINT32 mPacketDataBufferReadPos = 0;
};