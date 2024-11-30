#include "pch.h"
#include "NetworkManager.h"
#include "NetworkClient.h"
#include "NetworkContext.h"
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
			if (nullptr != client)
			{
				printf_s("Client Disconnected: %d\n", client->GetSessionID());
				RemoveClient(*client);
			}

			printf_s("WorkerThread Fail: %d\n", WSAGetLastError());
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
	std::unique_ptr<NetworkPacket> packet = client.GetPacket();

	if (nullptr == packet)
	{
		printf_s("_HandleReceive Error: Failed to GetPacket\n");
		return;
	}

	packet->Header.SessionID = client.GetSessionID();
	mDispatcher->PushPacket(std::move(packet));

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
		if (false == client.Send(context))
		{
			printf_s("_HandleSend Error: Failed to Send\n");
			return;
		}
	}
	else
	{
		client.SendComplete();
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
	auto sessionID = static_cast<uint32_t>(mClientList.size()) + 1;
	client->SetSessionID(sessionID);

	mDispatcher->AddSession(client);
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
