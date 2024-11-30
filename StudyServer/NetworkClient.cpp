#include "pch.h"
#include "NetworkClient.h"
#include "NetworkContext.h"

NetworkClient::NetworkClient()
{
	mReceiveContext = std::make_unique<NetworkContext>();
}

bool NetworkClient::Init()
{
	mLastCloseTimeInSeconds = UINT32_MAX;
	mIsConnected = true;
	return true;
}

bool NetworkClient::Send(NetworkContext& context)
{
	if (mSendContext != nullptr)
	{
		printf("Already Sending\n");
		return false;
	}

	mSendContext = context.shared_from_this();

	if (mSendContext->GetDataSize() == 0)
	{
		printf("Send Data Size is 0\n");
		return false;
	}

	mSendContext->ClearOverlapped();
	mSendContext->mContextType = ContextType::SEND;
	mSendContext->mSessionID = mSessionID;

	DWORD dwSendNumBytes = 0;
	WSABUF wsaBuf = { 0, };

	wsaBuf.len = static_cast<ULONG>(mSendContext->GetDataSize());
	wsaBuf.buf = reinterpret_cast<char*>(mSendContext->GetReadBuffer());

	int nRet = WSASend(mSocket,
		&wsaBuf,
		1,
		&dwSendNumBytes,
		0,
		(LPWSAOVERLAPPED)mSendContext.get(),
		NULL);

	if (nRet == SOCKET_ERROR && (ERROR_IO_PENDING != WSAGetLastError()))
	{
		printf("WSASend Error : %d\n", WSAGetLastError());
		return false;
	}

	return true;
}

void NetworkClient::SendComplete()
{
	mSendContext.reset();
	mSendContext = nullptr;
}

std::unique_ptr<NetworkPacket> NetworkClient::GetPacket()
{
	auto remainSize = mReceiveContext->GetDataSize();

	if (remainSize < sizeof(NetworkPacket::PacketHeader))
	{
		printf("PacketHeader Size Error\n");
		return nullptr;
	}

	auto packetHeader = reinterpret_cast<NetworkPacket::PacketHeader*>(mReceiveContext->GetReadBuffer());
	auto packetLength = packetHeader->BodyLength + sizeof(NetworkPacket::PacketHeader);

	auto packet = std::make_unique<NetworkPacket>();
	packet->Header = *packetHeader;

	if (remainSize < packetLength)
	{
		printf("PacketBody Size Error\n");
		return nullptr;
	}

	memcpy(packet->Body.data(), mReceiveContext->GetReadBuffer() + sizeof(NetworkPacket::PacketHeader), packet->GetBodySize());


	if (false == mReceiveContext->Read(packet->GetPacketSize()))
	{
		printf("Read Error\n");
		return nullptr;
	}

	return packet;
}

bool NetworkClient::Receive()
{
	DWORD dwFlag = 0;
	DWORD dwRecvNumBytes = 0;

	mReceiveContext->mContextType = ContextType::RECV;
	mReceiveContext->ClearOverlapped();

	WSABUF wsaBuf = { 0, };
	wsaBuf.buf = reinterpret_cast<char*>(mReceiveContext->GetWriteBuffer());
	wsaBuf.len = mReceiveContext->GetRemainSize();

	int nRet = WSARecv(mSocket,
		&wsaBuf,
		1,
		&dwRecvNumBytes,
		&dwFlag,
		(LPWSAOVERLAPPED)mReceiveContext.get(),
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

void NetworkClient::Reset()
{
	if (mReceiveContext != nullptr)
	{
		mReceiveContext->Reset();
	}

	if (mSendContext != nullptr)
	{
		mSendContext = nullptr;
	}

	mIsConnected = false;
	mSessionID = 0;
}

