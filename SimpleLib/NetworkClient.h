#pragma once
#include <winsock2.h>
#include <mutex>
#include <queue>

struct NetworkContext;
class NetworkClient
{
public:
	NetworkClient();
	virtual ~NetworkClient();

	void Init(const UINT32 sessionID);


private:
	UINT32 mSessionID = 0;
	SOCKET mSocket = INVALID_SOCKET;

	std::mutex mSendLock;
	std::queue<NetworkContext*> mSendQueue;
};
