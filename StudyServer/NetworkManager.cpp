#include "pch.h"
#include "NetworkContext.h"
#include "NetworkManager.h"
#include "Service.h"
#include "NetworkClient.h"
#include "ListenClient.h"
#include "NetworkDispatcher.h"
#include "ConfigLoader.h"

NetworkManager::NetworkManager() : 
	mIsRunning(false), 
	mIOCPHandle(INVALID_HANDLE_VALUE)
{
}

NetworkManager::~NetworkManager()
{
	WSACleanup();
	Shutdown();

	mIOThreadPool.clear();
	mClientList.clear();
	mClientPool.clear();
}

bool NetworkManager::Initialize()
{
	ServerConfig config = ConfigLoader::GetInstance().GetServerConfig();
	SystemConfig systemConfig = ConfigLoader::GetInstance().GetSystemConfig();
	mPacketPool = std::make_shared<MemoryPool<Packet>>(systemConfig.mPacketPoolSize);


	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		return false;
	}

	SYSTEM_INFO systemInfo;
	GetSystemInfo(&systemInfo);

	int ioThreadCount = systemInfo.dwNumberOfProcessors * systemConfig.mThreadPerCore;

	mIOCPHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, ioThreadCount);

	if (NULL == mIOCPHandle)
	{
		printf_s("CreateIoCompletionPort() Error: %d\n", GetLastError());
		return false;
	}

	mIsRunning = true;

	for (int i = 0; i < ioThreadCount; ++i)
	{
		mIOThreadPool.emplace_back([this]() { WorkerThread(); });
	}

	// Add Main Listener
	if (false == AddListener(0, config.mServerPort, config.mServerAddress))
	{
		return false;
	}

	// TODO: Add Sub Listeners

	printf("Server Bind and Listen Success\n");
	return true;
}

bool NetworkManager::AddListener(int index, int port, const std::string& address)
{
	std::shared_ptr<ListenClient> listenClient = std::make_shared<ListenClient>();

	// Initialize Listen Socket
	if (false == listenClient->Init())
	{
		return false;
	}

	if (false == listenClient->Listen(port, address))
	{
		return false;
	}

	auto hIOCP = CreateIoCompletionPort((HANDLE)listenClient->GetSocket(), mIOCPHandle, (ULONG_PTR)listenClient.get(), 0);

	if (INVALID_HANDLE_VALUE == hIOCP)
	{
		printf_s("CreateIoCompletionPort() Error: %d\n", GetLastError());
		return false;
	}

	// Post Accept
	listenClient->PostAccept();

	// Add Listener to List
	listenClient->SetSessionID(index);
	mListenClientList[index] = std::move(listenClient);

	return true;
}


void NetworkManager::Shutdown()
{
	mIsRunning = false;

	for (auto& listenClient : mListenClientList)
	{
		if (nullptr == listenClient)
		{
			continue;
		}
		listenClient->Close();
	}

	for (auto& client : mClientList)
	{
		if (nullptr == client)
		{
			continue;
		}
		client->Close();
	}

	for (auto& thread : mIOThreadPool)
	{
		if (thread.joinable())
		{
			thread.join();
		}
	}

	if (INVALID_HANDLE_VALUE != mIOCPHandle)
	{
		CloseHandle(mIOCPHandle);
	}

}

bool NetworkManager::RegisterService(int serviceID, std::unique_ptr<Service> service)
{
	if (nullptr == service)
	{
		return false;
	}

	auto sendFunction = [this](int sessionID,const Packet& packet)
		{
			auto pooledPacket = mPacketPool->Acquire();
			if (nullptr == pooledPacket)
			{
				printf_s("Send Packet Fail\n");
				return;
			}

			*pooledPacket = packet;

			if (false == PushSendPacket(sessionID, std::move(pooledPacket)))
			{
				printf_s("Send Packet Fail\n");
			}
			
		};

	service->mSendFunction = sendFunction;

	auto dispatcher = std::make_unique<NetworkDispatcher>();

	if (false == dispatcher->Initialize(std::move(service)))
	{
		return false;
	}

	mServiceList.emplace(serviceID, std::move(dispatcher));

	return true;
}

bool NetworkManager::PushSendPacket(int sessionID, MemoryPool<Packet>::UniquePtr packet)
{
	if (sessionID < 0 || sessionID >= mClientList.size())
	{
		printf_s("Invalid SessionID\n");
		return false;
	}

	auto& client = mClientList.at(sessionID);
	if (nullptr == client)
	{
		printf_s("Invalid Client\n");
		return false;
	}

	if (client->IsConnected() == false)
	{
		printf_s("Client Disconnected\n");
		return false;
	}

	client->EnqueuePacket(std::move(packet));

	// send only if it's not sending
	if (false == client->mSending.exchange(true))
	{ 
		if (false == client->Send())
		{
			printf_s("Send Packet Fail\n");
			return false;
		}
	}

	return true;
}

void NetworkManager::WorkerThread()
{
	BOOL bSuccess = TRUE;
	DWORD dwIoSize = 0;
	ULONG_PTR key = 0;
	LPOVERLAPPED lpOverlapped = NULL;

	while (mIsRunning)
	{
		bSuccess = GetQueuedCompletionStatus(mIOCPHandle, &dwIoSize, &key, &lpOverlapped, INFINITE);

		if (TRUE == bSuccess && 0 == dwIoSize && NULL == lpOverlapped)
		{
			mIsRunning = false;
			printf_s("WorkerThread Exit\n");
			continue;
		}

		if (nullptr == lpOverlapped)
		{
			printf_s("Invalid Overlapped\n");
			continue;
		}

		auto client = (NetworkClient*)key;
		auto context = (NetworkContext*)lpOverlapped;

		if (FALSE == bSuccess ||
			(0 == dwIoSize && ContextType::ACCEPT != context->mContextType))
		{
			if (nullptr != client)
			{
				printf_s("Client Disconnected: %d\n", client->GetSessionID());
				RemoveClient(*client);
			}

			printf_s("WorkerThread Fail: %d\n", WSAGetLastError());
			continue;
		}

		printf("WorkerThread Success: %d\n", dwIoSize);

		switch (context->mContextType)
		{
		case ContextType::ACCEPT:
		{
			auto listenClient = static_cast<ListenClient*>(client);

			if (nullptr == context || 
				nullptr == listenClient)
			{
				printf_s("WorkerThread Fail: Invalid Context or ListenClient\n");
				continue;
			}

			_HandleAccept(*listenClient, *context);
		}
		break;
		case ContextType::RECV:
		{
			_HandleReceive(*client, *context, dwIoSize);
		}
		break;
		case ContextType::SEND:
		{
			_HandleSend(*client, *context, dwIoSize);
		}
		break;
		default:
		{
			printf("Unknown Context Type\n");
		}
		break;
		}
	}
}


void NetworkManager::_HandleAccept(ListenClient& listenClient, NetworkContext& context)
{
	SOCKET clientSocket = context.mSocket;

	if (INVALID_SOCKET == clientSocket)
	{
		printf_s("_HandleAccept ERROR INVALID_SOCKET: %d\n", WSAGetLastError());
		return;
	}

	// Associate Client Socket to Listener 
	if (false == listenClient.Accept(clientSocket))
	{
		printf_s("_HandleAccept ERROR: Accept\n");
		return;
	}

	// Register Client
	std::shared_ptr<NetworkClient> client = nullptr;

	if (mClientPool.empty())
	{
		client = std::make_shared<NetworkClient>();
	}
	else
	{
		if (false == mClientPool.try_pop(client))
		{
			client = std::make_shared<NetworkClient>();
		}

		auto curTimeSec = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count();

		if (REUSE_SESSION_TIME < curTimeSec - client->GetLastCloseTime())
		{
			mClientPool.push(client);
			client = std::make_shared<NetworkClient>();
		}
	}

	client->Reset();
	client->Init();
	client->SetSocket(clientSocket);

	if (false == AddClient(std::move(client)))
	{
		printf_s("_HandleAccept ERROR: AddClient\n");
		return;
	}

	// Listen for Next Connection
	if (false == listenClient.PostAccept())
	{
		printf_s("_HandleAccept ERROR: PostAccept\n");
		return;
	}
}

void NetworkManager::_HandleReceive(NetworkClient& client, NetworkContext& context, int transferred)
{
	if (transferred <= 0)
	{
		printf_s("_HandleReceive Error: %d\n", WSAGetLastError());
		return;
	}

	if (false == context.Write(transferred))
	{
		printf_s("_HandleReceive Error: Failed to Write\n");
		return;
	}

	// Packet Deserialization
	auto packet = client.GetPacket(mPacketPool);

	if (nullptr == packet)
	{
		printf_s("_HandleReceive Error: Failed to GetPacket\n");
		return;
	}

	packet->SessionID = client.GetSessionID();

	if (mServiceList.empty())
	{
		printf_s("_HandleReceive Error: No Service\n");
		return;
	}


	// FIXME: For now, we're using the first service
	auto& dispatcher = mServiceList.begin()->second;

	if (nullptr == dispatcher)
	{
		printf_s("_HandleReceive Error: No Dispatcher\n");
		return;
	}

	dispatcher->PushPacket(std::move(packet));

	context.Read(transferred);

	// Bind for next receive
	client.Receive();
}

void NetworkManager::_HandleSend(NetworkClient& client, NetworkContext& context, int transferred)
{
	if (transferred <= 0)
	{
		printf_s("_HandleSend Error: %d\n", WSAGetLastError());
		return;
	}

	if (false == context.Read(transferred))
	{
		printf_s("_HandleSend Error: Failed to Read\n");
		return;
	}

	if (context.GetDataSize() > 0)
	{
		client.Send();
	}
	else
	{
		printf_s("Send Complete Data: %d\n", context.GetDataSize());
		client.mSending.store(false);
	}
}

bool NetworkManager::AddClient(std::shared_ptr<NetworkClient> client)
{
	if (nullptr == client)
	{
		return false;
	}

	HANDLE hIOCP = CreateIoCompletionPort((HANDLE)client->GetSocket(), mIOCPHandle, (ULONG_PTR)client.get(), 0);

	if (hIOCP == INVALID_HANDLE_VALUE)
	{
		printf_s("AddClient Error: CreateIoCompletionPort, %d\n", GetLastError());
		return false;
	}

	if (false == client->Receive())
	{
		printf_s("AddClient Error: Receive\n");
		return false;
	}

	// Reused from the pool
	if (client->GetSessionID() > 0 &&
		client->GetSessionID() < mClientList.size())
	{
		if (nullptr == mClientList.at(client->GetSessionID()))
		{
			mClientList[client->GetSessionID()] = std::move(client);
		}

		return true;
	}

	// For real world scenario, should use a session manager to assign session id
	auto sessionID = static_cast<uint32_t>(mClientList.size());
	client->SetSessionID(sessionID);

	mClientList.push_back(std::move(client));

	printf_s("Client Connected SessionID: %d\n", sessionID);

	return true;
}

void NetworkManager::RemoveClient(NetworkClient& client)
{
	auto sessionID = client.GetSessionID();

	if (sessionID < 0 || sessionID >= mClientList.size())
	{
		return;
	}

	auto& ptrClient = mClientList.at(sessionID);

	if (nullptr == ptrClient)
	{
		return;
	}

	ptrClient->Close();
	printf_s("Client Disconnected SessionID: %d\n", sessionID);

	// concurrent vector does not support erase so we're reusing this
	ptrClient->Reset();
	mClientPool.push(ptrClient);
}
