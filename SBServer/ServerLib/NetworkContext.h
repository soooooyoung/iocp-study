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

	class NetworkContext : public OVERLAPPED
	{
	public:
		NetworkContext(int index) : Index(index)
		{ mBuffer = new Buffer(); };
		virtual ~NetworkContext() { Reset(); }

		void SetContextType(const ContextType contextType) { mContextType = contextType; }
		ContextType GetContextType() const { return mContextType; }

		void Reset() {
			Internal = 0;
			InternalHigh = 0;
			Offset = 0;
			OffsetHigh = 0;
			hEvent = nullptr;
	
			mBuffer->Clear();
		}

		int Index{ 0 };
		Buffer* mBuffer;
	protected:
		ContextType mContextType = ContextType::NONE;
	};
}