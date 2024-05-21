#pragma once

#include <unordered_map>

#include "ConnectionPacket.hpp"
#include "PlayerMovePacket.hpp"
#include "EntityMovePacket.hpp"


enum class packetType : uint32_t
{
	CONNECTION = 0,
	PLAYER_MOVE = 1,
	ENTITY_MOVE = 2
};

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

	static PacketFactory& getInstance();
private:
	PacketFactory();
	std::unordered_map<packetType, std::shared_ptr<IPacket>> m_packets;	
};
