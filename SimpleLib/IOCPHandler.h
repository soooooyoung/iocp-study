#pragma once
#include <WinSock2.h>

class NetworkContext;
class NetworkClient;
class IOCPHandler 
{
public:
	IOCPHandler(void) {}
	virtual ~IOCPHandler(void) { WSACleanup(); }

	HANDLE GetIOCPHandle() const { return mIOCPHandle; }

	bool Init();
	bool Register(std::shared_ptr<NetworkClient> client);

	std::function<void(UINT32, std::uint8_t*, size_t)> PushSendPacket;
private:
	void WorkerThread();

	bool mIsRunning = false;
	HANDLE mIOCPHandle = INVALID_HANDLE_VALUE;
	std::vector<std::thread> mIOThreadList;


	void _HandleAccept(NetworkClient* host, NetworkContext& context);
	void _HandleReceive(UINT32 sessionID, NetworkContext& context);
	void _HandleSend(UINT32 sessionID);
};