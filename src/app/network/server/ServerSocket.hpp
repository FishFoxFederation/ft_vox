#pragma once

#include "Socket.hpp"
#include <cstring>
#include <memory>
#include <netdb.h>
#include <netinet/tcp.h>

#include "ConnectionSocket.hpp"

class ServerSocket : public Socket
{
public:
	static constexpr int MAX_BACKLOG = 10;
	ServerSocket(int port);
	virtual ~ServerSocket();

	ServerSocket(const ServerSocket& other) = delete;
	ServerSocket& operator=(const ServerSocket& other) = delete;

	ServerSocket(ServerSocket&& other);
	ServerSocket& operator=(ServerSocket&& other);

	std::shared_ptr<ConnectionSocket> accept();
private:

};
