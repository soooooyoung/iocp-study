#include "ListenClient.h"
#include "NetworkContext.h"
#include "StaticPool.h"

bool ListenClient::BindAndListen(int port, HANDLE iocpHandle)
{
	SOCKADDR_IN serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	int nRet = bind(mSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr));

	if (0 != nRet)
	{
		printf("bind() Error on ListenClient: %d\n", WSAGetLastError());
		return false;
	}

	// TODO: move hardcoded variables to server config later
	nRet = listen(mSocket, 5);
	if (0 != nRet)
	{
		printf("listen() Error on ListenClient: %d\n", WSAGetLastError());
		return false;
	}

	return true;
}

bool ListenClient::Accept(std::weak_ptr<NetworkClient> weakClient)
{
	auto pClient = weakClient.lock();

	if (nullptr == mContext||
		nullptr == pClient)
	{
		return false;
	}

	pClient->Init();

	mContext->Clear();
	mContext->mContextType = ContextType::ACCEPT;
	mContext->mSessionID = pClient->GetSessionID();

	DWORD dwRecvNumBytes = 0;
	DWORD dwFlag = 0;

	// AcceptEx: stays in a waiting state until a client attempts to connect 
	if (FALSE == AcceptEx(mSocket, 
		pClient->GetSocket(), 
		mAcceptBuffer.data(), 
		0, 
		sizeof(SOCKADDR_IN) + 16, 
		sizeof(SOCKADDR_IN) + 16, 
		&dwRecvNumBytes, 
		(LPOVERLAPPED)mContext.get()))
	{
		if (WSAGetLastError() != ERROR_IO_PENDING)
		{
			printf_s("AcceptEx Error : %d\n", WSAGetLastError());
			return false;
		}
	}

	return true;
}



