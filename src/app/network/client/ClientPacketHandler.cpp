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
		handleConnectionPacket(std::static_pointer_cast<ConnectionPacket>(packet));
		break;
	case IPacket::Type::PLAYER_CONNECTED:
		handlePlayerConnectedPacket(std::static_pointer_cast<PlayerConnectedPacket>(packet));
		break;
	case IPacket::Type::PLAYER_MOVE:
		handlePlayerMovePacket(std::static_pointer_cast<PlayerMovePacket>(packet));
		break;
	default:
		break;
	}
}

void ClientPacketHandler::handleConnectionPacket(std::shared_ptr<ConnectionPacket> packet)
{
	(void)packet;
}

void ClientPacketHandler::handlePlayerConnectedPacket(std::shared_ptr<PlayerConnectedPacket> packet)
{
	m_world.addPlayer(packet->GetId(), glm::vec3(0));
}

void ClientPacketHandler::handlePlayerMovePacket(std::shared_ptr<PlayerMovePacket> packet)
{
	m_world.updatePlayerPosition(packet->GetId(), packet->GetPosition() + packet->GetDisplacement());
}
