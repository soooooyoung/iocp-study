#pragma once
#include <memory>
#include <vector>
#include <array>
#include <span>
#include "Define.h"


class NetworkContext : public std::enable_shared_from_this<NetworkContext>
{
private:
	std::array<std::uint8_t, 8096> mBuffer;
	uint32_t mReadPos = 0;
	uint32_t mWritePos = 0;

public:
	WSAOVERLAPPED mWsaOverlapped = { 0, };
	ContextType mContextType = ContextType::NONE;
	SOCKET mSocket = INVALID_SOCKET;

public:
	NetworkContext();
	virtual ~NetworkContext() {}

	/* Reset Context */
	void Clear();

	/* Buffer Management */
	uint32_t GetRemainSize() { return mBuffer.size() - mWritePos; }
	uint32_t GetDataSize() { return mWritePos - mReadPos; }
	std::uint8_t* GetWriteBuffer() { AlignBuffer(); return mBuffer.data() + mWritePos; }
	std::uint8_t* GetReadBuffer() { return mBuffer.data() + mReadPos; }

	void ResetBuffer();
	void AlignBuffer();
	//void ResizeBuffer(size_t size);

	bool Write(std::span<std::uint8_t> data);
	bool Write(int size);

	bool Read(std::span<std::uint8_t> data);
	bool Read(int size);
};