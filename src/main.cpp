#include "application.hpp"

#include <iostream>
#include <exception>

int main(void)
{
	try
	{
		Application app;

		app.run();
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return 1;
	}
	return 0;
}
