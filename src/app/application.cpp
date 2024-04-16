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
	m_render_thread(m_settings, m_vulkan_api, m_world_scene, m_start_time),
	m_update_thread(m_settings, m_window, m_world_scene, m_start_time),
	m_block_update_thread(m_world_scene, m_vulkan_api, m_thread_pool)
{
	LOG_INFO("Application::Application()");

	m_world_scene.camera().setPosition(glm::dvec3(0.0f, 200.0f, 0.0f));
	// m_world_scene.camera().lookAt(glm::dvec3(0.0f, 0.0f, 0.0f));

	// int size = 10;
	// for (int x = -size; x < size; x++)
	// {
	// 	for (int z = -size; z < size; z++)
	// 	{
	// 		// 2d circular sine wave
	// 		int y = 3 * sin(0.4 * sqrt(x * x + z * z));
	// 		m_world_scene.addMeshData(1, WorldScene::Transform(glm::vec3(x, y, z)));
	// 	}
	// }
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

	m_render_thread.m_thread.request_stop();
	m_update_thread.m_thread.request_stop();
	m_block_update_thread.m_thread.request_stop();

	m_render_thread.m_thread.join();
	m_update_thread.m_thread.join();
	m_block_update_thread.m_thread.join();
}
