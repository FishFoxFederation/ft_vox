#pragma once

#include "Socket.hpp"
#include <cstring>
#include <memory>
#include <netdb.h>
#include <netinet/tcp.h>

#include "ConnectionSocket.hpp"

#define MAX_BACKLOG 10

class ServerSocket : public Socket
{
public:

	ServerSocket(int port);
	virtual ~ServerSocket();

	ServerSocket(const ServerSocket& other) = delete;
	ServerSocket& operator=(const ServerSocket& other) = delete;

	ServerSocket(ServerSocket&& other);
	ServerSocket& operator=(ServerSocket&& other);

	std::shared_ptr<Socket> accept();
private:

};
