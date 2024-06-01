#include "IPacket.hpp"

IPacket::IPacket()
{
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
	m_connection_id = connection.getConnectionId();
	Deserialize(connection.getReadBuffer().data());
	connection.reduceReadBuffer(Size());
}
