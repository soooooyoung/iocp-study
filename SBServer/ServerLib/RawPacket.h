#pragma once

#include <stdint.h>

#pragma pack(push, 1)
struct RawPacket
{
	struct Header
	{
		uint16_t mBodySize;
		uint16_t mPacketID;
	};

	Header mHeader;
	uint8_t* mBody;
};
#pragma pack(pop)