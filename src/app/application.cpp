#include "application.hpp"
#include "logger.hpp"

#include <iostream>

Application::Application():
	m_window("Vox", 800, 600),
	m_renderAPI(m_window.getGLFWwindow()),
	m_render_thread(m_renderAPI)
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

	while (!m_window.shouldClose())
	{
		glfwWaitEvents();
	}
}
