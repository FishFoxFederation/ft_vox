#pragma once

#include "Socket.hpp"

class ConnectionSocket : public Socket
{
public:
	ConnectionSocket(int sockfd);
	virtual ~ConnectionSocket();

	ConnectionSocket(const ConnectionSocket& other) = delete;
	ConnectionSocket& operator=(const ConnectionSocket& other) = delete;

	ConnectionSocket(ConnectionSocket&& other);
	ConnectionSocket& operator=(ConnectionSocket&& other);

	ssize_t send(const char *data, size_t size);
	ssize_t recv(char *data, size_t size);
protected:
	ConnectionSocket();
private:
};
