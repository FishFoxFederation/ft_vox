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

/**
 * @brief The server side interface with the network module.
 * 
 * @details This class is responsible for managing the connections and the packets to send and receive.
 * 
 * It is fully threadsafe and can be run in a separate thread.
 * This is where packets are received and sent and where new connections are accepted.
 * 
 * About the sendInfo struct and the NOWAIT flag:
 *  if the server is running on another thread this flag can be usefull.
 *  when this flag is set the send function will only queue the packet to be sent.
 *  All of the serialization and the syscalls will be done in the server thread.
 *  If the packet being sent isnt highly delay sensitive this flag should be used.
 * It wont change anything regarding performances if the server is running on the same thread as the sending thread.
 */
class Server
{
public:
	static constexpr int PING_TIMEOUT_MS = 5000;
	Server (int port);
	~Server();

	Server(const Server& other) = delete;
	Server& operator=(const Server& other) = delete;
	Server(Server&& other) = delete;
	Server& operator=(Server&& other) = delete;

	void runOnce(int timeout_ms);

	enum flags : uint8_t
	{
		ALL = 1,
		ALLEXCEPT = 1 << 1,
		NOWAIT = 1 << 2,
	};

	/**
	 * @brief arg struct used to send a packet
	 */
	struct sendInfo
	{
		/**
		 * @brief shared pointer to the packet to send
		 * 
		 */
		std::shared_ptr<IPacket> packet;

		/**
		 * @brief bitfield containing flags from the flags enum
		 * 
		 */
		uint8_t flag;

		/**
		 * @brief a connection id
		 * 
		 * if the flag is set to ALLEXCEPT this id will be used to exclude a client from the send
		 * 
		 * else this id will be used as the destination id
		 */
		uint64_t id = 0;
	};
	void send(const sendInfo & info);

	/**
	 * @brief send a ping packet to a client
	 * 
	 * @param id 
	 */
	void ping(uint64_t id);

	/**
	 * @brief disconnect a client
	 * 
	 * @param id 
	 */
	void disconnect(uint64_t id);

	/**
	 * @brief Get a ref to the list of packet that have been received
	 * 
	 * @return ThreadSafePacketQueue& 
	 */
	ThreadSafePacketQueue & getIncomingPackets() { return m_incoming_packets; }

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

	/**
	 * @brief map used to store the time a ping was sent to a client
	 * 
	 */
	std::unordered_map<uint64_t, std::chrono::time_point<std::chrono::high_resolution_clock>> m_pings;

private:

	Poller										m_poller;
	ServerSocket								m_server_socket;
	PacketFactory							&	m_packet_factory;
	ThreadSafePacketQueue						m_incoming_packets;
	ThreadSafeQueue<sendInfo>					m_outgoing_packets;
	std::unordered_map<uint64_t, std::shared_ptr<Connection>>	m_connections;
	std::mutex									m_connections_mutex;

	uint64_t									m_ids_counter;

	uint64_t get_new_id();

	int		readData(Connection & connection, uint64_t id);
	int		sendData(Connection & connection, uint64_t id);
	void	emptyOutgoingPackets();
	void 	emptyOldPings();

	void sendPacket(std::shared_ptr<IPacket> packet, const uint64_t & id);
	void sendAllExcept(std::shared_ptr<IPacket> packet, const uint64_t & except_id);
	void sendAll(std::shared_ptr<IPacket> packet);
};
