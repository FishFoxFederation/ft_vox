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

	void 	loadChunks(const glm::vec3 & playerPosition);
	void 	loadChunks(const std::vector<glm::vec3> & playerPositions);
	void	unloadChunks(const glm::vec3 & playerPosition);
	void	unloadChunks(const std::vector<glm::vec3> & playerPositions);

	void	renderChunks(const glm::vec3 & playerPosition);

	void	addColumnToLoadUnloadQueue(const glm::vec3 & nextPlayerPosition);
};
