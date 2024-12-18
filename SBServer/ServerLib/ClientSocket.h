#pragma once
#include <memory>

#include "NetworkSocket.h"

namespace NetworkLib
{
	class NetworkContext;
	class ClientSocket : public NetworkSocket, public std::enable_shared_from_this<ClientSocket>
	{
	public:
		ClientSocket() = default;
		ClientSocket(const SOCKET& socket) { mSocket = socket; }
		virtual ~ClientSocket() = default;

		bool OnConnect();

		bool Send(NetworkContext* context);
		bool Receive(NetworkContext* context);
		void Close();

		void SetSocket(SOCKET& socket);

		bool IsConnected() const { return mIsConnected; }

	private:
		bool mIsConnected = false;
	};
}