#include "LoadDistancePacket.hpp"

LoadDistancePacket::LoadDistancePacket()
{
}

LoadDistancePacket::LoadDistancePacket(uint32_t distance)
: m_distance(distance)
{
}

LoadDistancePacket::LoadDistancePacket(const LoadDistancePacket & other)
: IPacket(other), m_distance(other.m_distance)
{
}

LoadDistancePacket::LoadDistancePacket(LoadDistancePacket && other)
: IPacket(other), m_distance(other.m_distance)
{
}

LoadDistancePacket & LoadDistancePacket::operator=(const LoadDistancePacket & other)
{
	if (this != &other)
	{
		m_distance = other.m_distance;
		::IPacket::operator=(other);
	}
	return *this;
}

LoadDistancePacket & LoadDistancePacket::operator=(LoadDistancePacket && other)
{
	if (this != &other)
	{
		m_distance = other.m_distance;
		::IPacket::operator=(other);
	}
	return *this;
}

LoadDistancePacket::~LoadDistancePacket()
{
}

void LoadDistancePacket::Serialize(uint8_t * buffer) const
{
	// HEADER
	buffer += SerializeHeader(buffer);


	// BODY
	memcpy(buffer, &m_distance, sizeof(m_distance));
	buffer += sizeof(m_distance);
}

void LoadDistancePacket::Deserialize(const uint8_t * buffer)
{
	//skip over the header
	buffer += IPacket::STATIC_HEADER_SIZE;

	//read the distance
	memcpy(&m_distance, buffer, sizeof(m_distance));
	buffer += sizeof(m_distance);
}

uint32_t LoadDistancePacket::Size() const
{
	return IPacket::STATIC_HEADER_SIZE + sizeof(m_distance);
}

bool LoadDistancePacket::HasDynamicSize() const
{
	return false;
}

IPacket::Type LoadDistancePacket::GetType() const
{
	return IPacket::Type::LOAD_DISTANCE;
}

std::shared_ptr<IPacket> LoadDistancePacket::Clone() const
{
	return std::make_shared<LoadDistancePacket>(*this);
}

uint32_t LoadDistancePacket::GetDistance() const
{
	return m_distance;
}

void LoadDistancePacket::SetDistance(uint32_t distance)
{
	m_distance = distance;
}
