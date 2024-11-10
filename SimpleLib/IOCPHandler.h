#pragma once
#include <vector>
#include <WinSock2.h>
#include <thread>


class NetworkClient;
class IOCPHandler 
{
public:
	IOCPHandler(void) {}
	virtual ~IOCPHandler(void) { WSACleanup(); }

	bool Init();

	bool Register(NetworkClient* client);

private:
	void WorkerThread();

private:
	bool mIsRunning = false;

	HANDLE mIOCPHandle = INVALID_HANDLE_VALUE;
	std::vector<std::thread> mIOThreadList;
};