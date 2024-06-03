#include "ClientPacketHandler.hpp"

ClientPacketHandler::ClientPacketHandler(
	Client & client,
	World & world)
:	m_client(client),
	m_world(world)
{
	(void)m_client;
}

ClientPacketHandler::~ClientPacketHandler()
{
}

void ClientPacketHandler::handlePacket(std::shared_ptr<IPacket> packet)
{
	switch (packet->GetType())
	{
	case IPacket::Type::CONNECTION:
		handleConnectionPacket(std::dynamic_pointer_cast<ConnectionPacket>(packet));
		break;
	// case IPacket::Type::PLAYER_CONNECTED:
	// 	handlePlayerConnectedPacket(std::dynamic_pointer_cast<PlayerConnectedPacket>(packet));
	// 	break;
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
	default:
		break;
	}
}

void ClientPacketHandler::handleConnectionPacket(std::shared_ptr<ConnectionPacket> packet)
{
	LOG_INFO("Player connected: " << packet->GetId());
	m_world.addPlayer(packet->GetId(), packet->GetPosition());
}

void ClientPacketHandler::handlePlayerConnectedPacket(std::shared_ptr<PlayerConnectedPacket> packet)
{
	(void)packet;
	// LOG_INFO("Player connected: " << packet->GetId());
	// m_world.addPlayer(packet->GetId());
}

void ClientPacketHandler::handlePlayerMovePacket(std::shared_ptr<PlayerMovePacket> packet)
{
	glm::vec3 new_pos = packet->GetPosition() + packet->GetDisplacement();
	LOG_DEBUG("RECEIVED POS: " << new_pos.x << " " << new_pos.y << " " << new_pos.z);
	m_world.updatePlayerPosition(packet->GetId(), packet->GetPosition() + packet->GetDisplacement());
}

void ClientPacketHandler::handleDisconnectPacket(std::shared_ptr<DisconnectPacket> packet)
{
	LOG_INFO("Player disconnected: " << packet->GetPlayerId());
	m_world.removePlayer(packet->GetPlayerId());
}

void ClientPacketHandler::handleBlockActionPacket(std::shared_ptr<BlockActionPacket> packet)
{
	LOG_INFO("Block action: " << packet->GetPosition().x << " " << packet->GetPosition().y << " " << packet->GetPosition().z << " ");
	m_world.modifyBlock(packet->GetPosition(), packet->GetBlockID());
}

void ClientPacketHandler::handlePingPacket(std::shared_ptr<PingPacket> packet)
{
	auto now = std::chrono::high_resolution_clock::now();
	auto duration = now - m_client.m_pings[packet->GetId()];

	LOG_INFO("Ping: " << packet->GetId() << " " << duration.count() / 1e6 << "ms");
	m_client.m_pings.erase(packet->GetId());
}
