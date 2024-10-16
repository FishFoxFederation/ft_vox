#pragma once

#include <vector>
#include <thread>


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
