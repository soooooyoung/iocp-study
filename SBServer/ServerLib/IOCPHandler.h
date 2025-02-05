#pragma once
#include <WinSock2.h>
#include <vector>
#include <deque>
#include <thread>
#include <memory>
#include <array>
#include <algorithm>
#include <functional>
#include <concurrent_queue.h>
#include <concurrent_vector.h>

#include "NetworkConfig.h"
#include "ClientSocket.h"
#include "NetworkContext.h"
#include "Packet.h"

namespace NetworkLib
{
	class DebugLogger;
	class HostSocket;
	class IOCPHandler
	{
	public:
		IOCPHandler();
		virtual ~IOCPHandler();

		bool Initialize(const NetworkConfig& config);
		std::shared_ptr<HostSocket> AddHost(const std::string& address, const uint16_t port);

		bool Register(const SOCKET& socket, const NetworkSocket* completionKey);
		bool InitClient(NetworkLib::ClientSocket& client, SOCKET socket);

		bool PushPacket(NetworkContext& context, ClientSocket& client);
		Packet& PopPacket();

		int AllocClient();
		int AllocNetworkContext();

		void ReleaseClient(int index);
		void ReleaseNetworkContext(int index);

		bool HasPacket() const { return mPacketQueue.empty() == false; }

		void IOThread();

	private:
		HANDLE mIOCPHandle = INVALID_HANDLE_VALUE;
		std::vector<std::thread> mIOThreadPool;
		std::vector<std::shared_ptr<HostSocket>> mHostSockets;

		void _AcceptThread(std::weak_ptr<HostSocket> hostSocket);

		bool mIsRunning = false;

		std::vector<ClientSocket> mClients;
		concurrency::concurrent_queue<int> mClientIndexPool;

		std::vector<NetworkContext> mNetworkContexts;
		concurrency::concurrent_queue<int> mNetworkContextIndexPool;

		concurrency::concurrent_queue<Packet> mPacketQueue;

		std::unique_ptr<DebugLogger> mLogger;
	};
}