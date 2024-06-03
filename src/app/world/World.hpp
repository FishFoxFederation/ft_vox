#pragma once

#include "List.hpp"
#include "Player.hpp"
#include "WorldGenerator.hpp"
#include "logger.hpp"
#include "ThreadPool.hpp"
#include "glm/vec3.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>


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

protected:
	std::unordered_map<uint64_t, std::shared_ptr<Player>>	m_players;
	std::mutex												m_players_mutex;
	std::unordered_map<glm::ivec3, Chunk>					m_chunks;
	std::mutex											m_chunks_mutex;

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
};
