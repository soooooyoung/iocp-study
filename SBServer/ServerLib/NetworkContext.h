#pragma once

#include <string>
#include <cstdint>
#include <memory>
#include <WinSock2.h>
#include <functional>

#include "Buffer.h"

namespace NetworkLib
{
	enum class ContextType
	{
		NONE = 0,
		ACCEPT,
		CONNECT,
		SEND,
		RECEIVE
	};

	class NetworkContext : public OVERLAPPED, public std::enable_shared_from_this<NetworkContext>
	{

	public:
		NetworkContext() { }
		virtual ~NetworkContext() { Reset(); }

		Buffer* GetBuffer() { return &mBuffer; }

		void SetContextType(const ContextType contextType) { mContextType = contextType; }
		ContextType GetContextType() const { return mContextType; }

		void OnComplete(const DWORD transferred, const DWORD error) {
			if (mOnComplete != nullptr) {
				mOnComplete(transferred, error, mBuffer.GetReadBuffer());
			}
		}

		void SetOnComplete(std::function<void(const DWORD transferred, const DWORD error, uint8_t* data)> onComplete) {
			mOnComplete = onComplete;
		}

		void Reset() {
			mBuffer.Clear();

			Internal = 0;
			InternalHigh = 0;
			Offset = 0;
			OffsetHigh = 0;
			hEvent = nullptr;
		}

	protected:
	
		Buffer mBuffer;
		ContextType mContextType = ContextType::NONE;

		std::function<void(const DWORD transferred, const DWORD error, uint8_t* data)> mOnComplete;
	};
}