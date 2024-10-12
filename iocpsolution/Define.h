#pragma once


#include <winsock2.h>
#include <WS2tcpip.h>

const UINT32 MAX_SOCKBUF = 256; 
const UINT32 MAX_SOCK_SENDBUF = 4096;
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
