#include "Executor.hpp"
#include <iostream>

namespace task
{

Executor::Executor(unsigned const thread_count)
:m_joiner(m_threads)
{
	try
	{
		for(unsigned i = 0; i < thread_count; i++)
		{
			m_threads.push_back(std::thread(&Executor::workerThread, this, i));
		}
	}
	catch(...)
	{
		m_done = true;
		m_cond.notify_all();
		throw;
	}
}

Executor::~Executor()
{
	waitForAll();

	m_done = true;
	m_cond.notify_all();
}

std::future<void> Executor::run(TaskGraph & graph)
{
	std::shared_ptr<runningGraph> running_graph = std::make_shared<runningGraph>(graph);
	{
		std::lock_guard<std::mutex> lock(m_running_graphs_mutex);
		m_running_graphs.insert(running_graph);
	}
	{
		std::lock_guard<std::mutex> lock(m_queue_mutex);
		for (auto node : running_graph->runningNodes)
		{
			m_work_queue.push_back(info(node, running_graph));
			m_cond.notify_one();
		}
	}

	return running_graph->promise.get_future();
};

void Executor::waitForAll()
{
	std::unique_lock<std::mutex> lock(m_running_graphs_mutex);
	m_running_graphs_cond.wait(lock, [&] {return m_running_graphs.empty() || m_done; });
}

Executor::runningGraph::runningGraph(TaskGraph & graph)
:graph(graph)
{
	done = false;
	for (auto & node : graph.m_nodes)
	{
		dependencies[&node] = node.m_dependents.size();
		if (node.m_dependents.empty())
			runningNodes.insert(&node);
		else
			waitingNodes.insert(&node);
	}
	//check for cycles in the graph
	if (checkCycles())
	{
		done = true;
		runningNodes.clear();
		waitingNodes.clear();
		throw CycleError();
	}
}

bool Executor::runningGraph::checkCycles() const
{
	//if there is no root nodes but we have some nodes in the graph we have a cycle
	if (runningNodes.empty() && !waitingNodes.empty())
		return true;

	std::unordered_set<Node *> visited;
	std::function<bool(Node *)> visit = [&](Node * node) -> bool
	{
		if (visited.find(node) != visited.end())
			return true;
		visited.insert(node);
		for (auto child : node->m_sucessors)
		{
			if (visit(child))
				return true;
		}
		visited.erase(node);
		return false;
	};

	for (auto node : runningNodes)
	{
		if (visit(node))
			return true;
	}
	return false;
}

}
