#include "DisconnectPacket.hpp"

DisconnectPacket::DisconnectPacket()
: m_player_id(0)
{
}

DisconnectPacket::DisconnectPacket(uint32_t player_id)
: m_player_id(player_id)
{
}

DisconnectPacket::DisconnectPacket(const DisconnectPacket & other)
: IPacket(other), m_player_id(other.m_player_id)
{

}

DisconnectPacket::DisconnectPacket(DisconnectPacket && other)
: m_player_id(other.m_player_id)
{
}

DisconnectPacket & DisconnectPacket::operator=(DisconnectPacket && other)
{
	if (this != &other)
	{
		m_player_id = other.m_player_id;
		::IPacket::operator=(std::move(other));
	}
	return *this;
}

DisconnectPacket & DisconnectPacket::operator=(const DisconnectPacket & other)
{
	if (this != &other)
	{
		m_player_id = other.m_player_id;
		::IPacket::operator=(other);
	}
	return *this;
}

DisconnectPacket::~DisconnectPacket()
{
}

void DisconnectPacket::Serialize(uint8_t * buffer) const
{
	// HEADER
	buffer += SerializeHeader(buffer);

	// BODY
	std::memcpy(buffer, &m_player_id, sizeof(m_player_id));
	buffer += sizeof(m_player_id);
}

void DisconnectPacket::Deserialize(const uint8_t * buffer)
{
	// skip the header
	buffer += IPacket::STATIC_HEADER_SIZE;

	std::memcpy(&m_player_id, buffer, sizeof(m_player_id));
}

uint32_t DisconnectPacket::Size() const
{
	return IPacket::STATIC_HEADER_SIZE + sizeof(m_player_id);
}

bool DisconnectPacket::HasDynamicSize() const
{
	return false;
}

IPacket::Type DisconnectPacket::GetType() const
{
	return IPacket::Type::DISCONNECT;
}

std::shared_ptr<IPacket> DisconnectPacket::Clone() const
{
	return std::make_shared<DisconnectPacket>();
}

uint32_t DisconnectPacket::GetPlayerId() const
{
	return m_player_id;
}

void DisconnectPacket::SetPlayerId(uint32_t player_id)
{
	m_player_id = player_id;
}
