//��ó: �����ߴ��� ���� '�¶��� ���Ӽ���'����
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

// Overlapped structure Ȯ��
struct stOverlappedEx
{
	WSAOVERLAPPED m_wsaOverlapped;
	SOCKET m_socketClient;
	WSABUF m_wsaBuf;
	char m_szBuf[MAX_SOCKETBUF];
	IOOperation m_eOperation;
};

// Ŭ���̾�Ʈ ����
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

	// ������ �ּ������� ���ϰ� ���� ��Ű�� ���� ��û ������� ������ ���
	bool BindAndListen(int nBindPort)
	{
		SOCKADDR_IN stServerAddr;
		stServerAddr.sin_family = AF_INET;
		stServerAddr.sin_port = htons(nBindPort); // ������ ��Ʈ ����

		// ��� �ּҿ��� ������ ���
		stServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);

		// ������ ���� �ּҿ� cIOCompletionPort::mListenSocket�� ���ε�
		int nRet = bind(mListenSocket, (SOCKADDR*)&stServerAddr, sizeof(SOCKADDR_IN));
		if (SOCKET_ERROR == nRet)
		{
			printf("bind() Error: %d\n", WSAGetLastError());
			return false;
		}

		// Ŭ���̾�Ʈ�� ���� ��û�� �޾Ƶ��̱� ���� ������ ���
		nRet = listen(mListenSocket, MAX_CONNECTION_QUEUE); // ���� ���ť�� ũ��� 5
		if (SOCKET_ERROR == nRet)
		{
			printf("listen() Error: %d\n", WSAGetLastError());
			return false;
		}

		printf("bind() & listen() Success\n");
		return true;
	}

	// IOCP�� �����ϰ� �۾��� �����带 ����
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

		// WorkerThread ���� ���
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

		// WaitingThread Queue�� ��� ���·� ���� ������� ���� ���� ������ CPU �ھ� ���� * 2 + 1
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

	// Overlapped I/O �۾��� ���� ����� �޾� ó���ϴ� ������
	void WorkerThread()
	{
		// IOCP ��ü�� ���ϰ� CompletionKey�� ����
		stClientInfo* pClientInfo = NULL;
		// �Լ� ȣ�� ���� ����
		BOOL bSuccess = TRUE;
		// Overlapped I/O �۾����� ���۵� �������� ũ��
		DWORD dwIoSize = 0;
		// I/O �۾��� ���� ��û�� Overlapped ����ü�� ���� ������
		LPOVERLAPPED lpOverlapped = NULL;

		while (mIsWorkerRunning)
		{
			// IOCP ��ü���� Overlapped I/O �۾� ����� �޾ƿ�
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

			// Overlapped I/O �۾��� ����� ���� ó��
			switch (pOverlappedEx->m_eOperation)
			{
			case IOOperation::RECV:
			{
				pOverlappedEx->m_szBuf[dwIoSize] = NULL;
				printf("Recv Data: %s\n", pOverlappedEx->m_szBuf);

				// ���� �����͸� Ŭ���̾�Ʈ���� �ٽ� ����
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

	// ������� ������ �޴� ������
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

			// Ŭ���̾�Ʈ�� ���� ��û ���
			pClientInfo->m_socketClient = accept(mListenSocket, (SOCKADDR*)&stClientAddr, &nAddrLength);
			if (INVALID_SOCKET == pClientInfo->m_socketClient)
			{
				printf("accept() Error: %d\n", WSAGetLastError());
				continue;
			}

			// IOCP ��ü�� ������ ����
			bool bRet = BindIOCompletionPort(pClientInfo);
			if (false == bRet)
			{
				printf("BindIOCompletionPort() Error\n");
				CloseSocket(pClientInfo);
				continue;
			}

			// Recv Overlapped I/O ����
			bRet = BindRecv(pClientInfo);
			if (false == bRet)
			{
				printf("BindRecv() Error\n");
				CloseSocket(pClientInfo);
				continue;
			}

			char clientIP[32] = { 0, };
			// ������ Ŭ���̾�Ʈ�� IP�� ����
			inet_ntop(AF_INET, &stClientAddr.sin_addr, clientIP, 32 - 1);
			printf("Client Connected IP: %s\n SOCKET(%d)", clientIP, (int)pClientInfo->m_socketClient);

			++mClientCount;
		}
	}

	void CloseSocket(stClientInfo* pClientInfo, bool bIsForce = false)
	{
		struct linger stLinger = { 0, }; // SO_DONTLINGER�� �����ϸ� ������ ���� �� �����͸� ������ �ʰ� �ٷ� ������ �ȴ�.

		if (true == bIsForce)
		{
			stLinger.l_onoff = 1;
			stLinger.l_linger = 0;
		}

		// ������ ������ �ۼ����� �ߴ�
		shutdown(pClientInfo->m_socketClient, SD_BOTH);

		// ���� �ɼ� ����
		setsockopt(pClientInfo->m_socketClient, SOL_SOCKET, SO_LINGER, (char*)&stLinger, sizeof(stLinger));

		// ���� �ݱ�
		closesocket(pClientInfo->m_socketClient);

		// Ŭ���̾�Ʈ ���� �ʱ�ȭ
		pClientInfo->m_socketClient = INVALID_SOCKET;
	}

	// WSARecv()�� ȣ���Ͽ� Overlapped I/O�� ����
	bool BindRecv(stClientInfo* pClientInfo)
	{
		DWORD dwFlag = 0;
		DWORD dwRecvBytes = 0;

		// Overlapped I/O ������ ���� ������ ����
		pClientInfo->m_stRecvOverlappedEx.m_wsaBuf.len = MAX_SOCKETBUF;
		pClientInfo->m_stRecvOverlappedEx.m_wsaBuf.buf = pClientInfo->m_stRecvOverlappedEx.m_szBuf;
		pClientInfo->m_stRecvOverlappedEx.m_eOperation = IOOperation::RECV;

		// �����͸� �ޱ� ���� WSARecv() ȣ��
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

	// WSASend()�� ȣ���Ͽ� Overlapped I/O�� ����
	bool SendMsg(stClientInfo* pClientInfo, char* pMsg, int nLen)
	{
		DWORD dwSendBytes = 0;

		// ���۵� �޽����� ����
		CopyMemory(pClientInfo->m_stSendOverlappedEx.m_szBuf, pMsg, nLen);

		// Overlapped I/O ������ ���� ������ ����
		pClientInfo->m_stSendOverlappedEx.m_wsaBuf.len = nLen;
		pClientInfo->m_stSendOverlappedEx.m_wsaBuf.buf = pClientInfo->m_stSendOverlappedEx.m_szBuf;
		pClientInfo->m_stSendOverlappedEx.m_eOperation = IOOperation::SEND;

		// �����͸� ������ ���� WSASend() ȣ��
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

	// IOCP ��ü�� ���ϰ� CompletionKey�� ����
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

