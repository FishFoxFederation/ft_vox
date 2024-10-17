#include "tasks.hpp"
#include <iostream>

// compile with 
// c++ main_test.cpp Executor.cpp --std=c++20 -I ./ -I ../pool -I internal/ -pthread -g3 
int main()
{
	task::Executor executor(4);
	task::TaskGraph graph;
	task::TaskGraph sub_graph;

	task::Task A = graph.emplace([]() { std::cout << "Task 1" << std::endl; });
	task::Task B = graph.emplace([]() { std::cout << "Task 2" << std::endl; });
	task::Task C = graph.emplace([]() { std::cout << "Task 3" << std::endl; }); 

	task::Task D = sub_graph.emplace([]() { std::cout << "SUB GRAPH" << std::endl; });
	task::Task sub = graph.emplace(sub_graph);

	A.Name("A");
	B.Name("B");
	C.Name("C");
	D.Name("D");
	sub.Name("SUB");

	B.succceed(A);
	sub.succceed(A);
	C.succceed(sub);

	executor.run(graph).get();
};
