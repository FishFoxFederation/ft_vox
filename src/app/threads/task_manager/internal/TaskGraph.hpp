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

class TaskNode;
class Executor;
class Task;
class TaskGraph
{
public:

	friend class TaskNode;
	friend class task::Executor;
	// friend class task::Executor::runningGraph;
	

	TaskGraph(){};
	~TaskGraph(){};
	TaskGraph(const TaskGraph &) = delete;
	TaskGraph & operator=(const TaskGraph &) = delete;
	TaskGraph(TaskGraph &&) = default;
	TaskGraph & operator=(TaskGraph &&) = default;

	template <typename F>
	Task emplace(F && task)
	{
		auto & node = m_nodes.emplace_back(*this, std::forward<F>(task));
		return Task(&node);
	};

	Task emplace(TaskGraph & graph)
	{
		auto & node = m_nodes.emplace_back(*this, graph);
		return Task(&node);
	};

private:
	std::list<TaskNode>			m_nodes;
};

}
