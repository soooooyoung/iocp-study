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
	bool Listen(const int port, const std::string& address);

	/* Accept */
	bool PostAccept();
	bool Accept(SOCKET& clientSocket);
private:
    bool _ResolveAddress(const std::string& host, SOCKADDR_IN& addr) {
        addrinfo hints = {};
        hints.ai_family = AF_INET;  
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;

        addrinfo* infos = nullptr;

        if (getaddrinfo(host.c_str(), nullptr, &hints, &infos) == 0 && infos) {

            // Successfully resolved, copy result
            memcpy(&addr, infos->ai_addr, infos->ai_addrlen);
            freeaddrinfo(infos);
            return true;
        }

        // Fallback for invalid resolution
        inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

        return true;
    }
};