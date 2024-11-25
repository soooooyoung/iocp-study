#include "NetworkManager.h"
#include "NetworkClient.h"
#include "ListenClient.h"

NetworkManager::NetworkManager() : mIsRunning(false), mIOCPHandle(INVALID_HANDLE_VALUE)
{
}

NetworkManager::~NetworkManager()
{
	WSACleanup();
}

bool NetworkManager::Initialize()
{
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		return false;
	}

	SYSTEM_INFO systemInfo;
	GetSystemInfo(&systemInfo);

	int maxThreadCount = systemInfo.dwNumberOfProcessors * 2;

	mIOCPHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, maxThreadCount);

	if (NULL == mIOCPHandle)
	{
		printf_s("CreateIoCompletionPort() Error: %d\n", GetLastError());
		return false;
	}

	mIsRunning = true;

	for (int i = 0; i < maxThreadCount; ++i)
	{
		mThreadPool.emplace_back([this]() { WorkerThread(); });
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
	// Listeners don't need to use pooling
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

	for (auto& thread : mThreadPool)
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
			continue;
		}

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
			_HandleRecv(*client, *context, dwIoSize);
		}
		break;
		case ContextType::SEND:
		{
			printf_s("SEND\n");
		}
		break;
		default:
		{

			printf("Unknown ContextType\n");
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
		printf_s("Accept Completion Error on ListenClient: %d\n", WSAGetLastError());
		return;
	}

	//auto listenClient = static_cast<ListenClient*>(host);

	//if (nullptr == listenClient)
	//{
	//	printf_s("_HandleAccept ERROR: Invalid ListenClient\n");
	//	return;
	//}

	// Associate Client Socket to Listener 
	if (false == listenClient.Accept(clientSocket))
	{
		printf_s("_HandleAccept ERROR: Accept\n");
		return;
	}

	// Register Client
	auto client = std::make_shared<NetworkClient>();

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

void NetworkManager::_HandleRecv(NetworkClient& client, NetworkContext& context, int transferred)
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

	printf_s("Recv Data: %s\n", context.GetReadBuffer());

	context.ResetBuffer();
	client.Receive();
}

void NetworkManager::_HandleSend(NetworkClient& client, NetworkContext& context, int transferred)
{
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

	// For real world scenario, should use a session manager to assign session id
	client->SetSessionID(static_cast<std::int32_t>(mClientList.size()));
	mClientList.push_back(std::move(client));

	printf_s("Client Connected");

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

	ptrClient.reset();
	mClientList[sessionID] = nullptr;
}
