#pragma once

#include "tasks.hpp"
#include "Node.hpp"


namespace task
{
class Task
{
public:
	Task(TaskNode * node)
	{
		m_node = node;
	};
	~Task(){};
	Task(const Task &) = default;
	Task & operator=(const Task &) = default;
	Task(Task &&) = default;
	Task & operator=(Task &&) = default;

	/**
	 * @brief add a precedence link from this task to 
	 * 
	 * @param t 
	 * @return *this
	 */
	Task & precede(Task & t) {
		m_node->addSuccessor(*t.m_node);
		t.m_node->addDependent(*m_node);

		return *this;
	}

	/**
	 * @brief add a successor link from this task to t
	 * 
	 * @param t 
	 * @return *this
	 */
	Task & succceed(Task & t) {
		t.precede(*this);
		return *this;
	}

	/**
	 * @brief Set the underlying task
	 * 
	 * @tparam F 
	 * @param f 
	 */
	template<typename F>
	void set_task(F && f);

	void		Name(const std::string & name) { m_node->setName(name);}
	std::string	getName() const { return m_node->getName(); }

	void *getData() const;
	void setData(void * data);

	void reset();
	void resetTask();

	bool isEmpty() const;
	bool hasTask() const;
private:
	TaskNode * m_node;
};
}
