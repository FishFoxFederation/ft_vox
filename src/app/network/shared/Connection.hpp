#pragma once

#include <vector>
#include <mutex>
#include <iostream>
#include <memory>
#include "ConnectionSocket.hpp"
#include "Socket.hpp"
#include "logger.hpp"
#include "Tracy.hpp"

/**
 * @brief a bufferised and thread safe RAII wrapper for a socket representing a connection.
 * 
 * @details There are two internal buffers, one for reading and one for writing
 * 
 * Before doing any operation on the buffers, the appropriate mutex MUST be locked.
 * The mutexes are exposed so multiple operations of the same type can be done in a row.
 * 
 * About the read buffer and the read offset:
 *  to minimize the number of memory allocations the read buffer is not cleared/erased 
 *  after each read operation. Instead the user will call readuceReadBuffer
 *  to indicate the number of bytes read. This will increase the read offset.
 *  When the user calls getReadBufferPtr, it will return a pointer offset by the read offset.
 * The user can call clearReadBuffer once he has finished all his read operations to clear the buffer and reset the read offset.
 * 
 */
class Connection
{
public:
	/**
	 * @brief a constant representing the maximum size of the read buffer.
	 * when reading the connection will stop when this size is reached.
	 */
	constexpr static size_t READ_BUFFER_SIZE_MAX = 1024 * 1024 * 4; //  4 MB
	Connection(std::shared_ptr<Socket> socket);
	~Connection();

	Connection(const Connection& other) = delete;
	Connection& operator=(const Connection& other) = delete;

	Connection(Connection&& other);
	Connection& operator=(Connection&& other);

	/**
	 * @brief add the message to the write buffer and try to send it.
	 * 
	 * @param msg 
	 */
	void					queueAndSendMessage(const std::vector<uint8_t> & msg);

	/**
	 * @brief add the message to the write buffer without sending it.
	 * 
	 * @param msg 
	 */
	void 					queueMessage(const std::vector<uint8_t> & msg);

	/**
	 * @brief Get a pointer to the read buffer, offset by the internal read offset.
	 * 
	 * @return const uint8_t* 
	 */
	const uint8_t *			getReadBufferPtr() const;

	/**
	 * @brief Get the number of bytes that can be read from the read buffer.
	 * 
	 * @note this is not the true size of the internal read buffer,
	 *  because some bytes that have been read can still be in the read buffer
	 *  if it hasnt been cleared
	 * 
	 * @return size_t 
	 */
	size_t 					getReadBufferSize() const;

	/**
	 * @brief Notify the connection that size bytes have been read from the read buffer.
	 * 
	 * this will not cause any reallocation or any data to be moved.
	 * @param size 
	 */
	void 					reduceReadBuffer(size_t size);

	/**
	 * @brief Clear the read buffer and reset the read offset.
	 * 
	 * this can cause some reallocations but will most importantly be pretty slow.
	 * prefer calling reduceReadBuffer
	 * when doing all your operations and call this once you finished.
	 */
	void 					clearReadBuffer();

	/**
	 * @brief Get a ref to the write buffer
	 * 
	 * @return const std::vector<uint8_t>& 
	 */
	const std::vector<uint8_t> &	getWriteBufferRef() const;

	/**
	 * @brief get a ref to the read mutex
	 * 
	 * @return std::mutex
	 */
	LockableBase (std::mutex) & ReadLock() const;

	/**
	 * @brief get a ref to the write mutex
	 * 
	 * @return std::mutex
	 */
	LockableBase (std::mutex) & WriteLock() const;

	/**
	 * @brief Check if there is data in the write buffer
	 * 
	 * @return true 
	 * @return false 
	 */
	bool					dataToSend() const;

	/**
	 * @brief will call recv on the socket on a non blocking way. Filling the read buffer.
	 * 
	 * @return ssize_t 
	 */
	ssize_t 				recv();

	/**
	 * @brief will send as much data as possible from the write buffer.
	 * 
	 * @return ssize_t 
	 */
	ssize_t					sendQueue();


	const Socket &		getSocket() const;
	const uint64_t &	getConnectionId() const;
	void				setConnectionId(const uint64_t & connection_id);
private:

	std::shared_ptr<Socket>	m_socket;
	uint64_t				m_connection_id;
	size_t 					m_read_offset = 0;
	std::vector<uint8_t>	m_read_buffer;
	std::vector<uint8_t>	m_write_buffer;

	mutable TracyLockableN (std::mutex, m_mutex, "Connection Mutex");
	mutable TracyLockableN (std::mutex, m_write_mutex, "Write Mutex");
	mutable TracyLockableN (std::mutex, m_read_mutex, "Read Mutex");
};
