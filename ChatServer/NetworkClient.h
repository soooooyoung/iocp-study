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

	bool IsConnected() { return mIsConnect == 1; }

	SOCKET GetSocket() { return mSocket; }

	UINT64 GetLatestClosedTimeSec() { return mLatestClosedTimeSec; }

	char* RecvBuffer() { return mRecvBuf; }

	void Clear()
	{
	}
	bool OnConnect(HANDLE iocpHandle_, SOCKET socket_)
	{
		mSocket = socket_;
		mIsConnect = 1;

		Clear();

		// Associate the socket with the IOCP
		if (BindIOCompletionPort(iocpHandle_) == false)
		{
			return false;
		}

		return BindRecv();
	}

	//void OnDisconnect()
	//{
	//	mIsConnect = 0;
	//	mLatestClosedTimeSec = time(nullptr);
	//}

	bool AcceptCompletion()
	{
		printf_s("AcceptCompletion : SessionIndex(%d)\n", mIndex);

		if (OnConnect(mIOCPHandle, mSocket) == false)
		{
			return false;
		}

		SOCKADDR_IN clientAddr;
		int nAddrLen = sizeof(SOCKADDR_IN);
		char clientIP[32] = { 0, };
		inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, 32 - 1);
		printf("Client IP : %s SOCKET : %d\n", clientIP, (int) mSocket);

		return true;
	}

	bool BindIOCompletionPort(HANDLE iocpHandle_)
	{
		HANDLE hIOCP = CreateIoCompletionPort((HANDLE)GetSocket()
			, iocpHandle_
			, (ULONG_PTR)this, 0);

		if (hIOCP == INVALID_HANDLE_VALUE)
		{
			printf("CreateIoCompletionPort Error : %d\n", GetLastError());
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
			(LPWSAOVERLAPPED)&mRecvOverlappedEx,
			NULL);

		if (nRet == SOCKET_ERROR && (ERROR_IO_PENDING != WSAGetLastError()))
		{
			printf("WSARecv Error : %d\n", WSAGetLastError());
			return false;
		}
	}


	bool SendMsg(const UINT32 dataSize_, char* pMsg_)
	{
		auto sendOverlappedEx = new stOverlappedEx();
		ZeroMemory(sendOverlappedEx, sizeof(stOverlappedEx));
		sendOverlappedEx->m_wsaBuf.len = dataSize_;
		sendOverlappedEx->m_wsaBuf.buf = new char[dataSize_];
		CopyMemory(sendOverlappedEx->m_wsaBuf.buf, pMsg_, dataSize_);
		sendOverlappedEx->m_eOperation = IOOperation::SEND;

		std::lock_guard<std::mutex> lock(mSendLock);

		mSendQueue.push(sendOverlappedEx);
		if (mSendQueue.size() == 1)
		{
			SendIO();
		}

		return true;
	}

	void SendCompleted(const UINT32 dataSize_)
	{
		printf("SendCompleted bytes : %d\n", dataSize_);

		std::lock_guard<std::mutex> lock(mSendLock);

		delete[] mSendQueue.front()->m_wsaBuf.buf;
		delete mSendQueue.front();

		mSendQueue.pop();

		if (mSendQueue.empty() == false)
		{
			SendIO();
		}
	}
private:
	bool SendIO()
	{
		auto sendOverlappedEx = mSendQueue.front();

		DWORD dwRecvNumBytes = 0;
		int nRet = WSASend(mSocket,
			&(sendOverlappedEx->m_wsaBuf),
			1,
			&dwRecvNumBytes,
			0,
			(LPWSAOVERLAPPED)sendOverlappedEx,
			NULL);

		if (nRet == SOCKET_ERROR && (ERROR_IO_PENDING != WSAGetLastError()))
		{
			printf("WSASend Error : %d\n", WSAGetLastError());
			return false;
		}

		return true;
	}

	bool SetSocketOption()
	{
		int nRet = 0;
		int nOption = 1;

		nRet = setsockopt(mSocket, IPPROTO_TCP, TCP_NODELAY, (const char*)&nOption, sizeof(int));
		if (nRet == SOCKET_ERROR)
		{
			printf("setsockopt TCP_NODELAY Error : %d\n", WSAGetLastError());
			return false;
		}

		nOption = 0;
		nRet = setsockopt(mSocket, SOL_SOCKET, SO_RCVBUF, (const char*)&nOption, sizeof(int));
		if (nRet == SOCKET_ERROR)
		{
			printf("setsockopt SO_RCVBUF Error : %d\n", WSAGetLastError());
			return false;
		}

		return true;
	}
private:

	INT32 mIndex = 0;
	INT64 mIsConnect = 0;
	UINT64 mLatestClosedTimeSec = 0;

	HANDLE mIOCPHandle = INVALID_HANDLE_VALUE;
	SOCKET mSocket = INVALID_SOCKET;

	stOverlappedEx mAcceptContext;
	stOverlappedEx mRecvOverlappedEx;

	char mAcceptBuf[64];
	char mRecvBuf[MAX_SOCK_RECVBUF];

	std::mutex mSendLock;
	std::queue<stOverlappedEx*> mSendQueue;
};