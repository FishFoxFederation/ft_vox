#pragma once


#include "Server.hpp"
#include "Packets.hpp"
#include "World.hpp"
#include "ThreadPool.hpp"
#include "server_define.hpp"
#include "IncomingPacketList.hpp"
#include "logger.hpp"
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


	void					update();

	/*********************************\
	 * BLOCKS
	\*********************************/
	struct BlockUpdateData
	{
		enum class Type
		{
			PLACE,
			DESTROY,
			UPDATE,
			RANDOM
		};
		Type		type;
		glm::ivec3	position;
		BlockID		block;

	};
	void addBlockUpdate(const BlockUpdateData & data);
	void updateBlocks();
	void loadChunk(const glm::ivec3 & chunk_position);

	void placeBlock(const glm::vec3 & position, BlockID block);
	void setBlock(const glm::vec3 & position, BlockID block);

	ChunkLoadUnloadData			getChunksToUnload(
		const glm::vec3 & old_player_position,
		const glm::vec3 & new_player_position
	);


	/*********************************\
	 * TICKET MANAGER
	\*********************************/
	constexpr static int TICKET_LEVEL_ENTITY_UPDATE = 31;
	constexpr static int TICKET_LEVEL_BLOCK_UPDATE = 32;
	constexpr static int TICKET_LEVEL_BORDER = 33;
	constexpr static int TICKET_LEVEL_INACTIVE = 34;

	constexpr static int TICKET_LEVEL_SPAWN = 32;
	constexpr static int TICKET_LEVEL_PLAYER = 25;

	constexpr static int SERVER_LOAD_DISTANCE = TICKET_LEVEL_INACTIVE - TICKET_LEVEL_PLAYER;
	struct Ticket
	{
		bool operator==(const Ticket & other) const = default;
		int level;
		glm::ivec3 position;
		struct hash
		{
			std::size_t operator()(const ServerWorld::Ticket & ticket) const
			{
				return std::hash<int>()(ticket.level) ^ std::hash<glm::ivec3>()(ticket.position);
			}
		};

		std::size_t hash() const
		{
			return std::hash<int>()(level) ^ std::hash<glm::ivec3>()(position);
		}
	};
	typedef std::unordered_multimap<uint64_t, Ticket> TicketMultiMap;


	const std::unordered_set<glm::ivec3> &	getBlockUpdateChunks() const;
	const std::unordered_set<glm::ivec3> &	getEntityUpdateChunks() const;
	const TicketMultiMap &					getTickets() const;

	uint64_t								addTicket(const Ticket & ticket);
	void									removeTicket(const uint64_t & ticket_id);
	uint64_t								changeTicket(const uint64_t & old_ticket_id, const Ticket & new_ticket);

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
	TracyLockableN(std::mutex, m_players_info_mutex, "PlayerInfoMutex");

	IncomingPacketList m_incoming_packets;



	/*********************************\
	 * TICKET MANAGER
	\*********************************/
	// std::vector<Ticket> m_active_tickets;
	// std::vector<Ticket> m_tickets_to_add;
	// std::vector<Ticket> m_tickets_to_remove;
	TicketMultiMap						m_active_tickets;
	TicketMultiMap						m_tickets_to_add;
	std::unordered_multiset<uint64_t>	m_tickets_to_remove;

	std::unordered_set<glm::ivec3> m_block_update_chunks;
	std::unordered_set<glm::ivec3> m_entity_update_chunks;
	std::unordered_set<glm::ivec3> m_border_chunks;

	mutable TracyLockableN(std::mutex, m_tickets_mutex, "TicketManager");

	/*************************\
	 * METHODS
	\*************************/
	void			floodFill(const TicketMultiMap & tickets);
	void			clearChunksLoadLevel();
	void			updateTickets();

	/*********************************\
	 * BLOCKS
	\*********************************/
	std::queue<BlockUpdateData> m_block_updates;
	mutable TracyLockableN(std::mutex, m_block_updates_mutex, "BlockUpdateQueue");

	uint64_t				asyncGenChunk(const glm::ivec3 & chunk_position);
	ChunkLoadUnloadData		updateChunkObservations(uint64_t player_id);
	void 					removeChunkObservations(std::shared_ptr<Player> player);

	/*********************************\
	 * MISCELLANEOUS
	\*********************************/
	std::unordered_map<uint64_t, glm::dvec3>	m_last_tick_player_positions;
	std::unordered_map<uint64_t, glm::dvec3>	m_current_tick_player_positions;

	std::vector<uint64_t>						m_chunk_futures_ids;
	TracyLockableN(std::mutex, m_chunk_futures_ids_mutex, "ChunkFuturesIds");


	void savePlayerPositions();
	void updatePlayerPositions();
	void waitForChunkFutures();
};


	namespace std
{
	template<>
	struct hash<ServerWorld::Ticket>
	{
		std::size_t operator()(const ServerWorld::Ticket & ticket) const
		{
			return std::hash<int>()(ticket.level) ^ std::hash<glm::ivec3>()(ticket.position);
		}
	};
}
