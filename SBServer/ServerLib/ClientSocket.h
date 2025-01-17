#pragma once
#include <memory>

#include "NetworkSocket.h"
#include "Timer.h"
#include "Packet.h"

namespace NetworkLib
{
	const uint32_t MAX_PACKETS_PER_CLIENT = 150;

	class NetworkContext;
	class ClientSocket : public NetworkSocket, public std::enable_shared_from_this<ClientSocket>
	{
	public:
		ClientSocket(int index) {
			mIndex = index;
		}

		virtual ~ClientSocket();

		void SetSocket(SOCKET socket) { mSocket = socket; }

		bool OnConnect();
		bool OnReceive();

		bool Send(NetworkContext* context);
		bool Receive(NetworkContext* context);
		void Close();

		bool IsConnected() const { return mIsConnected; }
		int GetSessionID() const { return mIndex; }

	private:
		bool mIsConnected = false;

		int mIndex{ 0 };
		int mReceiveCount{ 0 };

		Timer mReceiveTimer = Timer([this]() {
			mReceiveCount = 0;
			}, 1000);
	};
}