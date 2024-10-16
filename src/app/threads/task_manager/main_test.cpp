#include "tasks.hpp"
#include <iostream>

// compile with 
// c++ main_test.cpp Executor.cpp --std=c++20 -I ./ -I ../pool -I internal/ -pthread -g3 
int main()
{
	task::Executor executor(4);
	task::internal::TaskGraph graph;

	task::Task A = graph.addNode([]() { std::cout << "Task 1" << std::endl; });
	task::Task B = graph.addNode([]() { std::cout << "Task 2" << std::endl; });
	task::Task C = graph.addNode([]() { std::cout << "Task 3" << std::endl; }); 

	A.setName("A");
	B.setName("B");
	C.setName("C");

	A.succceed(B);
	B.succceed(C);

	executor.run(graph);
	executor.waitForAll();
};
