#pragma once

#include "define.hpp"

#include "List.hpp"
#include "Player.hpp"
#include "WorldScene.hpp"
#include "WorldGenerator.hpp"
#include "logger.hpp"
#include "CreateMeshData.hpp"
#include "ThreadPool.hpp"
#include "VulkanAPI.hpp"
#include "Camera.hpp"

#include <unordered_map>
#include <shared_mutex>

class World
{

public:

	World(
		WorldScene & WorldScene,
		VulkanAPI & vulkanAPI,
		ThreadPool & threadPool
	);
	~World();

	World(World & other) = delete;
	World(World && other) = delete;
	World & operator=(World & other) = delete;
	World & operator=(World && other) = delete;


	void updateEntities();
	void updateBlock(glm::dvec3 position);

	void updatePlayerPosition(
		const uint64_t player_id,
		const int8_t forward,
		const int8_t backward,
		const int8_t left,
		const int8_t right,
		const int8_t up,
		const int8_t down,
		const double delta_time
	);
	void updatePlayerCamera(
		const uint64_t player_id,
		const double x_offset,
		const double y_offset
	);
	void updatePlayerTargetBlock(
		const uint64_t player_id
	);
	void playerAttack(
		const uint64_t player_id,
		bool attack
	);
	void playerUse(
		const uint64_t player_id,
		bool use
	);
	void updatePlayer(
		const uint64_t player_id,
		std::function<void(Player &)> update
	);

	//Server side 
	uint64_t	createPlayer(const glm::vec3 & position);

	//Client side
	void		addPlayer(const uint64_t player_id, const glm::vec3 & position);

	void		updatePlayerPosition(const uint64_t player_id, const glm::vec3 & position);

	Camera getCamera(const uint64_t player_id);
	glm::dvec3 getPlayerPosition(const uint64_t player_id);

	uint64_t m_my_player_id;

private:

	WorldScene &							m_worldScene;
	VulkanAPI &								m_vulkanAPI;
	ThreadPool &							m_threadPool;

	std::unordered_map<uint64_t, std::shared_ptr<Player>> m_players;

	/*************************************
	 *  CHUNKS AND MAP
	*************************************/
	WorldGenerator							m_worldGenerator;
	std::unordered_map<glm::ivec3, Chunk>	m_chunks;
	std::unordered_set<glm::ivec2>			m_loaded_chunks;
	std::mutex								m_chunks_mutex;

	std::unordered_set<glm::ivec2>			m_visible_chunks;
	std::mutex								m_visible_chunks_mutex;

	std::unordered_set<glm::ivec2> 			m_unload_set;
	std::mutex								m_unload_set_mutex;


	std::unordered_map<uint64_t, std::future<void>> m_futures;

	std::queue<uint64_t>					m_finished_futures;
	std::mutex								m_finished_futures_mutex;

	uint64_t								m_future_id = 0;

	std::queue<std::pair<glm::vec3, BlockID>> m_blocks_to_set;
	std::mutex								m_blocks_to_set_mutex;



	/*********************************************************
	 *
	 *  METHODS
	 *
	*********************************************************/


	/*************************************
	 *  CHUNKS AND MAP
	*************************************/
	/**
	 * @brief will return the block position relative to the chunk
	 *
	 * @param position the position of the block in the world
	 * @return glm::vec3
	 */
	static glm::vec3 	getBlockChunkPosition(const glm::vec3 & position);

	/**
	 * @brief will return the chunk position containing the block
	 *
	 * @param position the position of the block in the world
	 * @return glm::vec3
	 */
	static glm::vec3 	getChunkPosition(const glm::vec3 & position);

	/**
	 * @brief will load all chunks around the player
	 * @warning you must lock m_chunks_mutex before calling this function
	 *
	 * @param playerPosition
	 */
	void 	loadChunks(const glm::vec3 & playerPosition);

	/**
	 * @brief will load all chunks around the players
	 * @warning you must lock m_chunks_mutex before calling this function
	 *
	 * @param playerPositions
	 */
	void 	loadChunks(const std::vector<glm::vec3> & playerPositions);

	/**
	 * @brief will unload chunk that are too far from the player
	 * @warning you must lock m_chunks_mutex and unload_set_mutex before calling this function
	 * @param playerPosition
	 */
	void	unloadChunks(const glm::vec3 & playerPosition);

	/**
	 * @brief will unload chunk that are too far from the players
	 * @warning you must lock m_chunks_mutex and unload_set_mutex before calling this function
	 *
	 * @param playerPositions
	 */
	void	unloadChunks(const std::vector<glm::vec3> & playerPositions);

	/**
	 * @brief will mesh chunks that are meshable around the player
	 * @warning you must lock m_chunks_mutex as well as m_visible_chunks_mutex
	 * before calling this function
	 *
	 * @param playerPosition
	 */
	void	meshChunks(const glm::vec3 & playerPosition);

	/**
	 * @brief will mesh a chunk
	 *
	 * @param chunkPosition
	 */
	void 	meshChunk(const glm::ivec2 & chunkPosition);

	/**
	 * @brief Will load, unload and mesh chunks around the player
	 *
	 * @param playerPosition
	 */
	void 	updateChunks(const glm::vec3 & playerPosition);

	void 	doBlockSets();


	/*************************************
	 *  FUTURES
	 *************************************/
	void	waitForFinishedFutures();
	void	waitForFutures();

	/*************************************
	 *  ENTITIES
	 *************************************/
	bool hitboxCollisionWithBlock(const HitBox & hitbox, const glm::dvec3 & position);

	/*************************************
	 *  RAYCAST
	 *************************************/
	RayCastOnBlockResult rayCastOnBlock(
		const glm::vec3 & origin,
		const glm::vec3 & direction,
		const double max_distance
	);

};

class ClientWorld
{

};
