#include "Poller.hpp"

Poller::Poller()
{
	m_epolld_fd = epoll_create1(0);
	if (m_epolld_fd == -1)
	{
		throw std::runtime_error("Failed to create epoll instance");
	}
	std::memset(m_events, 0, sizeof(m_events));
}

Poller::~Poller()
{
	close();
}

Poller::Poller(Poller&& other)
{
	m_epolld_fd = other.m_epolld_fd;
	other.m_epolld_fd = -1;
	std::memcpy(m_events, other.m_events, sizeof(m_events));
}

Poller& Poller::operator=(Poller&& other)
{
	if (this != &other)
	{
		close();
		m_epolld_fd = other.m_epolld_fd;
		other.m_epolld_fd = -1;
		std::memcpy(m_events, other.m_events, sizeof(m_events));
	}
	return *this;
}

void Poller::close()
{
	if (m_epolld_fd != -1)
	{
		::close(m_epolld_fd);
		m_epolld_fd = -1;
	}
}

void Poller::add(const uint64_t & id, const Socket & socket, const uint32_t & idBis)
{
	epoll_event event;
	event.events = EPOLLIN | EPOLLOUT | EPOLLET;
	event.data.u64 = id;
	event.data.u32 = idBis;
	if (epoll_ctl(m_epolld_fd, EPOLL_CTL_ADD, socket.m_sockfd, &event) == -1)
		throw std::runtime_error("Failed to add socket to epoll instance");
}

void Poller::remove(const Socket & socket)
{
	if (epoll_ctl(m_epolld_fd, EPOLL_CTL_DEL, socket.m_sockfd, nullptr) == -1)
		throw std::runtime_error("Failed to remove socket from epoll instance");
}

std::pair<size_t, epoll_event*> Poller::wait(const int & timeout)
{
	int ret = epoll_wait(m_epolld_fd, m_events, EVENTS_SIZE, timeout);
	if (ret == -1)
		throw std::runtime_error("Failed to wait for events");
	return std::make_pair(ret, m_events);
}
