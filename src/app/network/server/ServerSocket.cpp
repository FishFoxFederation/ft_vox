#include "ServerSocket.hpp"

ServerSocket::ServerSocket(int port)
{
	struct addrinfo hints, *servinfo, *p;

	std::memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	int ret;

	if ((ret = getaddrinfo(NULL, std::to_string(port).c_str(), &hints, &servinfo)) != 0)
		throw std::runtime_error(gai_strerror(ret));
	
	for (p = servinfo; p != NULL; p = p->ai_next)
	{
		if ((m_sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
			continue;

		int yes = 1;
		if (setsockopt(m_sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
			throw std::runtime_error("Failed to set socket options");

		if (bind(m_sockfd, p->ai_addr, p->ai_addrlen) == -1)
		{
			this->close();
			continue;
		}

		break;
	}

	freeaddrinfo(servinfo);

	if (p == NULL)
		throw std::runtime_error("Failed to bind to port");

	if (listen(m_sockfd, MAX_BACKLOG) == -1)
		throw std::runtime_error("Failed to listen on socket");
}

ServerSocket::~ServerSocket()
{
}

ServerSocket::ServerSocket(ServerSocket&& other)
: Socket(std::move(other))
{
}

ServerSocket& ServerSocket::operator=(ServerSocket&& other)
{
	Socket::operator=(std::move(other));
	return *this;
}

std::shared_ptr<ConnectionSocket> ServerSocket::accept()
{
	struct sockaddr_storage their_addr;
	socklen_t addr_size = sizeof(their_addr);
	int new_fd = ::accept(m_sockfd, (struct sockaddr *)&their_addr, &addr_size);
	if (new_fd == -1)
		throw std::runtime_error("Failed to accept connection");
		
	return (std::make_shared<ConnectionSocket>(new_fd));
}
