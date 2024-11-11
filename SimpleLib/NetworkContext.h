#pragma once
#include <winsock2.h>
#include <Ws2tcpip.h>
#include <mswsock.h>
#include "NetworkPacket.h"

enum class ContextType
{
	NONE,
	ACCEPT,
	RECV,
	SEND
};

struct NetworkContext
{
	WSAOVERLAPPED mWsaOverlapped;
	ContextType mContextType;
	UINT32 mSessionID;

	void Clear()
	{
		ZeroMemory(&mWsaOverlapped, sizeof(WSAOVERLAPPED));
		mContextType = ContextType::NONE;
		mSessionID = 0;
	}
};