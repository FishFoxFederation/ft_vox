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
class Node
{
	friend class TaskGraph;
	friend class task::Executor;
public:
	Node(TaskGraph & graph, std::function<void()> task)
	:m_task(task)
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

	void setName(const std::string & name) { m_name = name; }
	std::string getName() const { return m_name; }
private:
	void * 					m_data;
	std::string				m_name;
	std::function<void()>	m_task;
	std::future<void>		m_future;
	std::vector<Node *> 	m_dependents;
	std::vector<Node *> 	m_sucessors;
};
}
