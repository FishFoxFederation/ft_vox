#pragma once

#include <memory>

#include "Connection.hpp"
struct HandleArgs;

enum class packetType : uint32_t
{
	CONNECTION = 0,
	PLAYER_MOVE = 1,
	ENTITY_MOVE = 2
};

class IPacket
{
public:
	virtual ~IPacket();

	IPacket(const IPacket&) = delete;
	IPacket& operator=(const IPacket&) = delete;
	IPacket(IPacket&&) = delete;
	IPacket& operator=(IPacket&&) = delete;

	virtual void			Serialize(uint8_t * buffer) const = 0;
	void					ExtractMessage(Connection & connection);
	virtual uint32_t		Size() const = 0;
	virtual enum packetType	GetType() const = 0;
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
