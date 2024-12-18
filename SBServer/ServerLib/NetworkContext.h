#pragma once

#include <string>
#include <cstdint>
#include <memory>
#include <WinSock2.h>

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
		NetworkContext();
		virtual ~NetworkContext();

		virtual void OnComplete(const DWORD transferred, const DWORD error) = 0;

		void SetBuffer(char* buffer);
		char* GetBuffer();
		void SetSize(const int32_t size);
		int32_t GetSize() const;

		void SetContextType(const ContextType contextType);
		ContextType GetContextType() const;

		virtual void Reset();
	protected:
		char* mBuffer = nullptr;
		int32_t mSize = 0;

		ContextType mContextType = ContextType::NONE;
	};
}