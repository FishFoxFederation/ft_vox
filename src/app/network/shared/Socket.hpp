#pragma once 

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <exception>
#include <stdexcept>

#include "Poller.hpp"

class Poller;

class Socket
{
public:
	virtual ~Socket();

	Socket(const Socket& other) = delete;
	Socket& operator=(const Socket& other) = delete;

	Socket(Socket&& other);
	Socket& operator=(Socket&& other);

	bool operator==(const Socket& other) const;
	bool operator==(const int & fd) const;
protected:
	Socket();
	int		m_sockfd = -1;
	void	close();
private:
	friend class Poller;
};
