#include "application.hpp"
#include "logger.hpp"
#include "HitBox.hpp"

#include <iostream>
#include <exception>

int main(int argc, char **argv)
{
	// logger.setTimestamp(false);
	logger.setLevel(Logger::DEBUG);
	try
	{
		std::string ip = IP_ADDRESS;
		int port = PORT;
		if (argc == 3)
		{
			ip = argv[1];
			port = std::stoi(argv[2]);
		}
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
