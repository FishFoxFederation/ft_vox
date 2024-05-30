#include "HandleArgs.hpp"


void PacketHandler::HandlePacket(std::shared_ptr<IPacket> packet)
{
	switch (packet->GetType())
	{
	case packetType::CONNECTION:
		break;
	case packetType::PLAYER_MOVE:
		break;
	case packetType::ENTITY_MOVE:
		break;
	default:
		break;
	}
}
