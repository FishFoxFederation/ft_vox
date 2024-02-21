#pragma once

#include "define.hpp"
#include "window.hpp"
#include "RenderThread.hpp"
#include "WorldScene.hpp"
#include "UpdateThread.hpp"

#include <cppVulkanAPI.hpp>

class Application
{

public:

	Application();
	~Application();

	void run();

private:

	WorldScene m_world_scene;
	Window m_window;
	vk::RenderAPI m_renderAPI;
	RenderThread m_render_thread;
	UpdateThread m_update_thread;
};
