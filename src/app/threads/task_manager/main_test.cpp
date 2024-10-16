#include "tasks.hpp"
#include <iostream>


int main()
{
	task::Executor executor(4);
	task::internal::TaskGraph graph;

	task::Task A = graph.addNode([]() { std::cout << "Task 1" << std::endl; });
	task::Task B = graph.addNode([]() { std::cout << "Task 2" << std::endl; });
	task::Task C = graph.addNode([]() { std::cout << "Task 3" << std::endl; throw std::runtime_error("Task 3 failed"); });

	// A.precede(B);
	// B.precede(C);
	A.succceed(B);
	B.succceed(C);

	executor.run(graph).get();
};
