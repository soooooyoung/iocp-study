#pragma once
#include <string>
#include <array>
#include <unordered_map>

/* Server Host Information */
struct ServerHost
{
	std::string mHostAddress = "";
	int mHostPort = 0;
	int mHostType = 0;
};

/* Server Configuration */
struct ServerConfig
{
	int mServerPort = 0;
	std::string mServerAddress = "";

	std::unordered_map<std::string, ServerHost> mServerHosts;
};

/* System Configuration */
struct SystemConfig
{
	int mPacketPoolSize = 0;
	int mMaxClientCount = 0;
	int mThreadPerCore = 0;
};

/* Configuration Loader */
class ConfigLoader
{
public:
	ConfigLoader();
	~ConfigLoader();

	bool Load();

	static ConfigLoader& GetInstance()
	{
		static ConfigLoader instance;
		return instance;
	}

	const ServerConfig& GetServerConfig() const
	{
		return mServerConfig;
	}

	const SystemConfig& GetSystemConfig() const
	{
		return mSystemConfig;
	}

private:
	std::string mConfigPath = "config.json";

	ServerConfig mServerConfig;
	SystemConfig mSystemConfig;
};