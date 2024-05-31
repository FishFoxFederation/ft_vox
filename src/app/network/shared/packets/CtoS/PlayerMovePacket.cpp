#include "PlayerMovePacket.hpp"

PlayerMovePacket::PlayerMovePacket()
	: m_id(0), m_position(0), m_displacement(0)
{
}

PlayerMovePacket::PlayerMovePacket(uint8_t id, glm::vec3 position, glm::vec3 displacement)
	: m_id(id), m_position(position), m_displacement(displacement)
{
}

PlayerMovePacket::PlayerMovePacket(const PlayerMovePacket & other)
	: m_id(other.m_id), m_position(other.m_position), m_displacement(other.m_displacement)
{
}

PlayerMovePacket::PlayerMovePacket(PlayerMovePacket && other)
	: m_id(other.m_id), m_position(other.m_position), m_displacement(other.m_displacement)
{
}

PlayerMovePacket & PlayerMovePacket::operator=(PlayerMovePacket && other)
{
	if (this != &other)
	{
		m_id = other.m_id;
		m_position = other.m_position;
		m_displacement = other.m_displacement;
	}
	return *this;
}

PlayerMovePacket::~PlayerMovePacket()
{
}

void PlayerMovePacket::Serialize(uint8_t * buffer) const
{
	buffer[0] = m_id;
	memccpy(buffer + sizeof(uint8_t), &m_position, sizeof(glm::vec3), sizeof(glm::vec3));
	memccpy(buffer + sizeof(uint8_t) + sizeof(glm::vec3), &m_displacement, sizeof(glm::vec3), sizeof(glm::vec3));
}

void PlayerMovePacket::Deserialize(const uint8_t * buffer)
{
	m_id = buffer[0];
	memccpy(&m_position, buffer + sizeof(uint8_t), sizeof(glm::vec3), sizeof(glm::vec3));
	memccpy(&m_displacement, buffer + sizeof(uint8_t) + sizeof(glm::vec3), sizeof(glm::vec3), sizeof(glm::vec3));
}

uint32_t PlayerMovePacket::Size() const
{
	return sizeof(uint8_t) + sizeof(glm::vec3) * 2;
}

IPacket::Type PlayerMovePacket::GetType() const
{
	return IPacket::Type::PLAYER_MOVE;
}

std::shared_ptr<IPacket> PlayerMovePacket::Clone() const
{
	return std::make_shared<PlayerMovePacket>();
}

uint8_t PlayerMovePacket::GetId() const
{
	return m_id;
}

glm::vec3 PlayerMovePacket::GetPosition() const
{
	return m_position;
}

glm::vec3 PlayerMovePacket::GetDisplacement() const
{
	return m_displacement;
}

void PlayerMovePacket::SetId(uint8_t id)
{
	m_id = id;
}

void PlayerMovePacket::SetPosition(glm::vec3 position)
{
	m_position = position;
}

void PlayerMovePacket::SetDisplacement(glm::vec3 displacement)
{
	m_displacement = displacement;
}
