#pragma once
#include "NetworkPacket.h"

enum class ContextType
{
	NONE,
	ACCEPT,
	RECV,
	SEND
};

class NetworkContext : public PoolObject<NetworkContext>
{
private:
	std::vector<std::uint8_t> mBuffer;
	UINT32 mReadPos = 0;
	UINT32 mWritePos = 0;

public:
	WSAOVERLAPPED mWsaOverlapped = { 0, };
	ContextType mContextType = ContextType::NONE;
	UINT32 mSessionID = 0;

	SOCKADDR* mLocalAddr = nullptr;
	SOCKADDR* mRemoteAddr = nullptr;
	SOCKET mSocket = INVALID_SOCKET;

public:
	NetworkContext();
	virtual ~NetworkContext() {}

	/* Reset Context */
	void Clear();

	/* Buffer Management */
	UINT32 GetRemainSize() { return mBuffer.size() - mWritePos; }
	UINT32 GetDataSize() { return mWritePos - mReadPos; }
	std::uint8_t* GetWriteBuffer() { AlignBuffer(); return mBuffer.data() + mWritePos; }
	std::uint8_t* GetReadBuffer() { return mBuffer.data() + mReadPos; }

	void ResetBuffer();
	void AlignBuffer();
	void ResizeBuffer(size_t size);
};