#pragma once

#include "define.hpp"

#include <thread>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>

class JoinThreads
{
public:
	explicit JoinThreads(std::vector<std::thread> & threads)
	: m_threads(threads) {}


	~JoinThreads()
	{
		for (size_t i = 0; i < m_threads.size(); i++)
		{
			if (m_threads[i].joinable())
			{
				m_threads[i].join();
			}
		}
	}
private:
	std::vector<std::thread> & m_threads;
};



class ThreadPool
{
public:
	ThreadPool();
	~ThreadPool();

	template<typename FunctionType>
	void submit(FunctionType f)
	{
		{
			std::unique_lock<std::mutex> lock(m_queue_mutex);
			m_work_queue.push(std::function<void()>(f));
			m_cond.notify_one();
		}
	}

private:
	std::atomic_bool					m_done;
	std::queue<std::function<void()> >	m_work_queue;
	std::mutex							m_queue_mutex;
	std::condition_variable				m_cond;
	std::vector<std::thread>			m_threads;

	void worker_thread();
};
