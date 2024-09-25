#pragma once

#include "List.hpp"
#include "Player.hpp"
#include "WorldGenerator.hpp"
#include "logger.hpp"
#include "ThreadPoolAccessor.hpp"
#include "Mob.hpp"
#include "hashes.hpp"
#include "Tracy.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>


#include <unordered_map>
#include <shared_mutex>

class World
{
public:
	World();
	~World();

	World(World & other) = delete;
	World(World && other) = delete;
	World & operator=(World & other) = delete;
	World & operator=(World && other) = delete;

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
	 * @brief Get the Chunk object
	 *
	 * @param position
	 * @retval std::shared_ptr<Chunk> if the chunk exists
	 * @retval nullptr if the chunk does not exist
	 */
	std::shared_ptr<Chunk>	getChunk(const glm::ivec3 & position) const;
	void 					insertChunk(const glm::ivec3 & position, std::shared_ptr<Chunk> chunk);
protected:
	std::unordered_map<uint64_t, std::shared_ptr<Player>>	m_players;
	TracyLockableN											(std::mutex, m_players_mutex, "Players");

	std::unordered_map<uint64_t, std::shared_ptr<Mob>>		m_mobs;
	TracyLockableN											(std::mutex, m_mobs_mutex, "Mobs");
	uint64_t												m_mob_id = 0;

	std::unordered_map<glm::ivec3, std::shared_ptr<Chunk>>	m_chunks;
	mutable TracyLockableN									(std::mutex,	m_chunks_mutex, "Chunks");

	ThreadPoolAccessor 									m_threadPool;


	WorldGenerator										m_world_generator;

	/**
	 * @brief Get a chunk ptr without locking the chunks mutex
	 *
	 * @warning you MUST lock the chunks mutex before calling this function
	 * @param position
	 * @retval std::shared_ptr<Chunk> if the chunk exists
	 * @retval nullptr if the chunk does not exist
	 */
	std::shared_ptr<Chunk> getChunkNoLock(const glm::ivec3 & position) const;


	void					insertChunkNoLock(const glm::ivec3 & position, std::shared_ptr<Chunk> chunk);
	/*************************************
	 *  FUTURES
	 *************************************/
	// void	waitForFinishedFutures();
	// void	waitForFutures();

	/*************************************
	 *  LIGHTS
	 *************************************/

	/**
	 * @brief Update the sky light of the chunk containing the block
	 *
	 * @param block_position the position of the block in the world
	 */
	void updateSkyLight(const glm::ivec3 & block_position);

	/**
	 * @brief Update the block light of the chunk containing the block
	 *
	 * @param block_position the position of the block in the world
	 */
	void updateBlockLight(const glm::ivec3 & block_position);
};
