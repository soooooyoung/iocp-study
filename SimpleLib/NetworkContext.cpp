#include "NetworkContext.h"
#include "SimpleCore.h"

NetworkContext::NetworkContext()
{
	// TODO: Read default buffer size from server config
	mBuffer.resize(8192);
}

void NetworkContext::ResetBuffer()
{
	mReadPos = 0;
	mWritePos = 0;
}

void NetworkContext::ResizeBuffer(size_t size)
{
	mBuffer.resize(size);
}

void NetworkContext::Clear()
{
	ResetBuffer();
	ZeroMemory(&mWsaOverlapped, sizeof(WSAOVERLAPPED));
	mContextType = ContextType::NONE;
	mSessionID = 0;
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

	// Reset the read position.
	mReadPos = 0;
}