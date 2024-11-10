#pragma once

#include "define.hpp"
#include <memory>

#include "DebugGui.hpp"
#include "Poller.hpp"
#include "ClientSocket.hpp"
#include "Connection.hpp"
#include "PacketFactory.hpp"
#include "ThreadSafePacketQueue.hpp"

class Client
{
public:
	Client(const std::string& ip, int port);
	~Client();

	Client(const Client& other) = delete;
	Client& operator=(const Client& other) = delete;
	Client(Client&& other) = delete;
	Client& operator=(Client&& other) = delete;

	void runOnce(const int & timeout = -1);
	void run();
	void sendPacket(std::shared_ptr<IPacket> packet);
	void sendPacketNoWait(std::shared_ptr<IPacket> packet);

	class ServerDisconnected : public std::exception
	{
	public:
		ServerDisconnected() {}
		virtual const char* what() const throw()
		{
			return "Server disconnected";
		}
	};

	std::shared_ptr<IPacket>	popPacket();
	size_t						getQueueSize() const;

	std::unordered_map<uint64_t, std::chrono::time_point<std::chrono::high_resolution_clock>> m_pings;
private:
	
	bool											m_running;
	Poller											m_poller;
	std::shared_ptr<Socket>							m_client_socket;
	Connection										m_connection;
	ThreadSafePacketQueue							m_incoming_packets;
	PacketFactory &									m_packet_factory = PacketFactory::GetInstance();
	ThreadSafePacketQueue							m_outgoing_packets;

	int		read_data();
	int		send_data();
	void	empty_outgoing_packets();
};
