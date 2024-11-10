#include "IPacket.hpp"

IPacket::IPacket()
{
}

IPacket::IPacket(const IPacket & other)
{
	m_connection_id = other.m_connection_id;
}

IPacket & IPacket::operator=(const IPacket & other)
{
	m_connection_id = other.m_connection_id;
	return *this;
}

IPacket::IPacket(IPacket && other)
{
	m_connection_id = other.m_connection_id;
}

IPacket & IPacket::operator=(IPacket && other)
{
	m_connection_id = other.m_connection_id;
	return *this;
}

IPacket::~IPacket()
{
}

void IPacket::ExtractMessage(Connection & connection)
{
	ZoneScoped;
	m_connection_id = connection.getConnectionId();
	Deserialize(connection.getReadBufferPtr());
	connection.reduceReadBuffer(Size());
}

size_t IPacket::SerializeHeader(uint8_t * buffer) const
{
	auto type = GetType();
	memcpy(buffer, &type, sizeof(type));
	buffer += sizeof(type);

	if (HasDynamicSize())
	{
		auto size = Size();
		memcpy(buffer, &size, sizeof(size));
		return DYNAMIC_HEADER_SIZE;
	}
	return STATIC_HEADER_SIZE;
}
