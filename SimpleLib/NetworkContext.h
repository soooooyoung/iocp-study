#pragma once
#include <winsock2.h>
#include <Ws2tcpip.h>
#include <mswsock.h>
#include <vector>
#include "NetworkPacket.h"

enum class ContextType
{
	NONE,
	ACCEPT,
	RECV,
	SEND
};

struct NetworkContext
{
	WSAOVERLAPPED mWsaOverlapped = { 0, };
	ContextType mContextType = ContextType::NONE;
	UINT32 mSessionID = 0;

	std::vector<BYTE> mBuffer;
	UINT32 mReadPos = 0;
	UINT32 mWritePos = 0;
	size_t mBufferSize = 0;

	SOCKADDR* mLocalAddr = nullptr;
	SOCKADDR* mRemoteAddr = nullptr;

	// TODO: Read default buffer size from server config
	NetworkContext()  
	{
		mBuffer.resize(8192);
	}

	UINT32 GetRemainSize() 
	{
		return mBuffer.size() - mWritePos;
	}

	UINT32 GetDataSize() 
	{
		return mWritePos - mReadPos;
	}

	void ResetBuffer()
	{
		mReadPos = 0;
		mWritePos = 0;
	}

	void ResizeBuffer(size_t size)
	{
		mBuffer.resize(size);
	}

	void Clear()
	{
		ResetBuffer();
		ZeroMemory(&mWsaOverlapped, sizeof(WSAOVERLAPPED));
		mContextType = ContextType::NONE;
		mSessionID = 0;
	}

	void AlignBuffer()
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

	BYTE* GetWriteBuffer()
	{
		AlignBuffer();
		return mBuffer.data() + mWritePos;
	}

	BYTE* GetReadBuffer()
	{
		return mBuffer.data() + mReadPos;
	}
};