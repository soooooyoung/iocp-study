#include "NetworkClient.h"

void NetworkClient::Init(const UINT32 sessionID)
{
}

//bool NetworkClient::OnConnect(HANDLE iocpHandle, SOCKET socket)
//{
//	mSocket = socket;
//
//	HANDLE hIOCP = CreateIoCompletionPort((HANDLE)mSocket
//		, iocpHandle
//		, (ULONG_PTR)this, 0);
//
//	if (hIOCP == INVALID_HANDLE_VALUE)
//	{
//		printf("CreateIoCompletionPort Error : %d\n", GetLastError());
//		return false;
//	}
//
//	return true;
//}
//
//bool NetworkClient::BindReceive(NetworkContext& context)
//{
//	return false;
//}
