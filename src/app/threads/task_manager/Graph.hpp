#pragma once

#include "tasks.hpp"

namespace task
{
class Graph
{
public:
	Graph();
	~Graph();
	Graph(const Graph &) = delete;
	Graph & operator=(const Graph &) = delete;
	Graph(Graph &&) = default;
	Graph & operator=(Graph &&) = default;

private:
};

}
