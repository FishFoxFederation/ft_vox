#pragma once

#include <memory>

#include "Connection.hpp"

struct HandleArgs;

/**
 * @brief an abstract class that servers as a base for all network packets
 * 
 * @details This class is used to define the interface of a packet,
 *  It has an enum for packet types,
 * pure virtual methods to serialize and deserialize the packet,
 * pure virtual methods to return the size of the packet
 * and if that packet has a dynamic size
 * 
 * #Packet header
 * A typical packet header in memory would look like this
 * 
 * [Type][Size ( if dynamic size)]
 * 
 * a whole packet in memory would look like this
 * 
 * [Header][Data]
 * 
 * The size returned by the Size() method is the size of the whole packet including the header
 * 
 * Some packet have dynamic size, for example a packet that contains a list of players,
 * the size of the packet will depend on the number of players connected
 * this add some obligations to the implementation and manipulation of the packet
 * 
 * 1. The Size() Method is no longer valid to call on an empty instance of the packet,
 *  it will only be called by a sender that wants to know the size he has to reserve in his send buffer
 * 2. The HasDynamicSize() method will return true
 * 3. The Serialize() method will have to write the size of the packet in the buffer
 * 4. The Deserialize() method will have to read the size of the packet from the buffer
 */
class IPacket
{
public:

	//ENUM_MAX SHOULD always be last element of the enum
	//it is used for iteration
	//you MUST NOT put custom values in the enum
	// eg (CONNECTION = 1) that is forbidden
	enum class Type : uint32_t
	{
		CONNECTION,
		PLAYER_CONNECTED,
		PLAYER_MOVE,
		DISCONNECT,
		BLOCK_ACTION,
		PING,
		PLAYER_LIST,
		CHUNK,
		CHUNK_REQUEST,
		CHUNK_UNLOAD,
		LOAD_DISTANCE,
		ENUM_MAX,
		ENUM_MIN = 0
	};
	const static inline uint32_t STATIC_HEADER_SIZE = sizeof(Type);
	const static inline uint32_t DYNAMIC_HEADER_SIZE = sizeof(Type) + sizeof(size_t);

	virtual ~IPacket();

	IPacket(const IPacket&);
	IPacket& operator=(const IPacket&);
	IPacket(IPacket&&);
	IPacket& operator=(IPacket&&);

	/**
	 * @brief will write the packet in the buffer, the buffer must at least be of size Size()
	 * 
	 * @param buffer 
	 */
	virtual void			Serialize(uint8_t * buffer) const = 0;

	/**
	 *@brief Extracts the implemented packet from the connection
	 *  
	 * @warning the connection MUST have the complete packet in its read buffer
	 *   this method should only be called by the PacketFactory
	 * 
	 *   moreover, the connection's read Mutex MUST be locked by the caller
	 * @param connection 
	 */
	void					ExtractMessage(Connection & connection);

	/**
	 * @brief return the size of the packet in bytes,
	 * usually used to reserve the right amount of memory in the send buffer
	 * 
	 * or to check if a connection contains a complete packet
	 * 
	 * @warning if the packet has a dynamic size YOU CANNOT call this method to check if the packet is complete,
	 * you MUST read the size from the packet header.
	 * 
	 * However you can still use it on an instance of a packet that has some data in it to estimate the size of the packet
	 * 
	 * @return uint32_t 
	 */
	virtual uint32_t		Size() const = 0;

	/**
	 * @brief Tell if the packet has a dynamic size
	 * 
	 * @return true 
	 * @return false 
	 */
	virtual bool			HasDynamicSize() const = 0;

	/**
	 * @brief Get the Type of the packet
	 * 
	 * @return enum Type 
	 */
	virtual enum Type		GetType() const = 0;

	/**
	 * @brief Create a new instance of the packet
	 * 
	 * @return std::shared_ptr<IPacket> 
	 */
	virtual std::shared_ptr<IPacket> Clone() const = 0;

	/**
	 * @brief Get the Connection Id ( eg source of the packet or destination if it's a response)
	 * 
	 * @return uint64_t 
	 */
	uint64_t		GetConnectionId() const { return m_connection_id; }

	/**
	 * @brief Set the Connection Id ( eg source of the packet or destination if it's a response)
	 * 
	 * @param connection_id 
	 */
	void			SetConnectionId(uint64_t connection_id) { m_connection_id = connection_id; }
protected:
	/**
	 * @brief read the packet from the buffer, the buffer must at least be of size Size()
	 * 
	 * @note this is protected because the method the user should call
	 * is ExtractMessage
	 * @param buffer 
 	*/
	virtual void		Deserialize(const uint8_t * buffer) = 0;

	/**
	 * @brief util function to serialize the header of the packet
	 * will serialize the type and the size of the packet if it has a dynamic size
	 * 
	 * @param buffer 
	 * @return size_t the number of bytes written
	 */
	size_t SerializeHeader(uint8_t * buffer) const;

	IPacket();
	uint64_t m_connection_id = 0;
private:

};
