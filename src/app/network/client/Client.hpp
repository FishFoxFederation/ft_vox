#pragma once

#include "define.hpp"
#include <memory>

#include "DebugGui.hpp"
#include "Poller.hpp"
#include "ClientSocket.hpp"
#include "Connection.hpp"
#include "PacketFactory.hpp"
#include "ThreadSafePacketQueue.hpp"

/**
 * @brief The Client side interface with the network module.
 * 
 * @details This class is responsible for managing the connection to the server 
 *  and sending and receiving packets.
 * 
 * It is fully threadsafe and can be run in a separate thread.
 * 
 * About the sendInfo struct and the ASYNC flag:
 * 
 *  if the client is running on another thread this flag can be usefull.
 *  when this flag is set the send function will only queue the packet to be sent.
 *  All of the serialization and the syscalls will be done in the client thread.
 *  If the packet being sent isnt highly delay sensitive this flag should be used.
 * It wont change anything regarding performances if the client is running on the same thread as the sending thread.
 * 
 * About the ServerDisconnected exception:
 * 
 * It is the client's way of of notifying the user that the server disconnected.
 * 
 * the runOnce method will throw this exception if the server disconnected.
 * 
 */
class Client
{
public:
	Client(const std::string& ip, int port);
	~Client();

	Client(const Client& other) = delete;
	Client& operator=(const Client& other) = delete;
	Client(Client&& other) = delete;
	Client& operator=(Client&& other) = delete;

	void runOnce(const int & timeout_ms = -1);

	enum flags : uint8_t
	{
		ASYNC = 1
	};

	struct sendInfo
	{
		std::shared_ptr<IPacket> packet;
		uint8_t flags;
	};
	void send(const sendInfo & info);

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
	
	Poller											m_poller;
	std::shared_ptr<Socket>							m_client_socket;
	Connection										m_connection;
	ThreadSafePacketQueue							m_incoming_packets;
	PacketFactory &									m_packet_factory = PacketFactory::GetInstance();
	ThreadSafePacketQueue							m_outgoing_packets;

	int		readData();
	int		sendData();
	void	emptyOutgoingPackets();

	void	sendPacket(std::shared_ptr<IPacket> packet);
};
