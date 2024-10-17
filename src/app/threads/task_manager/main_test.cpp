#include "tasks.hpp"
#include <iostream>

// compile with 
// c++ main_test.cpp Executor.cpp --std=c++20 -I ./ -I ../pool -I internal/ -pthread -g3 
int main()
{
	task::Executor executor(4);
	task::TaskGraph graph;
	task::TaskGraph graph2;

	task::Task A = graph.emplace([]() { std::cout << "Task 1" << std::endl; });
	task::Task B = graph.emplace([]() { std::cout << "Task 2" << std::endl; });
	task::Task C = graph.emplace([]() { std::cout << "Task 3" << std::endl; }); 
	task::Task D = graph2.emplace([]() { std::cout << "Task 4" << std::endl; });

	A.Name("A");
	B.Name("B");
	C.Name("C");

	A.succceed(B);
	B.succceed(C);
	// C.succceed(A); //this will throw a cycle error


	executor.run(graph);
	executor.waitForAll();
};
