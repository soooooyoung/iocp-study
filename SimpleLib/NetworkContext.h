#pragma once
#include <winsock2.h>
#include <Ws2tcpip.h>
#include <mswsock.h>
#include "NetworkPacket.h"

enum class ContextType
{
	ACCEPT,
	RECV,
	SEND
};


struct NetworkContext
{
	WSAOVERLAPPED mWsaOverlapped;
	ContextType mContextType;
};