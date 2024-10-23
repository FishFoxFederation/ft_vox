#pragma once

#include "define.hpp"

#include <thread>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <future>
#include "Tracy.hpp"
#include "tracy_globals.hpp"
#include "JoinThreads.hpp"

class ThreadPool
{
public:
	~ThreadPool();

	static ThreadPool & get_instance()
	{
		static ThreadPool instance;
		return instance;
	}

	std::future<void> submit(std::function<void()> f)
	{
		std::unique_lock<std::mutex> lock(m_queue_mutex);
		std::packaged_task<void()> task(std::move(f));
		std::future<void> res(task.get_future());
		m_work_queue.push(std::move(task));
		m_cond.notify_one();
		return res;
	}

private:
	ThreadPool();
	std::atomic_bool					m_done;
	std::queue<std::packaged_task<void()> >	m_work_queue;
	std::mutex							m_queue_mutex;
	std::condition_variable				m_cond;
	std::vector<std::thread>			m_threads;
	JoinThreads							m_joiner;

	void worker_thread(const int & id);
};

