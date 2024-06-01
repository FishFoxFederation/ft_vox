#include "Server.hpp"

Server::Server(int port)
: m_running(true), m_server_socket(port), m_packet_factory(PacketFactory::GetInstance()), m_ids_counter(1)
{
	m_poller.add(0, m_server_socket);
	LOG_INFO("Server started on port " + std::to_string(port));
}

Server::~Server()
{
}

void Server::stop()
{
	m_running = false;
}

void Server::runOnce(int timeout)
{
	auto [events_size, events] = m_poller.wait(timeout);

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
			LOG_INFO("New client connected");
			Connection connection(m_server_socket.accept());
			connection.setConnectionId(get_new_id());
			auto ret = m_connections.insert(std::make_pair(connection.getConnectionId(), std::move(connection)));
			LOG_INFO("New client id : " << std::to_string(ret.first->second.getConnectionId()));
			m_poller.add(ret.first->second.getConnectionId(), ret.first->second.getSocket());
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
			auto currentClient = m_connections.find(events[i].data.u64);
			if (currentClient == m_connections.end())
			{
				LOG_ERROR("Client not found");
				continue;
			}
			try {
				if (current_event & EPOLLIN)
					read_data(currentClient->second, currentClient->first);
				if (current_event & EPOLLOUT)
					send_data(currentClient->second, currentClient->first);
				if (current_event & EPOLLERR || current_event & EPOLLHUP)
				{
					LOG_INFO("EPOLLERR or EPOLLHUP");
					throw ClientDisconnected(currentClient->first);
				}
			} 
			catch (const ClientDisconnected & e)
			{
				LOG_INFO("Client disconnected :" + std::to_string(e.id()));
				m_poller.remove(currentClient->second.getSocket());
				m_connections.erase(currentClient);
			}
		}
	}
}

void Server::run()
{
	while(m_running)
	{
		runOnce(1000);
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

int Server::read_data(Connection & connection, uint64_t id)
{
	ssize_t ret;
	try {
		ret = connection.recv();
		if (ret == 0)
			throw ClientDisconnected(id);
		LOG_INFO("Data received");
		//insert code for detecting new packets
		// and packet handling as well as dispatching tasks
		auto ret = m_packet_factory.extractPacket(connection);
		if (ret.first)
		{
			LOG_INFO("Packet received :" + std::to_string((uint32_t)ret.second->GetType()));
			m_incoming_packets.push(ret.second);
		}
	}
	catch (const std::runtime_error & e)
	{
		throw ClientDisconnected(id);
	}
	return 0;
}

int Server::send_data(Connection & connection, uint64_t id)
{
	ssize_t ret;
	try
	{
		if( !connection.dataToSend() )
			return 0;
		LOG_INFO("sending data");
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

void Server::send(std::shared_ptr<IPacket> packet)
{
	auto currentClient = m_connections.find(packet->GetConnectionId());

	if (currentClient == m_connections.end())
	{
		LOG_ERROR("Client not found");
		return;
	}
	LOG_INFO("Sending packet :" + std::to_string((uint32_t)packet->GetType()));
	std::vector<uint8_t> buffer(packet->Size());
	packet->Serialize(buffer.data());
	currentClient->second.queueAndSendMessage(buffer);
}

void Server::sendAll(std::shared_ptr<IPacket> packet)
{
	std::vector<uint8_t> buffer(packet->Size());
	packet->Serialize(buffer.data());
	for (auto & [id, connection] : m_connections)
		connection.queueAndSendMessage(buffer);
}

void Server::sendAllExcept(std::shared_ptr<IPacket> packet, const uint64_t & id)
{
	std::vector<uint8_t> buffer(packet->Size());
	packet->Serialize(buffer.data());
	for (auto & [current_id, connection] : m_connections)
		if (current_id != id)
			connection.queueAndSendMessage(buffer);
}
