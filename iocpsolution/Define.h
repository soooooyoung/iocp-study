#pragma once


#include <winsock2.h>
#include <WS2tcpip.h>

const UINT32 MAX_SOCKBUF = 256; 
const UINT32 MAX_WORKER_THREAD = 4;

enum class IOOperation
{
	SEND,
	RECV
};

struct stOverlappedEx
{
	WSAOVERLAPPED m_wsaOverlapped;
	SOCKET m_socketClient;
	WSABUF m_wsaBuf;				// pointer to the buffer
	IOOperation m_eOperation;
};

struct stClientInfo
{
	INT32 mIndex = 0;

	SOCKET m_socketClient;
	stOverlappedEx m_stRecvOverlappedEx;
	stOverlappedEx m_stSendOverlappedEx;

	char mRecvBuf[MAX_SOCKBUF];	// buffer for recv()
	char mSendBuf[MAX_SOCKBUF];	// buffer for send()

	stClientInfo()
	{
		ZeroMemory(&m_stRecvOverlappedEx, sizeof(stOverlappedEx));
		ZeroMemory(&m_stSendOverlappedEx, sizeof(stOverlappedEx));
		m_socketClient = INVALID_SOCKET;
	}
};