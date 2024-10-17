#include "Executor.hpp"


namespace task
{

void Executor::workerThread(const int & id)
{
	while (!m_done)
	{
		// wait for a new node
		info taskInfo;
		{
			std::unique_lock<std::mutex> lock(m_queue_mutex);
			m_cond.wait(lock, [&] {return !m_work_queue.empty() || m_done; });
			if (m_done) break;
			taskInfo = std::move(m_work_queue.front());
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
}
