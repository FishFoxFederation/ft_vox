#pragma once

#include "task_declarations.hpp"
#include <exception>
#include "Executor.hpp"
#include "Task.hpp"
#include "TaskGraph.hpp"

namespace task
{


class BaseError : public std::exception
{
public:
	BaseError(const std::string & msg) : m_msg(msg) {}
	virtual const char * what() const noexcept override { return m_msg.c_str(); }
private:
	std::string m_msg;
};

class TaskNotFromSameGraphError : public BaseError
{
public:
	TaskNotFromSameGraphError() : BaseError("You tried to link tasks that are not from the same graph") {}
};

class CycleError : public BaseError
{
public:
	CycleError() : BaseError("You tried to run a graph with a cycle in it") {}
};
} // namespace task
