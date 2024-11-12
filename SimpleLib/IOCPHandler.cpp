#include "IOCPHandler.h"
#include "NetworkContext.h"
#include "NetworkClient.h"
#include "SimpleCore.h"
#include "ListenClient.h"
#include "NetworkManager.h"

bool IOCPHandler::Init()
{
	// Initialize Winsock
	WSADATA wsaData;

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		printf("WSAStartup Error : %d\n", WSAGetLastError());
		return false;
	}

	// Create IOCP Handle
	mIOCPHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

	if (mIOCPHandle == NULL)
	{
		printf("CreateIoCompletionPort Error : %d\n", GetLastError());
		return false;
	}

	mIsRunning = true;

	SYSTEM_INFO systemInfo;
	GetSystemInfo(&systemInfo);

	// Create Worker Threads
	for (DWORD i = 0; i < systemInfo.dwNumberOfProcessors; ++i)
	{
		mIOThreadList.emplace_back([this]() { WorkerThread(); });
	}
}

bool IOCPHandler::Register(std::shared_ptr<NetworkClient> client)
{
	HANDLE hIOCP = CreateIoCompletionPort((HANDLE)client->GetSocket(), mIOCPHandle, (ULONG_PTR)client.get(), 0);

	if (hIOCP == INVALID_HANDLE_VALUE)
	{
		printf("CreateIoCompletionPort Error : %d\n", GetLastError());
		return false;
	}

	return true;
}

void IOCPHandler::WorkerThread()
{
	BOOL bSuccess = TRUE;
	DWORD dwIoSize = 0;
	LPOVERLAPPED lpOverlapped = NULL;

	while (mIsRunning)
	{
		NetworkClient* pClient = nullptr;
	
		bSuccess = GetQueuedCompletionStatus(mIOCPHandle, &dwIoSize, (PULONG_PTR)&pClient, &lpOverlapped, INFINITE);

		if (TRUE == bSuccess && 0 == dwIoSize && NULL == lpOverlapped)
		{
			mIsRunning = false;
			continue;
		}

		if (nullptr == lpOverlapped || 
			nullptr == pClient)
		{
			continue;
		}

		auto context = (NetworkContext*)lpOverlapped;

		if (FALSE == bSuccess || 
			(0 == dwIoSize && ContextType::ACCEPT != context->mContextType))
		{
			//CloseSocket(pClient);
			continue;
		}

		switch (context->mContextType)
		{
		case ContextType::ACCEPT:
		{

		}
		break;
		case ContextType::RECV:
		{

		}
		break;
		case ContextType::SEND:
		{

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

void IOCPHandler::_HandleAccept(NetworkClient* host, NetworkContext& context)
{
	if (INVALID_SOCKET == context.mSocket)
	{
		printf("_HandleAccept ERROR: Invalid Socket\n");
		return;
	}

	// Get New Client
	auto client = StaticPool<NetworkClient>::GetInstance().Pop();
	client->SetSocket(context.mSocket);

	// Register Client
	if (false == NetworkManager::GetInstance().AddClient(client))
	{
		printf("_HandleAccept ERROR: RegisterClient\n");
		return;
	}

	// Post Accept Again
	ListenClient* listenClient = static_cast<ListenClient*>(host);

	if (nullptr == listenClient)
	{
		printf("_HandleAccept ERROR: Invalid ListenClient\n");
		return;
	}

	listenClient->PostAccept();
	return;
}

void IOCPHandler::_HandleReceive(UINT32 sessionID, NetworkContext& context)
{
}

void IOCPHandler::_HandleSend(UINT32 sessionID)
{
}
