#pragma once


#include "Packets.hpp"
#include "Client.hpp"
#include "ClientWorld.hpp"
#include "VulkanAPI.hpp"

class ClientPacketHandler
{
public:
	ClientPacketHandler(Client & client, ClientWorld & world);
	~ClientPacketHandler();

	ClientPacketHandler(const ClientPacketHandler& other) = delete;
	ClientPacketHandler& operator=(const ClientPacketHandler& other) = delete;
	ClientPacketHandler(ClientPacketHandler&& other) = delete;
	ClientPacketHandler& operator=(ClientPacketHandler&& other) = delete;

	void handlePacket(std::shared_ptr<IPacket> packet);
private:
	Client &			m_client;
	ClientWorld &		m_world;

	void handleConnectionPacket(std::shared_ptr<ConnectionPacket> packet);
	void handlePlayerMovePacket(std::shared_ptr<PlayerMovePacket> packet);
	void handleDisconnectPacket(std::shared_ptr<DisconnectPacket> packet);
	void handleBlockActionPacket(std::shared_ptr<BlockActionPacket> packet);
	void handlePingPacket(std::shared_ptr<PingPacket> packet);
	void handlePlayerListPacket(std::shared_ptr<PlayerListPacket> packet);
	void handleChunkPacket(std::shared_ptr<ChunkPacket> packet);
	void handleChunkUnloadPacket(std::shared_ptr<ChunkUnloadPacket> packet);
	void handleLoadDistancePacket(std::shared_ptr<LoadDistancePacket> packet);
};
