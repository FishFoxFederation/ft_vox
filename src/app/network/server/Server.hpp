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
#include "ThreadSafeQueue.hpp"
#include "ThreadSafePacketQueue.hpp"

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
	void runOnce(int timeout_ms);

	void stop();
	
	enum flags : uint8_t
	{
		ALL = 1,
		ALLEXCEPT = 1 << 1,
		NOWAIT = 1 << 2,
	};
	struct sendInfo
	{
		std::shared_ptr<IPacket> packet;
		uint8_t flag;
		uint64_t id = 0;
	};
	void send(const sendInfo & info);
	void ping(uint64_t id);

	void disconnect(uint64_t id);

	ThreadSafePacketQueue & get_incoming_packets() { return m_incoming_packets; }

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

	std::unordered_map<uint64_t, std::chrono::time_point<std::chrono::high_resolution_clock>> m_pings;
private:

	std::atomic<bool>							m_running;
	Poller										m_poller;
	ServerSocket								m_server_socket;
	PacketFactory							&	m_packet_factory;
	ThreadSafePacketQueue						m_incoming_packets;
	ThreadSafeQueue<sendInfo>					m_outgoing_packets;
	std::unordered_map<uint64_t, std::shared_ptr<Connection>>	m_connections;
	std::mutex									m_connections_mutex;


	uint64_t									m_ids_counter;

	uint64_t get_new_id();

	int		read_data(Connection & connection, uint64_t id);
	int		send_data(Connection & connection, uint64_t id);
	void	empty_outgoing_packets();

	void sendPacket(std::shared_ptr<IPacket> packet, const uint64_t & id);
	void sendAllExcept(std::shared_ptr<IPacket> packet, const uint64_t & except_id);
	void sendAll(std::shared_ptr<IPacket> packet);

};
