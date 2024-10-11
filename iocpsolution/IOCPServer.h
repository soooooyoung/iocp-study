#pragma once
#pragma comment(lib, "ws2_32.lib")

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

	// 네트워크 이벤트를 처리할 함수들
	virtual void OnConnect(const UINT32 clientIndex_) {}
	virtual void OnClose(const UINT32 clientIndex_) {}
	virtual void OnReceive(const UINT32 clientIndex_, const UINT32 size_, char* pData_) {}

private:
	void CreateClient(const UINT32 maxClientCount)
	{
		for (UINT32 i = 0; i < maxClientCount; ++i)
		{
			mClientInfos.emplace_back();
			mClientInfos[i].mIndex = i;
		}
	}

	bool CreateWorkerThread()
	{
		unsigned int uiThreadId = 0;
		for (int i = 0; i < MAX_WORKER_THREAD; i++)
		{
			mIOWorkerThreads.emplace_back([this]() {WorkerThread(); });
		}

		printf("WorkerThread 시작...\n");
		return true;
	}

	bool CreateAcceptorThread()
	{
		mAcceptorThread = std::thread([this]() {AcceptorThread(); });
		printf("AcceptThread 시작 \n");
		return true;
	}

	void CloseSocket(stClientInfo* pClientInfo, bool bIsForce = false)
	{
		auto clientIndex = pClientInfo->mIndex;

		struct linger stLinger = { 0,0 }; // SO_DONTLINGER로 설정

		if (true == bIsForce)
		{
			stLinger.l_onoff = 1;
		}

		// socketClose 소켓의 데이터 송수신을 모두 중단 시킨다.
		shutdown(pClientInfo->m_socketClient, SD_BOTH);

		setsockopt(pClientInfo->m_socketClient, SOL_SOCKET, SO_LINGER, (char*)&stLinger, sizeof(stLinger));

		closesocket(pClientInfo->m_socketClient);

		OnClose(clientIndex);
	}

	stClientInfo* GetEmptyClientInfo()
	{
		for (auto& client : mClientInfos)
		{
			if (INVALID_SOCKET == client.m_socketClient)
			{
				return &client;
			}
		}
	}

	bool BindIOCompletionPort(stClientInfo* pClientInfo)
	{
		auto hIOCP = CreateIoCompletionPort((HANDLE)pClientInfo->m_socketClient
			, mIOCPHandle
			, (ULONG_PTR)(pClientInfo), 0);

		if (NULL == hIOCP || mIOCPHandle != hIOCP)
		{
			printf("CreateIoCompletionPort()함수 실패: %d\n", GetLastError());
			return false;
		}

		return true;
	}

	bool BindRecv(stClientInfo* pClientInfo)
	{
		DWORD dwFlag = 0;
		DWORD dwRecvNumBytes = 0;

		// Overlapped input operation 위해 정보 세팅
		pClientInfo->m_stRecvOverlappedEx.m_wsaBuf.len = MAX_SOCKBUF;
		pClientInfo->m_stRecvOverlappedEx.m_wsaBuf.buf = pClientInfo->mRecvBuf;
		pClientInfo->m_stRecvOverlappedEx.m_eOperation = IOOperation::RECV;

		int nRet = WSARecv(pClientInfo->m_socketClient,
			&(pClientInfo->m_stRecvOverlappedEx.m_wsaBuf),
			1,
			&dwRecvNumBytes,
			&dwFlag,
			(LPWSAOVERLAPPED) & (pClientInfo->m_stRecvOverlappedEx),
			NULL);

		// if socket error, disconnect client
		if (nRet == SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING))
		{
			printf("WSARecv() 함수 실패: %d \n", WSAGetLastError());
			return false;
		}
		return true;
	}


	// WSASend Overlapped I/O 작업 시킨다
	bool SendMsg(stClientInfo* pClientInfo, char* pMsg, int nLen)
	{
		DWORD dwRecvNumBytes = 0;

		CopyMemory(pClientInfo->mSendBuf, pMsg, nLen);
		pClientInfo->mSendBuf[nLen] = '\0';

		pClientInfo->m_stSendOverlappedEx.m_wsaBuf.len = nLen;
		pClientInfo->m_stSendOverlappedEx.m_wsaBuf.buf = pClientInfo->mSendBuf;
		pClientInfo->m_stSendOverlappedEx.m_eOperation = IOOperation::SEND;

		int nRet = WSASend(pClientInfo->m_socketClient,
			&(pClientInfo->m_stSendOverlappedEx.m_wsaBuf),
			1,
			&dwRecvNumBytes,
			0,
			(LPWSAOVERLAPPED) & (pClientInfo->m_stRecvOverlappedEx),
			NULL);

		if (nRet == SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING))
		{
			printf("[에러] WSASend() 함수 실패  : %d\n", WSAGetLastError());
			return false;
		}

		return true;
	}


	void WorkerThread()
	{
		stClientInfo* pClientInfo = nullptr;
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
				OnReceive(pClientInfo->mIndex, dwIoSize, pClientInfo->mRecvBuf);

				SendMsg(pClientInfo, pClientInfo->mRecvBuf, dwIoSize);

				BindRecv(pClientInfo);
			}
			else if (IOOperation::SEND == pOverlappedEx->m_eOperation)
			{
				printf("[송신] bytes: %d, msg: %s\n", dwIoSize, pClientInfo->mSendBuf);
			}
			else
			{
				printf("socket(%d)에서 예외 상황\n", (int)pClientInfo->m_socketClient);
			}
		}
	}

	void AcceptorThread()
	{
		SOCKADDR_IN stClientAddr;
		int nAddrLen = sizeof(SOCKADDR_IN);

		while (mIsAcceptorRun)
		{
			stClientInfo* pClientInfo = GetEmptyClientInfo();
			if (NULL == pClientInfo)
			{
				printf("[에러] Client Full \n");
				return;
			}

			pClientInfo->m_socketClient = accept(mListenSocket, (SOCKADDR*)&stClientAddr, &nAddrLen);
			if (INVALID_SOCKET == pClientInfo->m_socketClient)
			{
				continue;
			}

			bool bRet = BindIOCompletionPort(pClientInfo);
			if (false == bRet)
			{
				return;
			}

			bRet = BindRecv(pClientInfo);
			if (false == bRet)
			{
				return;
			}

			OnConnect(pClientInfo->mIndex);

			++mClientCount;
		}
	}


	std::vector<stClientInfo> mClientInfos;
	SOCKET mListenSocket = INVALID_SOCKET;
	int mClientCount = 0;

	std::vector<std::thread> mIOWorkerThreads;
	std::thread mAcceptorThread;

	HANDLE mIOCPHandle = INVALID_HANDLE_VALUE;

	bool mIsWorkerRun = true;
	bool mIsAcceptorRun = true;
	char mSocketBuf[1024] = { 0, };
};