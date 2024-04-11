#pragma once

#include <chrono>
#include <array>

template <size_t ChronoCount = 1, size_t HistorySize = 1000>
class Timer
{

public:

	Timer()
	{
		start();
	}

	~Timer() = default;

	void start(const int index = 0)
	{
		m_data[index].start_time = std::chrono::steady_clock::now().time_since_epoch();
	}

	void stop(const int index = 0)
	{
		m_data[index].end_time = std::chrono::steady_clock::now().time_since_epoch();
		std::chrono::nanoseconds delta_time = m_data[index].end_time - m_data[index].start_time;

		m_data[index].total_time += delta_time - m_data[index].history[0];
		std::shift_left(m_data[index].history.begin(), m_data[index].history.end(), 1);
		m_data[index].history[HistorySize - 1] = delta_time;
	}

	template <typename T = std::chrono::nanoseconds>
	double elapsed(const int index = 0) const
	{
		return std::chrono::duration_cast<T>(m_data[index].delta_time).count();
	}

	template <typename T = std::chrono::nanoseconds>
	double average(const int index = 0) const
	{
		return std::chrono::duration_cast<T>(m_data[index].total_time).count() / HistorySize;
	}

private:

	struct Data
	{
		std::chrono::nanoseconds start_time;
		std::chrono::nanoseconds end_time;
		std::chrono::nanoseconds delta_time;

		std::chrono::nanoseconds total_time;

		std::array<std::chrono::nanoseconds, HistorySize> history;
	};
	
	std::array<Data, ChronoCount> m_data;

};
