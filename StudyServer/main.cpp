#include "pch.h"
#include "Server.h"
#include "ConfigLoader.h"

int main()
{
	if (false == ConfigLoader::GetInstance().Load())
	{
		return -1;
	}

	Server server;

	if (false == server.Initialize())
	{
		return -1;
	}

	server.Run();

	return 0;
}
