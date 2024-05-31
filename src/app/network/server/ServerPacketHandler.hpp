#pragma once

#include "Packets.hpp"
#include "World.hpp"

class ServerPacketHandler
{
public:
	ServerPacketHandler(Server & server);
	~ServerPacketHandler();

	ServerPacketHandler(const ServerPacketHandler& other) = delete;
	ServerPacketHandler& operator=(const ServerPacketHandler& other) = delete;
	ServerPacketHandler(ServerPacketHandler&& other) = delete;
	ServerPacketHandler& operator=(ServerPacketHandler&& other) = delete;

	void handlePacket(std::shared_ptr<IPacket> packet);

private:
	Server &	m_server;

	void handleConnectionPacket(std::shared_ptr<ConnectionPacket> packet);
	void handlePlayerMovePacket(std::shared_ptr<PlayerMovePacket> packet);
};
