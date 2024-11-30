#pragma once
#include <array>
#include <cstdint>


/*
* TODO: Multibyte integers must be converted to network byte order before sending
*/
#pragma pack(push, 1)
struct NetworkPacket
{
	struct PacketHeader
	{
		uint32_t BodyLength = 0;	// Length of the Body
		uint32_t PacketID = 0;     // ID of the packet
		uint32_t SessionID = 0;    // ID of the session
	};

	PacketHeader Header{};
	std::array<uint8_t, 8192> Body = { 0 };

	std::size_t GetPacketSize() const
	{
		return sizeof(PacketHeader) + GetBodySize();
	}

	std::size_t GetBodySize() const
	{
		return Header.BodyLength;
	}

	const uint8_t* GetBody()
	{
		return Body.data();
	}
};
#pragma pack(pop)