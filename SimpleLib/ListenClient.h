#pragma once
#include "NetworkClient.h"
#include <array>

class NetworkClient;
class ListenClient : public NetworkClient
{
public:
	ListenClient() {};
	virtual ~ListenClient() {};

	virtual bool Init() override;
	virtual void Reset() override;

	bool BindAndListen(int port, HANDLE iocpHandle);
	bool Accept();

	void Clear();
private:
	NetworkContext mContext;
};
