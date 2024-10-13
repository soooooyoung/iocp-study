#pragma once


#include <winsock2.h>
#include <WS2tcpip.h>
#include <MSWSock.h>

const UINT32 MAX_SOCK_RECVBUF = 256;
const UINT32 MAX_SOCK_SENDBUF = 4096;
const UINT64 REUSE_SESSION_WAIT_TIME_SEC = 3;

enum class IOOperation
{
	ACCEPT,
	SEND,
	RECV
};

struct stOverlappedEx
{
	WSAOVERLAPPED m_wsaOverlapped;
	WSABUF m_wsaBuf;				// pointer to the buffer
	IOOperation m_eOperation;
	UINT32 mSessionIndex;
};
