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

	/**
	 * @brief Get the list of Chunks that need to get a block update tick
	 * 
	 * @return const std::unordered_set<glm::ivec3>& 
	 */
	const std::unordered_set<glm::ivec3> &	getBlockUpdateChunks() const;

	/**
	 * @brief Get the list of Chunks that need to get an entity update tick
	 * 
	 * @return const std::unordered_set<glm::ivec3>& 
	 */
	const std::unordered_set<glm::ivec3> &	getEntityUpdateChunks() const;

	/**
	 * @brief Get the list of Tickets ( May contains duplicates )
	 * 
	 * @return const TicketMultiMap& 
	 */
	const TicketMultiMap &					getTickets() const;

	/**
	 * @brief Add a ticket to the ticket manager
	 * 
	 * The new ticket will be active on the next tick
	 * 
	 * You can keep track of the ticket for further deletion with the ticket id
	 * 
	 * @param ticket 
	 * @return uint64_t 
	 */
	uint64_t								addTicket(const Ticket & ticket);

	/**
	 * @brief Remove a ticket from the ticket manager
	 * 
	 * @param ticket_id 
	 */
	void									removeTicket(const uint64_t & ticket_id);

	/**
	 * @brief Shortcut to remove a ticket and add a new one
	 * 
	 * the new ticket will be active on the next tick
	 * 
	 * you can keep track of the ticket for further deletion with the ticket id
	 * 
	 * @param old_ticket_id 
	 * @param new_ticket 
	 * @return uint64_t 
	 */
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
	/**
	 * @brief will go through every load ticket to add and to remove and update the chunks load levels accordingly,
	 * 
	 * this function will also generate chunks that need to be generated,
	 * 
	 * it will use multiple threads and wait for them to finish
	 * 
	 * @warning NOT thread safe MUST be called from the main thread
 	*/
	void			updateTickets();

	/**
	 * @brief will apply every ticket in the ticket list to the world,
	 * 
	 * @warning NOT thread safe MUST be called from the main thread
	 * @warning will launch async tasks you MUST wait for tasks to finish
	 * 
	 * @param tickets 
	 */
	void			floodFill(const TicketMultiMap & tickets);

	/** @brief util for floodfill */
	void			clearChunksLoadLevel();


	/**
	 * @brief apply a ticket to a chunk
	 * 
	 * will update the load level and add the chunk to the corresponding lists
	 * 
	 * will also generate the chunk to a sufficient level of detail
	 * 
	 * @warning NOT thread safe MUST be called from the main thread
	 * 
	 * @param ticket 
	 */
	void			applyTicketToChunk(const Ticket & ticket);
	/*********************************\
	 * BLOCKS
	\*********************************/
	std::queue<BlockUpdateData> m_block_updates;
	mutable TracyLockableN(std::mutex, m_block_updates_mutex, "BlockUpdateQueue");

	/**
	 * @brief generate a chunk or a region of chunks asynchronously
	 * 
	 * 
	 * @param chunk_position 
	 * @param load_level desired load level
	 * @param current_level current load level
	 * @return the id of the future that will generate the chunk
	 */
	uint64_t				asyncGenChunk(const glm::ivec3 & chunk_position, int load_level, int current_level);
	ChunkLoadUnloadData		updateChunkObservations(uint64_t player_id);
	void 					removeChunkObservations(std::shared_ptr<Player> player);

	/*********************************\
	 * MISCELLANEOUS
	\*********************************/
	std::unordered_map<uint64_t, glm::dvec3>	m_last_tick_player_positions;
	std::unordered_map<uint64_t, glm::dvec3>	m_current_tick_player_positions;

	std::vector<uint64_t>						m_chunk_futures_ids;
	TracyLockableN(std::mutex, m_chunk_futures_ids_mutex, "ChunkFuturesIds");


	
	ChunkMap getChunkZone(glm::ivec3 zoneStart, glm::ivec3 zoneSize);

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
