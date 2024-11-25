#include "NetworkContext.h"


NetworkContext::NetworkContext()
{
	mBuffer.fill(0);
}

void NetworkContext::ResetBuffer()
{
	mReadPos = 0;
	mWritePos = 0;

	mBuffer.fill(0);
}

//void NetworkContext::ResizeBuffer(size_t size)
//{
//	mBuffer.resize(size);
//}

void NetworkContext::Clear()
{
	ResetBuffer();
	ZeroMemory(&mWsaOverlapped, sizeof(WSAOVERLAPPED));
	mContextType = ContextType::NONE;
	//mBuffer.clear();
	//mBuffer.shrink_to_fit();
}

void NetworkContext::AlignBuffer()
{
	if (mReadPos == 0)
		return;

	// Only align the buffer if the write position exceeds the read position.

	if (mReadPos < mWritePos)
	{
		// Move the remaining data to the front of the buffer.
		std::move(mBuffer.begin() + mReadPos, mBuffer.begin() + mWritePos, mBuffer.begin());
		// Adjust the read and write positions.
		mWritePos -= mReadPos;
	}
	else
	{
		mWritePos = 0;
	}
}

bool NetworkContext::Write(std::span<std::uint8_t> data)
{
	if (data.size() > GetRemainSize()) {
		AlignBuffer();
		if (data.size() > GetRemainSize()) {
			return false;
		}
	}

	std::copy(data.begin(), data.end(), mBuffer.begin() + mWritePos);
	mWritePos += data.size();
	return true;
}

bool NetworkContext::Write(int size)
{
	if (size > GetRemainSize())
	{
		return false;
	}

	mWritePos += size;
	return true;
}

bool NetworkContext::Read(std::span<std::uint8_t> data)
{
	if (data.size() > GetDataSize())
	{
		printf("DataSize: %d, GetDataSize: %d\n", data.size(), GetDataSize());
		return false;
	}

	std::copy(mBuffer.begin() + mReadPos, mBuffer.begin() + mReadPos + data.size(), data.begin());

	mReadPos += data.size();

	return true;
}

bool NetworkContext::Read(int size)
{
	if (size > GetDataSize())
	{
		return false;
	}

	mReadPos += size;
	return true;
}