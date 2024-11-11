#pragma once
#include "NetworkContext.h"
#include "NetworkClient.h"


class NetworkClient;
class ListenClient : public NetworkClient
{
public:
	ListenClient() {};
	virtual ~ListenClient() {};

	virtual bool Init() override;
	virtual void Reset() override;

	bool BindAndListen(int port);
	bool PostAccept();

	NetworkContext GetContext() { return mContext; };
private:
	NetworkContext mContext;
};
