#pragma once 

#include <thread>
#include <mutex>
#include <condition_variable>
#include "define.hpp"
#include "logger.hpp"

class Status
{
public:
	Status();
	~Status();
	Status(const Status & other);
	Status & operator=(const Status & other);
	Status(Status && other);
	Status & operator=(Status && other);

	/**
	 * @brief adds a reader to the status
	 * @warning WILL BLOCK until it can add a reader
	 */
	void lock_shared();

	/**
	 * @brief Tries to add a reader to the status, will NOT block
	 * 
	 * @return true if it added a reader \\
	 * @return false if it did not add a reader
	 */
	bool try_lock_shared();

	/**
	 * @brief removes a reader from the status
	 */
	void unlock_shared();

	/**
	 * @brief adds a writer to the status
	 * @warning WILL BLOCK until it can add a writer
	 */
	void lock();

	/**
	 * @brief Tries to add a writer to the status, will NOT block
	 * 
	 * @return true if it added a writer \\
	 * @return false if it did not add a writer
	 */
	bool try_lock();

	/**
	 * @brief removes a writer from the status
	 */
	void unlock();

	bool isLocked() const;
	bool isShareLocked() const;
	bool isLockable() const;
	bool isShareLockable() const;

private:
	int m_writer;
	int m_readers;
	mutable std::mutex m_mutex ;
	std::condition_variable m_cv;	
};
