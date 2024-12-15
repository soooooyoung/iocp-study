#include "pch.h"
#include "NetworkClient.h"
#include "NetworkContext.h"


NetworkClient::NetworkClient()
{
	mReceiveContext = std::make_unique<NetworkContext>();
	mSendContext = std::make_unique<NetworkContext>();
}

bool NetworkClient::Init()
{
	Reset();
	mLastCloseTimeInSeconds = UINT32_MAX;
	mIsConnected = true;
	return true;
}

bool NetworkClient::Send()
{
	if (nullptr == mSendContext)
	{
		return false;
	}

	if (mSendContext->GetDataSize() == 0)
	{
		if (mSendQueue.empty())
		{
			return true;
		}
		
		MemoryPool<Packet>::UniquePtr packet;

		mSendQueue.front().swap(packet);

		if (nullptr == packet)
		{
			printf_s("NetworkClient Send() fail: Packet is nullptr\n");
			return false;
		}

		if (false == mSendContext->Write(packet->Body.data(), packet->Body.size()))
		{
			printf_s("NetworkClient Send() fail: Write Packet Error\n");
			return false;
		}
	}

	printf_s("Sending Packet Data %d\n", mSendContext->GetDataSize());

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
		printf("NetworkClient Send() fail: WSASend Error: %d\n", WSAGetLastError());
		return false;
	}

	return true;
}

MemoryPool<Packet>::UniquePtr NetworkClient::GetPacket(std::shared_ptr<MemoryPool<Packet>> packetPool)
{
	auto remainSize = mReceiveContext->GetDataSize();

	if (remainSize < sizeof(NetworkPacket::PacketHeader))
	{
		printf("PacketHeader Size Error\n");
		return nullptr;
	}

	auto packetHeader = reinterpret_cast<NetworkPacket::PacketHeader*>(mReceiveContext->GetReadBuffer());
	auto packetLength = packetHeader->BodyLength + sizeof(NetworkPacket::PacketHeader);

	NetworkPacket rawPacket;
	rawPacket.Header = *packetHeader;

	if (remainSize < packetLength)
	{
		printf("PacketBody Size Error\n");
		return nullptr;
	}

	auto packet = packetPool->Acquire();
	memcpy(packet->Body.data(), mReceiveContext->GetReadBuffer() + sizeof(NetworkPacket::PacketHeader), rawPacket.GetBodySize());

	packet->PacketID = rawPacket.Header.PacketID;
	packet->BodyLength = rawPacket.Header.BodyLength;

	if (false == mReceiveContext->Read(rawPacket.GetPacketSize()))
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
		mSendContext->Reset();
	}

	mIsConnected = false;
	mSessionID = 0;
	mSending.store(false);
}

void NetworkClient::EnqueuePacket(std::unique_ptr<Packet> packet)
{
	printf_s("EnqueuePacket\n");
	mSendQueue.push(std::move(packet));
}

