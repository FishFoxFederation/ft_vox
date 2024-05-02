#include "ClientSocket.hpp"

ClientSocket::ClientSocket(const char *address, int port)
{
	struct addrinfo hints, *res, *p;
	int status;

	std::memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	//get a list of potential addresses for the server
	if (status = getaddrinfo(address, std::to_string(port).c_str(), &hints, &res) != 0)
		throw std::runtime_error(gai_strerror(status));

	//loop through all the results and connect to the first we can
	for (p = res; p != NULL; p = p->ai_next)
	{
		if ((m_sockfd = socket(p->ai_family, p->ai_socktype | SOCK_NONBLOCK , p->ai_protocol)) == -1)
			continue;

		if (connect(m_sockfd, p->ai_addr, p->ai_addrlen) == -1)
		{
			this->close();
			continue;
		}

		break;
	}

	freeaddrinfo(res);
	if (p == NULL)
		throw std::runtime_error("Failed to connect to server");

	//disable Nagle's algorithm
	int flag = 1;
	if (setsockopt(m_sockfd, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(int) == -1))
		throw std::runtime_error("Failed to disable Nagle's algorithm");
}

ClientSocket::~ClientSocket()
{
}

ClientSocket::ClientSocket(ClientSocket&& other)
: Socket(std::move(other))
{
}

ClientSocket& ClientSocket::operator=(ClientSocket&& other)
{
	Socket::operator=(std::move(other));
	return *this;
}
