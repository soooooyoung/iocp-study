#pragma once

#include <WinSock2.h>
#include <MSWSock.h>
#include <WS2tcpip.h>


enum class ContextType
{
	NONE,
	ACCEPT,
	RECV,
	SEND
};