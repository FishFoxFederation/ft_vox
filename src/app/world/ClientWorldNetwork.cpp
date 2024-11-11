#include "ClientWorld.hpp"

void ClientWorld::handlePacket(std::shared_ptr<IPacket> packet)
{
	switch (packet->GetType())
	{
	case IPacket::Type::CONNECTION:
		handleConnectionPacket(std::dynamic_pointer_cast<ConnectionPacket>(packet));
		break;
	case IPacket::Type::PLAYER_MOVE:
		handlePlayerMovePacket(std::dynamic_pointer_cast<PlayerMovePacket>(packet));
		break;
	case IPacket::Type::DISCONNECT:
		handleDisconnectPacket(std::dynamic_pointer_cast<DisconnectPacket>(packet));
		break;
	case IPacket::Type::BLOCK_ACTION:
		handleBlockActionPacket(std::dynamic_pointer_cast<BlockActionPacket>(packet));
		break;
	case IPacket::Type::PING:
		handlePingPacket(std::dynamic_pointer_cast<PingPacket>(packet));
		break;
	case IPacket::Type::PLAYER_LIST:
		handlePlayerListPacket(std::dynamic_pointer_cast<PlayerListPacket>(packet));
		break;
	case IPacket::Type::CHUNK:
		handleChunkPacket(std::dynamic_pointer_cast<ChunkPacket>(packet));
		break;
	case IPacket::Type::CHUNK_UNLOAD:
		handleChunkUnloadPacket(std::dynamic_pointer_cast<ChunkUnloadPacket>(packet));
		break;
	case IPacket::Type::LOAD_DISTANCE:
		handleLoadDistancePacket(std::dynamic_pointer_cast<LoadDistancePacket>(packet));
		break;
	default:
		break;
	}
}

void ClientWorld::handleConnectionPacket(std::shared_ptr<ConnectionPacket> packet)
{
	LOG_INFO("Player connected: " << packet->GetPlayerId());
	addPlayer(packet->GetPlayerId(), packet->GetPosition());
}

void ClientWorld::handlePlayerMovePacket(std::shared_ptr<PlayerMovePacket> packet)
{
	updatePlayerPosition(packet->GetPlayerId(), packet->GetPosition() + packet->GetDisplacement());
}

void ClientWorld::handleDisconnectPacket(std::shared_ptr<DisconnectPacket> packet)
{
	LOG_INFO("Player disconnected: " << packet->GetPlayerId());
	removePlayer(packet->GetPlayerId());
}

void ClientWorld::handleBlockActionPacket(std::shared_ptr<BlockActionPacket> packet)
{
	LOG_INFO("Block action: " << packet->GetPosition().x << " " << packet->GetPosition().y << " " << packet->GetPosition().z << " ");
	modifyBlock(packet->GetPosition(), packet->GetBlockID());
}

void ClientWorld::handlePingPacket(std::shared_ptr<PingPacket> packet)
{
	if (packet->GetCounter() == 0)
	{
		auto now = std::chrono::high_resolution_clock::now();
		auto duration = now - m_client.m_pings[packet->GetId()];

		LOG_INFO("Ping: " << packet->GetId() << " " << duration.count() / 1e6 << "ms");
		m_client.m_pings.erase(packet->GetId());
	}
	else
	{
		packet->SetCounter(packet->GetCounter() - 1);
		m_client.send({packet, 0});
	}
}

void ClientWorld::handlePlayerListPacket(std::shared_ptr<PlayerListPacket> packet)
{
	for (auto player : packet->GetPlayers())
		addPlayer(player.id, player.position);
}

void ClientWorld::handleChunkPacket(std::shared_ptr<ChunkPacket> packet)
{
	addChunk(std::move(packet->GetChunk()));
}

void ClientWorld::handleChunkUnloadPacket(std::shared_ptr<ChunkUnloadPacket> packet)
{
	removeChunk(packet->GetChunkPosition());
}

void ClientWorld::handleLoadDistancePacket(std::shared_ptr<LoadDistancePacket> packet)
{
	static bool first = true;
	LOG_INFO("SERVER Load distance: " << packet->GetDistance());
	setServerLoadDistance(packet->GetDistance());
	if (first)
	{
		setRenderDistance(packet->GetDistance());
		first = false;
	}
}
