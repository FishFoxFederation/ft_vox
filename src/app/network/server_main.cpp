#include "Connection.hpp"
#include "server/Server.hpp"
#include "PacketFactory.hpp"
#include "ServerPacketHandler.hpp"
#include <iostream>
#include <string>
#include <unordered_map>
#include <unistd.h>
#include <cstdlib>
#include <thread>
#include "logger.hpp"



int main()
{
	LOG_INFO("Server started");
	Server server(4245);
	ServerPacketHandler packet_handler(server);
	// std::thread server_thread([&server](){
	// 	server.run();
	// });

	IncomingPacketList & incoming_packets = server.get_incoming_packets();
	while (true)
	{
		server.runOnce(10);
		if (incoming_packets.size() > 0)
		{
			auto packet = incoming_packets.pop();
			std::cout << "Packet received" << std::endl;

			if (packet.get() == nullptr)
			{
				std::cout << "Packet is null" << std::endl;
				continue;
			}

			packet_handler.handlePacket(packet);
		}
	}
}
