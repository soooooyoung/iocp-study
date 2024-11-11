#include "ListenClient.h"
#include "NetworkContext.h"
#include "StaticPool.h"

bool ListenClient::Init()
{
	Reset();

	mSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);

	if (INVALID_SOCKET == mSocket)
	{
		printf("WSASocket() Error on ListenClient: %d\n", WSAGetLastError());
		return false;
	}

	// TODO: move hardcoded variables to server config later
	mContext.ResizeBuffer(64);

	return true;
}

void ListenClient::Reset()
{
	NetworkClient::Reset();
	Clear();
}

bool ListenClient::BindAndListen(int port, HANDLE iocpHandle)
{
	SOCKADDR_IN serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	int nRet = bind(mSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr));

	if (0 != nRet)
	{
		printf("bind() Error on ListenClient: %d\n", WSAGetLastError());
		return false;
	}

	// TODO: move hardcoded variables to server config later
	nRet = listen(mSocket, 5);
	if (0 != nRet)
	{
		printf("listen() Error on ListenClient: %d\n", WSAGetLastError());
		return false;
	}

	return true;
}

void ListenClient::Clear()
{
	mContext.Clear();
}

bool ListenClient::Accept()
{
	SOCKET clientSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);

	if (INVALID_SOCKET == clientSocket)
	{
		printf("WSASocket() Error on ListenClient: %d\n", WSAGetLastError());
		return false;
	}

	mContext.Clear();
	mContext.mContextType = ContextType::ACCEPT;

	DWORD dwRecvNumBytes = 0;
	DWORD dwFlag = 0;

	// AcceptEx: stays in a waiting state until a client attempts to connect 
	if (FALSE == AcceptEx(mSocket, 
		clientSocket,
		mContext.GetWriteBuffer(),
		0, 
		sizeof(SOCKADDR_IN) + 16, 
		sizeof(SOCKADDR_IN) + 16, 
		&dwRecvNumBytes,
		(LPOVERLAPPED)&mContext))
	{
		if (WSAGetLastError() != ERROR_IO_PENDING)
		{
			printf_s("AcceptEx Error : %d\n", WSAGetLastError());
			return false;
		}

		int localAddrLen = 0;
		int remoteAddrLen = 0;

		GetAcceptExSockaddrs(mContext.GetWriteBuffer(), 0,
			sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16,
			(SOCKADDR**)&mContext.mLocalAddr, &localAddrLen,
			(SOCKADDR**)&mContext.mRemoteAddr, &remoteAddrLen);
	}

	return true;
}



