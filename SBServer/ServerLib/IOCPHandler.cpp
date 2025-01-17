#include "pch.h"
#include "IOCPHandler.h"
#include "HostSocket.h"
#include "ClientSocket.h"
#include "NetworkContext.h"

namespace NetworkLib
{
	IOCPHandler::IOCPHandler()
	{
	}

	IOCPHandler::~IOCPHandler()
	{
		mIsRunning = false;

		for (auto& host : mHostSockets)
		{
			host->Close();
		}

		for (auto& thread : mIOThreadPool)
		{
			thread.join();
		}

		CloseHandle(mIOCPHandle);
	}

	bool IOCPHandler::Initialize(const NetworkConfig& config)
	{
		SYSTEM_INFO systemInfo;
		GetSystemInfo(&systemInfo);

		mIOCPHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, systemInfo.dwNumberOfProcessors);

		if (mIOCPHandle == INVALID_HANDLE_VALUE)
		{
			return false;
		}

		for (int i = 0; i < systemInfo.dwNumberOfProcessors; ++i)
		{
			mIOThreadPool.emplace_back([this]() { _IOThread(); });
		}

		for (auto& host : config.mServerHosts)
		{
			auto hostSocket = AddHost(host.second.mHostAddress, host.second.mHostPort);

			if (hostSocket == nullptr)
			{
				return false;
			}

			if (host.second.mHostType == HostType::Acceptor)
			{
				std::jthread acceptThread([this, hostSocket]() { _AcceptThread(hostSocket); });
			}
		}
	}

	std::shared_ptr<HostSocket> IOCPHandler::AddHost(const std::string& address, const uint16_t port)
	{
		auto hostSocket = std::make_shared<HostSocket>();

		if (false == hostSocket->CreateSocket())
		{
			return nullptr;
		}

		if (false == hostSocket->Bind(address, port))
		{
			return nullptr;
		}

		if (false == hostSocket->Listen())
		{
			return nullptr;
		}

		if (false == Register(hostSocket->GetSocket()))
		{
			return nullptr;
		}

		mHostSockets.emplace_back(hostSocket);
		return hostSocket;
	}

	bool IOCPHandler::Register(const SOCKET& socket)
	{
		HANDLE handle = CreateIoCompletionPort((HANDLE)socket, mIOCPHandle, 0, 0);

		if (handle == NULL)
		{
			return false;
		}

		return true;
	}

	void IOCPHandler::_IOThread()
	{
		while (mIsRunning)
		{
			DWORD transferred = 0;
			ULONG_PTR completionKey = 0;
			LPOVERLAPPED overlapped = nullptr;

			BOOL result = GetQueuedCompletionStatus(mIOCPHandle, &transferred, &completionKey, &overlapped, INFINITE);

			if (result == FALSE)
			{
				DWORD error = GetLastError();
				continue;
			}

			if (result == TRUE && transferred == 0 && overlapped == nullptr)
			{
				continue;
			}

			auto context = reinterpret_cast<NetworkContext*>(overlapped);
			auto client = reinterpret_cast<ClientSocket*>(completionKey);

			if (context == nullptr)
			{
				continue;
			}
		}
	}

	void IOCPHandler::_AcceptThread(std::weak_ptr<HostSocket> hostSocket)
	{
		while (mIsRunning)
		{
			auto sharedHostSocket = hostSocket.lock();

			if (sharedHostSocket == nullptr)
			{
				break;
			}

			SOCKET clientSocket = sharedHostSocket->Accept();

			if (clientSocket == INVALID_SOCKET)
			{
				continue;
			}

			auto client = std::make_shared<ClientSocket>(clientSocket);

			if (false == Register(client->GetSocket()))
			{
				client->Close();
			}

			if (false == client->OnConnect())
			{
				client->Close();
			}
		}
	}
}