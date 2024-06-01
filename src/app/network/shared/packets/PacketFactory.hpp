#pragma once

#include <unordered_map>

#include "PlayerConnectedPacket.hpp"
#include "ConnectionPacket.hpp"
#include "PlayerMovePacket.hpp"
#include "DisconnectPacket.hpp"
#include "BlockActionPacket.hpp"
// #include "EntityMovePacket.hpp"

class PacketFactory
{
public:
	~PacketFactory();

	PacketFactory(const PacketFactory& other) = delete;
	PacketFactory& operator=(const PacketFactory& other) = delete;

	PacketFactory(PacketFactory&& other) = delete;
	PacketFactory& operator=(PacketFactory&& other) = delete;

	ssize_t getSize(IPacket::Type id) const;

	std::pair<bool, IPacket::Type> getPacketType(const uint8_t * buffer, const size_t & size) const;
	std::pair<bool, std::shared_ptr<IPacket> > extractPacket(Connection & connection);

	static PacketFactory& GetInstance();
private:
	PacketFactory();
	std::unordered_map<IPacket::Type, std::shared_ptr<IPacket>> m_packets;	
};
