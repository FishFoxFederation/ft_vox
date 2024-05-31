#include "PlayerConnectedPacket.hpp"

PlayerConnectedPacket::PlayerConnectedPacket()
{
}

PlayerConnectedPacket::PlayerConnectedPacket(const uint8_t & id)
	: m_id(id)
{
}

PlayerConnectedPacket::~PlayerConnectedPacket()
{
}

PlayerConnectedPacket::PlayerConnectedPacket(PlayerConnectedPacket&& other)
{
	m_id = other.m_id;
}

PlayerConnectedPacket& PlayerConnectedPacket::operator=(PlayerConnectedPacket&& other)
{
	m_id = other.m_id;
	return *this;
}

void PlayerConnectedPacket::Serialize(uint8_t * buffer) const
{
	buffer[0] = m_id;
}

void PlayerConnectedPacket::Deserialize(const uint8_t * buffer)
{
	m_id = buffer[0];
}

uint32_t PlayerConnectedPacket::Size() const
{
	return sizeof(uint8_t);
}

IPacket::Type PlayerConnectedPacket::GetType() const
{
	return IPacket::Type::PLAYER_CONNECTED;
}

std::shared_ptr<IPacket> PlayerConnectedPacket::Clone() const
{
	return std::make_shared<PlayerConnectedPacket>();
}

uint8_t PlayerConnectedPacket::GetId() const
{
	return m_id;
}

void PlayerConnectedPacket::setId(uint8_t id)
{
	m_id = id;
}
