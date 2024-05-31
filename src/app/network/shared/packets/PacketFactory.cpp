#include "PacketFactory.hpp"

PacketFactory::PacketFactory()
{
	m_packets.insert(std::make_pair(IPacket::Type::CONNECTION, std::make_shared<ConnectionPacket>()));
	m_packets.insert(std::make_pair(IPacket::Type::PLAYER_CONNECTED, std::make_shared<PlayerConnectedPacket>()));
	m_packets.insert(std::make_pair(IPacket::Type::PLAYER_MOVE, std::make_shared<PlayerMovePacket>()));
}

PacketFactory::~PacketFactory()
{
}

ssize_t PacketFactory::getSize(IPacket::Type id) const
{
	auto it = m_packets.find(id);
	if (it != m_packets.end())
	{
		return it->second->Size();
	}
	return -1;
}

std::pair<bool, IPacket::Type> PacketFactory::getPacketType(const uint8_t * buffer, const size_t & size) const
{
	if (size < sizeof(uint32_t))
	{
		return std::make_pair(false, IPacket::Type::CONNECTION);
	}
	uint32_t id = *reinterpret_cast<const uint32_t*>(buffer);
	if (id >= static_cast<uint32_t>(IPacket::Type::CONNECTION) && id <= static_cast<uint32_t>(IPacket::Type::ENTITY_MOVE))
	{
		return std::make_pair(true, static_cast<IPacket::Type>(id));
	}
	return std::make_pair(false, IPacket::Type::CONNECTION);
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
