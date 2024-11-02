#include "IOCompletionPort.h"
#include <string>
#include <iostream>

const UINT16 MAX_CLIENTS = 100;
const UINT16 SERVER_PORT = 9000;


int main()
{
	IOCompletionPort iocp;
	iocp.InitSocket();

	iocp.BindAndListen(SERVER_PORT);
	iocp.StartServer(MAX_CLIENTS);

	printf("Server Start\n");
	while (true)
	{
		std::string inputCmd;
		std::getline(std::cin, inputCmd);

		if (inputCmd == "quit")
		{
			break;
		}
	}

	iocp.DestroyThread();
	return 0;
}