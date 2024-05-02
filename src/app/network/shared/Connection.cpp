#include "Connection.hpp"

Connection::Connection(ConnectionSocket && connection)
: ConnectionSocket(std::move(connection))
{
}

Connection::~Connection()
{
}

Connection::Connection(Connection&& other)
: ConnectionSocket(std::move(other))
{
}

Connection& Connection::operator=(Connection&& other)
{
	if (this != &other)
	{
		ConnectionSocket::operator=(std::move(other));
	}
	return *this;
}

std::vector<char> Connection::getReadBuffer() const
{
	std::lock_guard<std::mutex> lock(m_read_buffer_mutex);
	return m_read_buffer;
}

void Connection::reduceReadBuffer(size_t size)
{
	std::lock_guard<std::mutex> lock(m_read_buffer_mutex);
	m_read_buffer.erase(m_read_buffer.begin(), m_read_buffer.begin() + size);
}

void Connection::recv()
{
	char buffer[1024];
	ssize_t size = ConnectionSocket::recv(buffer, sizeof(buffer));
	if (size == -1)
	{
		if (errno != EAGAIN && errno != EWOULDBLOCK)
		{
			throw std::runtime_error("Error while receiving data");
		}
	}
	else
	{
		std::lock_guard<std::mutex> lock(m_read_buffer_mutex);
		m_read_buffer.insert(m_read_buffer.end(), buffer, buffer + size);
	}
}

void Connection::send()
{
	std::lock_guard<std::mutex> lock(m_write_buffer_mutex);
	if (m_write_buffer.empty())
		return;
	ssize_t size = ConnectionSocket::send(m_write_buffer.data(), m_write_buffer.size());
	if (size == -1)
	{
		if (errno != EAGAIN && errno != EWOULDBLOCK)
		{
			throw std::runtime_error("Error while sending data");
		}
	}
	else
		m_write_buffer.erase(m_write_buffer.begin(), m_write_buffer.begin() + size);
}
