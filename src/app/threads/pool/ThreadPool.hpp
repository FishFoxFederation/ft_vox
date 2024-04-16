#pragma once

#include "define.hpp"

#include <thread>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <future>

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
	std::future<typename std::invoke_result<FunctionType>::type> submit(FunctionType f)
	{
		typedef typename std::invoke_result<FunctionType>::type result_type;
		{
			std::unique_lock<std::mutex> lock(m_queue_mutex);
			std::packaged_task<result_type()> task(std::move(f));
			std::future<result_type> res(task.get_future());
			m_work_queue.push(std::move(task));
			m_cond.notify_one();
			return res;
		}
	}

private:
	std::atomic_bool					m_done;
	std::queue<std::packaged_task<void()> >	m_work_queue;
	std::mutex							m_queue_mutex;
	std::condition_variable				m_cond;
	std::vector<std::thread>			m_threads;
	JoinThreads							m_joiner;

	void worker_thread();
};
