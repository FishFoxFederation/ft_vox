#pragma once

#include <vector>
#include <mutex>
#include <iostream>
#include <memory>
#include "ConnectionSocket.hpp"
#include "Socket.hpp"

class Connection
{
public:
	Connection(std::shared_ptr<Socket> socket);
	~Connection();

	Connection(const Connection& other) = delete;
	Connection& operator=(const Connection& other) = delete;

	Connection(Connection&& other);
	Connection& operator=(Connection&& other);

	void					queueAndSendMessage(const std::vector<uint8_t> & msg);

	std::vector<uint8_t>	getReadBuffer() const;
	const std::vector<uint8_t> &	getReadBufferRef() const;
	void					reduceReadBuffer(size_t size);
	ssize_t 				recv();
	ssize_t					sendQueue();


	const Socket&		getSocket() const;
	const uint64_t		getConnectionId() const;
	void				setConnectionId(const uint64_t & connection_id);
private:

	std::shared_ptr<Socket>	m_socket;
	std::vector<uint8_t>	m_read_buffer;
	mutable	std::mutex		m_read_buffer_mutex;
	std::vector<uint8_t>	m_write_buffer;
	std::mutex				m_write_buffer_mutex;
	uint64_t				m_connection_id;
};