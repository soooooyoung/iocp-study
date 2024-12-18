#pragma once

#include <winsock2.h>
#include <Ws2tcpip.h>
#include <mswsock.h>

const UINT32 MAX_SOCK_RECVBUF = 256;
const UINT32 MAX_SOCK_SENDBUF = 4096;
const UINT64 REUSE_SESSION_WAIT_TIMESEC = 3;

enum class IOOperation
{
	ACCEPT,
	RECV,
	SEND
};

struct stOverlappedEx
{
	WSAOVERLAPPED m_wsaOverlapped; // Overlapped structure
	WSABUF m_wsaBuf; // Data buffer
	IOOperation m_eOperation; // IO operation
	UINT32 SessionIndex = 0;
};