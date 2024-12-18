#pragma once

#include <vector>
#include <queue>
#include <deque>
#include <thread>
#include <unordered_set>
#include <variant>
#include "JoinThreads.hpp"
#include "tasks.hpp"
#include "Future.hpp"
#include "Graph.hpp"
#include "TaskGraph.hpp"
#include "Tracy.hpp"
#include "tracy_globals.hpp"

namespace task
{

class Executor
{
	friend class TaskNode;
	friend class TaskGraph;
public:
	~Executor();
	
	static Executor & getInstance()
	{
		static Executor instance;
		return instance;
	}

	Executor(const Executor &) = delete;
	Executor & operator=(const Executor &) = delete;
	Executor(Executor &&) = delete;
	Executor & operator=(Executor &&) = delete;

	std::future<void> run(std::shared_ptr<TaskGraph> graph);

	template <typename F>
	std::future<void> run(F && f)
	{
		auto packed_f = [func = std::move(f), this]() mutable {
			m_running_graphs += 1;
			try {
				func();
			}
			catch (const std::exception & e) {
				m_running_graphs -= 1;
				m_running_graphs_cond.notify_all();

				throw;
			}
			m_running_graphs -= 1;
			m_running_graphs_cond.notify_all();
		};
		std::packaged_task<void()> task(std::move(packed_f));
		auto future = task.get_future();
		{
			std::lock_guard<std::mutex> lock(m_queue_mutex);
			m_work_queue.emplace_back(std::move(task));
		}
		m_cond.notify_one();
		return future;
	}

	void waitForAll();
private:
	Executor(unsigned const thread_count = std::thread::hardware_concurrency() - 2);
	struct runningGraph;
	// struct Module;
	struct Module
	{
		Module(std::vector<TaskNode *> & successors, std::size_t nodesToRun, Module * externalModule)
		: sucessors(successors), nodesToRun(nodesToRun), externalModule(externalModule) {};
		std::vector<TaskNode*>	rootNodes;
		std::vector<TaskNode*>  sucessors;
		std::atomic_int 		nodesToRun;
		Module *				externalModule; //some modules can be nested
	};

	struct info
	{
		info () : t(type::NONE) {}
		info (info && other) = default;
		info & operator=(info && other) = default;
		info (TaskNode * node, runningGraph * graph, Module * module)
		: data(NodeInfo{graph, node, module})
		{
			t = type::NODE;
		}
		info (TaskNode * node, runningGraph * graph, Module * internalModule, Module * externalModule)
		: data(GraphInfo{graph, node, internalModule})
		{
			t = type::GRAPH;
		}
		info (std::packaged_task<void()> task)
		{
			t = type::ASYNC;
			data = AsyncInfo{std::move(task)};
		}
		//maybe add a union and a type enum to enable other types of tasks to be queued
		enum class type : uint16_t
		{
			NONE,
			NODE,
			GRAPH,
			ASYNC
		};
		struct GraphInfo
		{
			runningGraph * graph;
			TaskNode * node;
			Module * internalModule;
		};
		struct NodeInfo
		{
			runningGraph * graph;
			TaskNode * node;
			Module * module = nullptr;
		};
		struct AsyncInfo
		{
			std::packaged_task<void()> task;
		};

		type t;
		std::variant<AsyncInfo, NodeInfo, GraphInfo> data;
	};

	struct runningGraph
	{
		
		runningGraph(std::shared_ptr<TaskGraph> graph);
		std::shared_ptr<TaskGraph>	graph;
		std::exception_ptr      eptr = nullptr;
		std::promise<void>		promise;
		std::atomic_bool 		done = false;
		std::mutex				mutex;
		std::atomic_int			graphDependencies;
		std::unordered_set<TaskNode*> waitingNodes;
		std::unordered_set<TaskNode*> runningNodes;
		std::unordered_map<TaskNode*, std::atomic_int> nodeDependencies;
		std::list<Module>			modules;
		// nodeStatus				selfNodes;
		// std::vector<nodeStatus> moduleNodes;

		std::unordered_map<TaskNode *, info> nodeInfos;
	private:
		bool checkCycles(const TaskGraph & graph) const;
	};

	

	typedef uint64_t runningGraphId;

	std::atomic_int 					m_running_graphs;
	std::mutex							m_running_graphs_mutex;
	std::condition_variable				m_running_graphs_cond;

	std::deque<info> m_work_queue;
	std::mutex					m_queue_mutex;

	std::atomic_bool			m_done;
	std::condition_variable		m_cond;
	std::vector<std::thread>	m_threads;
	JoinThreads					m_joiner;

	/*****************************************\
	 * WORKER THREADS
	\*****************************************/
	void workerThread(const int & id);
	void workerEndGraph(info::NodeInfo & node_info);
	void workerExecNode(info::NodeInfo & node_info);
	void workerExecAsync(info::AsyncInfo & async_info);
	void workerExecGraphNode(info::GraphInfo & graph_info);
	void workerEndNode(info::NodeInfo & node_info);
	void workerEndGraphNode(info::GraphInfo & node_info);
	void workerEndModule(info::NodeInfo & node_info);
	void workerUpdateSuccessors(info::NodeInfo & node_info);
};

}
