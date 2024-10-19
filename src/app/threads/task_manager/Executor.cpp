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
	runningGraph *running_graph = new runningGraph(graph);
	{
		std::lock_guard<std::mutex> lock(m_running_graphs_mutex);
		m_running_graphs += 1;
	}
	{
		std::lock_guard<std::mutex> lock(m_queue_mutex);
		std::lock_guard<std::mutex> lock2(running_graph->mutex);
		for (auto node : running_graph->runningNodes)
		{
			m_work_queue.emplace_back(std::move(running_graph->nodeInfos.at(node)));
			m_cond.notify_one();
			// m_work_queue.push_back(info(node, running_graph));
		}
	}

	return running_graph->promise.get_future();
};

void Executor::waitForAll()
{
	std::unique_lock<std::mutex> lock(m_running_graphs_mutex);
	m_running_graphs_cond.wait(lock, [&] {return m_running_graphs == 0 || m_done; });
}

Executor::runningGraph::runningGraph(TaskGraph & graph)
:graph(graph)
{
	done = false;

	std::function<void(TaskGraph &, bool, Module *)> visitModule = [&](TaskGraph & graph, bool root, Module * module)
	{
		if (checkCycles(graph))
			throw CycleError();
		for (auto & node : graph.m_nodes)
		{
			nodeDependencies[&node] = node.m_dependents.size();
			bool local_root = root && node.m_dependents.empty();
			switch(node.getType())
			{
				case TaskNode::type::GRAPH:
				{
					auto & sub_graph = *node.getGraph();
					if (sub_graph.m_nodes.empty())
						throw EmptyModuleError(node.getName());
					// + 1 for nodes to run becaue the root node is not in the module
					Module * mod = &modules.emplace_back(node.getSucessors(), sub_graph.m_nodes.size(), module);
					visitModule(sub_graph, local_root, mod);
					nodeInfos.insert({&node, info(&node, this, mod, module)});
					if (local_root)
						runningNodes.insert(&node);
					else if (module != nullptr
						&&	node.m_dependents.empty())
						module->rootNodes.push_back(&node);
					else
						waitingNodes.insert(&node);
					break;
				}
				case TaskNode::type::NODE:
				{
					nodeInfos.insert({&node, info(&node, this, module)});

					if (local_root)
						runningNodes.insert(&node);
					else if (module != nullptr
						&&	node.m_dependents.empty())
						module->rootNodes.push_back(&node);
					else
						waitingNodes.insert(&node);
					break;
				}
				default:
					break;
			}
		}
	};
	if (graph.m_nodes.empty())
		throw EmptyGraphError();

	try {
		visitModule(graph, true, nullptr);
	} catch (CycleError & e) {
		done = true;
		runningNodes.clear();
		waitingNodes.clear();
		throw e;
	}
}

bool Executor::runningGraph::checkCycles(const TaskGraph & graph) const
{
	std::vector<const TaskNode *> rootNodes;

	for (auto & node : graph.m_nodes)
	{
		if (node.m_dependents.empty())
			rootNodes.push_back(&node);
	}

	//if there is no root nodes but we have some nodes in the graph we have a cycle
	if (rootNodes.empty() && !graph.m_nodes.empty())
		return true;

	std::unordered_set<const TaskNode *> visited;
	std::function<bool(const TaskNode *)> visit = [&](const TaskNode * node) -> bool
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

	for (auto node : rootNodes)
	{
		if (visit(node))
			return true;
	}
	return false;
}

}
