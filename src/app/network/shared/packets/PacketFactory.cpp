#include "PacketFactory.hpp"

PacketFactory::PacketFactory()
{
	m_packets[packetType::CONNECTION] = std::make_shared<ConnectionPacket>();
	m_packets[packetType::PLAYER_MOVE] = std::make_shared<PlayerMovePacket>();
	// m_packets[packetType::ENTITY_MOVE] = std::make_shared<EntityMovePacket>();
}

PacketFactory::~PacketFactory()
{
}

std::shared_ptr<IPacket> PacketFactory::CreatePacket(packetType id, const uint8_t * buffer)
{
	auto it = m_packets.find(id);
	if (it != m_packets.end())
	{

	}
	return nullptr;
}

ssize_t PacketFactory::getSize(packetType id) const
{
	auto it = m_packets.find(id);
	if (it != m_packets.end())
	{
		return it->second->Size();
	}
	return -1;
}

std::pair<bool, packetType> PacketFactory::getPacketType(const uint8_t * buffer, const size_t & size) const
{
	if (size < sizeof(uint32_t))
	{
		return std::make_pair(false, packetType::CONNECTION);
	}
	uint32_t id = *reinterpret_cast<const uint32_t*>(buffer);
	if (id >= static_cast<uint32_t>(packetType::CONNECTION) && id <= static_cast<uint32_t>(packetType::ENTITY_MOVE))
	{
		return std::make_pair(true, static_cast<packetType>(id));
	}
	return std::make_pair(false, packetType::CONNECTION);
}

std::pair<bool, std::shared_ptr<IPacket>> PacketFactory::extractPacket(Connection & connection)
{
	const std::vector<uint8_t> & buffer = connection.getReadBufferRef();
	auto packetRet = getPacketType(buffer.data(), buffer.size());
	std::pair<bool, std::shared_ptr<IPacket>> ret = std::make_pair(false, nullptr);
	if (packetRet.first)
	{
		auto packet = m_packets.at(packetRet.second)->Clone();
		packet->ExtractMessage(connection);
		ret.first = true;
		ret.second = packet;
	}

	return ret;
}

PacketFactory& PacketFactory::GetInstance()
{
	static PacketFactory instance;
	return instance;
}
