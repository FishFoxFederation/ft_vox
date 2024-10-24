#include "tasks.hpp"
#include <iostream>

// compile with 
// c++ main_test.cpp Executor.cpp ExecutorWorker.cpp -std=c++20 -pthread -I . -I ../pool/ -I ./internal -g3
using namespace task;

struct test
{
	test()
	: graph(TaskGraph::create()), sub_graph(TaskGraph::create()), sub_graph2(TaskGraph::create())
	{
		Task task = graph->emplace(sub_graph);
		Task task2 = graph->emplace(sub_graph2);

		task2.precede(task);

		sub_graph2->emplace([]() { std::cout << "2" << std::endl; });
		sub_graph2->emplace([]() { std::cout << "2" << std::endl; });
		sub_graph2->emplace([]() { std::cout << "2" << std::endl; });
		sub_graph2->emplace([]() { std::cout << "2" << std::endl; });
		sub_graph2->emplace([]() { std::cout << "2" << std::endl; });

		sub_graph->emplace([]() { std::cout << "1" << std::endl; });
		sub_graph->emplace([]() { std::cout << "1" << std::endl; });
		sub_graph->emplace([]() { std::cout << "1" << std::endl; });
		sub_graph->emplace([]() { std::cout << "1" << std::endl; });
		sub_graph->emplace([]() { std::cout << "1" << std::endl; });
	};
	std::shared_ptr<TaskGraph> graph;
	std::shared_ptr<TaskGraph> sub_graph;
	std::shared_ptr<TaskGraph> sub_graph2;
};


std::shared_ptr<TaskGraph> getGraph()
{
	test t;

	return t.graph;
}

int main()
{
	task::Executor executor(4);
	std::shared_ptr<task::TaskGraph> graph = TaskGraph::create();
	std::shared_ptr<task::TaskGraph> sub_graph = TaskGraph::create();
	std::shared_ptr<task::TaskGraph> sub_graph2 = TaskGraph::create();
	std::shared_ptr<task::TaskGraph> sub_sub_graph = TaskGraph::create();

	task::Task A = graph->emplace([]() { std::cout << "A" << std::endl; });
	task::Task B = graph->emplace([]() { std::cout << "B" << std::endl; });
	task::Task C = graph->emplace([]() { std::cout << "C" << std::endl; }); 
	task::Task D = graph->emplace([]() { std::cout << "D" << std::endl; });



	task::Task submodule = graph->emplace(sub_graph);
	task::Task submodule2 		= sub_graph->emplace(sub_graph2);
	task::Task sub_sub_module   = sub_graph2->emplace(sub_sub_graph);


	task::Task sub_task		= sub_graph->emplace([]() { std::cout << "1" << std::endl; }).Name("1");
	task::Task sub_task2	= sub_graph2->emplace([]() { std::cout << "2" << std::endl; }).Name("2");
	task::Task sub_sub_task	= sub_sub_graph->emplace([]() { std::cout << "3" << std::endl; }).Name("3");

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

	// sub sub graph
	sub_sub_module.precede(sub_task2);

	executor.run(graph);
	executor.waitForAll();



	std::shared_ptr<task::TaskGraph> graph2 = getGraph();
	executor.run(graph2);
	executor.waitForAll();
};
