#pragma once

#include <unordered_map>

#include "Packets.hpp"

class PacketFactory
{
public:
	~PacketFactory();

	PacketFactory(const PacketFactory& other) = delete;
	PacketFactory& operator=(const PacketFactory& other) = delete;

	PacketFactory(PacketFactory&& other) = delete;
	PacketFactory& operator=(PacketFactory&& other) = delete;

	/**
	 * @brief Get a ref to the singleton instance of the PacketFactory
	 * 
	 */
	static PacketFactory& GetInstance();

	/**
	 * @brief Will try to extract a packet from the connection's buffer
	 * 
	 * @param connection 
	 * @return a pair containing a bool indicating if the extraction was sucessful and a shared pointer to the packet
	 */
	std::pair<bool, std::shared_ptr<IPacket> > extractPacket(Connection & connection);
private:
	/**
	 * @brief returned struct containing information about a
	 * potential packet inside a connection buffer
	 */
	struct packetInfo
	{
		/**
		 * @brief true if there is a complete packet in the connection buffer.
		 * 
		 */
		bool complete;

		/**
		 * @brief the type of the packet.
		 * 
		 */
		IPacket::Type type;

		/**
		 * @brief the size in bytes of the packet/
		 * 
		 */
		ssize_t size;
	};

	/**
	 * @brief Construct a new Packet Factory object
	 * Private constructor to prevent instanciation since this class
	 * is a singleton.
	 */
	PacketFactory();

	/**
	 * @brief Get information about the potential packet in a buffer.
	 * 
	 * @param buffer 
	 * @param size 
	 * @return a packetInfo struct containing information about the packet
 	*/
	packetInfo getPacketInfo(const uint8_t * buffer, const size_t & size) const;

	/**
	 * @brief a map containing a instance of every packet type indexed by their type
	 * 
	 * @details This map is used in 2 ways:  
	 * 1. To get the size of a packet, the Size() function a virtual method of IPacket is called,
	 *  some packets have a dynamic size ( eg the playerListPacket ) so there is a method named HasDynamicSize() that returns true in that case
	 * 	if there is a dynamic size then the packet's size is encoded in the next bytes on the buffer
	 * 2. To get a new instance of a packet, the IPacket() Clone method is called
	 * 
	 *  The packets are inserted in the packetFactory constructor,
	 * 	so if a new packet is added it should be added there.
	 *  It would be better to find a way to not have this map, the cloning part could easily be replaced by a switch statement,
	 *  however the size part would be harder to replace without having another place where a packet implementor would need to add their packet.
	 */
	std::unordered_map<IPacket::Type, std::shared_ptr<IPacket>> m_packets;	
};
