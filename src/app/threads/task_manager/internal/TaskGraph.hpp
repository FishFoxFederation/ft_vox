#pragma once

#include "tasks.hpp"
#include "Executor.hpp"
#include "Task.hpp"
#include "Node.hpp"
#include <functional>
#include <future>
#include <string>
#include <vector>
#include <map>
#include <list>

namespace task
{

class Node;
class Executor;
class Task;
class TaskGraph
{
public:

	friend class Node;
	friend class task::Executor;
	


	TaskGraph(){};
	~TaskGraph(){};
	TaskGraph(const TaskGraph &) = delete;
	TaskGraph & operator=(const TaskGraph &) = delete;
	TaskGraph(TaskGraph &&) = delete;
	TaskGraph & operator=(TaskGraph &&) = delete;

	template <typename F>
	Task emplace(F && task)
	{
		auto & node = m_nodes.emplace_back(*this, std::forward<F>(task));
		return Task(&node);
	};

private:
	std::list<Node>			m_nodes;
};


}
