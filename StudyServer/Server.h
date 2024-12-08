#pragma once

class NetworkManager;
class Server
{
public:
	Server();
	~Server();

	bool Initialize();
	void Shutdown();

	void Run();
private:
	std::unique_ptr<NetworkManager> mNetworkManager;
};
