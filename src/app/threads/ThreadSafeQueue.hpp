#pragma once

#include <queue>
#include <mutex>
#include "Tracy.hpp"


template <typename T>
class ThreadSafeQueue
{
public:
	ThreadSafeQueue() {};
	~ThreadSafeQueue() {};

	ThreadSafeQueue(const ThreadSafeQueue& other) = delete;
	ThreadSafeQueue& operator=(const ThreadSafeQueue& other) = delete;

	ThreadSafeQueue(ThreadSafeQueue&& other)
	{
		std::scoped_lock lock(m_mutex, other.m_mutex);
		m_queue = std::move(other.m_queue);
	}
	ThreadSafeQueue& operator=(ThreadSafeQueue&& other)
	{
		std::scoped_lock lock(m_mutex, other.m_mutex);
		m_queue = std::move(other.m_queue);
		return *this;
	}

	void push(const T& value)
	{
		std::lock_guard lock(m_mutex);
		m_queue.push(value);
	}

	void push(T&& value)
	{
		std::lock_guard lock(m_mutex);
		m_queue.push(std::move(value));
	}

	bool empty() const
	{
		std::lock_guard lock(m_mutex);
		return m_queue.empty();
	}

	size_t size() const
	{
		std::lock_guard lock(m_mutex);
		return m_queue.size();
	}

	T pop()
	{
		std::lock_guard lock(m_mutex);
		if (m_queue.empty())
			throw std::out_of_range("Queue is empty");
		auto value = m_queue.front();
		m_queue.pop();
		return value;
	}
private:
	std::queue<T> m_queue;
	mutable TracyLockableN(std::mutex, m_mutex, "Thread Safe Queue Mutex");
};
