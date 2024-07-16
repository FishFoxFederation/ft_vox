#pragma once

#include <vector>
#include <mutex>
#include <iostream>
#include <memory>
#include "ConnectionSocket.hpp"
#include "Socket.hpp"
#include "logger.hpp"
#include "Tracy.hpp"

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

	std::vector<uint8_t>			getReadBuffer() const;
	const std::vector<uint8_t> &	getReadBufferRef() const;
	const std::vector<uint8_t> &	getWriteBufferRef() const;
	// std::mutex & 			getReadBufferMutex();
	void					lock();
	void					unlock();
	void					reduceReadBuffer(size_t size);
	bool					dataToSend() const;
	ssize_t 				recv();
	ssize_t					sendQueue();


	const Socket &		getSocket() const;
	const uint64_t &	getConnectionId() const;
	void				setConnectionId(const uint64_t & connection_id);
private:

	std::shared_ptr<Socket>	m_socket;
	std::vector<uint8_t>	m_read_buffer;
	mutable TracyLockableN (std::mutex, m_mutex, "Connection Mutex");
	std::vector<uint8_t>	m_write_buffer;
	uint64_t				m_connection_id;
};
