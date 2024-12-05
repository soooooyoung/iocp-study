#pragma once
#include "NetworkClient.h"



class NetworkContext;
class ListenClient : public NetworkClient
{
public:
	ListenClient();
	virtual ~ListenClient() {}

	/* Initialize */
	virtual bool Init() override;

	/* Listen */
	bool Listen(const int port);

	/* Accept */
	bool PostAccept();
	bool Accept(SOCKET& clientSocket);
private:

};