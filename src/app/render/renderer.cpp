#include "renderer.hpp"

#include <iostream>

Renderer::Renderer(GLFWwindow *glfwWindow):
	renderAPI(glfwWindow)
{
}

Renderer::~Renderer()
{
}

void Renderer::draw()
{
	renderAPI.startDraw();
	renderAPI.endDraw();
}