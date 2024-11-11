#include "ServerPacketHandler.hpp"

ServerPacketHandler::ServerPacketHandler(Server & server, ServerWorld & world)
	: m_server(server), m_world(world)
{
}

ServerPacketHandler::~ServerPacketHandler()
{
}

void ServerPacketHandler::handlePacket(std::shared_ptr<IPacket> packet)
{
	ZoneScoped;
	if (!m_connection_to_player_id.contains(packet->GetConnectionId()) && packet->GetType() != IPacket::Type::CONNECTION)
	{
		LOG_INFO("Packet connection id does not match connection id");
		m_server.disconnect(packet->GetConnectionId());
		return;
	}
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
			m_world.handleBlockActionPacket(std::dynamic_pointer_cast<BlockActionPacket>(packet));
			break;
		}
		case IPacket::Type::PING:
		{
			handlePingPacket(std::dynamic_pointer_cast<PingPacket>(packet));
			break;
		}
		case IPacket::Type::LOAD_DISTANCE:
		{
			m_world.handleLoadDistancePacket(std::dynamic_pointer_cast<LoadDistancePacket>(packet));
			break;
		}
		default:
		{
			LOG_INFO("Unknown packet type: " << static_cast<int>(packet->GetType()));
			break;
		}
	}
}

void ServerPacketHandler::handleConnectionPacket(std::shared_ptr<ConnectionPacket> packet)
{
	ZoneScoped;
	uint64_t CurrentConnectionId = packet->GetConnectionId();
	uint32_t CurrentPlayerId = packet->GetPlayerId();
	glm::vec3 CurrentPlayerPosition = packet->GetPosition();
	glm::vec3 CurrentPlayerChunkPosition = m_world.getChunkPosition(CurrentPlayerPosition);
	CurrentPlayerChunkPosition.y = 0;
	(void)CurrentPlayerChunkPosition;

	//send new player to all other players
	std::shared_ptr<IPacket> packet_to_send = std::make_shared<ConnectionPacket>(*packet);
	packet_to_send->SetConnectionId(CurrentConnectionId);
	m_server.send({packet_to_send, Server::flags::ALLEXCEPT, packet->GetConnectionId()});


	//notify world of new player
	m_world.handleConnectionPacket(packet);


	//add new player to connection id map
	m_player_to_connection_id[CurrentPlayerId] = CurrentConnectionId;
	m_connection_to_player_id[CurrentConnectionId] = CurrentPlayerId;

	LOG_INFO("NEW PLAYER ID: " << CurrentPlayerId);
}

void ServerPacketHandler::handleDisconnectPacket(std::shared_ptr<DisconnectPacket> packet)
{
	ZoneScoped;
	auto connection_id = packet->GetConnectionId();
	auto player_id = m_connection_to_player_id[connection_id];

	LOG_INFO("DISCONNECT PACKET: " << player_id);
	m_player_to_connection_id.erase(player_id);
	m_connection_to_player_id.erase(connection_id);

	auto packet_to_send = std::make_shared<DisconnectPacket>(player_id);
	packet_to_send->SetConnectionId(connection_id);
	m_server.send({packet_to_send, Server::flags::ALL, connection_id});

	m_world.handleDisconnectPacket(packet);
}

void ServerPacketHandler::handlePlayerMovePacket(std::shared_ptr<PlayerMovePacket> packet)
{
	ZoneScoped;

	//send move to everyone	
	auto packet_to_send = std::make_shared<PlayerMovePacket>(*packet);
	packet_to_send->SetConnectionId(packet->GetConnectionId());
	m_server.send({packet_to_send, Server::flags::ALL, packet->GetConnectionId()});

	//transfer packet to world
	m_world.handlePlayerMovePacket(packet);

}

void ServerPacketHandler::handleBlockActionPacket(std::shared_ptr<BlockActionPacket> packet)
{
	ZoneScoped;
	auto packet_to_send = std::make_shared<BlockActionPacket>(*packet);
	packet_to_send->SetConnectionId(packet->GetConnectionId());
	m_world.setBlock(packet->GetPosition(), packet->GetBlockID());
	m_server.send({packet_to_send, Server::flags::ALL, packet->GetConnectionId()});
}

void ServerPacketHandler::handlePingPacket(std::shared_ptr<PingPacket> packet)
{
	ZoneScoped;
	if (packet->GetCounter() == 0)
	{
		auto now = std::chrono::high_resolution_clock::now();
		auto duration = now - m_server.m_pings[packet->GetId()];

		LOG_INFO("Ping: " << packet->GetId() << " " << duration.count() / 1e6 << "ms");
		m_server.m_pings.erase(packet->GetId());
	}
	else
	{
		packet->SetCounter(packet->GetCounter() - 1);
		m_server.ping(packet->GetId());
	}
}
