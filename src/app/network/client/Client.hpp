#pragma once

#include "define.hpp"
#include <memory>

#include "Poller.hpp"
#include "ClientSocket.hpp"
#include "Connection.hpp"
#include "IncomingPacketList.hpp"

class Client
{
public:
	Client(const std::string& ip, int port);
	~Client();

	Client(const Client& other) = delete;
	Client& operator=(const Client& other) = delete;
	Client(Client&& other) = delete;
	Client& operator=(Client&& other) = delete;

	void run();

	class ServerDisconnected : public std::exception
	{
	public:
		ServerDisconnected() {}
		virtual const char* what() const throw()
		{
			return "Server disconnected";
		}
	};

	std::shared_ptr<IPacket> pop_packet();
private:
	
	bool					m_running;
	Poller					m_poller;
	std::shared_ptr<Socket>	m_client_socket;
	Connection				m_connection;
	IncomingPacketList		m_incoming_packets;

	int		read_data();
	int		send_data();
};
