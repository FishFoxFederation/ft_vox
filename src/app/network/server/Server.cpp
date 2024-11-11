#include "Server.hpp"

Server::Server(int port)
: m_server_socket(port), m_packet_factory(PacketFactory::GetInstance()), m_ids_counter(1)
{
	m_poller.add(0, m_server_socket);
	LOG_INFO("Server started on port " + std::to_string(port));
}

Server::~Server()
{
}

void Server::runOnce(int timeout)
{
	ZoneScoped;

	emptyOutgoingPackets();
	emptyOldPings();
	auto [events_size, events] = m_poller.wait(timeout);
	if (errno == EINTR)
	{
		errno = 0;
		return;
	}
	/**********************************************
	 * Handle new events
	 * ********************************************/
	for(size_t i = 0; i < events_size; i++)
	{
		/**********************************************
		*	Handle new connections
		**********************************************/
		if (events[i].data.u64 == 0)
		{
			std::lock_guard lock(m_connections_mutex);
			LOG_INFO("New client connected");
			std::shared_ptr<Connection> connection = std::make_shared<Connection>(m_server_socket.accept());
			connection->setConnectionId(get_new_id());
			m_connections.insert(std::make_pair(connection->getConnectionId(), connection));
			LOG_INFO("New client id : " << std::to_string(connection->getConnectionId()));
			m_poller.add(connection->getConnectionId(), connection->getSocket());
		}

		/**********************************************
		 * Handle existing connections
		 * 			- Read data
		 * 			- Send data
		 * 			- Terminate connections
		 * ********************************************/
		else
		{
			auto current_event = events[i].events;
			std::shared_ptr<Connection> currentClient;
			{
				std::lock_guard lock(m_connections_mutex);
				auto it = m_connections.find(events[i].data.u64);
				if (it == m_connections.end())
				{
					LOG_ERROR("Client not found");
					continue;
				}
				currentClient = it->second;
			}
			try {
				if (current_event & EPOLLIN)
					readData(*currentClient, currentClient->getConnectionId());
				if (current_event & EPOLLOUT)
					sendData(*currentClient, currentClient->getConnectionId());
				if (current_event & EPOLLERR || current_event & EPOLLHUP)
				{
					LOG_INFO("EPOLLERR or EPOLLHUP");
					throw ClientDisconnected(currentClient->getConnectionId());
				}
			}
			catch (const ClientDisconnected & e)
			{
				LOG_INFO("Client disconnected : id: " << e.id() << " pushing disconnect packet");
				auto packet = std::make_shared<DisconnectPacket>();
				packet->SetConnectionId(e.id());
				m_incoming_packets.push(packet);
				disconnect(e.id());
			}
		}
	}
}

uint64_t Server::get_new_id()
{
	// id generator extremely advanced code do not touch
	//	or pay me pizza
	// while (1)
	// {
	// 	// If the counter is 0, increment it to 1 because the id 0 is reserved for the server socket
	// 	if (m_ids_counter == 0)
	// 		m_ids_counter++;
	// 	if (!m_connections.contains(m_ids_counter))
	// 		return m_ids_counter++;
	// 	m_ids_counter++;
	// }
	return m_ids_counter++;
}

int Server::readData(Connection & connection, uint64_t id)
{
	ZoneScoped;
	ssize_t ret;
	std::lock_guard lock(connection.ReadLock());
	try {

		ret = connection.recv();
		if (ret == 0)
			throw ClientDisconnected(id);
		auto ret = m_packet_factory.extractPacket(connection);
		while (ret.first)
		{
			if (ret.second->GetConnectionId() != id)
				LOG_ERROR("Packet connection id does not match connection id");
			m_incoming_packets.push(ret.second);
			ret = m_packet_factory.extractPacket(connection);
		}
		connection.clearReadBuffer();
	}
	catch (const std::runtime_error & e)
	{
		throw ClientDisconnected(id);
	}
	return 0;
}

int Server::sendData(Connection & connection, uint64_t id)
{
	ZoneScoped;
	ssize_t ret;
	std::lock_guard lock(connection.WriteLock());
	try
	{
		if( !connection.dataToSend() )
			return 0;
		ret = connection.sendQueue();
		if (ret == 0)
			throw ClientDisconnected(id);
	}
	catch (const std::runtime_error & e)
	{
		throw ClientDisconnected(id);
	}
	return ret;
}

void Server::disconnect(uint64_t id)
{
	std::lock_guard lock(m_connections_mutex);
	auto it = m_connections.find(id);
	std::shared_ptr<Connection> currentClient;
	if (it == m_connections.end())
	{
		LOG_ERROR("Server: Disconnect: Client not found");
		return;
	}
	currentClient = it->second;
	m_poller.remove(currentClient->getSocket());
	m_connections.erase(it);
}

void Server::emptyOutgoingPackets()
{
	ZoneScoped;
	while(m_outgoing_packets.size() != 0)
	{
		auto info = m_outgoing_packets.pop();
		info.flag &= ~NOWAIT;
		send(info);
	}
}

void Server::ping(uint64_t id)
{
	auto packet = std::make_shared<PingPacket>(id, 1);
	m_pings[id] = std::chrono::high_resolution_clock::now();
	send({packet, 0, id});
}

void Server::send(const Server::sendInfo & info)
{
	if (info.flag & NOWAIT)
	{
		m_outgoing_packets.push(info);
		return;
	}
	if (info.flag & ALLEXCEPT)
		sendAllExcept(info.packet, info.id);
	else if (info.flag & ALL)
		sendAll(info.packet);
	else
		sendPacket(info.packet, info.id);
}

void Server::sendAll(std::shared_ptr<IPacket> packet)
{
	std::lock_guard lock(m_connections_mutex);
	std::vector<uint8_t> buffer(packet->Size());
	packet->Serialize(buffer.data());
	for (auto & [id, connection] : m_connections)
	{
		std::lock_guard lock(connection->WriteLock());
		connection->queueAndSendMessage(buffer);
	}
}

void Server::sendAllExcept(std::shared_ptr<IPacket> packet, const uint64_t & id)
{
	std::vector<uint8_t> buffer(packet->Size());
	packet->Serialize(buffer.data());
	std::lock_guard lock(m_connections_mutex);
	for (auto & [current_id, connection] : m_connections)
	{
		if (current_id != id)
		{
			std::lock_guard lock(connection->WriteLock());
			connection->queueAndSendMessage(buffer);
		}
	}
}

void Server::sendPacket(std::shared_ptr<IPacket> packet, const uint64_t & id)
{
	ZoneScoped;
	packet->SetConnectionId(id);
	std::shared_ptr<Connection> currentClient;
	{
		std::lock_guard lock(m_connections_mutex);
		auto it = m_connections.find(packet->GetConnectionId());
		if (it == m_connections.end())
		{
			// LOG_ERROR("Server: Send: Client not found");
			return;
		}
		currentClient = it->second;
	}

	// LOG_INFO("Sending packet :" + std::to_string((uint32_t)packet->GetType()));
	std::vector<uint8_t> buffer(packet->Size());
	packet->Serialize(buffer.data());
	{
		std::lock_guard lock(currentClient->WriteLock());
		currentClient->queueAndSendMessage(buffer);
	}
}

void Server::emptyOldPings()
{
	static auto last = std::chrono::high_resolution_clock::now();

	auto now = std::chrono::high_resolution_clock::now();

	if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last).count() < PING_TIMEOUT_MS)
		return;
	last = now;
	auto it = m_pings.begin();

	while (it != m_pings.end())
	{
		if (now - it->second > std::chrono::milliseconds(PING_TIMEOUT_MS))
			it = m_pings.erase(it);
		else
			it++;
	}

}
