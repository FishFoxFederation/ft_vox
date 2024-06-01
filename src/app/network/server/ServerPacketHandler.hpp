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
	std::map<uint64_t, glm::vec3> m_player_positions;
	std::map<uint64_t, uint64_t> m_player_to_connection_id;
	std::map<uint64_t, uint64_t> m_connection_to_player_id;


	void handleConnectionPacket(std::shared_ptr<ConnectionPacket> packet);
	void handlePlayerMovePacket(std::shared_ptr<PlayerMovePacket> packet);
	void handleDisconnectPacket(std::shared_ptr<DisconnectPacket> packet);
	void handleBlockActionPacket(std::shared_ptr<BlockActionPacket> packet);


	void mirrorPacket(std::shared_ptr<IPacket> packet);
	void relayPacket(std::shared_ptr<IPacket> packet);
};
