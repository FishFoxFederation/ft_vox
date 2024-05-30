#include "Client.hpp"

Client::Client(const std::string& ip, int port) :
	m_running(true),
	m_poller(),
	m_client_socket(std::make_shared<ClientSocket>(ip.c_str(), port)),
	m_connection(m_client_socket)
{
	m_poller.add(0, *m_client_socket);
}

Client::~Client()
{
	m_running = false;
}

void Client::run()
{
	while (m_running)
		runOnce();
}

void Client::runOnce(const int & timeout)
{
	auto [events_size, events] = m_poller.wait(timeout);

	for (size_t i = 0; i < events_size; i++)
	{
		auto current_event = events[i].events;
		if (current_event & EPOLLIN)
			read_data();
		if (current_event & EPOLLOUT)
			send_data();
		if (current_event & EPOLLERR || current_event & EPOLLHUP)
			throw ServerDisconnected();
	}
}

int Client::read_data()
{
	ssize_t size;
	try {
		size = m_connection.recv();
		if (size == 0)
			throw ServerDisconnected();
	}
	catch (const std::runtime_error & e)
	{
		throw ServerDisconnected();
	}
	return 0;
}

int Client::send_data()
{
	ssize_t size;
	try {
		size = m_connection.sendQueue();
		if (size == 0)
			throw ServerDisconnected();
	}
	catch (const std::runtime_error & e)
	{
		throw ServerDisconnected();
	}
	return 0;
}