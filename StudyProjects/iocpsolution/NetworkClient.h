#pragma once
#include "Define.h"
#include <stdio.h>
#include <mutex>
#include <queue>

class NetworkClient
{
public:
	NetworkClient()
	{
		ZeroMemory(&mRecvOverlappedEx, sizeof(stOverlappedEx));
		mSocket = INVALID_SOCKET;
	}
	~NetworkClient() = default;

	void Init(const UINT32 index, HANDLE iocpHandle_)
	{
		mIndex = index;
		mIOCPHandle = iocpHandle_;
	}

	UINT32 GetIndex() { return mIndex; }

	bool IsConnected() { return mSocket != INVALID_SOCKET; }

	SOCKET GetSocket() { return mSocket; }

	UINT64 GetLatestClosedTimeSec() { return mLatestClosedTimeSec; }

	char* RecvBuffer() { return mRecvBuf; }

	bool OnConnect(HANDLE iocpHandle_, SOCKET socket_)
	{
		mSocket = socket_;
		mIsConnect = 1;

		Clear();

		if (false == BindIOCompletionPort(iocpHandle_))
		{
			return false;
		}

		return BindRecv();
	}

	void Close(bool bIsForce = false)
	{
		struct linger stLinger = { 0,0 }; // SO_DONTLINGER로 설정

		if (true == bIsForce)
		{
			stLinger.l_onoff = 1;
		}

		shutdown(mSocket, SD_BOTH);

		setsockopt(mSocket, SOL_SOCKET, SO_LINGER, (char*)&stLinger, sizeof(stLinger));

		mIsConnect = 0;
		mLatestClosedTimeSec = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();

		closesocket(mSocket);
		mSocket = INVALID_SOCKET;
	}

	bool PostAccept(SOCKET listenSock_, const UINT64 curTimeSec_)
	{
		printf_s("[DEBUG] PostAccept : SESSION INDEX(%d)\n", mIndex);

		mLatestClosedTimeSec = UINT32_MAX;

		mSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_IP,
			NULL, 0, WSA_FLAG_OVERLAPPED);

		if (INVALID_SOCKET == mSocket)
		{
			printf_s("[에러] WSASocket()함수 실패 : %d\n", WSAGetLastError());
			return false;
		}

		ZeroMemory(&mAcceptContext, sizeof(stOverlappedEx));

		DWORD bytes = 0;
		DWORD flags = 0;
		mAcceptContext.m_wsaBuf.len = 0;
		mAcceptContext.m_wsaBuf.buf = nullptr;
		mAcceptContext.m_eOperation = IOOperation::ACCEPT;
		mAcceptContext.mSessionIndex = mIndex;

		if (FALSE == AcceptEx(listenSock_, mSocket, mAcceptBuf, 0,
			sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, &bytes,
			(LPWSAOVERLAPPED)&mAcceptContext))
		{
			if (WSAGetLastError() != WSA_IO_PENDING)
			{
				printf_s("[에러] AcceptEx()함수 실패 : %d\n", WSAGetLastError());
				return false;
			}
		}

		return true;
	}

	bool AcceptCompletion()
	{
		printf_s("[DEBUG] AcceptCompletion : SESSION INDEX(%d)\n", mIndex);

		if (OnConnect(mIOCPHandle, mSocket) == false)
		{
			return false;
		}

		SOCKADDR_IN stClientAddr;
		int nAddrLen = sizeof(SOCKADDR_IN);
		char clientIP[32] = { 0, };
		inet_ntop(AF_INET, &stClientAddr.sin_addr, clientIP, 32 - 1);
		printf("[DEBUG] 클라이언트 접속 IP: %s SOCKET: %d\n", clientIP, (int)mSocket);

		return true;
	}


	bool BindIOCompletionPort(HANDLE iocpHandle_)
	{
		auto hIOCP = CreateIoCompletionPort((HANDLE)GetSocket()
			, iocpHandle_
			, (ULONG_PTR)(this), 0);

		if (hIOCP == INVALID_HANDLE_VALUE)
		{
			printf("[에러] CreateIoCompletionPort()함수 실패: %d\n", GetLastError());
			return false;
		}

		return true;
	}

	bool BindRecv()
	{
		DWORD dwFlag = 0;
		DWORD dwRecvNumBytes = 0;

		mRecvOverlappedEx.m_wsaBuf.len = MAX_SOCK_RECVBUF;
		mRecvOverlappedEx.m_wsaBuf.buf = mRecvBuf;
		mRecvOverlappedEx.m_eOperation = IOOperation::RECV;

		int nRet = WSARecv(mSocket,
			&(mRecvOverlappedEx.m_wsaBuf),
			1,
			&dwRecvNumBytes,
			&dwFlag,
			(LPWSAOVERLAPPED) & (mRecvOverlappedEx),
			NULL);

		if (nRet == SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING))
		{
			printf("[에러] WSARecv()함수 실패 : %d\n", WSAGetLastError());
			return false;
		}

		return true;
	}

	bool SendMsg(const UINT32 dataSize_, char* pMsg_)
	{
		auto sendOverlappedEx = new stOverlappedEx();
		ZeroMemory(sendOverlappedEx, sizeof(stOverlappedEx));
		sendOverlappedEx->m_wsaBuf.len = dataSize_;
		sendOverlappedEx->m_wsaBuf.buf = new char[dataSize_];
		CopyMemory(sendOverlappedEx->m_wsaBuf.buf, pMsg_, dataSize_);
		sendOverlappedEx->m_eOperation = IOOperation::SEND;

		std::lock_guard<std::mutex> guard(mSendLock);

		mSendQueue.push(sendOverlappedEx);

		if (mSendQueue.size() == 1)
		{
			SendIO();
		}

		return true;
	}

	void SendCompleted(const UINT32 dataSize_)
	{
		printf("[송신완료] bytes: %d\n", dataSize_);

		std::lock_guard<std::mutex> guard(mSendLock);

		delete[] mSendQueue.front()->m_wsaBuf.buf;
		delete mSendQueue.front();

		mSendQueue.pop();

		if (mSendQueue.empty() == false)
		{
			SendIO();
		}
	}

	void Clear() {

	}

private: 
	bool SendIO()
	{

		auto pSendOverlappedEx = mSendQueue.front();

		DWORD dwRecvNumBytes = 0;
		int nRet = WSASend(mSocket,
			&(pSendOverlappedEx->m_wsaBuf),
			1,
			&dwRecvNumBytes,
			0,
			(LPWSAOVERLAPPED)pSendOverlappedEx,
			NULL);

		if (nRet == SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING))
		{
			printf("[에러] WSASend() 함수 실패: %d \n", WSAGetLastError());
			return false;
		}

		return true;
	}

	bool SetSocketOption()
	{
		int opt = 1;
		if (SOCKET_ERROR == setsockopt(mSocket, IPPROTO_TCP, TCP_NODELAY,
			(const char*)&opt, sizeof(int)))
		{
			printf_s("[DEBUG] TCP_NODELAY ERROR: %d \n", GetLastError());
			return false;
		}

		opt = 0;
		if (SOCKET_ERROR == setsockopt(mSocket, SOL_SOCKET, SO_RCVBUF,
			(const char*)&opt, sizeof(int)))
		{
			printf_s("[DEBUG] SO_RCVBUF CHANGE ERROR: %d \n", GetLastError());
			return false;
		}

		return true;
	}

	INT32 mIndex = 0;
	HANDLE mIOCPHandle = INVALID_HANDLE_VALUE;

	INT64 mIsConnect = 0;
	UINT64 mLatestClosedTimeSec = 0;

	SOCKET mSocket; // 연결되는 소켓

	stOverlappedEx mAcceptContext;
	char mAcceptBuf[64];

	stOverlappedEx mRecvOverlappedEx; // RECV Overlapped I/O 작업용 변수
	char mRecvBuf[MAX_SOCK_RECVBUF]; // 데이터 버퍼

	std::mutex mSendLock;
	std::queue<stOverlappedEx*> mSendQueue;
};