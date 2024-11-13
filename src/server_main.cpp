#include "Connection.hpp"
#include "server/Server.hpp"
#include "PacketFactory.hpp"
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
#include "ServerBlockUpdateThread.hpp"
#include <signal.h>

bool running = true;

int main()
{
	logger.configure("log_server");
	LOG_INFO("Server started");
	signal(SIGINT, [](int signum) {
		running = false;
	});
	try {
	Server server(4245);
	ServerWorld world(server);
	ServerBlockUpdateThread block_update_thread(world);
	// ServerPacketHandler packet_handler(server, world);
	// std::thread server_thread([&server](){
	// 	server.run();
	// });
	ThreadSafePacketQueue & incoming_packets = server.getIncomingPackets();
	uint64_t last_time = 0;
	uint64_t last_time_count = 0;
	(void)last_time_count;
	uint64_t max_packet = 0;
	while (running)
	{
		ZoneScopedN("Server Loop");
		auto start = std::chrono::high_resolution_clock::now();
		server.runOnce(1);
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

			world.handlePacket(packet);
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
	catch (std::runtime_error & e)
	{
		if (errno == EINTR)
		{
			LOG_INFO("Server stopped");
			return 0;
		}
		LOG_CRITICAL("Runtime error: " << e.what());
	}
	catch (std::exception & e)
	{
		LOG_CRITICAL("Exception: " << e.what());
	}
	catch (...)
	{
		LOG_ERROR("Unknown exception");
	}
	LOG_INFO("Server stopped");
}
