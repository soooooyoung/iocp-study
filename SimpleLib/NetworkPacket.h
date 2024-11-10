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
    std::array<std::uint8_t, 1024 * 8> Body{};

    std::size_t GetPacketSize() const
    {
		return ntohl(Header.PacketLength);
    }

	std::size_t GetBodySize() const
	{
		return GetPacketSize() - sizeof(PacketHeader);
	}

	std::size_t GetHeaderSize() const
	{
		return sizeof(PacketHeader);
	}

	std::span<std::uint8_t> GetBody()
	{
		return std::span<std::uint8_t>(Body.data(), GetBodySize());
	}
};
#pragma pack(pop)