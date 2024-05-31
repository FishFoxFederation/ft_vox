#include "ConnectionPacket.hpp"

ConnectionPacket::ConnectionPacket()
{
}

ConnectionPacket::ConnectionPacket(uint32_t id, glm::vec3 position)
	: m_id(id), m_position(position)
{
}

ConnectionPacket::ConnectionPacket(ConnectionPacket && other)
	: m_id(other.m_id), m_position(other.m_position)
{
}

ConnectionPacket & ConnectionPacket::operator=(ConnectionPacket && other)
{
	if (this != &other)
	{
		m_id = other.m_id;
		m_position = other.m_position;
	}
	return *this;
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

IPacket::Type ConnectionPacket::GetType() const
{
	return IPacket::Type::CONNECTION;
}

uint32_t ConnectionPacket::GetId() const
{
	return m_id;
}
