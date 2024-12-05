#include "pch.h"
#include "ListenClient.h"
#include "NetworkContext.h"

ListenClient::ListenClient()
{

}

bool ListenClient::Init()
{
	mSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, NULL, WSA_FLAG_OVERLAPPED);

	if (mSocket == INVALID_SOCKET)
	{
		return false; 
	}

	return true;
}

bool ListenClient::Listen(const int port)
{
	// Configure
	SOCKADDR_IN serverAddr = {};
	serverAddr.sin_family = AF_INET;
	inet_pton(serverAddr.sin_family, "127.0.0.1", &serverAddr.sin_addr);
	serverAddr.sin_port = htons((unsigned short)port);

	// Bind socket
	if (bind(mSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) != 0)
	{
		closesocket(mSocket);
		printf_s("bind() Error on ListenClient: %d\n", WSAGetLastError());
		return false;
	}

	// Listen
	if (listen(mSocket, SOMAXCONN) == SOCKET_ERROR)
	{
		closesocket(mSocket);
		printf_s("listen() Error on ListenClient: %d\n", WSAGetLastError());
		return false;
	}

	printf_s("ListenClient is listening on port %d\n", port);

	return true;
}

bool ListenClient::PostAccept()
{
	SOCKET socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);

	if (INVALID_SOCKET == socket)
	{
		printf_s("WSASocket() Error on ListenClient: %d\n", WSAGetLastError());
		return false;
	}

	mReceiveContext->ClearOverlapped();
	mReceiveContext->ResetBuffer();
	mReceiveContext->mContextType = ContextType::ACCEPT;
	mReceiveContext->mSocket = socket;

	DWORD dwRecvNumBytes = 0;
	DWORD dwFlag = 0;
	
	if (FALSE == AcceptEx(mSocket, 
		socket, 
		mReceiveContext->GetWriteBuffer(),
		0, 
		sizeof(SOCKADDR_IN) + 16, 
		sizeof(SOCKADDR_IN) + 16, 
		&dwRecvNumBytes, 
		(LPOVERLAPPED)mReceiveContext.get()))
	{
		if (WSAGetLastError() != ERROR_IO_PENDING)
		{
			printf_s("AcceptEx Error : %d\n", WSAGetLastError());
			return false;
		}
	}

	return true;
}

bool ListenClient::Accept(SOCKET& clientSocket)
{
	if (INVALID_SOCKET == clientSocket)
	{
		printf_s("Accept Completion Error on ListenClient: %d\n", WSAGetLastError());
		return false;
	}

	if (SOCKET_ERROR == setsockopt(clientSocket, 
		SOL_SOCKET, 
		SO_UPDATE_ACCEPT_CONTEXT, 
		(char*)&mSocket, 
		sizeof(mSocket)))
	{
		printf_s("setsockopt() Error Associating Client Socket on ListenClient: %d\n", WSAGetLastError());
		return false;
	}

	int flag = 1;
	if (SOCKET_ERROR == setsockopt(clientSocket, IPPROTO_TCP, TCP_NODELAY, (const char*)&flag, sizeof(flag)))
	{
		printf_s("setsockopt() Error TCP_NODELAY on ListenClient: %d\n", WSAGetLastError());
		return false;
	}

	return true;
}