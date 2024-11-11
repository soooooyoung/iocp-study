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
	std::vector<BYTE> mBuffer;
	UINT32 mReadPos = 0;
	UINT32 mWritePos = 0;

public:
	WSAOVERLAPPED mWsaOverlapped = { 0, };
	ContextType mContextType = ContextType::NONE;
	UINT32 mSessionID = 0;
	SOCKADDR* mLocalAddr = nullptr;
	SOCKADDR* mRemoteAddr = nullptr;

public:
	NetworkContext();
	virtual ~NetworkContext() {}

	/* Reset Context */
	void Clear();

	/* Buffer Management */
	UINT32 GetRemainSize() { return mBuffer.size() - mWritePos; }
	UINT32 GetDataSize() { return mWritePos - mReadPos; }
	BYTE* GetWriteBuffer() { AlignBuffer(); return mBuffer.data() + mWritePos; }
	BYTE* GetReadBuffer() { return mBuffer.data() + mReadPos; }

	void ResetBuffer();
	void AlignBuffer();
	void ResizeBuffer(size_t size);
};