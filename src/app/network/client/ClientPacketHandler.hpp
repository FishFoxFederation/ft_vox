#pragma once


#include "Packets.hpp"
#include "World.hpp"
#include "WorldScene.hpp"
#include "VulkanAPI.hpp"

class ClientPacketHandler
{
public:
	ClientPacketHandler(Client & client, World & world);
	~ClientPacketHandler();

	ClientPacketHandler(const ClientPacketHandler& other) = delete;
	ClientPacketHandler& operator=(const ClientPacketHandler& other) = delete;
	ClientPacketHandler(ClientPacketHandler&& other) = delete;
	ClientPacketHandler& operator=(ClientPacketHandler&& other) = delete;

	void handlePacket(std::shared_ptr<IPacket> packet);
private:
	Client &	m_client;
	World &		m_world;

	void handleConnectionPacket(std::shared_ptr<ConnectionPacket> packet);
	void handlePlayerMovePacket(std::shared_ptr<PlayerMovePacket> packet);
	void handlePlayerConnectedPacket(std::shared_ptr<PlayerConnectedPacket> packet);
};
