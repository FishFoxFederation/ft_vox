#include "IPacket.hpp"

IPacket::IPacket()
{
}

IPacket::~IPacket()
{
}

void IPacket::ExtractMessage(Connection & connection)
{
	m_connection_id = connection.getConnectionId();
	Deserialize(connection.getReadBufferRef().data());
	connection.reduceReadBuffer(Size());
}
