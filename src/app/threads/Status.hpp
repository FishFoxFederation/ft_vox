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
	void addReader();

	/**
	 * @brief Tries to add a reader to the status, will NOT block
	 * 
	 * @return true if it added a reader \\
	 * @return false if it did not add a reader
	 */
	bool tryAddReader();

	/**
	 * @brief removes a reader from the status
	 */
	void removeReader();

	/**
	 * @brief adds a writer to the status
	 * @warning WILL BLOCK until it can add a writer
	 */
	void addWriter();

	/**
	 * @brief Tries to add a writer to the status, will NOT block
	 * 
	 * @return true if it added a writer \\
	 * @return false if it did not add a writer
	 */
	bool tryAddWriter();

	/**
	 * @brief removes a writer from the status
	 */
	void removeWriter();

	bool hasWriters() const;
	bool hasReaders() const;
	bool isReadable() const;
	bool isWritable() const;

private:
	int m_writers;
	int m_readers;
	mutable std::mutex m_mutex ;
	std::condition_variable m_cv;	
};
