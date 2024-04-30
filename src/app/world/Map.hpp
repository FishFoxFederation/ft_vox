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
#include "logger.hpp"
#include "hashes.hpp"
#include "CreateMeshData.hpp"


/**
 * @brief
 *
 */
class Map
{
public:
	Map(
		WorldScene & WorldScene,
		VulkanAPI & vulkanAPI,
		ThreadPool & threadPool
	);
	~Map();

	void update(glm::dvec3 playerPosition);

	WorldGenerator m_worldGenerator;
private:

	typedef std::list<std::future<void>>::iterator future_list_iterator;

	WorldScene &							m_worldScene;
	VulkanAPI &								m_vulkanAPI;
	ThreadPool &							m_threadPool;

	std::unordered_map<glm::ivec3, Chunk>	m_chunks;
	std::unordered_set<glm::ivec2>			m_loaded_columns;
	std::mutex								m_chunks_mutex;

	std::unordered_set<glm::ivec2>			m_visible_columns;
	std::mutex								m_visible_columns_mutex;

	std::unordered_set<glm::ivec2> 			m_unload_set;
	std::mutex								m_unload_set_mutex;


	std::unordered_map<uint64_t, std::future<void>> m_futures;

	std::queue<uint64_t>					m_finished_futures;
	std::mutex								m_finished_futures_mutex;

	uint64_t								m_future_id = 0;

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
	 * @warning you must lock m_chunks_mutex as well as m_visible_columns_mutex
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

};
