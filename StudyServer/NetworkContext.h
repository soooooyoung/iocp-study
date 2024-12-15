#pragma once

struct NetworkPacket;
class NetworkContext : public std::enable_shared_from_this<NetworkContext>, public OVERLAPPED
{
private:
	std::array<std::uint8_t, 8096> mBuffer;
	int mReadPos = 0;
	int mWritePos = 0;
public:
	ContextType mContextType = ContextType::NONE;
	SOCKET mSocket = INVALID_SOCKET;
	uint32_t mSessionID = 0;
public:
	NetworkContext();
	virtual ~NetworkContext() {}

	/* Reset Context for Next I/O Operation */
	void ClearOverlapped();

	/* Buffer Management */
	int GetRemainSize() { return mBuffer.size() - mWritePos; }
	int GetDataSize() { return mWritePos - mReadPos; }
	std::uint8_t* GetWriteBuffer() { AlignBuffer(); return mBuffer.data() + mWritePos; }
	std::uint8_t* GetReadBuffer() { return mBuffer.data() + mReadPos; }

	void ResetBuffer();
	void AlignBuffer();

	bool Write(void* data, std::size_t size);
	bool Write(int size);

	bool Read(std::span<std::uint8_t> data);
	bool Read(int size);

	void Reset();
};