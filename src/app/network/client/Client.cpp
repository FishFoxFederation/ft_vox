#include "Client.hpp"

Client::Client(const std::string& ip, int port) :
	m_poller(),
	m_client_socket(std::make_shared<ClientSocket>(ip.c_str(), port)),
	m_connection(m_client_socket)
{
	m_poller.add(0, *m_client_socket);
}

Client::~Client()
{
}

void Client::runOnce(const int & timeout)
{
	ZoneScoped;
	emptyOutgoingPackets();
	auto [events_size, events] = m_poller.wait(timeout);

	// if the poller was interrupted by a signal
	// just return and let the caller handle it
	if (errno == EINTR)
	{
		errno = 0;
		return;
	}
	/**********************************************
	 * Handle new events
	 * ********************************************/
	for (size_t i = 0; i < events_size; i++)
	{
		auto current_event = events[i].events;
		if (current_event & EPOLLIN)
			readData();
		if (current_event & EPOLLOUT)
			sendData();
		if (current_event & EPOLLERR || current_event & EPOLLHUP)
			throw ServerDisconnected();
	}
}

int Client::readData()
{
	ZoneScoped;
	ssize_t size;
	std::lock_guard lock(m_connection.ReadLock());
	try {
		size = m_connection.recv();
		if (size == 0)
			throw ServerDisconnected();

		auto ret = m_packet_factory.extractPacket(m_connection);
		while (ret.first)
		{
			m_incoming_packets.push(ret.second);
			ret = m_packet_factory.extractPacket(m_connection);
		}
		//only clearing here becasue optimization based on offsets
		//( see the Connection::reduceReadBuffer function )
		m_connection.clearReadBuffer();
		DebugGui::recv_buffer_size = m_connection.getReadBufferSize();
	}
	catch (const std::runtime_error & e)
	{
		throw ServerDisconnected();
	}
	return 0;
}

int Client::sendData()
{
	ZoneScoped;
	ssize_t size;
	std::lock_guard lock(m_connection.WriteLock());
	try {
		if( !m_connection.dataToSend() )
			return 0;
		size = m_connection.sendQueue();
		if (size == 0)
			throw ServerDisconnected();
		DebugGui::send_buffer_size = m_connection.getWriteBufferRef().size();
	}
	catch (const std::runtime_error & e)
	{
		throw ServerDisconnected();
	}
	return 0;
}

void Client::sendPacket(std::shared_ptr<IPacket> packet)
{
	ZoneScoped;
	// LOG_INFO("Sending packet :" + std::to_string((uint32_t)packet->GetType()));
	std::vector<uint8_t> buffer(packet->Size());
	packet->Serialize(buffer.data());
	{
		std::lock_guard lock(m_connection.WriteLock());
		m_connection.queueAndSendMessage(buffer);
	}
}

void Client::sendPacketNoWait(std::shared_ptr<IPacket> packet)
{
	ZoneScoped;
	m_outgoing_packets.push(packet);
}

std::shared_ptr<IPacket> Client::popPacket()
{
	return m_incoming_packets.pop();
}

size_t Client::getQueueSize() const
{
	return m_incoming_packets.size();
}

void Client::emptyOutgoingPackets()
{
	std::vector<uint8_t> buffer;
	while(m_outgoing_packets.size() != 0)
		sendPacket(m_outgoing_packets.pop());
}
