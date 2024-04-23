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
#include "ThreadPool.hpp"
#include "Block.hpp"
#include "Chunk.hpp"
#include "WorldGenerator.hpp"
#include "Player.hpp"
#include "logger.hpp"
#include "hashes.hpp"
#include "CreateMeshData.hpp"

#define CHUNKS_PER_WORLD 16

/**
 * @brief A Class that represents a game world
 * 
 * @details This class is responsible for managing the game world,
 * it is responsible for loading and unloading chunks, updating the world and generating new chunks.
 * It will be responsible for the multiplayer aspect of the game as well
 */
class World
{
public:
	World(
		WorldScene & WorldScene,
		VulkanAPI & vulkanAPI,
		ThreadPool & threadPool
	);
	~World();

	/**
	 * @brief Will update the world status based on the new player position
	 * (load and unload chunks, update the player position, etc.)
	 * 
	 * @param playerPosition 
	 */
	void update(glm::dvec3 playerPosition);

	// std::unordered_map<glm::ivec3, Chunk> & chunks() { return m_chunks; }

	WorldGenerator m_worldGenerator;
private:

	typedef std::list<std::future<void>>::iterator future_list_iterator;

	WorldScene &							m_worldScene;
	VulkanAPI &								m_vulkanAPI;
	ThreadPool &							m_threadPool;


	std::unordered_map<glm::ivec3, Chunk>	m_chunks;
	std::mutex								m_chunks_mutex;
	std::unordered_set<glm::ivec3>			m_visible_chunks;
	Player 									m_player;


	std::unordered_set<glm::ivec2>			m_loaded_columns;
	std::unordered_set<glm::ivec2>			m_visible_columns;
	std::mutex								m_visible_columns_mutex;

	std::unordered_map<uint64_t, std::future<void>> m_futures;

	std::queue<uint64_t>					m_finished_futures;
	std::mutex								m_finished_futures_mutex;

	uint64_t								m_future_id = 0;


	void	updateChunks(const glm::vec3 & nextPlayerPosition);

	/**
	 * @brief Will launch tasks to load the chunks around the player
	 * @warning This function will not wait for the tasks to finish
	 * @warning You MUST lock the m_chunks_mutex before calling this function
	 * 
	 * @param nextPlayerChunk2D 
	 */
	void	loadChunks(const glm::ivec2 & nextPlayerChunk2D);
	void	unloadChunks(const glm::ivec2 & nextPlayerChunk2D);
	void	meshChunks(const glm::ivec2 & nextPlayerChunk2D);
	void	waitForFinishedTasks();

	//TIMING UTILS
	
};
