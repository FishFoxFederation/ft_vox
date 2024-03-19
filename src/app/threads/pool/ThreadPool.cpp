#include "ThreadPool.hpp"

ThreadPool::ThreadPool()
{
	m_done = false;
	unsigned const thread_count = std::thread::hardware_concurrency();
	try
	{
		for (unsigned i = 0; i < thread_count; i++)
		{
			m_threads.push_back(std::thread(&ThreadPool::worker_thread, this));
		}
	}
	catch (...)
	{
		m_done = true;
		m_cond.notify_all();
		throw;
	}
}

ThreadPool::~ThreadPool()
{
	m_done = true;
	m_cond.notify_all();
}

void ThreadPool::worker_thread()
{
	while (!m_done)
	{
		std::function<void()> task;
		{
			std::unique_lock<std::mutex> lock(m_queue_mutex);
			m_cond.wait(lock, [&] {return !m_work_queue.empty() || m_done; });
			if (m_done) break;
			task = std::move(m_work_queue.front());
			m_work_queue.pop();
		}
		task();
	}
}
