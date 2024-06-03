#include "PacketFactory.hpp"

PacketFactory::PacketFactory()
{
	m_packets.insert(std::make_pair(IPacket::Type::CONNECTION, std::make_shared<ConnectionPacket>()));
	m_packets.insert(std::make_pair(IPacket::Type::PLAYER_CONNECTED, std::make_shared<PlayerConnectedPacket>()));
	m_packets.insert(std::make_pair(IPacket::Type::PLAYER_MOVE, std::make_shared<PlayerMovePacket>()));
	m_packets.insert(std::make_pair(IPacket::Type::DISCONNECT, std::make_shared<DisconnectPacket>()));
	m_packets.insert(std::make_pair(IPacket::Type::BLOCK_ACTION, std::make_shared<BlockActionPacket>()));
	m_packets.insert(std::make_pair(IPacket::Type::PING, std::make_shared<PingPacket>()));
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
	try {
		IPacket::Type type = static_cast<IPacket::Type>(id);

		return std::make_pair(m_packets.at(type)->Size() <= size, type);
	}
	catch (const std::out_of_range & e)
	{
		LOG_ERROR("PacketFactory::getPacketType: " << e.what());
		return std::make_pair(false, IPacket::Type::CONNECTION);
	}
	return std::make_pair(false, IPacket::Type::CONNECTION);
}

std::pair<bool, std::shared_ptr<IPacket>> PacketFactory::extractPacket(Connection & connection)
{
	std::pair<bool, IPacket::Type> packetRet;
	{
		std::lock_guard<std::mutex> lock(connection.getReadBufferMutex());
		const std::vector<uint8_t> & buffer = connection.getReadBufferRef();
		packetRet = getPacketType(buffer.data(), buffer.size());
	}

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
