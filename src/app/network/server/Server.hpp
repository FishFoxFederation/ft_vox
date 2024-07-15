#pragma once

#include "define.hpp"
#include <unordered_map>
#include <exception>
#include <atomic>

#include "logger.hpp"
#include "ServerSocket.hpp"
#include "Connection.hpp"
#include "Poller.hpp"
#include "PacketFactory.hpp"
#include "IncomingPacketList.hpp"

class Server
{
public:
	Server (int port);
	~Server();

	Server(const Server& other) = delete;
	Server& operator=(const Server& other) = delete;
	Server(Server&& other) = delete;
	Server& operator=(Server&& other) = delete;

	/**
	 * @brief start the server and blocks until the server is stopped
	 * you might wanna run this in a separate thread
	 */
	void run();
	void runOnce(int timeout);

	void stop();

	void send(std::shared_ptr<IPacket> packet);
	void sendAll(std::shared_ptr<IPacket> packet);
	void sendAllExcept(std::shared_ptr<IPacket> packet, const uint64_t & id);

	void disconnect(uint64_t id);

	IncomingPacketList & get_incoming_packets() { return m_incoming_packets; }

	class ClientDisconnected : public std::exception
	{
	public:
		ClientDisconnected(uint64_t id) : m_id(id) {}
		virtual const char* what() const throw()
		{
			return "Client disconnected";
		}
		uint64_t id() const { return m_id; }
	private:
		uint64_t m_id;
	};

private:

	std::atomic<bool>							m_running;
	Poller										m_poller;
	ServerSocket								m_server_socket;
	PacketFactory							&	m_packet_factory;
	IncomingPacketList							m_incoming_packets;
	std::unordered_map<uint64_t, Connection>	m_connections;


	uint64_t									m_ids_counter;

	uint64_t get_new_id();

	int		read_data(Connection & connection, uint64_t id);
	int		send_data(Connection & connection, uint64_t id);
};
