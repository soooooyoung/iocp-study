#pragma once

#include <WinSock2.h>
#include <WS2tcpip.h>

#include <thread>
#include <memory>

namespace NetworkLib {
	class IOCPHandler;
}

class Server
{
public:
	Server();
	virtual ~Server();

	bool StartServer();
	void Shutdown();

	void Run();
private:
	std::unique_ptr<NetworkLib::IOCPHandler> mIOCPHandler;
};