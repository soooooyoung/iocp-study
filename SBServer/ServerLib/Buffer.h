#pragma once
#include <vector>

namespace NetworkLib {

	const int MAX_BUFFER_SIZE = 8 * 1024;
	const int DEFAULT_BUFFER_SIZE = 1024;

	class Buffer
	{
	public:
		Buffer() : mBuffer(DEFAULT_BUFFER_SIZE)
		{
		}

		virtual ~Buffer()
		{
			Clear();
		}

		void Clear()
		{
			mReadPos = 0;
			mWritePos = 0;
			mBuffer.clear();
			mBuffer.resize(DEFAULT_BUFFER_SIZE);
		}

		void AlignBuffer()
		{
			if (mReadPos == 0)
				return;

			if (mReadPos < mWritePos)
			{
				std::memmove(mBuffer.data(), mBuffer.data() + mReadPos, GetDataSize());
				mWritePos -= mReadPos;
				mReadPos = 0;
			}
			else if (mReadPos == mWritePos)
			{
				mWritePos = 0;
				mReadPos = 0;
			}
		}

		uint8_t* GetWriteBuffer()
		{
			return mBuffer.data() + mWritePos;
		}

		uint8_t* GetReadBuffer()
		{
			return mBuffer.data() + mReadPos;
		}

		int GetRemainSize()
		{
			return mBuffer.size() - mWritePos;
		}

		int GetDataSize()
		{
			return mWritePos - mReadPos;
		}

		bool Write(const char* data, int size)
		{
			if (GetRemainSize() < size)
			{

				if (MAX_BUFFER_SIZE < mBuffer.size() + size)
				{
					return false;
				}
			}

			std::memcpy(mBuffer.data() + mWritePos, data, size);
			mWritePos += size;

			return true;
		}

		bool Write(int size)
		{
			mWritePos += size;
			return true;
		}

		bool Read(int size)
		{
			mReadPos += size;
			return true;
		}

	private:
		std::vector<uint8_t> mBuffer;

		int mReadPos = 0;
		int mWritePos = 0;
	};

}