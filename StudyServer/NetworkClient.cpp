#include "pch.h"
#include "NetworkClient.h"
#include "NetworkContext.h"
#include "NetworkPacket.h"

NetworkClient::NetworkClient()
{
	mContext = std::make_shared<NetworkContext>();
	mSendBuffer.ResetBuffer();
}

bool NetworkClient::Init()
{
	mLastCloseTimeInSeconds = UINT32_MAX;
	mIsConnected = true;
	return true;
}

bool NetworkClient::Send(NetworkContext& context)
{
	if (mSendBuffer.GetDataSize() > 0)
	{
		return false;
	}

	if (false == mSendBuffer.Write(context.GetReadBuffer(), context.GetDataSize()))
	{
		return false;
	}

	if (mSendBuffer.GetDataSize() == 0)
	{
		return false;
	}

	mSendBuffer.ClearOverlapped();
	mSendBuffer.mContextType = ContextType::SEND;

	DWORD dwSendNumBytes = 0;
	WSABUF wsaBuf = { 0, };

	wsaBuf.len = static_cast<ULONG>(mSendBuffer.GetDataSize());
	wsaBuf.buf = reinterpret_cast<char*>(mSendBuffer.GetReadBuffer());


	int nRet = WSASend(mSocket,
		&wsaBuf,
		1,
		&dwSendNumBytes,
		0,
		(LPWSAOVERLAPPED)&mSendBuffer,
		NULL);

	if (nRet == SOCKET_ERROR && (ERROR_IO_PENDING != WSAGetLastError()))
	{
		printf("WSASend Error : %d\n", WSAGetLastError());
		return false;
	}

	return true;
}

std::unique_ptr<NetworkPacket> NetworkClient::GetPacket()
{
	auto remainSize = mContext->GetDataSize();

	if (remainSize < sizeof(NetworkPacket::PacketHeader))
	{
		return nullptr;
	}

	auto packetHeader = reinterpret_cast<NetworkPacket::PacketHeader*>(mContext->GetReadBuffer());

	if (remainSize < packetHeader->PacketLength)
	{
		return nullptr;
	}

	auto packet = std::make_unique<NetworkPacket>();
	packet->Header.PacketLength = packetHeader->PacketLength;
	packet->Header.PacketID = packetHeader->PacketID;

	std::memcpy(packet->Body.data(), mContext->GetReadBuffer() + sizeof(NetworkPacket::PacketHeader), packet->GetBodySize());

	mContext->Read(packet->GetPacketSize());

	return packet;
}

bool NetworkClient::Receive()
{
	DWORD dwFlag = 0;
	DWORD dwRecvNumBytes = 0;

	mContext->mContextType = ContextType::RECV;
	mContext->ClearOverlapped();

	WSABUF wsaBuf = { 0, };
	wsaBuf.buf = reinterpret_cast<char*>(mContext->GetWriteBuffer());
	wsaBuf.len = mContext->GetRemainSize();

	int nRet = WSARecv(mSocket,
		&wsaBuf,
		1,
		&dwRecvNumBytes,
		&dwFlag,
		(LPWSAOVERLAPPED)mContext.get(),
		NULL);

	if (nRet == SOCKET_ERROR && (ERROR_IO_PENDING != WSAGetLastError()))
	{
		printf("WSARecv Error : %d\n", WSAGetLastError());
		return false;
	}

	return true;
}

void NetworkClient::Close(bool bIsForce)
{
	struct linger stLinger = { 0,0 }; // SO_DONTLINGER·Î ¼³Á¤

	if (true == bIsForce)
	{
		stLinger.l_onoff = 1;
	}

	shutdown(mSocket, SD_BOTH);

	setsockopt(mSocket, SOL_SOCKET, SO_LINGER, (char*)&stLinger, sizeof(stLinger));

	closesocket(mSocket);
	mSocket = INVALID_SOCKET;
	mLastCloseTimeInSeconds = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

