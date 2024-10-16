#pragma once

#include <vector>
#include <queue>
#include <deque>
#include <thread>
#include <unordered_set>
#include "JoinThreads.hpp"
#include "tasks.hpp"
#include "Future.hpp"
#include "Graph.hpp"
#include "TaskGraph.hpp"

namespace task
{

class Executor
{
	friend class internal::Node;
public:
	Executor(unsigned const thread_count = std::max(4u, std::thread::hardware_concurrency() - 2));
	~Executor();
	Executor(const Executor &) = delete;
	Executor & operator=(const Executor &) = delete;
	Executor(Executor &&) = delete;
	Executor & operator=(Executor &&) = delete;

	std::future<void> run(internal::TaskGraph & graph);
private:
	struct runningGraph
	{
		runningGraph(internal::TaskGraph & graph);
		internal::TaskGraph &	graph;
		std::exception_ptr      eptr = nullptr;
		std::promise<void>		promise;
		std::atomic_bool 		done = false;
		std::mutex				mutex;
		std::unordered_set<internal::Node*> waitingNodes;
		std::unordered_set<internal::Node*> runningNodes;
		std::unordered_map<internal::Node*, std::atomic_int> dependencies;
	};

	struct info
	{
		std::shared_ptr<runningGraph> graph;
		internal::Node * node;
	};

	typedef uint64_t runningGraphId;

	std::unordered_set<std::shared_ptr<runningGraph>>	m_running_graphs;
	std::mutex							m_running_graphs_mutex;

	std::deque<info> m_work_queue;
	std::mutex					m_queue_mutex;

	std::atomic_bool			m_done;
	std::condition_variable		m_cond;
	std::vector<std::thread>	m_threads;
	JoinThreads					m_joiner;

	void workerThread(const int & id);
	void workerEndGraph(std::shared_ptr<runningGraph> & current_graph, internal::Node * node);
};
}
