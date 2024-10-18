#include "tasks.hpp"
#include <iostream>

// compile with 
// c++ main_test.cpp Executor.cpp --std=c++20 -I ./ -I ../pool -I internal/ -pthread -g3 
int main()
{
	task::Executor executor(4);
	task::TaskGraph graph;
	task::TaskGraph sub_graph;
	task::TaskGraph sub_graph2;

	task::Task A = graph.emplace([]() { std::cout << "A" << std::endl; });
	task::Task B = graph.emplace([]() { std::cout << "B" << std::endl; });
	task::Task C = graph.emplace([]() { std::cout << "C" << std::endl; }); 
	task::Task D = graph.emplace([]() { std::cout << "D" << std::endl; });


	task::Task submodule = graph.emplace(sub_graph);
	task::Task submodule2 		= sub_graph.emplace(sub_graph2);


	task::Task sub_task		= sub_graph.emplace([]() { std::cout << "1" << std::endl; }).Name("1");
	// task::Task sub_task2	= sub_graph2.emplace([]() { std::cout << "2" << std::endl; }).Name("2");

	A.Name("A");
	B.Name("B");
	C.Name("C");
	D.Name("D");
	submodule.Name("SUB");
	submodule2.Name("SUB2");

	//first graph
	A.precede(B);
	B.precede(C);
	C.precede(submodule);
	submodule.precede(D);

	//sub graph
	submodule2.precede(sub_task);

	executor.run(graph);
	executor.waitForAll();
};
