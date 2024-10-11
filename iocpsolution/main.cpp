#include "EchoServer.h"
#include <string>
#include <iostream>

const UINT16 SERVER_PORT = 9000;
const UINT16 MAX_CLIENTS = 100;


int main()
{
	EchoServer server;

	server.InitSocket();

	server.BindAndListen(SERVER_PORT);
	server.StartServer(MAX_CLIENTS);

	printf("아무 키나 누를 때까지 대기합니다\n");
	while (true)
	{
		std::string inputCmd;
		std::getline(std::cin, inputCmd);

		if (inputCmd == "quit")
		{
			break;
		}
	}

	server.DestroyThread();
	return 0;
}