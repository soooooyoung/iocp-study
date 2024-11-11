#pragma once
#include <vector>
#include <WinSock2.h>
#include <thread>
#include <memory>

class NetworkClient;
class IOCPHandler 
{
public:
	IOCPHandler(void) {}
	virtual ~IOCPHandler(void) { WSACleanup(); }

	HANDLE GetIOCPHandle() const { return mIOCPHandle; }

	bool Init();
	bool Register(std::shared_ptr<NetworkClient> client);

private:
	void WorkerThread();

	bool mIsRunning = false;
	HANDLE mIOCPHandle = INVALID_HANDLE_VALUE;
	std::vector<std::thread> mIOThreadList;

	void _HandleAccept();
	void _HandleReceive();
	void _HandleSend();
};