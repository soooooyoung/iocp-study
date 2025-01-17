#pragma once
#include <string>
#include <unordered_map>

#include "../ServerLib/NetworkConfig.h"

class ConfigLoader
{
public:
	ConfigLoader() = default;
	virtual ~ConfigLoader() = default;

	bool LoadConfig(const char* filePath = "config.json");

	const NetworkLib::NetworkConfig& GetNetworkConfig() const { return mNetworkConfig; };

private:
	bool mIsLoaded{ false };
	NetworkLib::NetworkConfig mNetworkConfig{};
};
