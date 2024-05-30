#pragma once

#include <unordered_map>

#include "ConnectionPacket.hpp"
#include "PlayerMovePacket.hpp"
// #include "EntityMovePacket.hpp"

class PacketFactory
{
public:
	~PacketFactory();

	PacketFactory(const PacketFactory& other) = delete;
	PacketFactory& operator=(const PacketFactory& other) = delete;

	PacketFactory(PacketFactory&& other) = delete;
	PacketFactory& operator=(PacketFactory&& other) = delete;

	std::shared_ptr<IPacket> CreatePacket(packetType id, const uint8_t * buffer);

	ssize_t getSize(packetType id) const;

	std::pair<bool, packetType> getPacketType(const uint8_t * buffer, const size_t & size) const;
	std::pair<bool, std::shared_ptr<IPacket> > extractPacket(Connection & connection);

	static PacketFactory& GetInstance();
private:
	PacketFactory();
	std::unordered_map<packetType, std::shared_ptr<IPacket>> m_packets;	
};
