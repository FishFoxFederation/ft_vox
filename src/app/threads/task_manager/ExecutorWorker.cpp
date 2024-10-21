#include "Executor.hpp"
#include <iostream>


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
		switch(taskInfo.t)
		{
			case info::type::NODE:
				workerExecNode(std::get<info::NodeInfo>(taskInfo.data));
				break;
			case info::type::ASYNC:
				workerExecAsync(std::get<info::AsyncInfo>(taskInfo.data));
				break;
			case info::type::GRAPH:
				workerExecGraphNode(std::get<info::GraphInfo>(taskInfo.data));
				break;
			case info::type::NONE:
				break;
		}
	}
}

void Executor::workerEndGraph(info::NodeInfo & node_info)
{
	auto [current_graph, node, is_module] = node_info;
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
			m_running_graphs -= 1;
			m_running_graphs_cond.notify_all();
		}
	}
}

void Executor::workerExecAsync(info::AsyncInfo & async_info)
{
	async_info.task();
}

void Executor::workerExecGraphNode(info::GraphInfo & node_info)
{
	std::vector<TaskNode *> root_nodes;
	auto [current_graph, node, current_module] = node_info;

	//go into running graph and set all nodes as running
	{
		std::lock_guard<std::mutex> lock(current_graph->mutex);
		if (current_graph->done)
		{
			info::NodeInfo inf = {current_graph, node, current_module};
			workerEndGraph(inf);
			return;
		}
		for (auto current_node : current_module->rootNodes)
		{
			current_graph->waitingNodes.erase(current_node);
			current_graph->runningNodes.insert(current_node);
			root_nodes.push_back(current_node);
		}
	}
	//exec all root nodes of the modules
	{
		std::lock_guard<std::mutex> lock(m_queue_mutex);
		for (auto node : root_nodes)
		{
			m_work_queue.emplace_back(std::move(current_graph->nodeInfos.at(node)));
			m_cond.notify_one();
		}
	}

	// workerEndGraphNode(node_info);
	info::NodeInfo inf = {current_graph, node, current_module};
	{
		std::lock_guard<std::mutex> lock(current_graph->mutex);
		workerEndGraph(inf);
	}
}

void Executor::workerExecNode(info::NodeInfo & node_info)
{
	// execute it
	//test that current graph is still running
	auto [current_graph, node, is_module] = node_info;
	{
		std::lock_guard<std::mutex> lock(current_graph->mutex);
		if (current_graph->done)
		{
			workerEndGraph(node_info);
			return;
		}
	}

	//execute node
	try {
		std::get<TaskNode::NodeData>(node->m_data).m_task();
	} catch (...) {
	//if exception stop graph exec and set promise
		{
			std::lock_guard<std::mutex> lock(current_graph->mutex);
			current_graph->eptr = std::current_exception();
			current_graph->done = true;
			workerEndGraph(node_info);
			return;
		}
	}

	workerEndNode(node_info);

	//if node has children, set dependencies
	
}

void Executor::workerUpdateSuccessors(info::NodeInfo & node_info)
{
	auto [current_graph, node, module] = node_info;

	for (auto child : node->m_sucessors)
	{
		// std::cout << "Node " << child->getName() << " has dependencies" << std::endl;
		int dependencies = current_graph->nodeDependencies[child].fetch_sub(1) - 1;
		// if children has no more dependencies, add to queue
		if (dependencies < 0)
			throw std::runtime_error("Dependency error");
		if (dependencies == 0)
		{
			// std::cout << "Node " << child->getName() << " has no more dependencies" << std::endl;
			{
				std::lock_guard<std::mutex> lock(current_graph->mutex);
				current_graph->waitingNodes.erase(child);
				current_graph->runningNodes.insert(child);
			}
			{
				std::lock_guard<std::mutex> lock(m_queue_mutex);
				m_work_queue.push_back(std::move(current_graph->nodeInfos.at(child)));
				m_cond.notify_one();
			}
		}
	}
}

void Executor::workerEndGraphNode(info::GraphInfo & node_info)
{
	auto [current_graph, node, module] = node_info;
	info::NodeInfo inf = {current_graph, node, module};

	workerEndModule(inf);

	{
		std::lock_guard<std::mutex> lock(current_graph->mutex);
		workerEndGraph(inf);
	}
}

void Executor::workerEndNode(info::NodeInfo & node_info)
{
	auto [current_graph, node, module] = node_info;

	if(module != NULL)
		workerEndModule(node_info);

	if (!node->m_sucessors.empty())
		workerUpdateSuccessors(node_info);
	
	{
		std::lock_guard<std::mutex> lock(current_graph->mutex);
		workerEndGraph(node_info);
	}
}

void Executor::workerEndModule(info::NodeInfo & node_info)
{
	// auto current_graph = node_info.graph;
	auto node = node_info.node;
	auto module = node_info.module;
	// node->m_sucessors.clear();
	// std::cout << "Ending module from node " << node->getName() << std::endl;
	std::function<void(Module *)> endModules = [&](Module * mod)
	{
		int nodes_left = mod->nodesToRun.fetch_sub(1) - 1;
		if (nodes_left < 0)
			throw std::runtime_error("Dependency error");
		if (nodes_left == 0)
		{
			if (mod->externalModule != nullptr)
				endModules(mod->externalModule);
			node->m_sucessors.insert(node->m_sucessors.end(), mod->sucessors.begin(), mod->sucessors.end());
		}
	};
	
	endModules(module);
}

}
