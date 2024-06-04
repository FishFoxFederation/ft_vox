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
		case IPacket::Type::DISCONNECT:
		{
			handleDisconnectPacket(std::dynamic_pointer_cast<DisconnectPacket>(packet));
			break;
		}
		case IPacket::Type::BLOCK_ACTION:
		{
			mirrorPacket(packet);
			break;
		}
		case IPacket::Type::PING:
		{
			m_server.send(packet);
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
	std::shared_ptr<IPacket> packet_to_send = std::make_shared<ConnectionPacket>(*packet);
	packet_to_send->SetConnectionId(packet->GetConnectionId());
	m_server.sendAllExcept(packet_to_send, packet->GetConnectionId());


	//send all other players to new player
	std::vector<PlayerListPacket::PlayerInfo> players;
	for(auto player : m_player_positions)
		players.push_back(PlayerListPacket::PlayerInfo{player.first, player.second});
	packet_to_send = std::make_shared<PlayerListPacket>(players);
	packet_to_send->SetConnectionId(packet->GetConnectionId());
	m_server.send(packet_to_send);

	//add new player to list
	m_player_positions[packet->GetPlayerId()] = packet->GetPosition();

	//add new player to connection id map
	m_player_to_connection_id[packet->GetPlayerId()] = packet->GetConnectionId();
	m_connection_to_player_id[packet->GetConnectionId()] = packet->GetPlayerId();

	LOG_INFO("NEW PLAYER ID: " << packet->GetPlayerId());
}

void ServerPacketHandler::handleDisconnectPacket(std::shared_ptr<DisconnectPacket> packet)
{

	auto connection_id = packet->GetConnectionId();
	auto player_id = m_connection_to_player_id[connection_id];

	LOG_INFO("DISCONNECT PACKET: " << player_id);
	m_player_positions.erase(player_id);
	m_player_to_connection_id.erase(player_id);
	m_connection_to_player_id.erase(connection_id);

	auto packet_to_send = std::make_shared<DisconnectPacket>(player_id);
	packet_to_send->SetConnectionId(connection_id);
	m_server.sendAll(packet_to_send);
}

void ServerPacketHandler::handlePlayerMovePacket(std::shared_ptr<PlayerMovePacket> packet)
{
	// LOG_INFO("Player move: " << packet->GetId());
	auto packet_to_send = std::make_shared<PlayerMovePacket>(*packet);
	packet_to_send->SetConnectionId(packet->GetConnectionId());
	m_server.sendAll(packet_to_send);
	m_player_positions[packet->GetPlayerId()] = packet->GetPosition();
}

void ServerPacketHandler::mirrorPacket(std::shared_ptr<IPacket> packet)
{
	m_server.sendAll(packet);
}

void ServerPacketHandler::relayPacket(std::shared_ptr<IPacket> packet)
{
	m_server.sendAllExcept(packet, packet->GetConnectionId());
}
