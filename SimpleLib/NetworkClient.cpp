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

void NetworkClient::Reset()
{
	mSocket = INVALID_SOCKET;
	mSessionID = 0;
	mSendQueue.Clear();
}