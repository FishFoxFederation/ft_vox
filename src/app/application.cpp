#include "application.hpp"
#include "logger.hpp"

#include <iostream>

Application::Application():
	m_world_scene(),
	m_window("Vox", 800, 600),
	m_renderAPI(m_window.getGLFWwindow()),
	m_render_thread(m_renderAPI, m_world_scene)
{
	LOG_INFO("Application::Application()");

	m_world_scene.camera().setPosition(glm::vec3(20.0f, 20.0f, 20.0f));
	m_world_scene.camera().lookAt(glm::vec3(0.0f, 0.0f, 0.0f));

	int size = 10;
	for (int x = -size; x < size; x++)
	{
		for (int z = -size; z < size; z++)
		{
			// 2d circular sine wave
			int y = 3 * sin(0.4 * sqrt(x * x + z * z));
			m_world_scene.addMeshData(1, WorldScene::Transform(glm::vec3(x, y, z)));
		}
	}
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
