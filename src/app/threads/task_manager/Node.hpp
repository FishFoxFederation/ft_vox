#pragma once

#include <algorithm>
#include <vector>
#include <future>
#include <functional>

#include "tasks.hpp"
#include "Executor.hpp"

namespace task
{
	class Executor;
	class TaskGraph;
class TaskNode
{
	friend class TaskGraph;
	friend class task::Executor;
	// friend class task::Executor::runningGraph;
public:
	enum class type
	{
		NONE,
		NODE,
		GRAPH
	};
	//node constructor
	TaskNode(std::shared_ptr<TaskGraph> graph, std::function<void()> task)
	: m_type(type::NODE), m_data(NodeData{task, NULL}){};

	//graph constructor
	TaskNode(std::shared_ptr<TaskGraph> host_graph, std::shared_ptr<TaskGraph> composed_graph)
	: m_type(type::GRAPH), m_data(GraphData{composed_graph}){};

	~TaskNode(){};
	TaskNode(const TaskNode &) = delete;
	TaskNode & operator=(const TaskNode &) = delete;
	TaskNode(TaskNode &&) = delete;
	TaskNode & operator=(TaskNode &&) = delete;

	void addDependent(TaskNode & node){
		m_dependents.push_back(&node);
	};
	void addSuccessor(TaskNode & node){
		m_sucessors.push_back(&node);
	}
	void removeDependent(TaskNode & node){
		m_dependents.erase(std::remove(m_dependents.begin(), m_dependents.end(), &node), m_dependents.end());
	}
	void removeSucessor(TaskNode & node){
		m_sucessors.erase(std::remove(m_sucessors.begin(), m_sucessors.end(), &node), m_sucessors.end());
	}

	void setName(const std::string & name) { m_name = name; }
	std::string getName() const { return m_name; }

	type getType() const { return m_type; }
	std::shared_ptr<TaskGraph> getGraph() const { return std::get<GraphData>(m_data).graph; }

	std::vector<TaskNode *> & getDependents() { return m_dependents; }
	std::vector<TaskNode *> & getSucessors() { return m_sucessors; }
private:

	struct NodeData
	{
		std::function<void()>	m_task;
		void * 					m_data;
	};

	struct GraphData
	{
		std::shared_ptr<TaskGraph> graph;
	};
	std::string							m_name;
	std::vector<TaskNode *> 			m_dependents;
	std::vector<TaskNode *> 			m_sucessors;
	type 								m_type;
	std::variant<NodeData, GraphData>	m_data;
};
}
