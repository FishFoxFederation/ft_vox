#pragma once

#include "defines.hpp"
#include "input.hpp"

#include <GLFW/glfw3.h>

#include <string>

class Window
{

public:

	/**
	 * @brief Construct a new Window object
	 * 
	 * @param title The title of the window
	 * @param width The width of the window
	 * @param height The height of the window
	*/
	Window(const std::string& title, uint32_t width, uint32_t height);

	/**
	 * @brief Destroy the Window object 
	*/
	~Window();

	/**
	 * @brief Get the GLFWwindow window pointer
	 * 
	 * @return GLFWwindow* The GLFWwindow window pointer
	*/
	GLFWwindow* getGLFWwindow();

	/**
	 * @brief Check if the window should close
	*/
	bool shouldClose();

private:

	/**
	 * @brief The GLFWwindow window pointer
	*/
	GLFWwindow* m_window;

	/**
	 * @brief The Input object for the window
	*/
	Input m_input;

	/**
	 * @brief Initialize the GLFWwindow window pointer
	 * 
	 * @param title The title of the window
	 * @param width The width of the window
	 * @param height The height of the window
	 * 
	 * @return GLFWwindow* The GLFWwindow window pointer for the input constructor
	*/
	GLFWwindow* initWindow(const std::string& title, uint32_t width, uint32_t height);
};