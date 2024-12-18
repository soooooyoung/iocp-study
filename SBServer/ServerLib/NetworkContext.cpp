#include "NetworkContext.h"


namespace NetworkLib
{
	NetworkContext::NetworkContext()
	{
	}
	NetworkContext::~NetworkContext()
	{
	}
	void NetworkContext::OnComplete(const DWORD transferred, const DWORD error)
	{
	}
	void NetworkContext::SetBuffer(char* buffer)
	{
	}
	char* NetworkContext::GetBuffer() const
	{
		return nullptr;
	}
	void NetworkContext::SetSize(const int32_t size)
	{
	}
	int32_t NetworkContext::GetSize() const
	{
		return 0;
	}
	void NetworkContext::SetContextType(const ContextType contextType)
	{
	}
	ContextType NetworkContext::GetContextType() const
	{
		return ContextType();
	}

	void NetworkContext::Reset()
	{

	}
}