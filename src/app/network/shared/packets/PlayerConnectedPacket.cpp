#include "PlayerConnectedPacket.hpp"

PlayerConnectedPacket::PlayerConnectedPacket()
{
}

PlayerConnectedPacket::PlayerConnectedPacket(const uint32_t & id)
	: m_player_id(id)
{
}

PlayerConnectedPacket::~PlayerConnectedPacket()
{
}

PlayerConnectedPacket::PlayerConnectedPacket(PlayerConnectedPacket&& other)
: IPacket(std::move(other)), m_player_id(other.m_player_id)
{
}

PlayerConnectedPacket::PlayerConnectedPacket(const PlayerConnectedPacket& other)
: IPacket(other), m_player_id(other.m_player_id)
{
}

PlayerConnectedPacket& PlayerConnectedPacket::operator=(const PlayerConnectedPacket& other)
{
	if (this != &other)
	{
		IPacket::operator=(other);
		m_player_id = other.m_player_id;
	}
	return *this;
}

PlayerConnectedPacket& PlayerConnectedPacket::operator=(PlayerConnectedPacket&& other)
{
	if (this != &other)
	{
		IPacket::operator=(std::move(other));
		m_player_id = other.m_player_id;
	}
	return *this;
}

void PlayerConnectedPacket::Serialize(uint8_t * buffer) const
{
	// HEADER
	buffer += SerializeHeader(buffer);

	// DATA
	memcpy(buffer, &m_player_id, sizeof(m_player_id));
}

void PlayerConnectedPacket::Deserialize(const uint8_t * buffer)
{
	//skipping the header
	buffer += IPacket::STATIC_HEADER_SIZE;

	memcpy(&m_player_id, buffer, sizeof(m_player_id));
}

uint32_t PlayerConnectedPacket::Size() const
{
	return IPacket::STATIC_HEADER_SIZE + sizeof(m_player_id);
}

bool PlayerConnectedPacket::HasDynamicSize() const
{
	return false;
}

IPacket::Type PlayerConnectedPacket::GetType() const
{
	return IPacket::Type::PLAYER_CONNECTED;
}

std::shared_ptr<IPacket> PlayerConnectedPacket::Clone() const
{
	return std::make_shared<PlayerConnectedPacket>();
}

uint32_t PlayerConnectedPacket::GetPlayerId() const
{
	return m_player_id;
}

void PlayerConnectedPacket::SetPlayerId(uint32_t id)
{
	m_player_id = id;
}
