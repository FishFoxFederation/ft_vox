#include "PingPacket.hpp"

PingPacket::PingPacket()
	: m_id(4242)
{
}

PingPacket::PingPacket(uint64_t id)
	: m_id(id)
{
}

PingPacket::PingPacket(const PingPacket & other)
	: m_id(other.m_id)
{
}

PingPacket & PingPacket::operator=(const PingPacket & other)
{
	m_id = other.m_id;
	return *this;
}

PingPacket::PingPacket(PingPacket && other)
	: m_id(other.m_id)
{
}

PingPacket & PingPacket::operator=(PingPacket && other)
{
	if (this != &other)
	{
		IPacket::operator=(other);
		m_id = other.m_id;
	}
	return *this;
}

PingPacket::~PingPacket()
{
}

void PingPacket::Serialize(uint8_t * buffer) const
{
	uint32_t type = static_cast<uint32_t>(GetType());
	memcpy(buffer, &type, sizeof(uint32_t));
	buffer += sizeof(uint32_t);

	memcpy(buffer, &m_id, sizeof(m_id));
}

void PingPacket::Deserialize(const uint8_t * buffer)
{
	buffer += sizeof(uint32_t);

	memcpy(&m_id, buffer, sizeof(m_id));
}

uint32_t PingPacket::Size() const
{
	return sizeof(IPacket::Type) + sizeof(m_id);
}

IPacket::Type PingPacket::GetType() const
{
	return IPacket::Type::PING;
}

std::shared_ptr<IPacket> PingPacket::Clone() const
{
	return std::make_shared<PingPacket>();
}

uint64_t PingPacket::GetId() const
{
	return m_id;
}

void PingPacket::SetId(uint64_t id)
{
	m_id = id;
}
