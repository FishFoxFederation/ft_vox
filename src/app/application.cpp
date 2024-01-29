#include "application.hpp"
#include "logger.hpp"

#include <iostream>

Application::Application():
	m_window("Vox", 800, 600)
{
	LOG_INFO("Application::Application()");
}

Application::~Application()
{
	LOG_INFO("Application::~Application()");
}

void Application::run()
{
	LOG_INFO("Application::run()");

	Renderer renderer(m_window.getGLFWwindow());

	while (!m_window.shouldClose())
	{
		glfwWaitEvents();
		renderer.draw();
	}
}