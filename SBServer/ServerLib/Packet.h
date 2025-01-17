#pragma once
#include <array>

const int MAX_PACKET_SIZE = 1024;

enum class PacketProtocol
{
	NONE = 0,
	ECHO = 1,
};

struct Packet
{
	int mSessionID;
	int mPacketID;
	int mDataSize;
	int mDataIndex;

	std::array<uint8_t, MAX_PACKET_SIZE> mData;
};