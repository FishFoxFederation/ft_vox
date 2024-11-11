#pragma once

#include "Socket.hpp"
#include <cstring>
#include <netdb.h>
#include <netinet/tcp.h>


#include "logger.hpp"

/**
 * @brief A class that represents a client socket. Connects to a server.
 * 
 */
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
