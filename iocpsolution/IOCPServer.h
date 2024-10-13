#pragma once
#pragma comment(lib, "ws2_32.lib")

#include "Define.h"
#include "NetworkClient.h"
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

	bool InitSocket()
	{
		WSADATA wsaData;

		int nRet = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (0 != nRet)
		{
			printf("Error on WSAStartup(): %d\n", WSAGetLastError());
			return false;
		}

		mListenSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, NULL, WSA_FLAG_OVERLAPPED);

		if (INVALID_SOCKET == mListenSocket)
		{
			printf("[에러] socket() 함수 실패: %d \n", WSAGetLastError());
			return false;
		}

		printf("소켓 초기화 성공 \n");
		return true;
	}

	bool BindAndListen(int nBindPort)
	{
		SOCKADDR_IN stServerAddr;
		stServerAddr.sin_family = AF_INET;
		stServerAddr.sin_port = htons(nBindPort);
		stServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);

		int nRet = bind(mListenSocket, (SOCKADDR*)&stServerAddr, sizeof(SOCKADDR_IN));
		if (0 != nRet)
		{
			printf("[에러] bind()함수 실패 : %d\n", WSAGetLastError());
			return false;
		}

		nRet = listen(mListenSocket, 5);
		if (0 != nRet)
		{
			printf("[에러] listen()함수 실패 : %d\n", WSAGetLastError());
			return false;
		}

		printf("서버 등록 성공 \n");
		return true;
	}

	bool StartServer(const UINT32 maxClientCount)
	{
		CreateClient(maxClientCount);

		mIOCPHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, MAX_WORKER_THREAD);

		if (NULL == mIOCPHandle)
		{
			printf("[에러] CreateIoCompletionPort() 함수 실패: %d\n", GetLastError());
			return false;
		}

		bool bRet = CreateWorkerThread();
		if (false == bRet) {
			return false;
		}

		bRet = CreateAcceptorThread();
		if (false == bRet)
		{
			return false;
		}

		bRet = CreateSenderThread();
		if (false == bRet)
		{
			return false;
		}

		printf("서버 시작 \n");
		return true;
	}

	void DestroyThread()
	{
		mIsSenderRun = false;
		if (mSenderThread.joinable())
		{
			mSenderThread.join();
		}

		mIsWorkerRun = false;
		CloseHandle(mIOCPHandle);

		for (auto& th : mIOWorkerThreads)
		{
			if (th.joinable())
			{
				th.join();
			}
		}

		mIsAcceptorRun = false;
		closesocket(mListenSocket);

		if (mAcceptorThread.joinable())
		{
			mAcceptorThread.join();
		}
	}

	bool SendMsg(const UINT32 sessionIndex_, const UINT32 dataSize_, char* pData)
	{
		auto pClient = GetClientInfo(sessionIndex_);
		if (nullptr != pClient)
		{
			pClient->SendMsg(dataSize_, pData);
		}

		return false;
	}

	// 네트워크 이벤트를 처리할 함수들
	virtual void OnConnect(const UINT32 clientIndex_) {}
	virtual void OnClose(const UINT32 clientIndex_) {}
	virtual void OnReceive(const UINT32 clientIndex_, const UINT32 size_, char* pData_) {}

private:
	void CreateClient(const UINT32 maxClientCount)
	{
		for (UINT32 i = 0; i < maxClientCount; ++i)
		{
			auto client = new NetworkClient();
			client->Init(i);

			mClientInfos.push_back(client);
		}
	}

	bool CreateWorkerThread()
	{
		unsigned int uiThreadId = 0;
		for (int i = 0; i < MAX_WORKER_THREAD; i++)
		{
			mIOWorkerThreads.emplace_back([this]() { WorkerThread(); });
		}

		printf("WorkerThread 시작...\n");
		return true;
	}

	bool CreateAcceptorThread()
	{
		mAcceptorThread = std::thread([this]() { AcceptorThread(); });
		printf("AcceptThread 시작 \n");
		return true;
	}

	bool CreateSenderThread()
	{
		mIsSenderRun = true;
		mSenderThread = std::thread([this]() { SendThread(); });
		printf("SendThread 시작 \n");
		return true;
	}


	NetworkClient* GetEmptyClientInfo()
	{
		for (auto& client : mClientInfos)
		{
			if (false == client->IsConnected())
			{
				return client;
			}
		}

		return nullptr;
	}

	NetworkClient* GetClientInfo(const UINT32 index)
	{
		return mClientInfos[index];
	}

	void WorkerThread()
	{
		NetworkClient* pClientInfo = nullptr;
		BOOL bSuccess = TRUE;
		DWORD dwIoSize = 0;
		LPOVERLAPPED lpOverlapped = NULL;

		while (mIsWorkerRun)
		{
			bSuccess = GetQueuedCompletionStatus(mIOCPHandle,
				&dwIoSize,						// 실제 전송된 바이트
				(PULONG_PTR)&pClientInfo,		// CompletionKey
				&lpOverlapped,					// Overlapped 객체
				INFINITE);						// 대기할 시간

			if (TRUE == bSuccess && 0 == dwIoSize && NULL == lpOverlapped)
			{
				mIsWorkerRun = false;
				continue;
			}

			if (NULL == lpOverlapped)
			{
				continue;
			}

			if (FALSE == bSuccess || (0 == dwIoSize && TRUE == bSuccess))
			{
				CloseSocket(pClientInfo);
				continue;
			}

			auto pOverlappedEx = (stOverlappedEx*)lpOverlapped;

			if (IOOperation::RECV == pOverlappedEx->m_eOperation)
			{
				OnReceive(pClientInfo->GetIndex(), dwIoSize, pClientInfo->RecvBuffer());
				pClientInfo->BindRecv();
			}
			else if (IOOperation::SEND == pOverlappedEx->m_eOperation)
			{
				pClientInfo->SendCompleted(dwIoSize);
			}
			else
			{
				printf("Client Index(%d)에서 예외 상황\n", pClientInfo->GetIndex());
			}
		}
	}

	void AcceptorThread()
	{
		SOCKADDR_IN stClientAddr;
		int nAddrLen = sizeof(SOCKADDR_IN);

		while (mIsAcceptorRun)
		{
			NetworkClient* pClientInfo = GetEmptyClientInfo();
			if (NULL == pClientInfo)
			{
				printf("[에러] Client Full \n");
				return;
			}

			auto newSocket = accept(mListenSocket, (SOCKADDR*)&stClientAddr, &nAddrLen);
			if (INVALID_SOCKET == newSocket)
			{
				printf("[에러] INVALID_SOCKET \n");
				continue;
			}

			if (false == pClientInfo->OnConnect(mIOCPHandle, newSocket))
			{
				printf("[에러]  pClientInfo->OnConnect \n");
				pClientInfo->Close(true);
			}

			OnConnect(pClientInfo->GetIndex());
			++mClientCount;
		}
	}

	void SendThread()
	{
		while (mIsSenderRun)
		{
			for (auto& client : mClientInfos)
			{
				if (client->IsConnected())
				{
					client->SendIO();
				}
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}


	//소켓의 연결을 종료 시킨다.
	void CloseSocket(NetworkClient* pClientInfo, bool bIsForce = false)
	{
		auto clientIndex = pClientInfo->GetIndex();

		pClientInfo->Close(bIsForce);

		OnClose(clientIndex);
	}


	std::vector<NetworkClient*> mClientInfos;
	SOCKET mListenSocket = INVALID_SOCKET;
	int mClientCount = 0;

	std::vector<std::thread> mIOWorkerThreads;
	std::thread mAcceptorThread;
	std::thread mSenderThread;

	HANDLE mIOCPHandle = INVALID_HANDLE_VALUE;

	bool mIsWorkerRun = true;
	bool mIsAcceptorRun = true;
	bool mIsSenderRun = true;
};