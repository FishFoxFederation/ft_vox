#include "Connection.hpp"
#include "server/ServerSocket.hpp"
#include "Poller.hpp"
#include <iostream>
#include <string>
#include <unordered_map>
#include <unistd.h>
#include <cstdlib>

void send_all(std::unordered_map<uint64_t, Connection> & connections, const std::string & msg, int from)
{
	for(auto & [id, client] : connections)
	{
		client.queueMessage(std::to_string(from) + ": " + msg + "\n");
	}
}

int main()
{
	ServerSocket server(4245);
	Poller poller;
	std::unordered_map<uint64_t, Connection> connections;
	std::unordered_map<uint64_t, std::string> send_buffer;
	uint64_t id = 0;
	poller.add(id, server, 1);

	while(1)
	{
		std::pair<size_t, epoll_event*> events = poller.wait(-1);
		std::cout << "New events : size :" << events.first << "\n";

		for(size_t i = 0; i < events.first; i++)
		{
			if (events.second[i].data.u32 != 0)
			{
				std::cout << "New client connected\n";
				Connection connection(server.accept());
				auto ret = connections.insert(std::make_pair(id++, std::move(connection)));
				send_buffer.insert(std::make_pair(ret.first->first, ""));
				std::cout << "New client id : " << ret.first->first << "\n";
				poller.add(ret.first->first, ret.first->second);
			}

			else
			{
				auto currentClient = connections.find(events.second[i].data.u64);
				if (currentClient == connections.end())
				{
					std::cerr << "Client not found\n";
					continue;
				}
				if (currentClient != connections.end())
				{
					std::cout << "Client event\n";
					Connection & connection = currentClient->second;
					if (events.second[i].events & EPOLLIN)
					{
						connection.recv();
						std::string message = connection.getReadBuffer().data();
						if (message.find('\n') != std::string::npos)
						{
							connection.reduceReadBuffer(message.find('\n') + 1);
							std::cout << "Received: " << message << std::endl;
							send_all(send_buffer, message, currentClient->first);
						}
					}

					if (events.second[i].events & EPOLLOUT)
					{
						std::string & buffer = send_buffer.at(currentClient->first);
						if (buffer.empty())
							continue;
						size_t size = connection.queueMessage(buffer.c_str(), buffer.size());
						buffer.erase(0, size);
					}

					if (events.se)
					{
						poller.remove(connection);
						connections.erase(currentClient);
						send_buffer.erase(currentClient->first);
						send_all(send_buffer, "Client disconnected", currentClient->first);
					}
				}
			}
		}
	}
}
