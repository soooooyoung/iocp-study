#include "Server.h"
#include <iostream>

int main()
{
	Server server;
	if (false == server.StartServer())
	{
		std::cout << "Failed to start server" << std::endl;
		return -1;
	}

	server.Run();

	return 0;
}
