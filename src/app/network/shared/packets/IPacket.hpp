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
		ENUM_MAX,
	};
	const static inline uint32_t STATIC_HEADER_SIZE = sizeof(Type);
	const static inline uint32_t DYNAMIC_HEADER_SIZE = sizeof(Type) + sizeof(size_t);

	virtual ~IPacket();

	IPacket(const IPacket&);
	IPacket& operator=(const IPacket&);
	IPacket(IPacket&&);
	IPacket& operator=(IPacket&&);

	virtual void			Serialize(uint8_t * buffer) const = 0;
	void					ExtractMessage(Connection & connection);

	virtual uint32_t		Size() const = 0;
	virtual bool			HasDynamicSize() const = 0;

	virtual enum Type		GetType() const = 0;

	virtual std::shared_ptr<IPacket> Clone() const = 0;


	uint64_t		GetConnectionId() const { return m_connection_id; }
	void			SetConnectionId(uint64_t connection_id) { m_connection_id = connection_id; }
protected:
	virtual void		Deserialize(const uint8_t * buffer) = 0;
	IPacket();
	uint64_t m_connection_id = 0;
private:
};


class IServerToClientPacket : public IPacket
{
};

class IClientToServerPacket : public IPacket
{
};
