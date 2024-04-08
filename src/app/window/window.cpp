#include "window.hpp"

#include <stdexcept>

Window::Window(const std::string& title, uint32_t width, uint32_t height):
	m_input(initWindow(title, width, height))
{

}

Window::~Window()
{
	glfwDestroyWindow(m_window);
	glfwTerminate();
}

GLFWwindow* Window::initWindow(const std::string& title, uint32_t width, uint32_t height)
{
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	m_window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);

	if (!m_window)
	{
		throw std::runtime_error("failed to create window.");
	}

	return m_window;
}

GLFWwindow* Window::getGLFWwindow()
{
	return m_window;
}

bool Window::shouldClose()
{
	return glfwWindowShouldClose(m_window);
}
