#pragma once

#include "defines.hpp"

#include <GLFW/glfw3.h>

#include <queue>
#include <mutex>

class Input
{

public:

	/**
	 * @brief Construct a new Input object
	 *
	 * @param glfwWindow The GLFWwindow object
	 */
	Input(GLFWwindow* glfwWindow);

	/**
	 * @brief Destroy the Input object
	 */
	~Input();

	/**
	 * @brief Get the key state
	 *
	 * @param key The key to get the state of
	 * @return int The state of the key
	 */
	int getKeyState(int key);

private:

	std::queue<int> m_key_state[GLFW_KEY_LAST];
	std::mutex m_key_state_mutex;

	/**
	 * @brief The GLFWwindow object
	*/
	GLFWwindow* m_window;

	/**
	 * @brief The key callback function
	 *
	 * @param window The GLFWwindow object
	 * @param key The key pressed
	 * @param scancode The scancode of the key
	 * @param action The action of the key
	 * @param mods The mods of the key
	*/
	static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

};
