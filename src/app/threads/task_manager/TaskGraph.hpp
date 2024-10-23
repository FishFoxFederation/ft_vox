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
#include <memory>

namespace task
{

class TaskNode;
class Executor;
class Task;
class TaskGraph : public std::enable_shared_from_this<TaskGraph>
{
private:
	struct Private { explicit Private() = default;};
public:
	TaskGraph(Private){};

	friend class TaskNode;
	friend class task::Executor;
	// friend class task::Executor::runningGraph;
	
	static std::shared_ptr<TaskGraph> create()
	{
		return std::make_shared<TaskGraph>(Private());
	}
	std::shared_ptr<TaskGraph> getPtr()
	{
		return shared_from_this();
	}
	~TaskGraph(){};
	TaskGraph(const TaskGraph &) = delete;
	TaskGraph & operator=(const TaskGraph &) = delete;
	TaskGraph(TaskGraph &&) = default;
	TaskGraph & operator=(TaskGraph &&) = default;

	template <typename F>
	Task emplace(F && task)
	{
		auto & node = m_nodes.emplace_back(shared_from_this(), std::forward<F>(task));
		return Task(&node);
	}

	Task emplace(std::shared_ptr<TaskGraph> graph)
	{
		auto & node = m_nodes.emplace_back(shared_from_this(), graph);
		return Task(&node);
	}
	
	void clear()
	{
		m_nodes.clear();
	}

	bool empty()
	{
		return m_nodes.empty();
	}

private:
	std::list<TaskNode>			m_nodes;
};

}
