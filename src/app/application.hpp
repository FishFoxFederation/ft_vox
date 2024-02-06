#pragma once

#include "define.hpp"
#include "RenderThread.hpp"
#include "window.hpp"

#include <cppVulkanAPI.hpp>

class Application
{

public:

	Application();
	~Application();

	void run();

private:

	Window m_window;
	vk::RenderAPI m_renderAPI;
	RenderThread m_render_thread;
};
