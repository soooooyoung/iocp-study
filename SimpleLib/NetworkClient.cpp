#include "NetworkClient.h"
#include "NetworkContext.h"


NetworkClient::NetworkClient()
{
}

bool NetworkClient::Init()
{
	Reset();
}

void NetworkClient::Close(bool bIsForce = false)
{
	struct linger stLinger = { 0,0 }; 

	// bIsForce�� true�̸� SO_LINGER, timeout = 0���� �����Ͽ� ���� ����
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