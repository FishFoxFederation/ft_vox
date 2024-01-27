#include "application.hpp"
#include "logger.hpp"

#include <iostream>

Application::Application()
{
	LOG_INFO("Application::Application()");

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
	LOG_INFO("Application::~Application()");

	glfwDestroyWindow(window);
	glfwTerminate();
}

void Application::run()
{
	LOG_INFO("Application::run()");

	Renderer renderer(window);

	while (!glfwWindowShouldClose(window))
	{
		glfwWaitEvents();
		renderer.draw();
	}
}