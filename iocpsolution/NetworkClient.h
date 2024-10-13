#pragma once
#include "Define.h"
#include <stdio.h>
#include <mutex>

class NetworkClient
{
public:
	NetworkClient()
	{
		ZeroMemory(&mRecvOverlappedEx, sizeof(stOverlappedEx));
		ZeroMemory(&mSendOverlappedEx, sizeof(stOverlappedEx));
		mSock = INVALID_SOCKET;
	}
	~NetworkClient() = default;

	void Init(const UINT32 index)
	{
		mIndex = index;
	}

	UINT32 GetIndex() { return mIndex; }

	bool IsConnected() { return mSock != INVALID_SOCKET; }

	SOCKET GetSock() { return mSock; }

	char* RecvBuffer() { return mRecvBuf; }

	bool OnConnect(HANDLE iocpHandle_, SOCKET socket_)
	{
		mSock = socket_;

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

		shutdown(mSock, SD_BOTH);

		setsockopt(mSock, SOL_SOCKET, SO_LINGER, (char*)&stLinger, sizeof(stLinger));

		closesocket(mSock);
		mSock = INVALID_SOCKET;
	}


	bool BindIOCompletionPort(HANDLE iocpHandle_)
	{
		auto hIOCP = CreateIoCompletionPort((HANDLE)GetSock()
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

		mRecvOverlappedEx.m_wsaBuf.len = MAX_SOCKBUF;
		mRecvOverlappedEx.m_wsaBuf.buf = mRecvBuf;
		mRecvOverlappedEx.m_eOperation = IOOperation::RECV;

		int nRet = WSARecv(mSock,
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

	// 1개의 스레드에서만 호출해야 한다!
	bool SendMsg(const UINT32 dataSize_, char* pMsg_)
	{
		std::lock_guard<std::mutex> guard(mSendLock);

		// 초과 되면 데이터를 덮어 씌울수 있음
		if (mSendPos + dataSize_ > MAX_SOCK_SENDBUF)
		{
			/*printf("[에러] 송신 버퍼 초과\n");
			return false;*/
			mSendPos = 0;
		}

		auto pSendBuf = &mSendBuf[mSendPos];

		CopyMemory(pSendBuf, pMsg_, dataSize_);
		mSendPos += dataSize_;

		return true;
	}

	bool SendIO()
	{
		if (mSendPos <= 0 || mIsSending)
		{
			return false;
		}

		std::lock_guard<std::mutex> guard(mSendLock);

		mIsSending = true;

		CopyMemory(mSendingBuf, &mSendBuf[0], mSendPos);

		mSendOverlappedEx.m_wsaBuf.len = mSendPos;
		mSendOverlappedEx.m_wsaBuf.buf = &mSendingBuf[0];
		mSendOverlappedEx.m_eOperation = IOOperation::SEND;

		DWORD dwRecvNumBytes = 0;
		int nRet = WSASend(mSock,
			&(mSendOverlappedEx.m_wsaBuf),
			1,
			&dwRecvNumBytes,
			0,
			(LPWSAOVERLAPPED) & (mSendOverlappedEx),
			NULL);

		if (nRet == SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING))
		{
			printf("[에러] WSASend() 함수 실패: %d \n", WSAGetLastError());
			return false;
		}

		mSendPos = 0;
		return true;
	}

	void SendCompleted(const UINT32 dataSize_)
	{
		mIsSending = false;
		printf("[송신완료] bytes: %d\n", dataSize_);
	}

	void Clear() {
		ZeroMemory(&mRecvOverlappedEx, sizeof(stOverlappedEx));
		ZeroMemory(&mSendOverlappedEx, sizeof(stOverlappedEx));
		ZeroMemory(mRecvBuf, MAX_SOCKBUF);
		ZeroMemory(mSendBuf, MAX_SOCK_SENDBUF);
		ZeroMemory(mSendingBuf, MAX_SOCK_SENDBUF);
		mSendPos = 0;
		mIsSending = false;
	}

private: 
	INT32 mIndex = 0;
	SOCKET mSock; // 연결되는 소켓
	stOverlappedEx mRecvOverlappedEx; // RECV Overlapped I/O 작업용 변수
	stOverlappedEx mSendOverlappedEx; // SEND Overlapped I/O 작업용 변수

	char mRecvBuf[MAX_SOCKBUF]; // 데이터 버퍼

	std::mutex mSendLock;
	bool mIsSending = false;
	UINT64 mSendPos = 0;
	char mSendBuf[MAX_SOCK_SENDBUF];
	char mSendingBuf[MAX_SOCK_SENDBUF];
};