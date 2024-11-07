#include "Connection.hpp"

Connection::Connection(std::shared_ptr<Socket> socket)
: m_socket(socket)
{
}

Connection::~Connection()
{
}

Connection::Connection(Connection&& other)
: m_socket(std::move(other.m_socket)), m_connection_id(other.m_connection_id)
{
	m_read_buffer = std::move(other.m_read_buffer);
	m_write_buffer = std::move(other.m_write_buffer);
}

Connection& Connection::operator=(Connection&& other)
{
	if (this != &other)
	{
		m_socket = std::move(other.m_socket);
		m_connection_id = other.m_connection_id;
		m_read_buffer = std::move(other.m_read_buffer);
		m_write_buffer = std::move(other.m_write_buffer);
	}
	return *this;
}

std::vector<uint8_t> Connection::getReadBuffer() const
{
	return m_read_buffer;
}

const std::vector<uint8_t> & Connection::getReadBufferRef() const
{
	return m_read_buffer;
}

const std::vector<uint8_t> & Connection::getWriteBufferRef() const
{
	return m_write_buffer;
}

void Connection::reduceReadBuffer(size_t size)
{
	ZoneScoped;
	m_read_buffer.erase(m_read_buffer.begin(), m_read_buffer.begin() + size);
}

ssize_t Connection::recv()
{
	ZoneScoped;
	char buffer[1024];
	ssize_t total_size = 0;
	while (1)
	{
		ssize_t size = ::recv(m_socket->getFd(), buffer, sizeof(buffer), MSG_DONTWAIT);
		if (size == -1)
		{
			if (errno != EAGAIN && errno != EWOULDBLOCK)
			{
				throw std::runtime_error("Error while receiving data");
			}
			else 
				break;
		}
		else if (size == 0)
		{
			return 0;
		}
		else
		{
			m_read_buffer.insert(m_read_buffer.end(), buffer, buffer + size);
		}
		total_size += size;
	}
	return total_size;
}

ssize_t Connection::sendQueue()
{
	ZoneScoped;
	if (m_write_buffer.empty())
		return 0;
	// LOG_INFO("Sending data");
	ssize_t size = 0;
	size = ::send(m_socket->getFd(), m_write_buffer.data(), m_write_buffer.size(), MSG_DONTWAIT | MSG_NOSIGNAL);
	if (size == -1)
	{
		if (errno != EAGAIN && errno != EWOULDBLOCK)
		{
			LOG_ERROR("Error while sending data: " + std::string(strerror(errno)));
			throw std::runtime_error("Error while sending data: " + std::string(strerror(errno)));
		}
	}
	else
		m_write_buffer.erase(m_write_buffer.begin(), m_write_buffer.begin() + size);
	return size;
}

void Connection::queueAndSendMessage(const std::vector<uint8_t> & msg)
{
	ZoneScoped;
	m_write_buffer.insert(m_write_buffer.end(), msg.begin(), msg.end());
	sendQueue();
}

void Connection::queueMessage(const std::vector<uint8_t> & msg)
{
	ZoneScoped;
	m_write_buffer.insert(m_write_buffer.end(), msg.begin(), msg.end());
}

const Socket & Connection::getSocket() const
{
	return *m_socket;
}

const uint64_t & Connection::getConnectionId() const
{
	return m_connection_id;
}

void Connection::setConnectionId(const uint64_t & connection_id)
{
	m_connection_id = connection_id;
}

bool Connection::dataToSend() const
{
	return !m_write_buffer.empty();
}

LockableBase (std::mutex) & Connection::WriteLock() const
{
	return m_write_mutex;
}

LockableBase (std::mutex) & Connection::ReadLock() const
{
	return m_read_mutex;
}
