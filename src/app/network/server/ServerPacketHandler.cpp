#include "ServerPacketHandler.hpp"

ServerPacketHandler::ServerPacketHandler(Server & server)
	: m_server(server)
{
}

ServerPacketHandler::~ServerPacketHandler()
{
}

void ServerPacketHandler::handlePacket(std::shared_ptr<IPacket> packet)
{
	switch(packet->GetType())
	{
		case IPacket::Type::CONNECTION:
		{
			handleConnectionPacket(std::dynamic_pointer_cast<ConnectionPacket>(packet));			
			break;
		}
		case IPacket::Type::PLAYER_MOVE:
		{
			handlePlayerMovePacket(std::dynamic_pointer_cast<PlayerMovePacket>(packet));
			break;
		}
		default:
		{
			break;
		}
	}
}

void ServerPacketHandler::handleConnectionPacket(std::shared_ptr<ConnectionPacket> packet)
{
	auto packet_to_send = std::make_shared<PlayerConnectedPacket>(packet->GetId());
	packet_to_send->SetConnectionId(packet->GetConnectionId());
	m_server.sendAllExcept(packet_to_send, packet->GetConnectionId());
}

void ServerPacketHandler::handlePlayerMovePacket(std::shared_ptr<PlayerMovePacket> packet)
{
	auto packet_to_send = std::make_shared<PlayerMovePacket>(*packet);
	packet_to_send->SetConnectionId(packet->GetConnectionId());
	m_server.sendAll(packet_to_send);		
}
