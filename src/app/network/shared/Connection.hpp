#pragma once

#include <vector>
#include <mutex>
#include <iostream>
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

	void 	queueAndSendMessage(const std::string & msg);

	std::vector<char> getReadBuffer() const;
	void	reduceReadBuffer(size_t size);

	void 	recv();
	void	sendQueue();
private:
	std::vector<char>	m_read_buffer;
	mutable	std::mutex	m_read_buffer_mutex;
	std::vector<char>	m_write_buffer;
	std::mutex			m_write_buffer_mutex;
};
