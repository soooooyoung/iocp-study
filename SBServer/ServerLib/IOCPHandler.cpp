#include "pch.h"
#include "IOCPHandler.h"
#include "HostSocket.h"
#include "ClientSocket.h"
#include "NetworkContext.h"
#include "RawPacket.h"
#include "DebugLogger.h"

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
		mLogger = std::make_unique<DebugLogger>();
		mLogger->InitializeAsync();

		SYSTEM_INFO systemInfo;
		GetSystemInfo(&systemInfo);

		mIOCPHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, systemInfo.dwNumberOfProcessors);

		if (mIOCPHandle == INVALID_HANDLE_VALUE)
		{
			return false;
		}

		mIsRunning = true;

		for (int i = 1; i < 111; ++i)
		{
			mClients.emplace_back(ClientSocket(i));
			mClientIndexPool.push(i);
		}

		for (int i = 0; i < 1000; ++i)
		{
			mNetworkContexts.emplace_back(NetworkContext(i));
			mNetworkContextIndexPool.push(i);
		}

		for (int i = 0; i < systemInfo.dwNumberOfProcessors; ++i)
		{
			mIOThreadPool.emplace_back([this]() { IOThread(); });
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

		return true;
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

		if (false == Register(hostSocket->GetSocket(), hostSocket.get()))
		{
			return nullptr;
		}

		mHostSockets.emplace_back(hostSocket);
		mLogger->LogAsync("AddHost {}:{}", address, port);

		return hostSocket;
	}

	bool IOCPHandler::Register(const SOCKET& socket, const NetworkSocket* completionKey)
	{
		HANDLE handle = CreateIoCompletionPort((HANDLE)socket, mIOCPHandle, (ULONG_PTR)completionKey, 0);

		if (handle == NULL)
		{
			return false;
		}

		return true;
	}

	bool IOCPHandler::PushPacket(NetworkContext& context, ClientSocket& client)
	{
		if (context.mBuffer->GetDataSize() < sizeof(RawPacket::Header))
		{
			return false;
		}

		auto rawPacket = reinterpret_cast<RawPacket*>(context.mBuffer->GetReadBuffer());

		if (context.mBuffer->GetDataSize() < rawPacket->mHeader.mBodySize + sizeof(RawPacket::Header))
		{
			return false;
		}

		if (false == client.OnReceive())
		{
			context.Reset();
			client.Close();
			mNetworkContextIndexPool.push(context.Index);
			mClientIndexPool.push(client.GetSessionID());
			return false;
		}

		Packet packet{};
		packet.mSessionID = client.GetSessionID();
		packet.mPacketID = rawPacket->mHeader.mPacketID;
		packet.mDataSize = rawPacket->mHeader.mBodySize;
		packet.mDataIndex = context.Index;

		std::memcpy(packet.mData.data(), rawPacket->mBody, packet.mDataSize);

		mLogger->LogAsync("Received Packet SessionID:{} PacketID:{} DataSize:{}", packet.mSessionID, packet.mPacketID, packet.mDataSize);

		mPacketQueue.push(std::move(packet));
		return true;
	}

	Packet& IOCPHandler::PopPacket()
	{
		Packet packet{};
		mPacketQueue.try_pop(packet);
		return packet;
	}

	int IOCPHandler::AllocClient()
	{
		int index = 0;

		if (false == mClientIndexPool.try_pop(index))
		{
			return -1;
		}

		return index;
	}

	int IOCPHandler::AllocNetworkContext()
	{
		int index = 0;

		if (false == mNetworkContextIndexPool.try_pop(index))
		{
			return -1;
		}

		return index;
	}

	void IOCPHandler::ReleaseClient(int index)
	{
		mClientIndexPool.push(index);
	}

	void IOCPHandler::ReleaseNetworkContext(int index)
	{
		mNetworkContextIndexPool.push(index);
	}

	void IOCPHandler::IOThread()
	{
		while (mIsRunning)
		{
			DWORD transferred = 0;
			ULONG_PTR completionKey = 0;
			LPOVERLAPPED overlapped = nullptr;

			BOOL result = GetQueuedCompletionStatus(mIOCPHandle, &transferred, &completionKey, &overlapped, INFINITE);

			if (result == FALSE)
			{
				continue;
			}

			auto context = reinterpret_cast<NetworkContext*>(overlapped);
			auto client = reinterpret_cast<ClientSocket*>(completionKey);

			if (result == TRUE && transferred == 0 && overlapped == nullptr) 
			{
				if (client != nullptr)
				{
					client->Close();
					ReleaseClient(client->GetSessionID());
					spdlog::info("Client SessionID:{} Disconnected", client->GetSessionID());
				}

				continue;
			}

			if (context == nullptr)
			{
				continue;
			}

			switch (context->GetContextType())
			{
			case ContextType::RECEIVE:
			{
				spdlog::info("Received Data SessionID:{} Size:{}", client->GetSessionID(), transferred);

				context->mBuffer->Write(transferred);

				PushPacket(*context, *client);

				if (false == client->Receive(*context))
				{
					client->Close();
					ReleaseClient(client->GetSessionID());
				}
			}
			continue;

			default:
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

			mLogger->LogAsync("Accepted ClientSocket:{}", clientSocket);

			int clientIndex = AllocClient();
			int contextIndex = AllocNetworkContext();

			if (clientIndex < 0 ||
				contextIndex < 0)
			{
				closesocket(clientSocket);
				continue;
			}

			auto& client = mClients[clientIndex];

			if (false == InitClient(client, clientSocket))
			{
				ReleaseClient(clientIndex);
				continue;
			}

			auto& context = mNetworkContexts[contextIndex];

			context.Reset();
			context.SetContextType(ContextType::RECEIVE);

			if (false == client.Receive(context))
			{
				closesocket(clientSocket);
				ReleaseClient(clientIndex);
				ReleaseNetworkContext(contextIndex);
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}

	bool IOCPHandler::InitClient(NetworkLib::ClientSocket& client, SOCKET socket)
	{
		client.SetSocket(socket);

		if (false == Register(client.GetSocket(), &client))
		{
			client.Close();
		}

		if (false == client.OnConnect())
		{
			client.Close();
		}

		spdlog::info("Client SessionID:{} Connected", client.GetSessionID());

		return true;
	}
}