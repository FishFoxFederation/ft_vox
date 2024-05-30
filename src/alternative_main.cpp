#include "application.hpp"
#include "logger.hpp"
#include "HitBox.hpp"

#include <iostream>
#include <exception>

int main(void)
{
	logger.setLevel(Logger::DEBUG);
	try
	{
		Application app;
		
		app.run();
	}
	catch (const std::exception& e)
	{
		LOG_CRITICAL(e.what());
		return 1;
	}
	return 0;
}
