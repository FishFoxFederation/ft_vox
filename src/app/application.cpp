#include "application.hpp"
#include "logger.hpp"

#include <iostream>

Application::Application():
	m_start_time(std::chrono::steady_clock::now().time_since_epoch()),
	m_settings(),
	m_world_scene(),
	m_window("Vox", 800, 600),
	m_vulkan_api(m_window.getGLFWwindow()),
	m_thread_pool(),
	m_world(m_world_scene, m_vulkan_api, m_thread_pool),
	m_render_thread(m_settings, m_vulkan_api, m_world_scene, m_start_time),
	m_update_thread(m_settings, m_window, m_world_scene, m_world, m_start_time),
	m_block_update_thread(m_world_scene, m_world)
{
	LOG_INFO("Application::Application()");

	m_world_scene.entity_mesh_list.add(m_vulkan_api.cube_mesh_id, glm::vec3(0.0f, 220.0f, 0.0f));
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
