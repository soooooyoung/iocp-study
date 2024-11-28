#include "pch.h"
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


void NetworkContext::ClearOverlapped()
{
	Internal = 0;
	InternalHigh = 0;
	Offset = 0;
	OffsetHigh = 0;
	hEvent = 0;
}

void NetworkContext::AlignBuffer()
{
	if (mReadPos == 0)
		return;

	// Only align the buffer if the write position exceeds the read position.
	if (mReadPos < mWritePos)
	{
		// Move the remaining data to the front of the buffer.
		std::memmove(mBuffer.data(), mBuffer.data() + mReadPos, GetDataSize());

		// Adjust the read and write positions.
		mWritePos -= mReadPos;
	}
	else
	{
		ResetBuffer();
	}
}

bool NetworkContext::Write(void* data, std::size_t size)
{
	if (size > GetRemainSize())
	{
		AlignBuffer();

		if (size > GetRemainSize())
		{
			return false;
		}
	}

	std::memcpy(mBuffer.data() + mWritePos, data, size);

	mWritePos += size;
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
		return false;
	}

	std::memcpy(data.data(), mBuffer.data() + mReadPos, data.size());
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