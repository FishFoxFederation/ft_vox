#pragma once

#include "Socket.hpp"
#include <cstring>
#include <memory>
#include <sys/epoll.h>

#define EVENTS_SIZE 20

class Socket;

class Poller
{
public:
	Poller();
	virtual ~Poller();

	Poller(const Poller& other) = delete;
	Poller& operator=(const Poller& other) = delete;

	Poller(Poller&& other);
	Poller& operator=(Poller&& other);

	void							add(const uint64_t & id, const Socket & socket);
	void							remove(const Socket & socket);
	std::pair<size_t, epoll_event*>	wait(const int & timeout);
private:
	int			m_epolld_fd = -1;
	epoll_event	m_events[EVENTS_SIZE];

	void close();
};
