#pragma once
#include "NetworkClient.h"
#include <array>

class NetworkClient;
class ListenClient : public NetworkClient
{
public:
	ListenClient() {};
	virtual ~ListenClient() {};

	bool BindAndListen(int port, HANDLE iocpHandle);
	bool Accept(std::weak_ptr<NetworkClient> client);
private:
	std::array<BYTE, 64> mAcceptBuffer = { 0, };
};
