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
	uint32_t type = static_cast<uint32_t>(GetType());
	memcpy(buffer, &type, sizeof(uint32_t));
	buffer += sizeof(uint32_t);

	buffer[0] = m_id;
}

void PlayerConnectedPacket::Deserialize(const uint8_t * buffer)
{
	buffer += sizeof(uint32_t);

	m_id = buffer[0];
}

uint32_t PlayerConnectedPacket::Size() const
{
	return sizeof(IPacket::Type) + sizeof(m_id);
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
