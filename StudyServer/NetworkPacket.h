#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdint>
#include <array>
#include <span>
#include <WinSock2.h>

#pragma pack(push, 1)
struct NetworkPacket
{
	struct PacketHeader
	{
		UINT32 PacketLength = 0; // Length of the entire packet
		UINT32 PacketID = 0;     // ID of the packet
	};

	PacketHeader Header{};
	std::array<std::uint8_t, 8096> Body{};

	std::size_t GetPacketSize() const
	{
		return ntohl(Header.PacketLength);
	}

	std::size_t GetBodySize() const
	{
		return GetPacketSize() - sizeof(PacketHeader);
	}

	const uint8_t* GetBody()
	{
		return Body.data();
	}
};
#pragma pack(pop)