#include "pch.h"
#include "NetworkSocket.h"

namespace NetworkLib
{
	bool NetworkSocket::SetSocketReusable(const SOCKET& socket)
	{
		int32_t opt = 1;
		/* SO_REUSEADDR */
		if (SOCKET_ERROR == setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt)))
		{
			return false;
		}

		return true;
	}

	// Non-blocking mode means that operations like recv, send, connect, and accept will return immediately instead of blocking the program until the operation completes.
	bool NetworkSocket::SetSocketNonBlocking(const SOCKET& socket)
	{
		unsigned long ul = 1;

		/* ioctlsocket */
		// Windows-specific function used to manipulate the mode of a socket.
		if (SOCKET_ERROR == ioctlsocket(socket, FIONBIO, &ul))
		{
			return false;
		}

		return true;
	}

	bool NetworkSocket::SetSocketBlocking(const SOCKET& socket)
	{
		unsigned long ul = 0;
		/* ioctlsocket */
		// Windows-specific function used to manipulate the mode of a socket.
		if (SOCKET_ERROR == ioctlsocket(socket, FIONBIO, &ul))
		{
			return false;
		}
		return true;
	}


	bool NetworkSocket::SetSocketNoDelay(const SOCKET& socket)
	{
		int flag = 1;
		if (SOCKET_ERROR == setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, (const char*)&flag, sizeof(flag)))
		{
			return false;
		}

		return true;
	}
}