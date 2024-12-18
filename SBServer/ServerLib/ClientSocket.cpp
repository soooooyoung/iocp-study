#include "ClientSocket.h"
#include "NetworkContext.h"

namespace NetworkLib
{
	ClientSocket::ClientSocket()
	{
	}
	ClientSocket::~ClientSocket()
	{
	}

	bool ClientSocket::Send(NetworkContext* context)
	{
		if (context == nullptr)
		{
			return false;
		}

		DWORD sendBytes = 0;
		DWORD flags = 0;
		WSABUF wsaBuf = {};

		wsaBuf.buf = context->GetBuffer();
		wsaBuf.len = context->GetSize();

		int result = WSASend(mSocket, &wsaBuf, 1, &sendBytes, flags, (LPWSAOVERLAPPED)context, nullptr);

		if (result == SOCKET_ERROR)
		{
			int error = WSAGetLastError();
			if (error != WSA_IO_PENDING)
			{
				return false;
			}
		}

		return true;
	}

	bool ClientSocket::Receive(NetworkContext* context)
	{
		if (context == nullptr)
		{
			return false;
		}

		DWORD recvBytes = 0;
		DWORD flags = 0;
		WSABUF wsaBuf = {};

		wsaBuf.buf = context->GetBuffer();
		wsaBuf.len = context->GetSize();

		int result = WSARecv(mSocket, &wsaBuf, 1, &recvBytes, &flags, (LPWSAOVERLAPPED)context, nullptr);
		if (result == SOCKET_ERROR)
		{
			int error = WSAGetLastError();
			if (error != WSA_IO_PENDING)
			{
				return false;
			}
		}

		return true;
	}

	void ClientSocket::Close()
	{
		struct linger stLinger = { 0,0 }; 

		shutdown(mSocket, SD_BOTH);
		setsockopt(mSocket, SOL_SOCKET, SO_LINGER, (char*)&stLinger, sizeof(stLinger));
		closesocket(mSocket);

		mSocket = INVALID_SOCKET;
	}

	bool ClientSocket::SetSocketOptions()
	{
		if (false == SetSocketReusable(mSocket))
		{
			return false;
		}

		if (false == SetSocketNonBlocking(mSocket))
		{
			return false;
		}

		if (false == SetSocketNoDelay(mSocket))
		{
			return false;
		}

		return true;
	}

	void ClientSocket::SetSocket(SOCKET& socket)
	{
		mSocket = socket;
	}
}