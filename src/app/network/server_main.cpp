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
	for(auto & [id,client] : connections)
	{
		if (id != from)
			client.queueAndSendMessage(std::to_string(from) + ": " + msg);
	}
}

int main()
{
	ServerSocket server(4245);
	Poller poller;
	std::unordered_map<uint64_t, Connection> connections;
	uint64_t id = 1;
	poller.add(0, server, 1);

	bool online = true;
	while(online)
	{
		std::pair<size_t, epoll_event*> events = poller.wait(-1);
		std::cout << "New events : size :" << events.first << "\n";

		std::cout << "connections size : " << connections.size() << "\n";

		for(size_t i = 0; i < events.first; i++)
		{
			if (events.second[i].data.u64 == 0)
			{
				std::cout << "New client connected\n";
				Connection connection(server.accept());
				auto ret = connections.insert(std::make_pair(id++, std::move(connection)));
				std::cout << "New client id : " << ret.first->first << "\n";
				send_all(connections, "Joined !", ret.first->first);
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
						if (!connection.getReadBuffer().empty())
						{
							std::string message = connection.getReadBuffer().data();
							if (message.find('\n') != std::string::npos)
							{
								if (message == "exit\n")
								{
									online = false;
									break;
								}
								connection.reduceReadBuffer(message.find('\n') + 1);
								std::cout << "Received: " << message << std::endl;
								send_all(connections, message, currentClient->first);
							}
						}
					}

					if (events.second[i].events & EPOLLOUT)
					{
						connection.sendQueue();
					}

					if (events.second[i].events & EPOLLHUP || events.second[i].events & EPOLLRDHUP)
					{
						send_all(connections, "Client disconnected", currentClient->first);
						poller.remove(connection);
						connections.erase(currentClient);
					}
				}
			}
		}
	}
}
