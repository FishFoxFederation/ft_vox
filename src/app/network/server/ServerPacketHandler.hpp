#pragma once

#include "Packets.hpp"
#include "Server.hpp"
#include "ServerWorld.hpp"
#include "glm/glm.hpp"
#include "server_define.hpp"

#include <unordered_map>

class ServerPacketHandler
{
public:
	ServerPacketHandler(Server & server, ServerWorld & world);
	~ServerPacketHandler();

	ServerPacketHandler(const ServerPacketHandler& other) = delete;
	ServerPacketHandler& operator=(const ServerPacketHandler& other) = delete;
	ServerPacketHandler(ServerPacketHandler&& other) = delete;
	ServerPacketHandler& operator=(ServerPacketHandler&& other) = delete;

	void handlePacket(std::shared_ptr<IPacket> packet);

private:
	Server &	m_server;
	ServerWorld & m_world;
	
	std::unordered_map<uint64_t, uint64_t> m_player_to_connection_id;
	std::unordered_map<uint64_t, uint64_t> m_connection_to_player_id;


	void handleConnectionPacket(std::shared_ptr<ConnectionPacket> packet);
	void handlePlayerMovePacket(std::shared_ptr<PlayerMovePacket> packet);
	void handleDisconnectPacket(std::shared_ptr<DisconnectPacket> packet);
	void handleBlockActionPacket(std::shared_ptr<BlockActionPacket> packet);
	void handlePingPacket(std::shared_ptr<PingPacket> packet);

};
