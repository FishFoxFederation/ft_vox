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
{
	m_player_id = other.m_player_id;
}

PlayerConnectedPacket& PlayerConnectedPacket::operator=(PlayerConnectedPacket&& other)
{
	m_player_id = other.m_player_id;
	return *this;
}

void PlayerConnectedPacket::Serialize(uint8_t * buffer) const
{
	uint32_t type = static_cast<uint32_t>(GetType());
	memcpy(buffer, &type, sizeof(uint32_t));
	buffer += sizeof(uint32_t);

	memcpy(buffer, &m_player_id, sizeof(m_player_id));
}

void PlayerConnectedPacket::Deserialize(const uint8_t * buffer)
{
	//skipping type
	buffer += sizeof(uint32_t);

	memcpy(&m_player_id, buffer, sizeof(m_player_id));
}

uint32_t PlayerConnectedPacket::Size() const
{
	return sizeof(IPacket::Type) + sizeof(m_player_id);
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
