#pragma once

#include "define.hpp"

#include <glm/vec3.hpp>
#include <unordered_map>
#include <unordered_set>
#include <future>
#include <queue>
#include <deque>
#include <list>

#include "VulkanAPI.hpp"
#include "WorldScene.hpp"
#include "Block.hpp"
#include "Chunk.hpp"
#include "WorldGenerator.hpp"
#include "hashes.hpp"
#include "Player.hpp"
#include "logger.hpp"

#define CHUNKS_PER_WORLD 16

/**
 * @brief 
 * 
 */
class World
{
public:
	World(WorldScene & WorldScene, VulkanAPI & vulkanAPI);
	~World();

	void update(glm::dvec3 playerPosition);

	// std::unordered_map<glm::ivec3, Chunk> & chunks() { return m_chunks; }

	WorldGenerator m_worldGenerator;
private:
	WorldScene &							m_worldScene;
	VulkanAPI &								m_vulkanAPI;
	std::unordered_map<glm::ivec3, Chunk>	m_chunks;
	std::mutex								m_chunks_mutex;
	std::unordered_set<glm::ivec3>			m_visible_chunks;
	Player 									m_player;

	std::list<glm::ivec3>					m_chunk_gen_queue;
	std::mutex								m_chunk_gen_queue_mutex;

	std::list<glm::ivec3>					m_chunk_unload_queue;
	std::mutex								m_chunk_unload_queue_mutex;

	void	doChunkGen(const int & number_of_chunks);
	void	doChunkLoadUnload(const int & number_of_chunks);
	void	addChunksToLoadUnloadQueue(const glm::vec3 & playerPosition);
};
