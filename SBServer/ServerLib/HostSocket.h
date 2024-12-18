#pragma once
#include <memory>

#include "NetworkSocket.h"

namespace NetworkLib
{
	class NetworkContext;
	class HostSocket : public NetworkSocket, public std::enable_shared_from_this<HostSocket>
	{
	public:
		HostSocket();
		virtual ~HostSocket();

		bool Bind(const std::string& address, const uint16_t port);
		bool Listen();

		SOCKET Accept();
		bool PostAccept(SOCKET& socket, NetworkContext* context);

		bool CreateSocket();
	private:
		SOCKADDR_IN _ResolveAddress(const std::string& address);
	};
}