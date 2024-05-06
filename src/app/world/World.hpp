#pragma once

#include "define.hpp"

#include "List.hpp"
#include "Entity.hpp"
#include "WorldScene.hpp"
#include "WorldGenerator.hpp"
#include "logger.hpp"
#include "CreateMeshData.hpp"
#include "ThreadPool.hpp"
#include "VulkanAPI.hpp"
#include "Camera.hpp"
#include "ECS.hpp"

#include <unordered_map>

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


	void updateEntities(
		const double delta_time
	);
	void updateBlock(glm::dvec3 position);

	void updatePlayer(
		const uint64_t player_id,
		const int8_t forward,
		const int8_t backward,
		const int8_t left,
		const int8_t right,
		const int8_t up,
		const int8_t down
	);
	void updatePlayer(
		const uint64_t player_id,
		std::function<void(Player &)> update
	);
	Camera getCamera(const uint64_t player_id);
	glm::dvec3 getPlayerPosition(const uint64_t player_id);

	uint64_t m_my_player_id;

private:
	WorldScene &							m_worldScene;
	VulkanAPI &								m_vulkanAPI;
	ThreadPool &							m_threadPool;

	IdList<uint64_t, std::shared_ptr<Entity>> m_entities;
	ECS										m_ecs;
	ECS::Entity								m_player_entity;

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
	 * @brief Will load, unload and mesh chunks around the player
	 *
	 * @param playerPosition
	 */
	void 	updateChunks(const glm::vec3 & playerPosition);


	/*************************************
	 *  FUTURES
	 *************************************/
	void	waitForFinishedFutures();
	void	waitForFutures();

	/*************************************
	 *  ENTITIES
	 *************************************/
	bool 	hitboxCollisionWithBlock(const HitBox & hitbox, const glm::dvec3 & position);

};

class ClientWorld
{

};
