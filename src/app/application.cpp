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

	m_world_scene.camera().setPosition(glm::vec3(-2.0f, 2.0f, 2.0f));
	m_world_scene.camera().moveDirection(-150.0f, 150.0f);

	m_world_scene.addMeshData(1, glm::mat4(1.0f));
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
