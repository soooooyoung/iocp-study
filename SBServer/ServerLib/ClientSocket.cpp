#include "pch.h"
#include "ClientSocket.h"
#include "NetworkContext.h"
#include "spdlog/spdlog.h"

namespace NetworkLib
{
	ClientSocket::~ClientSocket()
	{
		Close();
	}

	bool ClientSocket::OnConnect()
	{
		if (mSocket == INVALID_SOCKET)
		{
			return false;
		}

		if (false == SetSocketNoDelay(mSocket))
		{
			return false;
		}

		if (false == SetSocketNonBlocking(mSocket))
		{
			return false;
		}

		mIsConnected = true;

		mReceiveCount = 0;
		mReceiveTimer.Reset();

		return true;
	}

	bool ClientSocket::OnReceive()
	{
		if (mReceiveTimer.Update())
		{
			return true;
		}

		mReceiveCount++;

		if (mReceiveCount > MAX_PACKETS_PER_CLIENT)
		{
			spdlog::error("ClientSocket Receive OverFlow");
			return false;
		}

		return true;
	}

	bool ClientSocket::Send(NetworkContext& context)
	{
		Buffer* buffer = context.mBuffer;

		DWORD sendBytes = 0;
		DWORD flags = 0;
		WSABUF wsaBuf{};

		wsaBuf.buf = (char*)buffer->GetReadBuffer();
		wsaBuf.len = buffer->GetDataSize();

		int result = WSASend(mSocket, &wsaBuf, 1, &sendBytes, flags, (LPWSAOVERLAPPED)&context, nullptr);

		if (result == SOCKET_ERROR)
		{
			int error = WSAGetLastError();
			if (error != WSA_IO_PENDING)
			{
				spdlog::error("WSASend Error:{}", error);
				return false;
			}
		}

		return true;
	}

	bool ClientSocket::Receive(NetworkContext& context)
	{
		Buffer* buffer = context.mBuffer;

		DWORD recvBytes = 0;
		DWORD flags = 0;
		WSABUF wsaBuf{};

		wsaBuf.buf = (char*)buffer->GetWriteBuffer();
		wsaBuf.len = buffer->GetRemainSize();

		int result = WSARecv(mSocket, &wsaBuf, 1, &recvBytes, &flags, (LPWSAOVERLAPPED)&context, nullptr);

		if (result == SOCKET_ERROR)
		{
			int error = WSAGetLastError();

			if (error != WSA_IO_PENDING)
			{
				spdlog::error("WSARecv Error:{}", error);
				return false;
			}
		}

		return true;
	}

	void ClientSocket::Close()
	{
		if (mSocket != INVALID_SOCKET)
		{
			struct linger stLinger = { 0,0 }; 

			shutdown(mSocket, SD_BOTH);
			setsockopt(mSocket, SOL_SOCKET, SO_LINGER, (char*)&stLinger, sizeof(stLinger));
			closesocket(mSocket);
		}

		mSocket = INVALID_SOCKET;
	}

}