#include "World.hpp"

World::World(WorldScene & WorldScene, VulkanAPI & vulkanAPI, ThreadPool & threadPool)
: m_worldGenerator()
, m_worldScene(WorldScene)
, m_vulkanAPI(vulkanAPI)
, m_threadPool(threadPool)
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

// void World::doChunkLoadUnload(const int & number_of_tasks)
// {
// 	std::vector<std::future<void>> futures;
// 	{
// 		std::lock_guard<std::mutex> lock(m_chunk_gen_queue_mutex);
// 		std::lock_guard<std::mutex> lock2(m_chunks_mutex);

// 		for(int i = 0; i < number_of_tasks / 2 && !m_chunk_gen_queue.empty(); i++)
// 		{
// 			glm::ivec3 chunkPos = m_chunk_gen_queue.front();
// 			m_chunk_gen_queue.pop_front();
// 			if(m_chunks.contains(chunkPos))
// 				continue;
// 			LOG_INFO("Generating chunk: " << chunkPos.x << " " << chunkPos.y << " " << chunkPos.z);
// 			futures.push_back(std::async(std::launch::async, [this, chunkPos]()
// 			{
// 				/**************************************************************
// 				 * CHUNK LOADING FUNCTION
// 				 **************************************************************/
// 				Chunk * chunk = nullptr;
// 				{
// 					std::lock_guard<std::mutex> lock(m_chunks_mutex);
// 					auto [it, success] = m_chunks.insert(std::make_pair(chunkPos, m_worldGenerator.generateChunk(chunkPos.x, chunkPos.y, chunkPos.z)));
// 					chunk = &it->second;
// 				}

// 				uint64_t mesh_id = m_vulkanAPI.createMesh(*chunk, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
// 				chunk->setMeshID(mesh_id);
// 				if(mesh_id != VulkanAPI::no_mesh_id)
// 					m_worldScene.addMeshData(mesh_id, glm::vec3(chunkPos * CHUNK_SIZE));
// 			}));
// 		}
// 	}

// 	{
// 		std::lock_guard<std::mutex> lock(m_chunk_unload_queue_mutex);
// 		std::lock_guard<std::mutex> lock2(m_chunks_mutex);


// 		std::vector<uint64_t> chunk_meshes_id_to_remove;
// 		for(int i = 0; i < number_of_tasks / 2 && !m_chunk_unload_queue.empty(); i++)
// 		{
// 			glm::ivec3 chunkPos = m_chunk_unload_queue.front();
// 			m_chunk_unload_queue.pop_front();

// 			futures.push_back(std::async(std::launch::async, [this, chunkPos]()
// 			{
// 				/**************************************************************
// 				 * CHUNK UNLOADING FUNCTION
// 				 **************************************************************/
// 				std::unique_lock<std::mutex> lock(m_chunks_mutex);
// 				LOG_INFO("Unloading chunk: " << chunkPos.x << " " << chunkPos.y << " " << chunkPos.z);

// 				if(!m_chunks.contains(chunkPos))
// 					return;
// 				uint64_t mesh_id = m_chunks.at(chunkPos).getMeshID();
// 				m_chunks.erase(chunkPos);
// 				lock.unlock();


// 				m_worldScene.removeMesh(mesh_id);
// 				m_vulkanAPI.destroyMesh(mesh_id);
// 			}));
// 			// glm::ivec3 chunkPos = m_chunk_unload_queue.front();
// 			// m_chunk_unload_queue.pop();
// 			// if(!m_chunks.contains(chunkPos))
// 			// 	continue;
// 			// LOG_INFO("Unloading chunk: " << chunkPos.x << " " << chunkPos.y << " " << chunkPos.z);

// 			// uint64_t mesh_id = m_chunks.at(chunkPos).getMeshID();
// 			// chunk_meshes_id_to_remove.push_back(mesh_id);
// 			// m_worldScene.removeMesh(mesh_id);
// 			// m_vulkanAPI.destroyMesh(mesh_id);
// 			// m_chunks.erase(chunkPos);
// 		}
// 	}

// 	for(auto & future : futures)
// 		future.get();
// }

void World::addChunksToLoadUnloadQueue(const glm::vec3 & nextPlayerPosition)
{
	std::lock_guard<std::mutex> lock(m_chunks_mutex);
	std::lock_guard<std::mutex> lock2(m_chunk_gen_set_mutex);
	std::lock_guard<std::mutex> lock3(m_chunk_unload_set_mutex);


	DebugGui::chunk_count_history.push(m_chunks.size());
	DebugGui::chunk_load_queue_size_history.push(m_chunk_gen_set.size());
	DebugGui::chunk_unload_queue_size_history.push(m_chunk_unload_set.size());




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
	for(int x = -LOAD_DISTANCE; x < LOAD_DISTANCE; x++)
	{
		for(int y = -LOAD_DISTANCE; y < LOAD_DISTANCE; y++)
		{
			if (y + nextPlayerChunk.y < 0 || (y + nextPlayerChunk.y) * CHUNK_SIZE > WORLD_Y_MAX) continue;
			for(int z = -LOAD_DISTANCE; z < LOAD_DISTANCE; z++)
			{
				glm::ivec3 chunkPos = glm::ivec3(x, y, z) + nextPlayerChunk;

				if(!m_chunks.contains(chunkPos))
				{
					// LOG_INFO("Adding chunk to queue: " << chunkPos.x << " " << chunkPos.y << " " << chunkPos.z);

					if(m_chunk_gen_set.contains(chunkPos))
						continue;
					m_chunk_gen_set.insert(chunkPos);

					m_chunk_futures.push(m_threadPool.submit([this, chunkPos]()
					{
						/**************************************************************
						 * CHUNK LOADING FUNCTION
						 **************************************************************/
						Chunk chunk = m_worldGenerator.generateChunk(chunkPos.x, chunkPos.y, chunkPos.z);
						chunk.setMeshID(0);
						{
							std::lock_guard<std::mutex> lock(m_chunks_mutex);
							std::lock_guard<std::mutex> lock2(m_chunk_gen_set_mutex);

							if (!m_chunk_gen_set.contains(chunkPos))
								return;
							m_chunk_gen_set.erase(chunkPos);
							// auto [it, success] = m_chunks.insert(std::make_pair(chunkPos, std::move(chunk)));
							m_chunks.insert(std::make_pair(chunkPos, std::move(chunk)));
						}
					}));
				}
			}
		}
	}

	/*// for(int x = -RENDER_DISTANCE; x < RENDER_DISTANCE; x++)
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
	// }*/

	for (auto & [pos, chunk] : m_chunks)
	{
		/**************************************************************
		 * UNLOAD CHUNKS
		 **************************************************************/
		float distance = glm::distance(glm::vec3(pos), glm::vec3(playerChunk));
		const glm::ivec3 chunkPos = pos;
		if (distance > LOAD_DISTANCE)
		{
			// LOG_INFO("Adding chunk to unload queue: " << pos.x << " " << pos.y << " " << pos.z);

			// if(m_chunk_unload_set.contains(pos))
			// 	continue;
			// m_chunk_unload_set.insert(pos);
			m_chunk_gen_set.erase(pos);			

			m_chunk_futures.push(m_threadPool.submit([this, chunkPos]()
			{
				/**************************************************************
				 * CHUNK UNLOADING FUNCTION
				 **************************************************************/
				std::unique_lock<std::mutex> lock(m_chunks_mutex);

				// LOG_INFO("Unloading chunk: " << chunkPos.x << " " << chunkPos.y << " " << chunkPos.z);

				uint64_t mesh_id = m_chunks.at(chunkPos).getMeshID();
				m_chunks.erase(chunkPos);
				lock.unlock();

				m_worldScene.removeMesh(mesh_id);
				m_vulkanAPI.destroyMesh(mesh_id);
			}));
		}
		/**************************************************************
		* RENDER CHUNKS
		****************************************************************/
		else if (distance <= RENDER_DISTANCE && chunk.getMeshID() == VulkanAPI::no_mesh_id)
		{
			// LOG_INFO("Adding chunk to load queue: " << pos.x << " " << pos.y << " " << pos.z);
			// if (m_chunk_render_set.contains(pos) || m_chunk_unload_set.contains(pos))
				// continue;
			m_chunk_futures.push(m_threadPool.submit([this, chunkPos]()
			{
				/**************************************************************
				 * CALCULATE MESH FUNCTION
				 **************************************************************/
				std::pair<Chunk *, uint64_t> chunk_pair, z_pos, z_neg, x_pos, x_neg, y_pos, y_neg;
				{
					std::lock_guard<std::mutex> lock(m_chunks_mutex);
					z_pos.first = m_chunks.contains(chunkPos + glm::ivec3(0, 0, 1)) ? &m_chunks.at(chunkPos + glm::ivec3(0, 0, 1)) : nullptr;
					z_neg.first = m_chunks.contains(chunkPos + glm::ivec3(0, 0, -1)) ? &m_chunks.at(chunkPos + glm::ivec3(0, 0, -1)) : nullptr;
					x_pos.first = m_chunks.contains(chunkPos + glm::ivec3(1, 0, 0)) ? &m_chunks.at(chunkPos + glm::ivec3(1, 0, 0)) : nullptr;
					x_neg.first = m_chunks.contains(chunkPos + glm::ivec3(-1, 0, 0)) ? &m_chunks.at(chunkPos + glm::ivec3(-1, 0, 0)) : nullptr;
					y_pos.first = m_chunks.contains(chunkPos + glm::ivec3(0, 1, 0)) ? &m_chunks.at(chunkPos + glm::ivec3(0, 1, 0)) : nullptr;
					y_neg.first = m_chunks.contains(chunkPos + glm::ivec3(0, -1, 0)) ? &m_chunks.at(chunkPos + glm::ivec3(0, -1, 0)) : nullptr;
					auto it = m_chunks.find(chunkPos);
					if (it == m_chunks.end())
						return;
					chunk_pair.first = &it->second;
				}

				uint64_t mesh_id = m_vulkanAPI.createMesh(*chunk_pair.first, x_pos.first, x_neg.first, y_pos.first, y_neg.first, z_pos.first, z_neg.first);
				chunk_pair.first->setMeshID(mesh_id);
				if(mesh_id != VulkanAPI::no_mesh_id)
					m_worldScene.addMeshData(mesh_id, glm::vec3(chunkPos * CHUNK_SIZE));
			}));
		}
	}
}

void World::update(glm::dvec3 nextPlayerPosition)
{
	// if (playerPosition.length() == 0.0f) return;

	addChunksToLoadUnloadQueue(nextPlayerPosition);

	while(!m_chunk_futures.empty())
	{
		m_chunk_futures.front().wait();
		m_chunk_futures.front().get();
		m_chunk_futures.pop();
	}

	if (m_chunk_gen_set.size() > 0 || m_chunk_unload_set.size() > 0 || m_chunk_render_set.size() > 0)
	{
		LOG_INFO("Chunk gen: " << m_chunk_gen_set.size() << " Chunk unload: " << m_chunk_unload_set.size() << " Chunk render: " << m_chunk_render_set.size());
	}
	// doChunkLoadUnload(20);

	m_player.setPosition(nextPlayerPosition);
}
