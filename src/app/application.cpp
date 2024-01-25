#include "application.hpp"

#include <iostream>

Application::Application()
{
	std::cout << "Application::Application()" << std::endl;
}

Application::~Application()
{
	std::cout << "Application::~Application()" << std::endl;
}

void Application::run()
{
	std::cout << "Application::run()" << std::endl;
}