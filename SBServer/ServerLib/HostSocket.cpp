#include "HostSocket.h"
#include "NetworkContext.h"

namespace NetworkLib {

	HostSocket::HostSocket()
	{
	}

	HostSocket::~HostSocket()
	{
	}

	bool HostSocket::CreateSocket()
	{
		mSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, NULL, WSA_FLAG_OVERLAPPED);
		if (mSocket == INVALID_SOCKET)
		{
			printf_s("socket() Error: %d\n", WSAGetLastError());
			return false;
		}

		return true;
	}

	bool HostSocket::Bind(const std::string& address, const uint16_t port)
	{
		// Set Address
		auto serverAddr = _ResolveAddress(address);

		// Set Port
		serverAddr.sin_port = htons(port);

		// Bind socket
		if (bind(mSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) != 0)
		{
			closesocket(mSocket);
			printf_s("bind() Error: %d\n", WSAGetLastError());
			return false;
		}
	}

	SOCKADDR_IN HostSocket::_ResolveAddress(const std::string& host)
	{
		SOCKADDR_IN addr = {};
		addrinfo hints = {};
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;

		addrinfo* infos = nullptr;

		if (getaddrinfo(host.c_str(), nullptr, &hints, &infos) == 0 && infos) {

			// Successfully resolved, copy result
			memcpy(&addr, infos->ai_addr, infos->ai_addrlen);
			freeaddrinfo(infos);
			return addr;
		}

		// Fallback for invalid resolution
		inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

		return addr;
	}

	bool HostSocket::Listen()
	{
		if (listen(mSocket, SOMAXCONN) != 0)
		{
			closesocket(mSocket);
			printf_s("listen() Error: %d\n", WSAGetLastError());
			return false;
		}

		return true;
	}

	SOCKET HostSocket::Accept()
	{ 
		SOCKET clientSocket = accept(mSocket, NULL, NULL);

		if (clientSocket == INVALID_SOCKET)
		{
			printf_s("accept() Error: %d\n", WSAGetLastError());
		}

		return clientSocket;
	}

	bool HostSocket::PostAccept(SOCKET& socket, NetworkContext* context)
	{
		context->Reset();
		context->SetContextType(ContextType::ACCEPT);

		DWORD dwRecvNumBytes = 0;
		DWORD dwFlag = 0;

		if (FALSE == AcceptEx(mSocket,
			socket,
			(LPVOID)context->GetBuffer(),
			0,
			sizeof(SOCKADDR_IN) + 16,
			sizeof(SOCKADDR_IN) + 16,
			&dwRecvNumBytes,
			(LPOVERLAPPED)context))
		{
			if (WSAGetLastError() != ERROR_IO_PENDING)
			{
				printf_s("AcceptEx() Error: %d\n", WSAGetLastError());
				return false;
			}
		}

		return true;
	}
}