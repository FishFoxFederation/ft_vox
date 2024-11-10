#include "ConnectionPacket.hpp"

ConnectionPacket::ConnectionPacket()
{
}

ConnectionPacket::ConnectionPacket(uint32_t id, glm::vec3 position)
	: m_player_id(id), m_position(position)
{
}

ConnectionPacket::ConnectionPacket(const ConnectionPacket & other)
	: IPacket(other), m_player_id(other.m_player_id), m_position(other.m_position)
{
}

ConnectionPacket & ConnectionPacket::operator=(const ConnectionPacket & other)
{
	if (this != &other)
	{
		m_player_id = other.m_player_id;
		m_position = other.m_position;
	}
	return *this;
}

ConnectionPacket::ConnectionPacket(ConnectionPacket && other)
: m_player_id(other.m_player_id), m_position(other.m_position)
{
}

ConnectionPacket & ConnectionPacket::operator=(ConnectionPacket && other)
{
	if (this != &other)
	{
		m_player_id = other.m_player_id;
		m_position = other.m_position;
	}
	return *this;
}

ConnectionPacket::~ConnectionPacket()
{
}

void ConnectionPacket::Serialize(uint8_t * buffer) const
{
	// HEADER
	buffer += SerializeHeader(buffer);


	// BODY
	memcpy(buffer, &m_player_id, sizeof(m_player_id));
	buffer += sizeof(m_player_id);

	memcpy(buffer, &m_position, sizeof(m_position));
	buffer += sizeof(m_position);
}

void ConnectionPacket::Deserialize(const uint8_t * buffer)
{
	// Skip over the header
	buffer += IPacket::STATIC_HEADER_SIZE;

	memcpy(&m_player_id, buffer, sizeof(m_player_id));
	buffer += sizeof(m_player_id);

	memcpy(&m_position, buffer, sizeof(m_position));
	buffer += sizeof(glm::vec3);
}

uint32_t ConnectionPacket::Size() const
{
	return IPacket::STATIC_HEADER_SIZE + sizeof(m_player_id) + sizeof(m_position);
}

bool ConnectionPacket::HasDynamicSize() const
{
	return false;
}

std::shared_ptr<IPacket> ConnectionPacket::Clone() const
{
	return std::make_shared<ConnectionPacket>();
}

IPacket::Type ConnectionPacket::GetType() const
{
	return IPacket::Type::CONNECTION;
}

uint32_t ConnectionPacket::GetPlayerId() const
{
	return m_player_id;
}

glm::vec3 ConnectionPacket::GetPosition() const
{
	return m_position;
}

void ConnectionPacket::SetPlayerId(uint32_t id)
{
	m_player_id = id;
}

void ConnectionPacket::SetPosition(glm::vec3 position)
{
	m_position = position;
}
