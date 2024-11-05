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
	Deserialize(connection.getReadBuffer().data());
	connection.reduceReadBuffer(Size());
}
