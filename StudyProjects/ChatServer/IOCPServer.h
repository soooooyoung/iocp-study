#pragma once
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "mswsock.lib")

#include "NetworkClient.h"
#include "Define.h"
#include <thread>
#include <vector>

class IOCPServer
{
public:
	IOCPServer(void) {}

	virtual ~IOCPServer(void)
	{
		WSACleanup();
	}

	bool Init(const UINT32 maxIOThreadCount)
	{
		WSADATA wsaData;

		int nRet = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (0 != nRet)
		{
			printf("WSAStartup() Error: %d\n", WSAGetLastError());
			return false;
		}

		// 연결지향형 TCP, Overlapped IO 소켓 생성
		mListenSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);

		if (INVALID_SOCKET == mListenSocket)
		{
			printf("WSASocket() Error: %d\n", WSAGetLastError());
			return false;
		}

		MaxIOThreadCount = maxIOThreadCount;

		printf("Server Init\n");
		return true;
	}

	bool BindAndListen(int port)
	{
		SOCKADDR_IN serverAddr;
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_port = htons(port);
		serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

		int nRet = bind(mListenSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr));

		if (0 != nRet)
		{
			printf("bind() Error: %d\n", WSAGetLastError());
			return false;
		}

		nRet = listen(mListenSocket, 5);
		if (0 != nRet)
		{
			printf("listen() Error: %d\n", WSAGetLastError());
			return false;
		}

		mIOCPHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, MaxIOThreadCount);

		if (NULL == mIOCPHandle)
		{
			printf("CreateIoCompletionPort() Error: %d\n", GetLastError());
			return false;
		}

		auto hIOCP = CreateIoCompletionPort((HANDLE)mListenSocket, mIOCPHandle, 0, 0);
		if (nullptr == hIOCP)
		{
			printf("CreateIoCompletionPort() Error: %d\n", GetLastError());
			return false;
		}

		printf("Server Bind and Listen Success\n");
		return true;
	}

	bool StartServer(const UINT32 maxClientCount)
	{
		CreateClient(maxClientCount);

		bool bRet = CreateWorkerThread();
		if (false == bRet)
		{
			return false;
		}

		bRet = CreateAcceptorThread();
		if (false == bRet)
		{
			return false;
		}

		printf("Server Start\n");
		return true;
	}

	void DestroyThread()
	{
		mIsWorkerRunning = false;

		for (auto& thread : mIOThreadList)
		{
			if (thread.joinable())
			{
				thread.join();
			}
		}

		mIsAcceptorRunning = false;
		closesocket(mListenSocket);

		if (mAcceptorThread.joinable())
		{
			mAcceptorThread.join();
		}
	}

	bool SendMsg(const UINT32 clientIndex, const UINT32 size, char* pData)
	{
		auto pClient = GetClient(clientIndex);

		if (nullptr == pClient)
		{
			return false;
		}

		return pClient->SendMsg(size, pData);
	}

	virtual void OnConnect(const UINT32 clientIndex) {}
	virtual void OnClose(const UINT32 clientIndex) {}
	virtual void OnReceive(const UINT32 clientIndex, UINT32 size, char* pData) {}
private:

	void WorkerThread()
	{
		NetworkClient* pClient = nullptr;
		BOOL bSuccess = TRUE;
		DWORD dwIoSize = 0;
		LPOVERLAPPED lpOverlapped = NULL;

		while (mIsWorkerRunning)
		{
			bSuccess = GetQueuedCompletionStatus(mIOCPHandle, &dwIoSize, (PULONG_PTR)&pClient, &lpOverlapped, INFINITE);

			if (TRUE == bSuccess && 0 == dwIoSize && NULL == lpOverlapped)
			{
				mIsWorkerRunning = false;
				continue;
			}

			if (NULL == lpOverlapped)
			{
				continue;
			}

			auto pOverlappedEx = (stOverlappedEx*)lpOverlapped;

			// 클라이언트 접속 끊김
			if (FALSE == bSuccess || (0 == dwIoSize && IOOperation::ACCEPT != pOverlappedEx->m_eOperation))
			{
				CloseSocket(pClient);
				continue;
			}

			switch (pOverlappedEx->m_eOperation)
			{
				case IOOperation::ACCEPT:
				{
					pClient = GetClient(pOverlappedEx->SessionIndex);

					if (nullptr == pClient)
					{
						continue;
					}

					if (pClient->AcceptCompletion() == false)
					{
						CloseSocket(pClient);
						continue;
					}

					// 클라이언트 접속
					mClientCount++;
					OnConnect(pClient->GetIndex());
				}
				break;
				case IOOperation::RECV:
				{
					if (nullptr == pClient)
					{
						continue;
					}

					OnReceive(pClient->GetIndex(), dwIoSize, pClient->RecvBuffer());
					pClient->BindRecv();
				}
				break;
				case IOOperation::SEND:
				{
					if (nullptr == pClient)
					{
						continue;
					}

					pClient->SendCompleted(dwIoSize);
				}
				break;
				default:
				{
					printf("Unknown Operation. ClientIndex : %d\n", pClient->GetIndex());
				}
				break;
			}
		}
	}

	void AcceptorThread()
	{
		while (mIsAcceptorRunning)
		{
			auto curTimeSec = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count();

			for (auto& client : mClientList)
			{
				if (client->IsConnected())
				{
					continue;
				}

				if ((UINT64)curTimeSec < client->GetLatestClosedTimeSec())
				{
					continue;
				}

				auto diff = curTimeSec - client->GetLatestClosedTimeSec();
				if (diff <= REUSE_SESSION_WAIT_TIMESEC)
				{
					continue;
				}

				client->PostAccept(mListenSocket, curTimeSec);

				std::this_thread::sleep_for(std::chrono::milliseconds(32));			
			}
		}
	}

	bool CreateWorkerThread()
	{
		for (UINT32 i = 0; i < MaxIOThreadCount; ++i)
		{
			mIOThreadList.emplace_back([this]() { WorkerThread();  });	
		}

		printf("Worker Thread Created\n");
		return true;
	}

	bool CreateAcceptorThread()
	{
		mAcceptorThread = std::thread([this]() { AcceptorThread(); });

		printf("Acceptor Thread Created\n");
		return true;
	}

	void CreateClient(const UINT32 maxClientCount)
	{
		for (UINT32 i = 0; i < maxClientCount; ++i)
		{
			auto pClient = new NetworkClient;
			pClient->Init(i, mIOCPHandle);
			mClientList.push_back(pClient);
		}
	}

	NetworkClient* GetClient(const UINT32 clientIndex)
	{
		if (clientIndex >= mClientList.size())
		{
			return nullptr;
		}

		return mClientList[clientIndex];
	}

	NetworkClient* GetEmptyClient()
	{
		for (auto& client : mClientList)
		{
			if (client->IsConnected() == false)
			{
				return client;
			}
		}

		return nullptr;
	}

	void CloseSocket(NetworkClient* pClient, bool bIsForce = false)
	{
		if (false == pClient->IsConnected())
		{
			return;
		}

		auto clientIndex = pClient->GetIndex();
		pClient->Close(bIsForce);
	}

private:
	bool mIsWorkerRunning = true;
	bool mIsAcceptorRunning = true;
	
	int mClientCount = 0;
	UINT32 MaxIOThreadCount = 0;

	std::vector<NetworkClient*> mClientList;
	std::vector<std::thread> mIOThreadList;
	std::thread mAcceptorThread;

	HANDLE mIOCPHandle = INVALID_HANDLE_VALUE;
	SOCKET mListenSocket = INVALID_SOCKET;
};