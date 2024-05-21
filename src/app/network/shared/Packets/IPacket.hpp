#pragma once

#include <memory>
#include "World.hpp"
#include "Server.hpp"

class Server;
class IPacket
{
public:
	virtual ~IPacket() = default;

	IPacket(const IPacket&) = delete;
	IPacket& operator=(const IPacket&) = delete;
	IPacket(IPacket&&) = delete;
	IPacket& operator=(IPacket&&) = delete;

	virtual void		Serialize(uint8_t * buffer) const = 0;
	virtual void		Deserialize(const uint8_t * buffer) = 0;
	virtual uint32_t	Size() const = 0;

	virtual std::shared_ptr<IPacket> Clone() const = 0;

	virtual void	Handle(Server & server) const = 0;

	uint64_t		GetConnectionId() const { return m_connection_id; }
	void			SetConnectionId(uint64_t connection_id) { m_connection_id = connection_id; }
protected:
	IPacket(uint64_t connection_id) : m_connection_id(connection_id) { };
private:
	uint64_t m_connection_id;
};


class IServerToClientPacket : public IPacket
{
};

class IClientToServerPacket : public IPacket
{
};
