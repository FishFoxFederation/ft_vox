#include "application.hpp"
#include "logger.hpp"

#include <iostream>

Application::Application(const int & player_id, const std::string & ip_address, const int & port)
:
	m_start_time(std::chrono::steady_clock::now().time_since_epoch()),
	m_client(ip_address, port),
	m_settings(),
	m_window("Vox", 800, 600),
	m_render_api(m_window.getGLFWwindow()),
	m_event_manager(),
	m_sound_engine(),
	m_world(m_render_api, m_sound_engine, m_event_manager, m_client, player_id),
	m_render_thread(m_settings, m_render_api, m_start_time),
	m_update_thread(m_client, m_settings, m_window, m_world, m_render_api, m_sound_engine, m_event_manager, m_start_time),
	m_block_update_thread(m_world),
	m_network_thread(m_client)
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
