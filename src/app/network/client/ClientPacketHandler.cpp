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
	case IPacket::Type::PLAYER_CONNECTED:
		handlePlayerConnectedPacket(std::dynamic_pointer_cast<PlayerConnectedPacket>(packet));
		break;
	case IPacket::Type::PLAYER_MOVE:
		handlePlayerMovePacket(std::dynamic_pointer_cast<PlayerMovePacket>(packet));
		break;
	case IPacket::Type::DISCONNECT:
		handleDisconnectPacket(std::dynamic_pointer_cast<DisconnectPacket>(packet));
		break;
	default:
		break;
	}
}

void ClientPacketHandler::handlePlayerConnectedPacket(std::shared_ptr<PlayerConnectedPacket> packet)
{
	LOG_INFO("Player connected: " << packet->GetId());
	m_world.addPlayer(packet->GetId(), glm::vec3(0));
}

void ClientPacketHandler::handlePlayerMovePacket(std::shared_ptr<PlayerMovePacket> packet)
{
	// LOG_DEBUG("RECEIVED POS: " << packet->GetPosition().x << " " << packet->GetPosition().y << " " << packet->GetPosition().z);
	m_world.updatePlayerPosition(packet->GetId(), packet->GetPosition() + packet->GetDisplacement());
}

void ClientPacketHandler::handleDisconnectPacket(std::shared_ptr<DisconnectPacket> packet)
{
	LOG_INFO("Player disconnected: " << packet->GetPlayerId());
	m_world.removePlayer(packet->GetPlayerId());
}
