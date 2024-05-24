#include "ConnectionPacket.hpp"

ConnectionPacket::ConnectionPacket()
{
}

ConnectionPacket::ConnectionPacket(uint32_t id, glm::vec3 position)
	: m_id(id), m_position(position)
{
}

ConnectionPacket::~ConnectionPacket()
{
}

void ConnectionPacket::Serialize(uint8_t * buffer) const
{
	memcpy(buffer, &m_id, sizeof(uint32_t));
	memcpy(buffer + sizeof(uint32_t), &m_position, sizeof(glm::vec3));
}

void ConnectionPacket::Deserialize(const uint8_t * buffer)
{
	memcpy(&m_id, buffer, sizeof(uint32_t));
	memcpy(&m_position, buffer + sizeof(uint32_t), sizeof(glm::vec3));
}

uint32_t ConnectionPacket::Size() const
{
	return sizeof(uint32_t) + sizeof(glm::vec3);
}

std::shared_ptr<IPacket> ConnectionPacket::Clone() const
{
	return std::make_shared<ConnectionPacket>();
}

void ConnectionPacket::Handle(const HandleArgs & args) const
{
	//here we are in the client side
	if (args.env == HandleArgs::Env::CLIENT)
	{
		args.world->m_my_player_id = m_id;
		args.world->addPlayer(m_id, m_position);
	}
}
