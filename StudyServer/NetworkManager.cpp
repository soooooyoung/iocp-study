#include "NetworkManager.h"
#include "NetworkClient.h"
#include "NetworkDispatcher.h"
#include "ListenClient.h"

NetworkManager::NetworkManager() : mIsRunning(false), mIOCPHandle(INVALID_HANDLE_VALUE)
{
	mDispatcher = new NetworkDispatcher();
}

NetworkManager::~NetworkManager()
{
	WSACleanup();
}

bool NetworkManager::Initialize()
{
	if (false == mDispatcher->Initialize())
	{
		return false;
	}

	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		return false;
	}

	SYSTEM_INFO systemInfo;
	GetSystemInfo(&systemInfo);


	int ioThreadCount = systemInfo.dwNumberOfProcessors / 2;

	// FIXME: configure in server settings
	int packetThreadCount = 1;

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

	for (int i = 0; i < packetThreadCount; ++i)
	{
		mPacketPool.emplace_back([this]() { SendThread(); });
	}

	for (int i = 0; i < MAX_LISTEN_COUNT; ++i)
	{
		// FIXME: hardcoded port
		if (false == AddListener(i, 9000 + i))
		{
			return false;
		}
	}

	printf("Server Bind and Listen Success\n");
	return true;
}

bool NetworkManager::AddListener(int index, int port)
{
	std::shared_ptr<ListenClient> listenClient = std::make_shared<ListenClient>();

	// Initialize Listen Socket
	if (false == listenClient->Init())
	{
		return false;
	}

	if (false == listenClient->Listen(port))
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
			RemoveClient(client);
			printf_s("RemoveClient: %d\n", WSAGetLastError());
			continue;
		}

		printf_s("ContextType: %d, IoSize: %d\n", context->mContextType, dwIoSize);

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

void NetworkManager::SendThread()
{
	while (mIsRunning)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1));

		if (mSendQueue.empty())
		{
			continue;
		}

		std::shared_ptr<NetworkContext> context = nullptr;

		if (false == mSendQueue.try_pop(context))
		{
			continue;
		}

		if (context->mSessionID < 0 || context->mSessionID >= mClientList.size())
		{
			continue;
		}

		// concurrent safe as long as index does not exceed the size
		auto client = mClientList.at(context->mSessionID);

		if (nullptr == client)
		{
			continue;
		}

		if (false == client->Send(*context))
		{
			printf_s("SendThread: Send Error\n");
		}

		// 여기서 초기화
		context.reset();
	}
}

void NetworkManager::_HandleAccept(ListenClient& listenClient, NetworkContext& context)
{
	SOCKET clientSocket = context.mSocket;

	if (INVALID_SOCKET == clientSocket)
	{
		printf_s("Accept Completion Error on ListenClient: %d\n", WSAGetLastError());
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
	}

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
		printf_s("Recv Error: %d\n", WSAGetLastError());
		return;
	}

	if (false == context.Write(transferred))
	{
		printf_s("Context Read Error: Read\n");
		return;
	}

	// Push to Dispatcher for Deserialization Queue
	mDispatcher->EnqueueClientPacket(client.GetContext());

	client.Receive();
}

void NetworkManager::_HandleSend(NetworkClient& client, NetworkContext& context, int transferred)
{
	if (transferred <= 0)
	{
		printf_s("_HandleSend: Send Error: %d\n", WSAGetLastError());
		return;
	}

	context.Read(transferred);
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
		printf_s("CreateIoCompletionPort Error : %d\n", GetLastError());
		return false;
	}

	if (false == client->Receive())
	{
		printf_s("Client Receive() Error\n");
		return false;
	}


	// Reused from the pool
	if (client->GetSessionID() > 0 &&
		client->GetSessionID() < mClientList.size())
	{
		if (nullptr != mClientList.at(client->GetSessionID()))
		{
			return true;
		}

		return false;
	}

	// For real world scenario, should use a session manager to assign session id
	client->SetSessionID(static_cast<std::int32_t>(mClientList.size()));
	mClientList.push_back(std::move(client));

	printf_s("Client Connected\n");

	return true;
}

void NetworkManager::RemoveClient(NetworkClient* client)
{
	if (nullptr == client)
	{
		return;
	}

	auto sessionID = client->GetSessionID();

	if (sessionID < 0 || sessionID >= mClientList.size())
	{
		return;
	}

	auto ptrClient = mClientList.at(sessionID);

	if (nullptr == ptrClient)
	{
		return;
	}

	ptrClient->Close();
	printf_s("Client Disconnected Session: %d\n", sessionID);

	// concurrent vector does not support erase so we're reusing this
	mClientPool.push(ptrClient);

	//ptrClient.reset();
	//mClientList[sessionID] = nullptr;
}

bool NetworkManager::PushSend(int sessionID, void* data, int transferred)
{
	std::shared_ptr<NetworkContext> context = std::make_shared<NetworkContext>();
	if (false == context->Write(data, transferred))
	{
		return false;
	}

	context->mContextType = ContextType::SEND;
	context->mSessionID = sessionID;
	mSendQueue.push(std::move(context));

	return true;
}
