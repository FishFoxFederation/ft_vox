#include "PingPacket.hpp"

PingPacket::PingPacket()
	: m_id(4242), m_counter(0)
{
}

PingPacket::PingPacket(uint64_t id, uint8_t counter)
	: m_id(id), m_counter(counter)
{
}

PingPacket::PingPacket(const PingPacket & other)
	: IPacket(other), m_id(other.m_id), m_counter(other.m_counter)
{
}

PingPacket & PingPacket::operator=(const PingPacket & other)
{
	m_id = other.m_id;
	m_counter = other.m_counter;
	return *this;
}

PingPacket::PingPacket(PingPacket && other)
	: m_id(other.m_id), m_counter(other.m_counter)
{
}

PingPacket & PingPacket::operator=(PingPacket && other)
{
	if (this != &other)
	{
		IPacket::operator=(other);
		m_id = other.m_id;
		m_counter = other.m_counter;
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
	buffer += sizeof(m_id);

	memcpy(buffer, &m_counter, sizeof(m_counter));
	buffer += sizeof(m_counter);
}

void PingPacket::Deserialize(const uint8_t * buffer)
{
	buffer += sizeof(uint32_t);

	memcpy(&m_id, buffer, sizeof(m_id));
	buffer += sizeof(m_id);

	memcpy(&m_counter, buffer, sizeof(m_counter));
	buffer+= sizeof(m_counter);
}

uint32_t PingPacket::Size() const
{
	return sizeof(IPacket::Type) + sizeof(m_id) + sizeof(m_counter);
}

bool PingPacket::HasDynamicSize() const
{
	return false;
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

uint8_t PingPacket::GetCounter() const
{
	return m_counter;
}

void PingPacket::SetCounter(uint8_t counter)
{
	m_counter = counter;
}
