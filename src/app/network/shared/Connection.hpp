#pragma once

#include <vector>
#include <mutex>
#include "ConnectionSocket.hpp"

class Connection : public ConnectionSocket
{
public:
	Connection(ConnectionSocket && connection);
	~Connection();

	Connection(const Connection& other) = delete;
	Connection& operator=(const Connection& other) = delete;

	Connection(Connection&& other);
	Connection& operator=(Connection&& other);

	template <typename charIterator>
	void queueMessage(charIterator begin, charIterator end)
	{
		std::lock_guard<std::mutex> lock(m_write_buffer_mutex);
		this->m_write_buffer.insert(m_write_buffer.end(), begin, end);
	}

	void queueMessage(const std::string & data){queueMessage(data.begin(), data.end());}

	std::vector<char> getReadBuffer() const;
	void	reduceReadBuffer(size_t size);

	void 	recv();
	void	send();
private:
	std::vector<char>	m_read_buffer;
	mutable	std::mutex	m_read_buffer_mutex;
	std::vector<char>	m_write_buffer;
	std::mutex			m_write_buffer_mutex;
};
