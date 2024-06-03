#pragma once

#include <memory>

#include "Connection.hpp"

struct HandleArgs;


class IPacket
{
public:
	enum class Type : uint32_t
	{
		CONNECTION = 0,
		PLAYER_CONNECTED = 1,
		PLAYER_MOVE = 2,
		ENTITY_MOVE = 3,
		DISCONNECT = 4,
		BLOCK_ACTION = 5,
		PING = 6,
		ENUM_MAX,
	};
	virtual ~IPacket();

	IPacket(const IPacket&);
	IPacket& operator=(const IPacket&);
	IPacket(IPacket&&);
	IPacket& operator=(IPacket&&);

	virtual void			Serialize(uint8_t * buffer) const = 0;
	void					ExtractMessage(Connection & connection);
	virtual uint32_t		Size() const = 0;
	virtual enum Type	GetType() const = 0;
	virtual std::shared_ptr<IPacket> Clone() const = 0;

	// virtual void	Handle(const HandleArgs & args) const = 0;


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
