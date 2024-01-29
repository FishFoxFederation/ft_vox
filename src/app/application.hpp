#pragma once

#include "define.hpp"
#include "renderer.hpp"
#include "window.hpp"

#include <GLFW/glfw3.h>

class Application
{

public:

	Application();
	~Application();

	void run();

private:

	Window m_window;
};