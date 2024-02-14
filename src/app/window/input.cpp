#include "input.hpp"

#include <iostream>

Input::Input(GLFWwindow* glfwWindow):
	m_window(glfwWindow)
{
	glfwSetWindowUserPointer(m_window, this);

	glfwSetKeyCallback(m_window, keyCallback);
}

Input::~Input()
{

}

int Input::getKeyState(int key)
{
	int state = GLFW_RELEASE;

	std::lock_guard<std::mutex> lock(m_key_state_mutex);

	if (!m_key_state[key].empty())
	{
		state = m_key_state[key].front();

		if (m_key_state[key].size() > 1)
		{
			m_key_state[key].pop();
		}
	}

	return state;
}

void Input::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	(void)scancode;
	(void)mods;

	Input* input = static_cast<Input*>(glfwGetWindowUserPointer(window));

	if (action != GLFW_REPEAT)
	{
		std::lock_guard<std::mutex> lock(input->m_key_state_mutex);
		input->m_key_state[key].push(action);
	}

	switch (key)
	{
		case GLFW_KEY_ESCAPE:
		{
			if (action == GLFW_PRESS)
				glfwSetWindowShouldClose(window, GLFW_TRUE);
			break;
		}
		default:
		{
			std::cout << "Key: " << key << std::endl;
			break;
		}
	}
}
