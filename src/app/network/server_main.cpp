#include "ConnectionSocket.hpp"
#include "server/ServerSocket.hpp"
#include "Poller.hpp"
#include <iostream>
#include <string>
#include <unordered_map>
#include <unistd.h>
#include <cstdlib>

void send_all(std::unordered_map<uint64_t, std::string> & send_buffer, const std::string & msg, int from)
{
	for(auto & buffer : send_buffer)
	{
		buffer.second += std::to_string(from) + ": " + msg + "\n";
	}
}

int main()
{
	ServerSocket server(4245);
	Poller poller;
	std::unordered_map<uint64_t, ConnectionSocket> connections;
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
				ConnectionSocket connection = server.accept();
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
					ConnectionSocket & connection = currentClient->second;
					if (events.second[i].events & EPOLLIN)
					{
						char buffer[1024];
						size_t size = connection.recv(buffer, sizeof(buffer));
						buffer[size] = '\0';
						std::string message(buffer);
						if (message.empty())
						{
							poller.remove(connection);
							connections.erase(currentClient);
							send_buffer.erase(currentClient->first);
							send_all(send_buffer, "Client disconnected", currentClient->first);
						}
						else
						{
							std::cout << "Received: " << message << std::endl;
							send_all(send_buffer, message, currentClient->first);
						}
					}

					if (events.second[i].events & EPOLLOUT)
					{
						std::string & buffer = send_buffer.at(currentClient->first);
						if (buffer.empty())
							continue;
						size_t size = connection.send(buffer.c_str(), buffer.size());
						buffer.erase(0, size);
					}
				}
			}
		}
	}
}
