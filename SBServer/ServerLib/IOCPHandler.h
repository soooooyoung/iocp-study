#pragma once
#include <WinSock2.h>
#include <vector>
#include <thread>
#include <memory>

#include "NetworkConfig.h"

namespace NetworkLib
{
	class HostSocket;
	class NetworkContext;
	class IOCPHandler
	{
	public:
		IOCPHandler();
		virtual ~IOCPHandler();

		bool Initialize(const NetworkConfig& config);
		std::shared_ptr<HostSocket> AddHost(const std::string& address, const uint16_t port);

		bool Register(const SOCKET& socket);

	private:
		HANDLE mIOCPHandle = INVALID_HANDLE_VALUE;
		std::vector<std::thread> mIOThreadPool;
		std::vector<std::shared_ptr<HostSocket>> mHostSockets;

		void _IOThread();
		void _AcceptThread(std::weak_ptr<HostSocket> hostSocket);

		bool mIsRunning = false;
	};
}