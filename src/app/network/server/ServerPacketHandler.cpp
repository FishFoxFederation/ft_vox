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
	uint64_t CurrentConnectionId = packet->GetConnectionId();
	uint32_t CurrentPlayerId = packet->GetPlayerId();
	glm::vec3 CurrentPlayerPosition = packet->GetPosition();
	glm::vec3 CurrentPlayerChunkPosition = m_world.getChunkPosition(CurrentPlayerPosition);

	//send new player to all other players
	std::shared_ptr<IPacket> packet_to_send = std::make_shared<ConnectionPacket>(*packet);
	packet_to_send->SetConnectionId(CurrentConnectionId);
	m_server.sendAllExcept(packet_to_send, packet->GetConnectionId());


	//send all other players to new player
	std::vector<PlayerListPacket::PlayerInfo> players;
	for(auto player : m_player_positions)
		players.push_back(PlayerListPacket::PlayerInfo{player.first, player.second});
	packet_to_send = std::make_shared<PlayerListPacket>(players);
	packet_to_send->SetConnectionId(CurrentConnectionId);
	m_server.send(packet_to_send);

	//send chunks around player
	for (int x = -SERVER_LOAD_DISTANCE; x <= SERVER_LOAD_DISTANCE; x++)
	{
		for (int z = -SERVER_LOAD_DISTANCE; z <= SERVER_LOAD_DISTANCE; z++)
		{
			glm::ivec3 chunkPos(x + CurrentPlayerChunkPosition.x, 0, z + CurrentPlayerChunkPosition.z);
			packet_to_send = std::make_shared<ChunkPacket>(*m_world.getAndLoadChunk(chunkPos));
			packet_to_send->SetConnectionId(CurrentConnectionId);
			m_server.send(packet_to_send);
		}
	}
	// packet_to_send = std::make_shared<ChunkPacket>(m_world.getChunk(glm::ivec3(0, 0, 0)));
	// packet_to_send->SetConnectionId(CurrentConnectionId);
	// m_server.send(packet_to_send);

	// packet_to_send = std::make_shared<ChunkPacket>(m_world.getChunk(glm::ivec3(0, 0, 1)));
	// packet_to_send->SetConnectionId(CurrentConnectionId);
	// m_server.send(packet_to_send);

	// packet_to_send = std::make_shared<ChunkPacket>(m_world.getChunk(glm::ivec3(1, 0, 0)));
	// packet_to_send->SetConnectionId(CurrentConnectionId);
	// m_server.send(packet_to_send);

	// packet_to_send = std::make_shared<ChunkPacket>(m_world.getChunk(glm::ivec3(1, 0, 1)));
	// packet_to_send->SetConnectionId(CurrentConnectionId);
	// m_server.send(packet_to_send);




	//add new player to list
	m_player_positions[CurrentPlayerId] = CurrentPlayerPosition;

	//add new player to connection id map
	m_player_to_connection_id[CurrentPlayerId] = CurrentConnectionId;
	m_connection_to_player_id[CurrentConnectionId] = CurrentPlayerId;

	LOG_INFO("NEW PLAYER ID: " << CurrentPlayerId);
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

	glm::vec3 old_position = m_player_positions[packet->GetPlayerId()];
	glm::vec3 new_position = packet->GetPosition();

	//send move to everyone	
	auto packet_to_send = std::make_shared<PlayerMovePacket>(*packet);
	packet_to_send->SetConnectionId(packet->GetConnectionId());
	m_server.sendAll(packet_to_send);

	m_player_positions[packet->GetPlayerId()] = new_position;

	//if player changed chunk, re trigger the chunk load/unload
	glm::ivec3 old_chunk = m_world.getChunkPosition(old_position);
	glm::ivec3 new_chunk = m_world.getChunkPosition(new_position);
	if (old_chunk != new_chunk)
	{
		auto chunk_data = m_world.getChunksToUnload(old_position, new_position);
		for (auto chunk : chunk_data.chunks_to_load)
		{
			auto packet_to_send = std::make_shared<ChunkPacket>(*chunk);
			packet_to_send->SetConnectionId(packet->GetConnectionId());
			m_server.send(packet_to_send);
		}

		for (auto chunk : chunk_data.chunks_to_unload)
		{
			auto packet_to_send = std::make_shared<ChunkUnloadPacket>(chunk);
			packet_to_send->SetConnectionId(packet->GetConnectionId());
			m_server.send(packet_to_send);
		}
	}
}

void ServerPacketHandler::handleBlockActionPacket(std::shared_ptr<BlockActionPacket> packet)
{
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
	m_server.sendAll(packet);
}

void ServerPacketHandler::relayPacket(std::shared_ptr<IPacket> packet)
{
	m_server.sendAllExcept(packet, packet->GetConnectionId());
}
