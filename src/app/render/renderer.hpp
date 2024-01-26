#pragma once

#include "define.hpp"

#define VULKAN_INCLUDE_GLFW
#include <cppVulkanAPI.hpp>

class Renderer
{

public:

	/**
	 * @brief Construct a new Renderer object
	*/
	Renderer(GLFWwindow *glfwWindow);

	/**
	 * @brief Destroy the Renderer object
	*/
	~Renderer();

	Renderer(Renderer& renderer) = delete;
	Renderer(Renderer&& renderer) = delete;
	Renderer& operator=(Renderer& renderer) = delete;

	void draw();

private:

	vk::RenderAPI renderAPI;

};