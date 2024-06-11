#include "Connection.hpp"
#include "server/Server.hpp"
#include "PacketFactory.hpp"
#include "ServerPacketHandler.hpp"
#include "ServerWorld.hpp"
#include <iostream>
#include <string>
#include <unordered_map>
#include <unistd.h>
#include <cstdlib>
#include <thread>
#include "logger.hpp"
#include <chrono>
#include "tracy_globals.hpp"

int main()
{
	LOG_INFO("Server started");
	Server server(4245);
	ServerWorld world;
	ServerPacketHandler packet_handler(server, world);
	// std::thread server_thread([&server](){
	// 	server.run();
	// });
	IncomingPacketList & incoming_packets = server.get_incoming_packets();
	uint64_t last_time = 0;
	uint64_t last_time_count = 0;
	uint64_t max_packet = 0;
	while (true)
	{
		ZoneScopedN("Server Loop");
		auto start = std::chrono::high_resolution_clock::now();
		server.runOnce(100);
		uint64_t packet_count = 0;
		while (incoming_packets.size() > 0)
		{
			packet_count++;
			if (packet_count > max_packet)
			{
				max_packet = packet_count;
				// std::cout << "Max packet: " << max_packet << std::endl;
			}
			auto packet = incoming_packets.pop();

			if (packet.get() == nullptr)
			{
				std::cout << "Packet is null" << std::endl;
				continue;
			}

			packet_handler.handlePacket(packet);
		}
		auto end = std::chrono::high_resolution_clock::now();
		if (last_time > 1e9)
		{
			last_time = 0;
			last_time_count = 0;
		}
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
		last_time += duration.count();
		last_time_count++;
		// std::cout << "Time: " << duration.count() << "ms" << std::endl;
		// std::cout << "Avg time: " << last_time / last_time_count << "ms, PacketCount: " << packet_count << std::endl;
	}
}
