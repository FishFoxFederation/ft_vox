#include "World.hpp"

World::World(WorldScene & WorldScene, VulkanAPI & vulkanAPI)
: m_worldGenerator()
, m_worldScene(WorldScene)
, m_vulkanAPI(vulkanAPI)
, m_player()
{
	//generate chunks 4 distance from the player
	// for(int x = -2; x < 2; x++)
	// {
	// 	for(int y = -2; y < 2; y++)
	// 	{
	// 		for(int z = -2; z < 2; z++)
	// 		{
	// 			m_chunks.insert(std::make_pair(glm::ivec3(x, y, z), m_worldGenerator.generateChunk(x, y, z)));
	// 			m_visible_chunks.insert(glm::ivec3(x, y, z));
	// 		}
	// 	}
	// }
}

World::~World()
{
}

void World::doChunkLoadUnload(const int & number_of_tasks)
{
	std::vector<std::future<void>> futures;
	{
		std::lock_guard<std::mutex> lock(m_chunk_gen_queue_mutex);
		std::lock_guard<std::mutex> lock2(m_chunks_mutex);

		for(int i = 0; i < number_of_tasks / 2 && !m_chunk_gen_queue.empty(); i++)
		{
			glm::ivec3 chunkPos = m_chunk_gen_queue.front();
			m_chunk_gen_queue.pop_front();
			if(m_chunks.contains(chunkPos))
				continue;
			LOG_INFO("Generating chunk: " << chunkPos.x << " " << chunkPos.y << " " << chunkPos.z);
			futures.push_back(std::async(std::launch::async, [this, chunkPos]()
			{
				Chunk * chunk = nullptr;
				{
					std::lock_guard<std::mutex> lock(m_chunks_mutex);
					auto [it, success] = m_chunks.insert(std::make_pair(chunkPos, m_worldGenerator.generateChunk(chunkPos.x, chunkPos.y, chunkPos.z)));
					chunk = &it->second;
				}

				uint64_t mesh_id = m_vulkanAPI.createMesh(*chunk);
				chunk->setMeshID(mesh_id);
				if(mesh_id != VulkanAPI::no_mesh_id)
					m_worldScene.addMeshData(mesh_id, glm::vec3(chunkPos * CHUNK_SIZE));
			}));
		}
	}

	{
		std::lock_guard<std::mutex> lock(m_chunk_unload_queue_mutex);
		std::lock_guard<std::mutex> lock2(m_chunks_mutex);


		std::vector<uint64_t> chunk_meshes_id_to_remove;
		for(int i = 0; i < number_of_tasks / 2 && !m_chunk_unload_queue.empty(); i++)
		{
			glm::ivec3 chunkPos = m_chunk_unload_queue.front();
			m_chunk_unload_queue.pop_front();

			futures.push_back(std::async(std::launch::async, [this, chunkPos]()
			{
				std::unique_lock<std::mutex> lock(m_chunks_mutex);
				LOG_INFO("Unloading chunk: " << chunkPos.x << " " << chunkPos.y << " " << chunkPos.z);

				if(!m_chunks.contains(chunkPos))
					return;
				uint64_t mesh_id = m_chunks.at(chunkPos).getMeshID();
				m_chunks.erase(chunkPos);
				lock.unlock();


				m_worldScene.removeMesh(mesh_id);
				m_vulkanAPI.destroyMesh(mesh_id);
			}));
			// glm::ivec3 chunkPos = m_chunk_unload_queue.front();
			// m_chunk_unload_queue.pop();
			// if(!m_chunks.contains(chunkPos))
			// 	continue;
			// LOG_INFO("Unloading chunk: " << chunkPos.x << " " << chunkPos.y << " " << chunkPos.z);

			// uint64_t mesh_id = m_chunks.at(chunkPos).getMeshID();
			// chunk_meshes_id_to_remove.push_back(mesh_id);
			// m_worldScene.removeMesh(mesh_id);
			// m_vulkanAPI.destroyMesh(mesh_id);
			// m_chunks.erase(chunkPos);
		}
	}

	for(auto & future : futures)
		future.get();
}

void World::addChunksToLoadUnloadQueue(const glm::vec3 & nextPlayerPosition)
{
	std::lock_guard<std::mutex> lock(m_chunks_mutex);
	std::lock_guard<std::mutex> lock2(m_chunk_unload_queue_mutex);
	std::lock_guard<std::mutex> lock3(m_chunk_gen_queue_mutex);
	/**************************************************************
	 * GENERATE CHUNKS DYNAMICALLY
	 **************************************************************/
	glm::ivec3 playerChunk = glm::ivec3(m_player.position()) / CHUNK_SIZE;
	glm::ivec3 nextPlayerChunk = glm::ivec3(nextPlayerPosition) / CHUNK_SIZE;
	glm::ivec3 playerChunkDiff = playerChunk - nextPlayerChunk;

	if (playerChunkDiff == glm::ivec3(0)) return;
	

	/**************************************************************
	 * LOAD CHUNKS
	 **************************************************************/
	for(int x = -RENDER_DISTANCE; x < RENDER_DISTANCE; x++)
	{
		for(int y = -RENDER_DISTANCE; y < RENDER_DISTANCE; y++)
		{
			if (y + nextPlayerChunk.y < 0 || (y + nextPlayerChunk.y) * CHUNK_SIZE > WORLD_Y_MAX) continue;
			for(int z = -RENDER_DISTANCE; z < RENDER_DISTANCE; z++)
			{
				glm::ivec3 chunkPos = glm::ivec3(x, y, z) + nextPlayerChunk;
				if(!m_chunks.contains(chunkPos))
				{
					// LOG_INFO("Adding chunk to queue: " << chunkPos.x << " " << chunkPos.y << " " << chunkPos.z);

					auto it = std::find(m_chunk_unload_queue.begin(), m_chunk_unload_queue.end(), chunkPos);
					if (it != m_chunk_unload_queue.end())
						m_chunk_unload_queue.erase(it);
					m_chunk_gen_queue.push_back(chunkPos);
				}
			}
		}
	}

	/**************************************************************
	 * UNLOAD CHUNKS
	 **************************************************************/
	{
	std::vector<uint64_t> chunk_meshes_id_to_remove;
	// for(int x = -RENDER_DISTANCE; x < RENDER_DISTANCE; x++)
	// {
	// 	for(int y = -RENDER_DISTANCE; y < RENDER_DISTANCE; y++)
	// 	{
	// 		for(int z = -RENDER_DISTANCE; z < RENDER_DISTANCE; z++)
	// 		{
	// 			//create a relative position
	// 			glm::ivec3 chunkPos = glm::ivec3(x, y, z) - playerChunkDiff;
	// 			// if (chunkPos.y < 0) continue;
	// 			// if (chunkPos.y * CHUNK_SIZE > WORLD_Y_MAX) continue;


	// 			//check if position is outside the render distance (0, 0, 0 would be the current player position)
	// 			if (chunkPos.x < -RENDER_DISTANCE || chunkPos.x > RENDER_DISTANCE
	// 				|| chunkPos.y < -RENDER_DISTANCE || chunkPos.y > RENDER_DISTANCE
	// 				|| chunkPos.z < -RENDER_DISTANCE || chunkPos.z > RENDER_DISTANCE)
	// 			{

	// 				chunkPos += playerChunk; //get the real position

	// 				std::unique_lock<std::mutex> lock(m_chunks_mutex);
	// 				if(m_chunks.contains(chunkPos))
	// 				{
	// 					std::lock_guard<std::mutex> lock2(m_chunk_unload_queue_mutex);
	// 					LOG_INFO("Adding chunk to unload queue: " << chunkPos.x << " " << chunkPos.y << " " << chunkPos.z);
	// 					m_chunk_unload_queue.push(chunkPos);
	// 				}
	// 			}

	// 		}
	// 	}
	// }
		for (auto & [pos, chunk] : m_chunks)
		{
			if (glm::distance(glm::vec3(pos), glm::vec3(playerChunk)) > RENDER_DISTANCE + 2)
			{
				LOG_INFO("Adding chunk to unload queue: " << pos.x << " " << pos.y << " " << pos.z);
				auto it = std::find(m_chunk_gen_queue.begin(), m_chunk_gen_queue.end(), pos);
				if (it != m_chunk_gen_queue.end())
					m_chunk_gen_queue.erase(it);
				m_chunk_unload_queue.push_back(pos);
			}
		}
	}

	// for (int x = -(RENDER_DISTANCE * playerChunkDiff.x); x < RENDER_DISTANCE * playerChunkDiff.x; x++)
	// {
	// 	for (int y = -(RENDER_DISTANCE * playerChunkDiff.y); y < RENDER_DISTANCE * playerChunkDiff.y; y++)
	// 	{
	// 		for (int z = -(RENDER_DISTANCE * playerChunkDiff.z); z < RENDER_DISTANCE * playerChunkDiff.z; z++)
	// 		{
}

void World::update(glm::dvec3 nextPlayerPosition)
{
	// if (playerPosition.length() == 0.0f) return;

	addChunksToLoadUnloadQueue(nextPlayerPosition);
	doChunkLoadUnload(20);




	m_player.setPosition(nextPlayerPosition);
}
