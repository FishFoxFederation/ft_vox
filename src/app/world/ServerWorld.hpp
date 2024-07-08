#pragma once 


#include "Server.hpp"
#include "Packets.hpp"
#include "World.hpp"
#include "ThreadPool.hpp"
#include "server_define.hpp"
#include "IncomingPacketList.hpp"
#include <unordered_set>

class ServerWorld : public World
{
public:
	ServerWorld(Server & server);
	~ServerWorld();

	ServerWorld(ServerWorld & other) = delete;
	ServerWorld(ServerWorld && other) = delete;
	ServerWorld & operator=(ServerWorld & other) = delete;
	ServerWorld & operator=(ServerWorld && other) = delete;

	struct ChunkLoadUnloadData
	{
		std::vector<std::shared_ptr<Chunk>>	chunks_to_load;
		std::vector<glm::ivec3>				chunks_to_unload;
	};

	std::shared_ptr<Chunk>	getAndLoadChunk(const glm::ivec3 & chunk_position);

	void					loadChunk(const glm::ivec3 & chunk_position);

	void					setBlock(const glm::vec3 & position, BlockID block);

	ChunkLoadUnloadData			getChunksToUnload(
		const glm::vec3 & old_player_position,
		const glm::vec3 & new_player_positiom
	);

	void					update();

	/*********************************\
	 * TICKET MANAGER
	\*********************************/
	constexpr static int TICKET_LEVEL_ENTITY_UPDATE = 31;
	constexpr static int TICKET_LEVEL_BLOCK_UPDATE = 32;
	constexpr static int TICKET_LEVEL_BORDER = 33;
	constexpr static int TICKET_LEVEL_INACTIVE = 34;
	struct Ticket
	{
		int level;
		glm::ivec3 position;
	};

	std::vector<glm::ivec3> getBlockUpdateChunks() const;
	std::vector<glm::ivec3> getEntityUpdateChunks() const;
	std::vector<Ticket>		getTickets() const;

	void					addTicket(const Ticket & ticket);
	void 					removeTicket(const Ticket & ticket);

	/***********************************\
	 * NETWORK
	\***********************************/
	void handlePacket(std::shared_ptr<IPacket> packet);
	void handleBlockActionPacket(std::shared_ptr<BlockActionPacket> packet);
	void handleConnectionPacket(std::shared_ptr<ConnectionPacket> packet);
	void handlePlayerMovePacket(std::shared_ptr<PlayerMovePacket> packet);
	void handleDisconnectPacket(std::shared_ptr<DisconnectPacket> packet);
	void sendChunkLoadUnloadData(const ChunkLoadUnloadData & data, uint64_t player_id);


private:
	/*********************************\
	 * NETWORK
	\*********************************/
	Server & m_server;
	std::unordered_map<uint64_t, uint64_t> m_player_to_connection_id;
	std::unordered_map<uint64_t, uint64_t> m_connection_to_player_id;
	IncomingPacketList m_incoming_packets;



	/*********************************\
	 * TICKET MANAGER
	\*********************************/
	std::vector<Ticket> m_active_tickets;
	std::vector<Ticket> m_tickets_to_add;
	std::vector<Ticket> m_tickets_to_remove;

	std::unordered_set<glm::ivec3> m_block_update_chunks;
	std::unordered_set<glm::ivec3> m_entity_update_chunks;
	std::unordered_set<glm::ivec3> m_border_chunks;
	
	mutable TracyLockableN(std::mutex, m_tickets_mutex, "TicketManager");
	void floodFill(const std::vector<Ticket> & tickets);
	void clearChunksLoadLevel();
	void updateTickets();



	/*********************************\
	 * MISCELLANEOUS
	\*********************************/
	std::unordered_map<uint64_t, glm::dvec3> m_old_player_positions;

	ChunkLoadUnloadData updateChunkObservations(const uint64_t & player_id);

	void updatePlayerPositions();
};
