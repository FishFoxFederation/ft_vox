#pragma once

#include "define.hpp"
#include "renderer.hpp"

#include <GLFW/glfw3.h>

class Application
{

public:

	Application();
	~Application();

	void run();

private:

	GLFWwindow* window;
};