#include "application.hpp"
#include "logger.hpp"
#include <signal.h>

#include <iostream>

//mandatory use of a global for the signal handler
std::atomic<bool> app_running = true;

Application::Application():
	m_start_time(std::chrono::steady_clock::now().time_since_epoch()),
	m_settings(),
	m_world_scene(),
	m_window("Vox", 800, 600),
	m_vulkan_api(m_window.getGLFWwindow()),
	m_thread_pool(),
	m_world(m_world_scene, m_vulkan_api, m_thread_pool),
	m_render_thread(m_settings, m_vulkan_api, m_world_scene, m_start_time, eptr),
	m_update_thread(m_settings, m_window, m_world_scene, m_world, m_start_time, eptr)
{
	LOG_INFO("Application::Application()");
}

Application::~Application()
{
}

void Application::run()
{
	LOG_INFO("Application::run()");
	signal(SIGINT, [](int signum) {
		(void)signum;
		app_running = false;
		});

	while (app_running
		&& m_thread_pool.running()
		&& m_update_thread.running()
		&& m_render_thread.running())
	{
		if (m_window.shouldClose())
		{
			app_running = false;
			break;
		}
		glfwWaitEventsTimeout(0.2);
	}
	LOG_INFO("Application stopped running");
	//after application has stopped there are two possibilities
	//either it is a normal user-requested shutdown, in that case we can just call the destructors
	//of the classes and any running tasks will be completed before shutdown

	//or it is because an error occured, error are represented by exception throughout the project
	//if an error happenned, it is needed to stop any running tasks and stop as soon as possible.
	if (eptr) 
	{
		//an error happenned manual shutdown and clearing is mandatory
		m_update_thread.stop();
		m_render_thread.stop();
		m_thread_pool.stop();
		m_world.clearTasks();

		std::rethrow_exception(eptr);
	}
}
