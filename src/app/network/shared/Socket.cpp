#include "Socket.hpp"

Socket::Socket()
{
}

Socket::~Socket()
{
	close();
}

Socket::Socket(Socket&& other)
{
	close();
	m_sockfd = other.m_sockfd;
	other.m_sockfd = -1;
}

Socket& Socket::operator=(Socket&& other)
{
	close();
	m_sockfd = other.m_sockfd;
	other.m_sockfd = -1;
	return *this;
}

bool Socket::operator==(const Socket& other) const
{
	return m_sockfd == other.m_sockfd;
}

bool Socket::operator==(const int & fd) const
{
	return m_sockfd == fd;
}

void Socket::close()
{
	if (m_sockfd != -1)
	{
		::close(m_sockfd);
		m_sockfd = -1;
	}
}
