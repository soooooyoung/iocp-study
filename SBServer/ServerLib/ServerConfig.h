#pragma once
#include <string>
#include <unordered_map>

enum HostType
{
	None = 0,
	Acceptor
};

struct ServerHost
{
	std::string mHostAddress = "";
	int mHostPort = 0;
	HostType mHostType = HostType::None;
};

struct ServerConfig
{
	int mMaxSessionCount = 0;
	
	int mServerPort = 0;
	std::string mServerAddress = "";
	std::unordered_map<std::string, ServerHost> mServerHosts;
};