#include "ServerWorld.hpp"

void ServerWorld::handlePacket(std::shared_ptr<IPacket> packet)
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
			handleBlockActionPacket(std::dynamic_pointer_cast<BlockActionPacket>(packet));
			break;
		}
		default:
		{
			LOG_INFO("World: Unknown packet type: " << static_cast<int>(packet->GetType()));
			break;
		}
	}
}

void ServerWorld::handleConnectionPacket(std::shared_ptr<ConnectionPacket> packet)
{
	ZoneScoped;
	uint64_t CurrentConnectionId = packet->GetConnectionId();
	uint32_t CurrentPlayerId = packet->GetPlayerId();
	glm::vec3 CurrentPlayerPosition = packet->GetPosition();
	glm::vec3 CurrentPlayerChunkPosition = getChunkPosition(CurrentPlayerPosition);

	std::lock_guard lock(m_players_info_mutex);
	m_player_to_connection_id.insert({CurrentPlayerId, CurrentConnectionId});
	m_connection_to_player_id.insert({CurrentConnectionId, CurrentPlayerId});

	//send player list to player
	std::vector<PlayerListPacket::PlayerInfo> players;
	{
		std::lock_guard lock(m_players_mutex);
		for(auto & [player_id, player] : m_players)
			players.push_back({player_id, player->transform.position});
	}
	auto packet_to_send = std::make_shared<PlayerListPacket>(players);
	packet_to_send->SetConnectionId(CurrentConnectionId);
	m_server.send(packet_to_send);

	//create new player
	std::shared_ptr<Player> player = std::make_shared<Player>();
	player->connection_id = CurrentConnectionId;
	player->player_id = CurrentPlayerId;
	player->transform.position = CurrentPlayerPosition;
	{
		std::lock_guard lock(m_players_mutex);
		m_players.insert({CurrentPlayerId, player});
	}
	//create player ticket
	Ticket ticket{ TICKET_LEVEL_PLAYER, CurrentPlayerChunkPosition };

	//add ticket
	player->player_ticket_id = addTicket(ticket);
}

void ServerWorld::handleDisconnectPacket(std::shared_ptr<DisconnectPacket> packet)
{
	uint64_t connection_id = packet->GetConnectionId();
	//not using the player_id in the packet because you cannot trust
	//the internet only the socket
	uint64_t player_id = 0;
	{
		std::lock_guard lock(m_players_info_mutex);
		player_id = m_connection_to_player_id.at(connection_id); 
		m_connection_to_player_id.erase(connection_id);
		m_player_to_connection_id.erase(player_id);
		m_last_tick_player_positions.erase(player_id);
		m_current_tick_player_positions.erase(player_id);
	}

	std::shared_ptr<Player> player;
	{
		std::lock_guard lock(m_players_mutex);
		player = m_players.at(player_id);
		m_players.erase(player_id);
	}

	//remove player ticket
	std::lock_guard lock(player->mutex);
	removeTicket(player->player_ticket_id);

	//remove player observations
	removeChunkObservations(player);
	{
		// std::lock_guard lock(m_tickets_mutex);
		// for(auto it : m_active_tickets)
			// LOG_INFO ("Active ticket: " << it.position.x << " " << it.position.z << " level: " << it.level);
	}
}

void ServerWorld::handleBlockActionPacket(std::shared_ptr<BlockActionPacket> packet)
{
	LOG_INFO("World: Block action packet pos:" << packet->GetPosition().x << " " << packet->GetPosition().y << " " << packet->GetPosition().z);
	BlockUpdateData data;
	data.position = packet->GetPosition();
	data.block = packet->GetBlockID();
	if (packet->GetAction() == BlockActionPacket::Action::PLACE)
		data.type = BlockUpdateData::Type::PLACE;
	addBlockUpdate(data);
}

void ServerWorld::handlePlayerMovePacket(std::shared_ptr<PlayerMovePacket> packet)
{
	uint64_t player_id = packet->GetPlayerId();
	std::shared_ptr<Player> player;
	{
		std::lock_guard lock(m_players_mutex);
		player = m_players.at(player_id);
	}
	std::lock_guard lock(player->mutex);
	// glm::dvec3 current_position = player->transform.position;
	glm::dvec3 new_position		= packet->GetPosition() + packet->GetDisplacement();

	//apply movement
	player->transform.position	= new_position;
}

void ServerWorld::sendChunkLoadUnloadData(const ChunkLoadUnloadData & data, uint64_t player_id)
{
	uint64_t connection_id = 0;

	connection_id = m_player_to_connection_id.at(player_id);
	for(auto chunk : data.chunks_to_load)
	{
		//send chunk to player
		auto packet_to_send = std::make_shared<ChunkPacket>(*chunk);
		packet_to_send->SetConnectionId(connection_id);
		m_server.send(packet_to_send);
	}

	for(auto chunk_position : data.chunks_to_unload)
	{
		//send chunk unload to player
		auto packet_to_send = std::make_shared<ChunkUnloadPacket>(chunk_position);
		packet_to_send->SetConnectionId(connection_id);
		m_server.send(packet_to_send);
	}
}
