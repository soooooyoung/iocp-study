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
	WSABUF m_wsaBuf;
	IOOperation m_eOperation;
};

struct stClientInfo
{
	SOCKET m_socketClient;
	stOverlappedEx m_stRecvOverlappedEx;
	stOverlappedEx m_stSendOverlappedEx;

	char m_RecvBuf[MAX_SOCKBUF];
	char m_SendBuf[MAX_SOCKBUF];

	stClientInfo()
	{
		ZeroMemory(&m_stRecvOverlappedEx, sizeof(stOverlappedEx));
		ZeroMemory(&m_stSendOverlappedEx, sizeof(stOverlappedEx));
		m_socketClient = INVALID_SOCKET;
	}
};