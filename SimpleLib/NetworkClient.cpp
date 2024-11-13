#include "SimpleCore.h"
#include "NetworkClient.h"
#include "NetworkContext.h"


bool NetworkClient::Init()
{
	Reset();

	return true;
}

void NetworkClient::Close(bool bIsForce)
{
	struct linger stLinger = { 0,0 }; 

	// bIsForce가 true이면 SO_LINGER, timeout = 0으로 설정하여 강제 종료
	if (true == bIsForce)
	{
		stLinger.l_onoff = 1;
	}

	shutdown(mSocket, SD_BOTH);
	setsockopt(mSocket, SOL_SOCKET, SO_LINGER, (char*)&stLinger, sizeof(stLinger));
	closesocket(mSocket);

	mLatestClosedTimeSec = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
	mSocket = INVALID_SOCKET;
}

void NetworkClient::OnConnect(SOCKET& socket)
{
	// TCP sockets utilize Nagle's algorithm by default
	// This algorithm is designed to reduce the number of small packets sent over the network
	// This is great for reducing network traffic, but not so great for real-time games
	// To disable Nagle's algorithm, use the TCP_NODELAY option
	int flag = 1;
	setsockopt(mSocket, IPPROTO_TCP, TCP_NODELAY, (const char*)&flag, sizeof(flag));
}

void NetworkClient::PushSend(std::uint8_t* pData, size_t size)
{



}

void NetworkClient::Reset()
{
	mSocket = INVALID_SOCKET;
	mSessionID = 0;
	mSendQueue.Clear();
}