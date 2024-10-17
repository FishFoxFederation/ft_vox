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
			m_work_queue.push_back({running_graph, node});
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

void Executor::workerThread(const int & id)
{
	while (!m_done)
	{
		Node * node;
		std::shared_ptr<runningGraph> current_graph;
		// wait for a new node
		{
			std::unique_lock<std::mutex> lock(m_queue_mutex);
			m_cond.wait(lock, [&] {return !m_work_queue.empty() || m_done; });
			if (m_done) break;
			info nodeInfo = std::move(m_work_queue.front());
			node = nodeInfo.node;
			current_graph = nodeInfo.graph;
			m_work_queue.pop_front();
		}

		// execute it
		//test that current graph is still running
		{
			std::lock_guard<std::mutex> lock(current_graph->mutex);
			if (current_graph->done)
			{
				workerEndGraph(current_graph, node);
				continue;
			}
		}

		//execute node

		try {
			node->m_task();
		} catch (...) {
		//if exception stop graph exec and set promise
			{
				std::lock_guard<std::mutex> lock(current_graph->mutex);
				current_graph->eptr = std::current_exception();
				current_graph->done = true;
				workerEndGraph(current_graph, node);
				continue;
			}
		}

		{
			//remove from running nodes
			std::lock_guard<std::mutex> lock(current_graph->mutex);
			if (node->m_sucessors.empty())
			{
				workerEndGraph(current_graph, node);
				continue;
			}
			else
				current_graph->runningNodes.erase(node);
		}

		//if node has children, set dependencies
		for (auto child : node->m_sucessors)
		{
			int dependencies = current_graph->dependencies[child].fetch_sub(1) - 1;
			// if children has no more dependencies, add to queue
			if (dependencies == 0)
			{
				{
					std::lock_guard<std::mutex> lock(current_graph->mutex);
					current_graph->waitingNodes.erase(child);
					current_graph->runningNodes.insert(child);
				}
				{
					std::lock_guard<std::mutex> lock(m_queue_mutex);
					m_work_queue.push_back({current_graph, child});
					m_cond.notify_one();
				}
			}
		}
	}
}

void Executor::workerEndGraph(std::shared_ptr<runningGraph> & current_graph, Node * node)
{
	current_graph->runningNodes.erase(node);
	if (current_graph->runningNodes.empty())
	{
		//set promise
		if (current_graph->eptr == nullptr)
			current_graph->promise.set_value();
		else
			current_graph->promise.set_exception(current_graph->eptr);
		
		//remove running graph
		{
			std::lock_guard<std::mutex> lock(m_running_graphs_mutex);
			m_running_graphs.erase(current_graph);
			m_running_graphs_cond.notify_all();
		}
	}
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
