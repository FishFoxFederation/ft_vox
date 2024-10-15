#pragma once

#include "tasks.hpp"
#include <functional>
#include <future>
#include <string>
#include <vector>

namespace task::internal
{

class TaskGraph
{
public:
	typedef uint64_t NodeId;
	class Node
	{
	public:

	private:
		TaskGraph &				m_graph;
		NodeId				m_id;
		void * 					m_data;
		std::string				m_name;
		std::function<void()>	m_task;
		std::future<void>		m_future;
		std::vector<NodeId> 	m_dependents;
		std::vector<NodeId> 	m_sucessors;
	};

private:

};
}
