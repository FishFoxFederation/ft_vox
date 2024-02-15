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

	/**
	 * @brief Get the mouse button state
	 *
	 * @param button The button to get the state of
	 * @return int The state of the button
	 */
	int getMouseButtonState(int button);

	/**
	 * @brief Get the cursor position
	 *
	 * @param xpos The x position of the cursor
	 * @param ypos The y position of the cursor
	 */
	void getCursorPos(double& xpos, double& ypos);

private:

	std::queue<int> m_key_state[GLFW_KEY_LAST];
	std::mutex m_key_state_mutex;

	std::queue<int> m_mouse_button_state[GLFW_MOUSE_BUTTON_LAST];
	std::mutex m_mouse_button_state_mutex;

	double m_cursor_x;
	double m_cursor_y;
	std::mutex m_cursor_pos_mutex;

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

	/**
	 * @brief The mouse button callback function
	 *
	 * @param window The GLFWwindow object
	 * @param button The button pressed
	 * @param action The action of the button
	 * @param mods The mods of the button
	*/
	static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);

	/**
	 * @brief The cursor position callback function
	 *
	 * @param window The GLFWwindow object
	 * @param xpos The x position of the cursor
	 * @param ypos The y position of the cursor
	*/
	static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos);

};
