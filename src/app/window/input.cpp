#include "input.hpp"

#include <iostream>

Input::Input(GLFWwindow* glfwWindow):
	m_window(glfwWindow)
{
	glfwSetWindowUserPointer(m_window, this);

	glfwSetKeyCallback(m_window, keyCallback);
	glfwSetMouseButtonCallback(m_window, mouseButtonCallback);
	glfwSetCursorPosCallback(m_window, cursorPosCallback);

	glfwSetInputMode(m_window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
}

Input::~Input()
{

}

Input::KeyState Input::getKeyState(int key)
{
	KeyState state = KeyState::NONE;

	std::lock_guard lock(m_key_state_mutex);

	if (!m_key_state[key].empty())
	{
		state = m_key_state[key].front();
		m_key_state[key].pop();
	}

	return state;
}

Input::KeyState Input::getMouseButtonState(int button)
{
	KeyState state = KeyState::RELEASED;

	std::lock_guard lock(m_mouse_button_state_mutex);

	if (!m_mouse_button_state[button].empty())
	{
		state = m_mouse_button_state[button].front();

		if (m_mouse_button_state[button].size() > 1)
		{
			m_mouse_button_state[button].pop();
		}
	}

	return state;
}

void Input::getCursorPos(double& xpos, double& ypos)
{
	std::lock_guard lock(m_cursor_mutex);
	xpos = m_cursor_x;
	ypos = m_cursor_y;
}

void Input::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	(void)scancode;
	(void)mods;

	Input* input = static_cast<Input*>(glfwGetWindowUserPointer(window));

	if (action != GLFW_REPEAT)
	{
		std::lock_guard lock(input->m_key_state_mutex);
		input->m_key_state[key].push(static_cast<Input::KeyState>(action));
	}

	switch (key)
	{
		case GLFW_KEY_ESCAPE:
		{
			if (action == GLFW_PRESS)
			{
				std::lock_guard lock(input->m_cursor_mutex);
				if (input->m_cursor_captured)
				{
					glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
					input->m_cursor_captured = false;
				}
				else
				{
					glfwSetWindowShouldClose(window, GLFW_TRUE);
				}
			}
			break;
		}
		case GLFW_KEY_C:
		{
			if (action == GLFW_PRESS)
			{
				std::lock_guard lock(input->m_cursor_mutex);
				if (input->m_cursor_captured)
				{
					glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
					input->m_cursor_captured = false;
				}
				else
				{
					glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
					input->m_cursor_captured = true;
				}
			}
			break;
		}
		default:
		{
			break;
		}
	}
}

void Input::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	(void)mods;

	Input* input = static_cast<Input*>(glfwGetWindowUserPointer(window));

	std::lock_guard lock(input->m_mouse_button_state_mutex);
	input->m_mouse_button_state[button].push(static_cast<Input::KeyState>(action));

	// if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS && !input->m_cursor_captured)
	// {
	// 	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	// 	input->m_cursor_captured = true;
	// }
}

void Input::cursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
	Input* input = static_cast<Input*>(glfwGetWindowUserPointer(window));

	std::lock_guard lock(input->m_cursor_mutex);
	input->m_cursor_x = xpos;
	input->m_cursor_y = ypos;
}
