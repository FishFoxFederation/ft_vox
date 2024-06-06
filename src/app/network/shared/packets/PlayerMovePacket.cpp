#include "PlayerMovePacket.hpp"

PlayerMovePacket::PlayerMovePacket()
	: m_player_id(0), m_position(0), m_displacement(0)
{
}

PlayerMovePacket::PlayerMovePacket(uint64_t id, glm::dvec3 position, glm::dvec3 displacement)
	: m_player_id(id), m_position(position), m_displacement(displacement)
{
}

PlayerMovePacket::PlayerMovePacket(const PlayerMovePacket & other)
	: IPacket(other), m_player_id(other.m_player_id), m_position(other.m_position), m_displacement(other.m_displacement)
{
}

PlayerMovePacket::PlayerMovePacket(PlayerMovePacket && other)
	: IPacket(other), m_player_id(other.m_player_id), m_position(other.m_position), m_displacement(other.m_displacement)
{
}

PlayerMovePacket & PlayerMovePacket::operator=(const PlayerMovePacket & other)
{
	if (this != &other)
	{
		m_player_id = other.m_player_id;
		m_position = other.m_position;
		m_displacement = other.m_displacement;
		::IPacket::operator=(other);
	}
	return *this;
}

PlayerMovePacket & PlayerMovePacket::operator=(PlayerMovePacket && other)
{
	if (this != &other)
	{
		m_player_id = other.m_player_id;
		m_position = other.m_position;
		m_displacement = other.m_displacement;
		::IPacket::operator=(std::move(other));
	}
	return *this;
}

PlayerMovePacket::~PlayerMovePacket()
{
}

void PlayerMovePacket::Serialize(uint8_t * buffer) const
{
	uint32_t type = static_cast<uint32_t>(GetType());
	memcpy(buffer, &type, sizeof(uint32_t));
	buffer += sizeof(uint32_t);

	buffer[0] = m_player_id;
	buffer += sizeof(m_player_id);

	memcpy(buffer, &m_position, sizeof(glm::dvec3));
	buffer += sizeof(glm::dvec3);

	memcpy(buffer, &m_displacement, sizeof(glm::dvec3));

	// LOG_INFO("SERIALIZE: id: " << (int)m_player_id << "\n position: " << m_position.x << " " << m_position.y << " " << m_position.z << "\n displacement: " << m_displacement.x << " " << m_displacement.y << " " << m_displacement.z);
}

void PlayerMovePacket::Deserialize(const uint8_t * buffer)
{
	buffer += sizeof(uint32_t);

	memcpy(&m_player_id, buffer, sizeof(m_player_id));
	buffer += sizeof(m_player_id);

	memcpy(&m_position, buffer, sizeof(glm::dvec3));
	buffer += sizeof(glm::dvec3);

	memcpy(&m_displacement, buffer, sizeof(glm::dvec3));
	buffer += sizeof(glm::dvec3);

	// LOG_INFO("DESERIALIZE: id: " << (int)m_player_id << "\n position: " << m_position.x << " " << m_position.y << " " << m_position.z << "\n displacement: " << m_displacement.x << " " << m_displacement.y << " " << m_displacement.z);
}

uint32_t PlayerMovePacket::Size() const
{
	// packet type + id + position + displacement
	return IPacket::STATIC_HEADER_SIZE + sizeof(m_player_id) + sizeof(glm::dvec3) * 2;
}

bool PlayerMovePacket::HasDynamicSize() const
{
	return false;
}

IPacket::Type PlayerMovePacket::GetType() const
{
	return IPacket::Type::PLAYER_MOVE;
}

std::shared_ptr<IPacket> PlayerMovePacket::Clone() const
{
	return std::make_shared<PlayerMovePacket>();
}

uint64_t PlayerMovePacket::GetPlayerId() const
{
	return m_player_id;
}

glm::dvec3 PlayerMovePacket::GetPosition() const
{
	return m_position;
}

glm::dvec3 PlayerMovePacket::GetDisplacement() const
{
	return m_displacement;
}

void PlayerMovePacket::SetPlayerId(uint64_t id)
{
	m_player_id = id;
}

void PlayerMovePacket::SetPosition(glm::dvec3 position)
{
	m_position = position;
}

void PlayerMovePacket::SetDisplacement(glm::dvec3 displacement)
{
	m_displacement = displacement;
}
