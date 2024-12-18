#pragma once

#include "NetworkSocket.h"

namespace NetworkLib
{
	class NetworkContext;
	class ClientSocket : public NetworkSocket
	{
	public:
		ClientSocket() = default;
		virtual ~ClientSocket() = default;

		bool Send(NetworkContext* context);
		bool Receive(NetworkContext* context);
		void Close();

		bool SetSocketOptions();
		void SetSocket(SOCKET& socket);
	};
}