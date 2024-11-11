#include "IOCPHandler.h"
#include "NetworkContext.h"
#include "NetworkClient.h"
#include "SimpleCore.h"

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
