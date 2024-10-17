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
	friend class Node;
public:
	Executor(unsigned const thread_count = std::max(4u, std::thread::hardware_concurrency() - 2));
	~Executor();
	Executor(const Executor &) = delete;
	Executor & operator=(const Executor &) = delete;
	Executor(Executor &&) = delete;
	Executor & operator=(Executor &&) = delete;

	std::future<void> run(TaskGraph & graph);

	void waitForAll();
private:
	struct runningGraph
	{
		runningGraph(TaskGraph & graph);
		TaskGraph &	graph;
		std::exception_ptr      eptr = nullptr;
		std::promise<void>		promise;
		std::atomic_bool 		done = false;
		std::mutex				mutex;
		std::unordered_set<Node*> waitingNodes;
		std::unordered_set<Node*> runningNodes;
		std::unordered_map<Node*, std::atomic_int> dependencies;
	private:
		bool checkCycles() const;
	};

	struct info
	{
		//maybe add a union and a type enum to enable other types of tasks to be queued
		std::shared_ptr<runningGraph> graph;
		Node * node;
	};

	typedef uint64_t runningGraphId;

	std::unordered_set<std::shared_ptr<runningGraph>>	m_running_graphs;
	std::mutex							m_running_graphs_mutex;
	std::condition_variable				m_running_graphs_cond;

	std::deque<info> m_work_queue;
	std::mutex					m_queue_mutex;

	std::atomic_bool			m_done;
	std::condition_variable		m_cond;
	std::vector<std::thread>	m_threads;
	JoinThreads					m_joiner;

	void workerThread(const int & id);
	void workerEndGraph(std::shared_ptr<runningGraph> & current_graph, Node * node);
};
}
