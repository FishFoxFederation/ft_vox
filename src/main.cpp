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
		int player_id = 1;
		if (argc == 4)
		{
			ip = argv[1];
			port = std::stoi(argv[2]);
			player_id = std::stoi(argv[3]);
		}
		if (argc == 2)
		{
			player_id = std::stoi(argv[1]);
		}
		Application app(player_id, ip, port);

		app.run();
	}
	catch (const std::exception& e)
	{
		LOG_CRITICAL(e.what());
		return 1;
	}
	return 0;
}
