#pragma once

#include "tasks.hpp"
#include "Executor.hpp"
#include <functional>
#include <future>
#include <string>
#include <vector>
#include <map>
#include <list>

namespace task
{
	class Executor;
}

namespace task::internal
{

class Node;
class TaskGraph
{
public:

	friend class Node;
	friend class task::Executor;
	


	TaskGraph(){};
	~TaskGraph(){};
	TaskGraph(const TaskGraph &) = delete;
	TaskGraph & operator=(const TaskGraph &) = delete;
	TaskGraph(TaskGraph &&) = delete;
	TaskGraph & operator=(TaskGraph &&) = delete;

	template <typename F>
	Node * addNode(F && task)
	{
		return &m_nodes.emplace_back(*this, std::forward<F>(task));
	};

private:
	std::list<Node>			m_nodes;
};

class Node
{
	friend class TaskGraph;
	friend class task::Executor;
public:
	Node(TaskGraph & graph, std::function<void()> task)
	:m_graph(graph), m_task(task)
	{};
	~Node(){};
	Node(const Node &) = delete;
	Node & operator=(const Node &) = delete;
	Node(Node &&) = delete;
	Node & operator=(Node &&) = delete;

	void addDependent(Node & node){
		m_dependents.push_back(&node);
	};
	void addSuccessor(Node & node){
		m_sucessors.push_back(&node);
	}
	void removeDependent(Node & node){
		m_dependents.erase(std::remove(m_dependents.begin(), m_dependents.end(), &node), m_dependents.end());
	}
	void removeSucessor(Node & node){
		m_sucessors.erase(std::remove(m_sucessors.begin(), m_sucessors.end(), &node), m_sucessors.end());
	}
private:
	TaskGraph &				m_graph;
	void * 					m_data;
	std::string				m_name;
	std::function<void()>	m_task;
	std::future<void>		m_future;
	std::vector<Node *> 	m_dependents;
	std::vector<Node *> 	m_sucessors;
};
}
