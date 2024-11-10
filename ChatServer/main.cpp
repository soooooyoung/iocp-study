#include "ChatServer.h"
#include <string>
#include <iostream>

const UINT16 SERVER_PORT = 9000;
const UINT16 MAX_CLIENTS = 3;
const UINT32 MAX_IO_WORKER_THREAD = 4;

int main()
{
	ChatServer server;

	server.Init(MAX_IO_WORKER_THREAD);

	server.BindAndListen(SERVER_PORT);
	server.Run(MAX_CLIENTS);

	printf("Press any key to continue\n");
	while (true)
	{
		std::string inputCmd;
		std::getline(std::cin, inputCmd);

		if (inputCmd == "quit")
		{
			break;
		}
	}

	server.End();
	return 0;
}