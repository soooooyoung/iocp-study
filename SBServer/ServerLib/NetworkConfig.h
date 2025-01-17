#pragma once
#include <string>
#include <unordered_map>

namespace NetworkLib
{
	enum HostType
	{
		None = 0,
		Acceptor
	};

	struct ServerHost
	{
		HostType mHostType = HostType::None;

		std::string mHostAddress{};
		int mHostPort{ 0 };
	};

	struct NetworkConfig
	{
		int mMaxSessionCount{ 0 };

		std::string mServerAddress{};
		int mServerPort{ 0 };

		std::unordered_map<std::string, ServerHost> mServerHosts;
	};
}