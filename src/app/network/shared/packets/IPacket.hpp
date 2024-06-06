#pragma once

#include <memory>

#include "Connection.hpp"

struct HandleArgs;


class IPacket
{
public:
	enum class Type : uint32_t
	{
		CONNECTION,
		PLAYER_CONNECTED,
		PLAYER_MOVE,
		ENTITY_MOVE,
		DISCONNECT,
		BLOCK_ACTION,
		PING,
		PLAYER_LIST,
		CHUNK,
		CHUNK_REQUEST,
		CHUNK_UNLOAD,
		ENUM_MAX,
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
	 * @brief will extract the packet from the connection, the connection must have enough data to read the packet
	 * so you should extract the header first
	 * 
	 * @param connection 
	 */
	void					ExtractMessage(Connection & connection);

	/**
	 * @brief return the size of the packet in bytes
	 * 
	 * @warning if the packet has a dynamic size, you cannot use this function on an empty instance of a packet to estimate the size of a buffer
	 * you MUST read the header of the packet, but for any other case, you can use this function
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
	 * @param buffer 
 	*/
	virtual void		Deserialize(const uint8_t * buffer) = 0;
	IPacket();
	uint64_t m_connection_id = 0;
private:
};
