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

	//create new player

	//create player ticket
}

void ServerWorld::sendChunkLoadUnloadData(const ChunkLoadUnloadData & data, uint64_t player_id)
{
	uint64_t connection_id = m_player_to_connection_id.at(player_id);
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
