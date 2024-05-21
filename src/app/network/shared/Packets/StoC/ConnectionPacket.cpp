#include "ConnectionPacket.hpp"

ConnectionPacket::ConnectionPacket()
{
}

ConnectionPacket::ConnectionPacket(uint8_t id, glm::vec3 position)
	: m_id(id), m_position(position)
{
}

ConnectionPacket::~ConnectionPacket()
{
}

void ConnectionPacket::Serialize(uint8_t * buffer) const
{
	buffer[0] = m_id;
	memcpy(buffer + 1, &m_position, sizeof(glm::vec3));
}

void ConnectionPacket::Deserialize(const uint8_t * buffer)
{
	m_id = buffer[0];
	memcpy(&m_position, buffer + 1, sizeof(glm::vec3));
}

uint32_t ConnectionPacket::Size() const
{
	return sizeof(uint8_t) + sizeof(glm::vec3);
}

std::shared_ptr<IPacket> ConnectionPacket::Clone() const
{
	return std::make_shared<ConnectionPacket>();
}

void ConnectionPacket::Handle(World& world) const
{
	//idk
}
