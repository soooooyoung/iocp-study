#include <iostream>
#include "../ServerLib/IOCPHandler.h"
#include "ConfigLoader.h"

int main()
{
	ConfigLoader configLoader;
	if (false == configLoader.LoadConfig("config.json"))
	{
		std::cout << "Failed to load config file" << std::endl;
		return -1;
	}

	NetworkLib::IOCPHandler handler;
	if (false == handler.Initialize(configLoader.GetNetworkConfig()))
	{
		std::cout << "Failed to initialize IOCPHandler" << std::endl;
		return -1;
	}

	std::cout << "Server is running..." << std::endl;

	while (true)
	{
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
}
