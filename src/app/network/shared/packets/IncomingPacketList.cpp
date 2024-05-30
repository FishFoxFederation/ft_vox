#include "IncomingPacketList.hpp"

IncomingPacketList::IncomingPacketList()
{
}

IncomingPacketList::~IncomingPacketList()
{
}

IncomingPacketList::IncomingPacketList(IncomingPacketList&& other)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	m_packets = std::move(other.m_packets);
}

IncomingPacketList& IncomingPacketList::operator=(IncomingPacketList&& other)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	m_packets = std::move(other.m_packets);
	return *this;
}

void IncomingPacketList::push(std::shared_ptr<IPacket> packet)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	m_packets.push(packet);
}

std::shared_ptr<IPacket> IncomingPacketList::pop()
{
	std::lock_guard<std::mutex> lock(m_mutex);
	if (m_packets.empty())
		return nullptr;
	auto packet = m_packets.front();
	m_packets.pop();
	return packet;
}

size_t IncomingPacketList::size() const
{
	std::lock_guard<std::mutex> lock(m_mutex);
	return m_packets.size();
}
