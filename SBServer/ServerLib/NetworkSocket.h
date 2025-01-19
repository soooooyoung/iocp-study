#pragma once 

#include "stdint.h"

namespace NetworkLib
{
	class NetworkSocket 
	{
	public:
		NetworkSocket() = default;
		virtual ~NetworkSocket() = default;

		bool SetSocketReusable(const SOCKET& socket);
		bool SetSocketNonBlocking(const SOCKET& socket);
		bool SetSocketBlocking(const SOCKET& socket);
		bool SetSocketNoDelay(const SOCKET& socket);

		SOCKET GetSocket() const { return mSocket; }

	protected:
		SOCKET mSocket = INVALID_SOCKET;
	};
}