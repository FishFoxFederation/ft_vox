#pragma once

#include "logger.hpp"
#include "ThreadPool.hpp"
#include <functional>
#include <unordered_set>

class ThreadPoolAccessor
{
public:
	ThreadPoolAccessor();
	~ThreadPoolAccessor();

	ThreadPoolAccessor(const ThreadPoolAccessor &) = delete;
	ThreadPoolAccessor & operator=(const ThreadPoolAccessor &) = delete;
	ThreadPoolAccessor(ThreadPoolAccessor &&) = delete;
	ThreadPoolAccessor & operator=(ThreadPoolAccessor &&) = delete;

	/**
	 * @brief Submit a task to the threadpool, the task will be executed asynchronously
	 * 
	 * @tparam F return type of the function
	 * @param f function to execute
	 * @return id of the task, you can use this id to wait for the task to finish
	 */
	template<typename F>
	uint64_t submit(F && f)
	{
		std::lock_guard<std::mutex> lock(m_futures_mutex);

		typedef typename std::invoke_result<F>::type result_type;
		//create a packaged task with the function
		std::shared_ptr<std::packaged_task<result_type()>> task_ptr = std::make_shared<std::packaged_task<result_type()>>(std::move(f));

		//encapsulate inside another void function with error handling + id tracking
		uint64_t id = m_counter++;
		std::function<void()> wrapper_func = [task_ptr = std::move(task_ptr), this, id] () mutable
		{
			std::exception_ptr eptr = nullptr;

			(*task_ptr)();

			try
			{
				task_ptr->get_future().get();
			}
			catch (...)
			{
				eptr = std::current_exception();
			}

			{
				std::lock_guard<std::mutex> lock(m_finished_tasks_mutex);
				m_finished_tasks.insert(id);
			}

			if (eptr != nullptr)
				std::rethrow_exception(eptr);
		};

		//submit the task to the thread pool
		std::future<void> future_v = m_thread_pool.submit(wrapper_func);
		m_futures.insert(std::make_pair(id, std::move(future_v)));

		return id;
	}

	/**
	 * @brief wait for all tasks to finish
	 */
	void waitForAll();

	/**
	 * @brief wait for all finished task
	 */
	void waitForFinishedTasks();

	/**
	 * @brief wait for a specific task to finish
	 * 
	 * @param id id of the task to wait for
	 */
	void waitForTask(uint64_t id);

	/**
	 * @brief wait for a list of tasks to finish
	 * 
	 * @param ids std::vector of ids of the tasks to wait for
	 */
	void waitForTasks(const std::vector<uint64_t> & ids);
private:
	ThreadPool &										m_thread_pool;
	uint64_t											m_counter = 0;

	std::unordered_set<uint64_t>						m_finished_tasks;
	std::mutex											m_finished_tasks_mutex;
	std::unordered_map<uint64_t, std::future<void>>		m_futures;
	std::mutex 											m_futures_mutex;

	/**
	 * @brief waits a task
	 * @warning you must lock m_mutex BEFORE calling this function
	 * 
	 * @param id 
	 */
	void waitTask(uint64_t id);
};
