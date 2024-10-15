#pragma once

#include "tasks.hpp"
#include "TaskGraph.hpp"


namespace task
{
class Task
{
public:
	Task();
	~Task();
	Task(const Task &) = delete;
	Task & operator=(const Task &) = delete;
	Task(Task &&) = delete;
	Task & operator=(Task &&) = delete;

	/**
	 * @brief add a precedence link from this task to 
	 * 
	 * @param t 
	 * @return *this
	 */
	Task & precede(Task & t);

	/**
	 * @brief add a successor link from this task to t
	 * 
	 * @param t 
	 * @return *this
	 */
	Task & succceed(Task & t);

	/**
	 * @brief Set the underlying task
	 * 
	 * @tparam F 
	 * @param f 
	 */
	template<typename F>
	void set_task(F && f);

	void		setName(const std::string & name);
	std::string	getName() const;

	void *getData() const;
	void setData(void * data);

	void reset();
	void resetTask();

	bool isEmpty() const;
	bool hasTask() const;
private:
	// internal::TaskGraph						&	m_graph;
	std::shared_ptr<internal::TaskGraph::Node>	m_node;
};
}
