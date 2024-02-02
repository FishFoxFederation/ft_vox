#pragma once

#include "define.hpp"
#include "RenderThread.hpp"
#include "window.hpp"

#include <GLFW/glfw3.h>
#define VULKAN_INCLUDE_GLFW
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
};
