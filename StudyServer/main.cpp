#include "pch.h"
#include "NetworkManager.h"
#include "SecurePool.h"

int main()
{
	NetworkManager networkManager;
	if (false == networkManager.Initialize())
	{
		return -1;
	}

	while (true)
	{
		Sleep(1000);
	}

	networkManager.Shutdown();

	return 0;
}
