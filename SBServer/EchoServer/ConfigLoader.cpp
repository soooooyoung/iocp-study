#include "ConfigLoader.h"

#include <nlohmann/json.hpp>
#include <fstream>

using json = nlohmann::json;
using ServerHost = NetworkLib::ServerHost;
using HostType = NetworkLib::HostType;

bool ConfigLoader::LoadConfig(const char* filePath)
{
	mIsLoaded = false;

	std::ifstream file(filePath);
	json data = json::parse(file);

	if (data.is_null())
	{
		return false;
	}

	mNetworkConfig.mMaxSessionCount == data.value<int>("MaxSessionCount", 0);
	mNetworkConfig.mServerAddress = data.value<std::string>("ServerAddress", "");
	mNetworkConfig.mServerPort = data.value<int>("ServerPort", 0);

	mNetworkConfig.mServerHosts.clear();
	mNetworkConfig.mServerHosts.emplace(std::make_pair("Acceptor",
		ServerHost{ HostType::Acceptor,
		mNetworkConfig.mServerAddress,
		mNetworkConfig.mServerPort }));

	auto& hosts = data["Hosts"];

	if (hosts.is_null())
	{
		mIsLoaded = true;
		return true;
	}

	for (auto it = hosts.begin(); it != hosts.end(); ++it)
	{
		auto& host = *it;
		auto hostName = host.value<std::string>("Name", "");

		ServerHost serverHost{};
		serverHost.mHostType = static_cast<HostType>(host.value<int>("HostType", 0));
		serverHost.mHostAddress = host.value<std::string>("HostAddress", "");
		serverHost.mHostPort = host.value<int>("HostPort", 0);

		mNetworkConfig.mServerHosts.emplace(std::make_pair(hostName, serverHost));
	}

	mIsLoaded = true;
	return true;
}
