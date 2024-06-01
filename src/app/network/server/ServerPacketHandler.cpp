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

	//send new player to all other players
	auto packet_to_send = std::make_shared<PlayerConnectedPacket>(packet->GetId());
	packet_to_send->SetConnectionId(packet->GetConnectionId());
	m_server.sendAllExcept(packet_to_send, packet->GetConnectionId());


	//send all other players to new player
	for(auto & player : m_player_positions)
	{
		auto packet_to_send = std::make_shared<PlayerConnectedPacket>(player.first);
		packet_to_send->SetConnectionId(packet->GetConnectionId());
		m_server.send(packet_to_send);
	}

	//add new player to list
	m_player_positions[packet->GetId()] = packet->GetPosition();
}

void ServerPacketHandler::handlePlayerMovePacket(std::shared_ptr<PlayerMovePacket> packet)
{
	LOG_INFO("Player move: " << packet->GetId());
	auto packet_to_send = std::make_shared<PlayerMovePacket>(*packet);
	packet_to_send->SetConnectionId(packet->GetConnectionId());
	m_server.sendAll(packet_to_send);
	m_player_positions[packet->GetId()] = packet->GetPosition();
}
