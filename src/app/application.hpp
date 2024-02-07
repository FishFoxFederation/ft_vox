#pragma once

#include "define.hpp"
#include "window.hpp"
#include "RenderThread.hpp"
#include "WorldScene.hpp"

#include <cppVulkanAPI.hpp>

class Application
{

public:

	Application();
	~Application();

	void run();

private:

	WorldScene m_worldScene;
	Window m_window;
	vk::RenderAPI m_renderAPI;
	RenderThread m_render_thread;
};
