#include "application.hpp"

#include <iostream>

Application::Application()
{
	std::cout << "Application::Application()" << std::endl;

	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	window = glfwCreateWindow(800, 600, "vox", nullptr, nullptr);

	if (!window)
	{
		throw std::runtime_error("failed to create window.");
	}

}

Application::~Application()
{
	std::cout << "Application::~Application()" << std::endl;

	glfwDestroyWindow(window);
	glfwTerminate();
}

void Application::run()
{
	std::cout << "Application::run()" << std::endl;

	Renderer renderer(window);

	while (!glfwWindowShouldClose(window))
	{
		glfwWaitEvents();
		renderer.draw();
	}
}