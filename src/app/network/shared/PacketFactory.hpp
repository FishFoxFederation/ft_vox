#pragma once

#include <unordered_map>

#include "PlayerConnectedPacket.hpp"
#include "ConnectionPacket.hpp"
#include "PlayerMovePacket.hpp"
#include "DisconnectPacket.hpp"
#include "BlockActionPacket.hpp"
#include "PingPacket.hpp"
// #include "EntityMovePacket.hpp"

class PacketFactory
{
public:
	~PacketFactory();

	PacketFactory(const PacketFactory& other) = delete;
	PacketFactory& operator=(const PacketFactory& other) = delete;

	PacketFactory(PacketFactory&& other) = delete;
	PacketFactory& operator=(PacketFactory&& other) = delete;

	// ssize_t getSize(IPacket::Type id) const;

	/**
	 * @brief Will try to extract a packet from the connection's buffer
	 * 
	 * @param connection 
	 * @return a pair containing a bool indicating if the extraction was sucessful and a shared pointer to the packet
	 */
	std::pair<bool, std::shared_ptr<IPacket> > extractPacket(Connection & connection);

	static PacketFactory& GetInstance();
private:
	struct packetInfo
	{
		bool complete;
		IPacket::Type type;
		ssize_t size;
	};
	/**
	 * @brief Get information about the potential packet in a buffer.
	 * 
	 * @param buffer 
	 * @param size 
	 * @return a struct containing information if the packet is complete
 	*/
	packetInfo getPacketInfo(const uint8_t * buffer, const size_t & size) const;
	PacketFactory();
	std::unordered_map<IPacket::Type, std::shared_ptr<IPacket>> m_packets;	
};
