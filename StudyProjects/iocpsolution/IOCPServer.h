#pragma once
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "mswsock.lib")

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

	bool Init(const UINT32 maxIOWorkerThreadCount_)
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

		MaxIOWorkerThreadCount = maxIOWorkerThreadCount_;

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

		// 접속 대기큐 5개
		nRet = listen(mListenSocket, 5);
		if (0 != nRet)
		{
			printf("[에러] listen()함수 실패 : %d\n", WSAGetLastError());
			return false;
		}

		mIOCPHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, MaxIOWorkerThreadCount);
		if (NULL == mIOCPHandle)
		{
			printf("[에러] CreateIoCompletionPort() 함수 실패: %d\n", GetLastError());
			return false;
		}

		auto hIOCP = CreateIoCompletionPort((HANDLE)mListenSocket, mIOCPHandle, (UINT32)0, 0);
		if (hIOCP == INVALID_HANDLE_VALUE)
		{
			printf("[에러] listen socket IOCP bind 실패: %d\n", WSAGetLastError());
			return false;
		}

		printf("서버 등록 성공 \n");
		return true;
	}

	bool StartServer(const UINT32 maxClientCount)
	{
		CreateClient(maxClientCount);

		bool bRet = CreateWorkerThread();
		if (false == bRet) {
			return false;
		}

		bRet = CreateAcceptorThread();
		if (false == bRet)
		{
			return false;
		}

		printf("서버 시작 \n");
		return true;
	}

	void DestroyThread()
	{
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
			client->Init(i, mIOCPHandle);

			mClientInfos.push_back(client);
		}
	}

	bool CreateWorkerThread()
	{
		unsigned int uiThreadId = 0;
		for (int i = 0; i < MaxIOWorkerThreadCount; i++)
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


			auto pOverlappedEx = (stOverlappedEx*)lpOverlapped;

			if (FALSE == bSuccess || (0 == dwIoSize && IOOperation::ACCEPT != pOverlappedEx->m_eOperation))
			{
				CloseSocket(pClientInfo);
				continue;
			}

			switch (pOverlappedEx->m_eOperation)
			{
			case IOOperation::ACCEPT:
			{
				pClientInfo = GetClientInfo(pOverlappedEx->mSessionIndex);
				if (nullptr == pClientInfo)
				{
					printf("Client Index(%d) == nullptr\n", pOverlappedEx->mSessionIndex);
					break;
				}

				if (pClientInfo->AcceptCompletion())
				{
					++mClientCount;
					OnConnect(pClientInfo->GetIndex());
				}
				else
				{
					CloseSocket(pClientInfo, true);
				}
			}
			case IOOperation::RECV:
			{
				OnReceive(pClientInfo->GetIndex(), dwIoSize, pClientInfo->RecvBuffer());
				pClientInfo->BindRecv();
			}
			break;

			case IOOperation::SEND:
			{
				pClientInfo->SendCompleted(dwIoSize);
			}
			break;

			default:
				printf("Client Index(%d)에서 예외 상황\n", pClientInfo->GetIndex());
				break;
			}
		}
	}

	void AcceptorThread()
	{
		while (mIsAcceptorRun)
		{
			auto curTimeSec = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();

			for (auto client : mClientInfos)
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
				if (diff < REUSE_SESSION_WAIT_TIME_SEC)
				{
					continue;
				}

				client->PostAccept(mListenSocket, curTimeSec);
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(32));
		}
	}

	//소켓의 연결을 종료 시킨다.
	void CloseSocket(NetworkClient* pClientInfo, bool bIsForce = false)
	{
		auto clientIndex = pClientInfo->GetIndex();

		pClientInfo->Close(bIsForce);

		OnClose(clientIndex);
	}

	UINT32 MaxIOWorkerThreadCount = 0;

	std::vector<NetworkClient*> mClientInfos;
	SOCKET mListenSocket = INVALID_SOCKET;
	int mClientCount = 0;

	std::vector<std::thread> mIOWorkerThreads;
	std::thread mAcceptorThread;

	HANDLE mIOCPHandle = INVALID_HANDLE_VALUE;

	bool mIsWorkerRun = true;
	bool mIsAcceptorRun = true;
};