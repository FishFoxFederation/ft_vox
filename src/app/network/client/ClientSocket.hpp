#pragma once

#include "Socket.hpp"
#include <cstring>
#include <netdb.h>
#include <netinet/tcp.h>


#include "logger.hpp"

class ClientSocket : public Socket
{
public:
	ClientSocket(const char *address, int port);
	virtual ~ClientSocket();

	ClientSocket(const ClientSocket& other) = delete;
	ClientSocket& operator=(const ClientSocket& other) = delete;

	ClientSocket(ClientSocket&& other);
	ClientSocket& operator=(ClientSocket&& other);

private:
};
