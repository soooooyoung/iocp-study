//출처: 강정중님의 저서 '온라인 게임서버'에서
#pragma once
#pragma comment(lib, "ws2_32.lib")

#include <winsock2.h>
#include <Ws2tcpip.h>

#include <thread>
#include <vector>

#define MAX_SOCKETBUF 1024
#define MAX_WORKER_THREAD 4
#define MAX_CONNECTION_QUEUE 5

enum class IOOperation
{
	RECV,
	SEND
};

// Overlapped structure 확장
struct stOverlappedEx
{
	WSAOVERLAPPED m_wsaOverlapped;
	SOCKET m_socketClient;
	WSABUF m_wsaBuf;
	char m_szBuf[MAX_SOCKETBUF];
	IOOperation m_eOperation;
};

// 클라이언트 정보
struct stClientInfo
{
	SOCKET m_socketClient;
	stOverlappedEx m_stRecvOverlappedEx;
	stOverlappedEx m_stSendOverlappedEx;

	stClientInfo()
	{
		ZeroMemory(&m_stRecvOverlappedEx, sizeof(stOverlappedEx));
		ZeroMemory(&m_stSendOverlappedEx, sizeof(stOverlappedEx));
		m_socketClient = INVALID_SOCKET;
	}
};

class IOCompletionPort
{
public:
	IOCompletionPort(void) {}
	~IOCompletionPort(void)
	{
		WSACleanup();
	}

	bool InitSocket()
	{
		WSADATA wsaData;

		int nRet = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (nRet != 0)
		{
			printf("WSAStartup() Error: %d\n", WSAGetLastError());
			return false;
		}

		mListenSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);

		if (INVALID_SOCKET == mListenSocket)
		{
			printf("WSASocket() Error: %d\n", WSAGetLastError());
			return false;
		}

		printf("WSASocket() Success\n");
		return true;
	} 

	// 서버의 주소정보를 소켓과 연결 시키고 접속 요청 대기위해 소켓을 등록
	bool BindAndListen(int nBindPort)
	{
		SOCKADDR_IN stServerAddr;
		stServerAddr.sin_family = AF_INET;
		stServerAddr.sin_port = htons(nBindPort); // 서버의 포트 설정

		// 모든 주소에서 접속을 허용
		stServerAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

		// 지정한 서버 주소와 cIOCompletionPort::mListenSocket을 바인딩
		int nRet = bind(mListenSocket, (SOCKADDR*)&stServerAddr, sizeof(SOCKADDR_IN));
		if (SOCKET_ERROR == nRet)
		{
			printf("bind() Error: %d\n", WSAGetLastError());
			return false;
		}

		// 클라이언트의 접속 요청을 받아들이기 위해 소켓을 등록
		nRet = listen(mListenSocket, MAX_CONNECTION_QUEUE); // 접속 대기큐의 크기는 5
		if (SOCKET_ERROR == nRet)
		{
			printf("listen() Error: %d\n", WSAGetLastError());
			return false;
		}

		printf("bind() & listen() Success\n");
		return true;
	}

	// IOCP를 생성하고 작업자 스레드를 생성
	bool StartServer(const UINT32 maxClientCount)
	{
		CreateClient(maxClientCount);

		mIOCPHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, MAX_WORKER_THREAD);
		if (NULL == mIOCPHandle)
		{
			printf("CreateIoCompletionPort() Error: %d\n", GetLastError());
			return false;
		}

		mIsWorkerRunning = true;
		mIsAccepterRunning = true;

		bool bRet = CreateWorkerThread();
		if (false == bRet)
		{
			printf("CreateWorkerThread() Error\n");
			return false;
		}

		bRet = CreateAcceptorThread();
		if (false == bRet)
		{
			printf("CreateAcceptorThread() Error\n");
			return false;
		}
	}

	void DestroyThread()
	{
		mIsWorkerRunning = false;
		mIsAccepterRunning = false;

		// WorkerThread 종료를 위해 PostQueuedCompletionStatus() 호출
		for (int i = 0; i < MAX_WORKER_THREAD; ++i)
		{
			PostQueuedCompletionStatus(mIOCPHandle, 0, NULL, NULL);
		}

		// WorkerThread 종료 대기
		for (auto& workerThread : mIOWorkerThreads)
		{
			if (workerThread.joinable())
			{
				workerThread.join();
			}
		}

		closesocket(mListenSocket);

		if (mAcceptorThread.joinable())
		{
			mAcceptorThread.join();
		}

		// 모든 클라이언트 소켓 닫기
		for (auto& clientInfo : mClientInfos)
		{
			if (clientInfo.m_socketClient != INVALID_SOCKET)
			{
				CloseSocket(&clientInfo);
			}
		}

		// IOCP 객체 닫기
		CloseHandle(mIOCPHandle);

		printf("DestroyThread() Success\n");
	}

private:
	void CreateClient(const UINT32 maxClientCount)
	{
		for (UINT32 i = 0; i < maxClientCount; ++i)
		{
			mClientInfos.emplace_back();
		}
	}

	bool CreateWorkerThread()
	{
		unsigned int uiThreadId = 0;

		// WaitingThread Queue에 대기 상태로 넣을 쓰레드들 생성 권장 개수는 CPU 코어 개수 * 2 + 1
		for (int i = 0; i < MAX_WORKER_THREAD; ++i)
		{
		
			mIOWorkerThreads.emplace_back([this]() { WorkerThread(); });
		}

		printf("CreateWorkerThread() Success\n");
		return true;
	}

	bool CreateAcceptorThread()
	{
		mAcceptorThread = std::thread([this]() { AcceptorThread(); });
		printf("CreateAcceptorThread() Success\n");
		return true;
	}

	// Overlapped I/O 작업에 대한 결과를 받아 처리하는 쓰레드
	void WorkerThread()
	{
		// IOCP 객체와 소켓과 CompletionKey를 연결
		stClientInfo* pClientInfo = NULL;
		// 함수 호출 성공 여부
		BOOL bSuccess = TRUE;
		// Overlapped I/O 작업에서 전송된 데이터의 크기
		DWORD dwIoSize = 0;
		// I/O 작업을 위해 요청한 Overlapped 구조체를 받을 포인터
		LPOVERLAPPED lpOverlapped = NULL;

		while (mIsWorkerRunning)
		{
			// IOCP 객체에서 Overlapped I/O 작업 결과를 받아옴
			bSuccess = GetQueuedCompletionStatus(mIOCPHandle, &dwIoSize, (PULONG_PTR)&pClientInfo, &lpOverlapped, INFINITE);

			if (TRUE == bSuccess && 0 == dwIoSize && NULL == lpOverlapped)
			{
				mIsWorkerRunning = false;
				break;
			}

			if (NULL == lpOverlapped)
			{
				continue;
			}

			if (FALSE == bSuccess || (0 == dwIoSize && TRUE == bSuccess))
			{
				printf("socket(%d) Client Disconnected\n", (int)pClientInfo->m_socketClient);
				CloseSocket(pClientInfo);
				continue;
			}

			stOverlappedEx* pOverlappedEx = (stOverlappedEx*)lpOverlapped;

			// Overlapped I/O 작업의 결과에 따라 처리
			switch (pOverlappedEx->m_eOperation)
			{
			case IOOperation::RECV:
			{
				pOverlappedEx->m_szBuf[dwIoSize] = NULL;
				printf("Recv Data: %s\n", pOverlappedEx->m_szBuf);

				// 받은 데이터를 클라이언트에게 다시 보냄
				SendMsg(pClientInfo, pOverlappedEx->m_szBuf, dwIoSize);
				BindRecv(pClientInfo);
			}
			break;

			case IOOperation::SEND:
			{
				printf("Send Data: %s\n", pOverlappedEx->m_szBuf);
			}
			break;
			default:
			{
				printf("socket(%d) Unknown Operation\n", (int)pClientInfo->m_socketClient);
			}
			}
		}
	}

	// 사용자의 접속을 받는 쓰레드
	void AcceptorThread()
	{
		SOCKADDR_IN stClientAddr;
		int nAddrLength = sizeof(SOCKADDR_IN);

		while (mIsAccepterRunning)
		{
			stClientInfo* pClientInfo = GetEmptyClientInfo();
			if (NULL == pClientInfo)
			{
				printf("Client Full\n");
				continue;
			}

			// 클라이언트의 접속 요청 대기
			pClientInfo->m_socketClient = accept(mListenSocket, (SOCKADDR*)&stClientAddr, &nAddrLength);
			if (INVALID_SOCKET == pClientInfo->m_socketClient)
			{
				printf("accept() Error: %d\n", WSAGetLastError());
				continue;
			}

			// IOCP 객체와 소켓을 연결
			bool bRet = BindIOCompletionPort(pClientInfo);
			if (false == bRet)
			{
				printf("BindIOCompletionPort() Error\n");
				CloseSocket(pClientInfo);
				continue;
			}

			// Recv Overlapped I/O 시작
			bRet = BindRecv(pClientInfo);
			if (false == bRet)
			{
				printf("BindRecv() Error\n");
				CloseSocket(pClientInfo);
				continue;
			}

			char clientIP[32] = { 0, };
			// 접속한 클라이언트의 IP를 얻어옴
			inet_ntop(AF_INET, &stClientAddr.sin_addr, clientIP, 32 - 1);
			printf("Client Connected IP: %s\n SOCKET(%d)", clientIP, (int)pClientInfo->m_socketClient);

			++mClientCount;
		}
	}

	void CloseSocket(stClientInfo* pClientInfo, bool bIsForce = false)
	{
		struct linger stLinger = { 0, }; // SO_DONTLINGER로 설정하면 소켓이 닫힐 때 데이터를 보내지 않고 바로 닫히게 된다.

		if (true == bIsForce)
		{
			stLinger.l_onoff = 1;
			stLinger.l_linger = 0;
		}

		// 소켓의 데이터 송수신을 중단
		shutdown(pClientInfo->m_socketClient, SD_BOTH);

		// 소켓 옵션 설정
		setsockopt(pClientInfo->m_socketClient, SOL_SOCKET, SO_LINGER, (char*)&stLinger, sizeof(stLinger));

		// 소켓 닫기
		closesocket(pClientInfo->m_socketClient);

		// 클라이언트 정보 초기화
		pClientInfo->m_socketClient = INVALID_SOCKET;
	}

	// WSARecv()를 호출하여 Overlapped I/O를 시작
	bool BindRecv(stClientInfo* pClientInfo)
	{
		DWORD dwFlag = 0;
		DWORD dwRecvBytes = 0;

		// Overlapped I/O 시작을 위해 정보를 설정
		pClientInfo->m_stRecvOverlappedEx.m_wsaBuf.len = MAX_SOCKETBUF;
		pClientInfo->m_stRecvOverlappedEx.m_wsaBuf.buf = pClientInfo->m_stRecvOverlappedEx.m_szBuf;
		pClientInfo->m_stRecvOverlappedEx.m_eOperation = IOOperation::RECV;

		// 데이터를 받기 위해 WSARecv() 호출
		int nRet = WSARecv(pClientInfo->m_socketClient, &pClientInfo->m_stRecvOverlappedEx.m_wsaBuf, 1, &dwRecvBytes, &dwFlag, (LPWSAOVERLAPPED)&pClientInfo->m_stRecvOverlappedEx, NULL);

		if (SOCKET_ERROR == nRet)
		{
			int nError = WSAGetLastError();
			if (WSA_IO_PENDING != nError)
			{
				printf("WSARecv() Error: %d\n", nError);
				return false;
			}
		}

		return true;
	}

	// WSASend()를 호출하여 Overlapped I/O를 시작
	bool SendMsg(stClientInfo* pClientInfo, char* pMsg, int nLen)
	{
		DWORD dwSendBytes = 0;

		// 전송될 메시지를 복사
		CopyMemory(pClientInfo->m_stSendOverlappedEx.m_szBuf, pMsg, nLen);

		// Overlapped I/O 시작을 위해 정보를 설정
		pClientInfo->m_stSendOverlappedEx.m_wsaBuf.len = nLen;
		pClientInfo->m_stSendOverlappedEx.m_wsaBuf.buf = pClientInfo->m_stSendOverlappedEx.m_szBuf;
		pClientInfo->m_stSendOverlappedEx.m_eOperation = IOOperation::SEND;

		// 데이터를 보내기 위해 WSASend() 호출
		int nRet = WSASend(pClientInfo->m_socketClient, &pClientInfo->m_stSendOverlappedEx.m_wsaBuf, 1, &dwSendBytes, 0, (LPWSAOVERLAPPED)&pClientInfo->m_stSendOverlappedEx, NULL);

		if (SOCKET_ERROR == nRet)
		{
			int nError = WSAGetLastError();
			if (WSA_IO_PENDING != nError)
			{
				printf("WSASend() Error: %d\n", nError);
				return false;
			}
		}

		return true;
	}

	// IOCP 객체와 소켓과 CompletionKey를 연결
	bool BindIOCompletionPort(stClientInfo* pClientInfo)
	{
		HANDLE hIOCP = CreateIoCompletionPort((HANDLE)pClientInfo->m_socketClient, mIOCPHandle, (ULONG_PTR)pClientInfo, 0);
		if (NULL == hIOCP || (mIOCPHandle != hIOCP))
		{
			printf("CreateIoCompletionPort() Error: %d\n", GetLastError());
			return false;
		}

		return true;
	}

	stClientInfo* GetEmptyClientInfo()
	{
		for (auto& clientInfo : mClientInfos)
		{
			if (INVALID_SOCKET == clientInfo.m_socketClient)
			{
				return &clientInfo;
			}
		}

		return nullptr;
	}

	std::vector<stClientInfo> mClientInfos;

	SOCKET mListenSocket = INVALID_SOCKET;
	int mClientCount = 0;
	
	std::vector<std::thread> mIOWorkerThreads;
	std::thread mAcceptorThread;
	HANDLE mIOCPHandle = INVALID_HANDLE_VALUE;

	bool mIsWorkerRunning = false;
	bool mIsAccepterRunning = false;

	char mSocketBuf[1024] = { 0, };
};

