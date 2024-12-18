#pragma once

#include <string>
#include <cstdint>

#pragma comment(lib, "ws2_32")

#include <WinSock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>    

namespace NetworkLib
{
	class NetworkSocket 
	{
	public:
		NetworkSocket() = default;
		virtual ~NetworkSocket() = default;

		bool SetSocketReusable(const SOCKET& socket);
		bool SetSocketNonBlocking(const SOCKET& socket);
		bool SetSocketNoDelay(const SOCKET& socket);

	protected:
		SOCKET mSocket = INVALID_SOCKET;
	};
}