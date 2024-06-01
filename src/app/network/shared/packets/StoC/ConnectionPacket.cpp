#include "ConnectionPacket.hpp"

ConnectionPacket::ConnectionPacket()
{
}

ConnectionPacket::ConnectionPacket(uint32_t id, glm::vec3 position)
: m_id(id), m_position(position)
{
}

ConnectionPacket::ConnectionPacket(const ConnectionPacket & other)
: m_id(other.m_id), m_position(other.m_position)
{
}

ConnectionPacket & ConnectionPacket::operator=(const ConnectionPacket & other)
{
	if (this != &other)
	{
		m_id = other.m_id;
		m_position = other.m_position;
	}
	return *this;
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

	uint32_t type = static_cast<uint32_t>(GetType());
	memcpy(buffer, &type, sizeof(uint32_t));
	buffer += sizeof(uint32_t);

	memcpy(buffer, &m_id, sizeof(m_id));
	buffer += sizeof(m_id);

	memcpy(buffer, &m_position, sizeof(m_position));
	buffer += sizeof(m_position);
}

void ConnectionPacket::Deserialize(const uint8_t * buffer)
{
	buffer += sizeof(uint32_t);

	memcpy(&m_id, buffer, sizeof(m_id));
	buffer += sizeof(m_id);

	memcpy(&m_position, buffer, sizeof(m_position));
	buffer += sizeof(glm::vec3);
}

uint32_t ConnectionPacket::Size() const
{
	return sizeof(IPacket::Type) + sizeof(m_id) + sizeof(m_position);
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

glm::vec3 ConnectionPacket::GetPosition() const
{
	return m_position;
}

void ConnectionPacket::SetId(uint32_t id)
{
	m_id = id;
}

void ConnectionPacket::SetPosition(glm::vec3 position)
{
	m_position = position;
}
