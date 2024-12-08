#include "pch.h"
#include "ConfigLoader.h"
#include "JSON.h"

ConfigLoader::ConfigLoader()
{
}

ConfigLoader::~ConfigLoader()
{
}

bool ConfigLoader::Load()
{
	auto config = JSON::ReadJsonFromFile(mConfigPath);

	if (config.is_null())
	{
		return false;
	}

	auto& system = config["system"];

	if (system.is_null())
	{
		return true;
	}

	mSystemConfig.mThreadPerCore = system.value<int>("thread_per_core", 1);
	mSystemConfig.mMaxClientCount = system.value<int>("max_client_count", 1);

	auto& server = config["server"];

	if (server.is_null())
	{
		return false;
	}

	mServerConfig.mServerAddress = server.value<std::string>("ip", "");
	mServerConfig.mServerPort = server.value<int>("port", 0);

	auto& hosts = config["hosts"];

	if (hosts.is_null() || hosts.empty())
	{
		return true;
	}

	if (hosts.is_array())
	{
		for (auto& hostInfo : hosts)
		{
			auto name = hostInfo.value<std::string>("name", "");

			if (name.empty())
			{
				continue;
			}

			ServerHost host;
			host.mHostAddress = hostInfo.value<std::string>("ip", "");
			host.mHostPort = hostInfo.value<int>("port", 0);
			host.mHostType = hostInfo.value<int>("type", 0);

			mServerConfig.mServerHosts[name] = host;
		}
		return true;
	}

	return true;
}
