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
		case IPacket::Type::CHUNK_REQUEST:
		{
			handleChunkRequestPacket(std::dynamic_pointer_cast<ChunkRequestPacket>(packet));
			break;
		}
		case IPacket::Type::PING:
		{
			m_server.send(packet);
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
	m_server.sendAllExcept(packet_to_send, packet->GetConnectionId());


	//send all other players to new player
	// std::vector<PlayerListPacket::PlayerInfo> players;
	// for(auto player : m_player_positions)
	// 	players.push_back(PlayerListPacket::PlayerInfo{player.first, player.second});
	// packet_to_send = std::make_shared<PlayerListPacket>(players);
	// packet_to_send->SetConnectionId(CurrentConnectionId);
	// m_server.send(packet_to_send);


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
	m_server.sendAll(packet_to_send);

	m_world.handleDisconnectPacket(packet);
}

void ServerPacketHandler::handlePlayerMovePacket(std::shared_ptr<PlayerMovePacket> packet)
{
	ZoneScoped;
	// LOG_INFO("Player move: " << packet->GetId());


	//send move to everyone	
	auto packet_to_send = std::make_shared<PlayerMovePacket>(*packet);
	packet_to_send->SetConnectionId(packet->GetConnectionId());
	m_server.sendAll(packet_to_send);

	//transfer packet to world
	m_world.handlePlayerMovePacket(packet);

	//for now ignore if player goes above or under the world
	// old_chunk.y = 0;
	// new_chunk.y = 0;
	// if (old_chunk != new_chunk)
	// {
	// 	LOG_INFO("Player moved to new chunk: " << new_chunk.x << " " << new_chunk.y << " " << new_chunk.z);
	// 	auto chunk_data = m_world.getChunksToUnload(old_position, new_position);
	// 	for (auto chunk : chunk_data.chunks_to_load)
	// 	{
	// 		auto packet_to_send = std::make_shared<ChunkPacket>(*chunk);
	// 		packet_to_send->SetConnectionId(packet->GetConnectionId());
	// 		m_server.send(packet_to_send);
	// 	}

	// 	for (auto chunk : chunk_data.chunks_to_unload)
	// 	{
	// 		auto packet_to_send = std::make_shared<ChunkUnloadPacket>(chunk);
	// 		packet_to_send->SetConnectionId(packet->GetConnectionId());
	// 		m_server.send(packet_to_send);
	// 	}
	// }
}

void ServerPacketHandler::handleBlockActionPacket(std::shared_ptr<BlockActionPacket> packet)
{
	ZoneScoped;
	auto packet_to_send = std::make_shared<BlockActionPacket>(*packet);
	packet_to_send->SetConnectionId(packet->GetConnectionId());
	m_world.setBlock(packet->GetPosition(), packet->GetBlockID());
	m_server.sendAll(packet_to_send);
}

void ServerPacketHandler::handleChunkRequestPacket(std::shared_ptr<ChunkRequestPacket> packet)
{
	(void)packet;
	// auto current_connection_id = packet->GetConnectionId();
	// // auto current_player_id = m_connection_to_player_id.at(current_connection_id);
	// // glm::vec3 player_position = m_player_positions.at(current_player_id);

	// glm::ivec3 chunk_pos = packet->GetChunkPos();
	// // glm::ivec3 player_chunk_pos = player_position / CHUNK_SIZE_VEC3;

	// // if (glm::distance(chunk_pos, player_chunk_pos) > SERVER_LOAD_DISTANCE)
	// // {
	// // 	LOG_INFO("Chunk request out of range: " << chunk_pos.x << " " << chunk_pos.y << " " << chunk_pos.z);
	// // 	return;
	// // }

	// Chunk & chunk = m_world.getAndLoadChunk(chunk_pos);
	// auto packet_to_send = std::make_shared<ChunkPacket>(chunk);
	// packet_to_send->SetConnectionId(current_connection_id);
	// m_server.send(packet_to_send);
}

void ServerPacketHandler::mirrorPacket(std::shared_ptr<IPacket> packet)
{
	ZoneScoped;
	m_server.sendAll(packet);
}

void ServerPacketHandler::relayPacket(std::shared_ptr<IPacket> packet)
{
	ZoneScoped;
	m_server.sendAllExcept(packet, packet->GetConnectionId());
}
