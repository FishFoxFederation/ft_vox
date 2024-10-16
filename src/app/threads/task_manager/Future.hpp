#pragma once

#include "tasks.hpp"
#include <future>

namespace task
{

template <typename T>
class Future : public std::future<T>
{
public:
	Future();
	~Future();
	Future(const Future &) = delete;
	Future & operator=(const Future &) = delete;
	Future(Future &&) = default;
	Future & operator=(Future &&) = default;

	/**
	 * @brief will try to cancel the taskFlow
	 */
	void cancel();
private:
};
}
