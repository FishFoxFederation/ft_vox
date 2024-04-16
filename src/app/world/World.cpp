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

void World::addColumnToLoadUnloadQueue(const glm::vec3 & nextPlayerPosition)
{
	std::lock_guard<std::mutex> lock(m_chunks_mutex);
	std::lock_guard<std::mutex> lock2(m_chunk_gen_set_mutex);
	std::lock_guard<std::mutex> lock3(m_chunk_unload_set_mutex);
	std::lock_guard<std::mutex> lock4(m_visible_columns_mutex);


	// glm::ivec3 playerChunk = glm::ivec3(m_player.position()) / CHUNK_SIZE_IVEC3;
	glm::ivec3 nextPlayerChunk = glm::ivec3(nextPlayerPosition) / CHUNK_SIZE_IVEC3;
	// glm::ivec3 playerChunkDiff = playerChunk - nextPlayerChunk;

	glm::ivec2 nextPlayerChunk2D = glm::ivec2(nextPlayerChunk.x, nextPlayerChunk.z);

	// if (nextPlayerChunk == playerChunk) return;

	/**************************************************************
	 * LOAD CHUNKS
	 **************************************************************/
	//here coords are in 2D because we are working with chunk columns
	//when we need specific 3D coords we always use 0 Y
	for(int x = -LOAD_DISTANCE; x < LOAD_DISTANCE; x++)
	{
		for(int z = -LOAD_DISTANCE; z < LOAD_DISTANCE; z++)
		{
			//transform the relative position to the real position
			glm::ivec2 chunkPos2D = glm::ivec2(x, z) + nextPlayerChunk2D;
			glm::ivec3 chunkPos3D = glm::ivec3(chunkPos2D.x, 0, chunkPos2D.y);
			if(!m_loaded_columns.contains(chunkPos2D))
			{
				auto it = m_chunks.find(glm::ivec3(chunkPos2D.x, 0, chunkPos2D.y));
				if (it != m_chunks.end())
					continue;
				m_chunks.insert(std::make_pair(chunkPos3D, Chunk(chunkPos3D)));
				uint64_t current_id = m_future_id++;
				std::future<void> future = m_threadPool.submit([this, chunkPos2D, current_id]()
				{
					/**************************************************************
					 * CHUNK LOADING FUNCTION
					 **************************************************************/
					std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
					// LOG_DEBUG("Loading chunk: " << chunkPos2D.x << " " << chunkPos2D.y);
					Chunk chunk = m_worldGenerator.generateChunkColumn(chunkPos2D.x, chunkPos2D.y);
					{
						std::lock_guard<std::mutex> lock(m_chunks_mutex);
						m_loaded_columns.insert(chunkPos2D);
						m_chunks.at(glm::ivec3(chunk.x(), chunk.y() , chunk.z())) = std::move(chunk);
					}
					
					{
						std::lock_guard<std::mutex> lock(m_finished_futures_mutex);
						m_finished_futures.push(current_id);
						// LOG_DEBUG("Chunk loaded: " << chunkPos2D.x << " " << chunkPos2D.y);
					}
					std::chrono::duration time_elapsed = std::chrono::steady_clock::now() - start;
					DebugGui::chunk_gen_time_history.push(std::chrono::duration_cast<std::chrono::microseconds>(time_elapsed).count());
				});
				m_futures.insert(std::make_pair(current_id, std::move(future)));
			}
		}
	}

	for(auto pos2D : m_loaded_columns)
	{
		const glm::ivec3 pos3D = glm::ivec3(pos2D.x, 0, pos2D.y);
		// float distance = glm::distance(glm::vec3(pos3D), glm::vec3(nextPlayerChunk.x, 0, nextPlayerChunk.z));
		float distanceX = std::abs(pos2D.x - nextPlayerChunk2D.x);
		float distanceZ = std::abs(pos2D.y - nextPlayerChunk2D.y);
		/**************************************************************
		 * UNLOAD CHUNKS
		 **************************************************************/
		if (distanceX > LOAD_DISTANCE || distanceZ > LOAD_DISTANCE) 
		{
			if (!m_chunks.at(pos3D).status.isClear())
			{
				// LOG_DEBUG("Chunk is busy: " << pos2D.x << " " << pos2D.y);
				continue;
			}
			// LOG_DEBUG("Unloading chunk: " << pos2D.x << " " << pos2D.y);
			m_chunks.at(pos3D).status.setFlag(Chunk::ChunkStatus::DELETING);
			// m_loaded_columns.erase(pos2D);
			// m_visible_columns.erase(pos2D);
			uint64_t current_id = m_future_id++;
			std::future<void> future = m_threadPool.submit([this, pos2D, current_id]()
			{
				/**************************************************************
				 * CHUNK UNLOADING FUNCTION
				 **************************************************************/
				std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
				uint64_t mesh_id;
				const glm::ivec3 pos3D = glm::ivec3(pos2D.x, 0, pos2D.y);
				{
					std::lock_guard<std::mutex> lock(m_chunks_mutex);
					std::lock_guard<std::mutex> lock2(m_visible_columns_mutex);

					mesh_id = m_chunks.at(pos3D).getMeshID();
					m_chunks.erase(pos3D);
					m_loaded_columns.erase(pos2D);
					m_visible_columns.erase(pos2D);
				}
				m_worldScene.removeMesh(mesh_id);
				m_vulkanAPI.destroyMesh(mesh_id);

				{
					std::lock_guard<std::mutex> lock(m_finished_futures_mutex);
					m_finished_futures.push(current_id);
				}

				std::chrono::duration time_elapsed = std::chrono::steady_clock::now() - start;
				DebugGui::chunk_unload_time_history.push(std::chrono::duration_cast<std::chrono::microseconds>(time_elapsed).count());
			});
			m_futures.insert(std::make_pair(current_id, std::move(future)));
		}

		/**************************************************************
		 * RENDER CHUNKS
		 * *******************************************************/
		else if (distanceX < RENDER_DISTANCE && distanceZ < RENDER_DISTANCE
			&& !m_visible_columns.contains(pos2D)
			&& !m_chunks.at(pos3D).status.isSet(Chunk::ChunkStatus::MESHING))
		{
			/*********
			 * SETTING STATUSES
			*********/
			//set chunks as working
			for(int x = -1; x < 2; x++)
			{
				for(int z = -1; z < 2; z++)
				{
					glm::ivec3 chunkPos = glm::ivec3(x, 0, z) + glm::ivec3(pos2D.x, 0, pos2D.y);
					if(m_chunks.contains(chunkPos))
						m_chunks.at(chunkPos).status.addWorking();
				}
			}
			//set the current chunk as meshing
			m_chunks.at(pos3D).status.setFlag(Chunk::ChunkStatus::MESHING);


			/********
			* PUSHING TASK TO THREAD POOL 
			********/
			uint64_t current_id = m_future_id++;
			std::future<void> future = m_threadPool.submit([this, pos2D, pos3D, current_id]()
			{
				/**************************************************************
				 * CALCULATE MESH FUNCTION
				 **************************************************************/
				LOG_DEBUG("Meshing chunk: " << pos2D.x << " " << pos2D.y);
				std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
				{
					std::lock_guard<std::mutex> lock(m_visible_columns_mutex);
					m_visible_columns.insert(pos2D);
					// LOG_DEBUG("Chunk added to visible set " << pos2D.x << " " << pos2D.y);
				}
				//create all mesh data needed ( pointers to neighbors basically )
				std::unique_lock<std::mutex> lock(m_chunks_mutex);
				CreateMeshData mesh_data(pos3D, {1, 1, 1}, m_chunks);
				Chunk & chunk = m_chunks.at(pos3D);
				lock.unlock();

				mesh_data.create(); //CPU intensive task to create the mesh
				//storing mesh in the GPU
				uint64_t mesh_id = m_vulkanAPI.storeMesh(mesh_data.vertices, mesh_data.indices);

				chunk.setMeshID(mesh_id);
				//adding mesh id to the scene so it is rendered
				if(mesh_id != VulkanAPI::no_mesh_id)
					m_worldScene.addMeshData(mesh_id, glm::vec3(pos3D * CHUNK_SIZE_IVEC3));

				/*********************
				 * SETTING STATUSES
				**********************/
				for (auto vec2D : mesh_data.chunks)
				{
					for (auto vec1D : vec2D)
					{
						for(auto & chunk_ptr : vec1D)
						{
							if (chunk_ptr == nullptr)
								continue;
							chunk_ptr->status.removeWorking();
						}
					}
				}
				chunk.status.clearFlag(Chunk::ChunkStatus::MESHING);

				std::chrono::duration time_elapsed = std::chrono::steady_clock::now() - start;
				DebugGui::chunk_render_time_history.push(std::chrono::duration_cast<std::chrono::microseconds>(time_elapsed).count());
				{
					std::lock_guard<std::mutex> lock(m_finished_futures_mutex);
					m_finished_futures.push(current_id);
					LOG_DEBUG("Chunk meshed: " << pos2D.x << " " << pos2D.y);
				}
			});
			m_futures.insert(std::make_pair(current_id, std::move(future)));
		}
	}
}

// void World::addChunksToLoadUnloadQueue(const glm::vec3 & nextPlayerPosition)
// {
// 	std::lock_guard<std::mutex> lock(m_chunks_mutex);
// 	std::lock_guard<std::mutex> lock2(m_chunk_gen_set_mutex);
// 	std::lock_guard<std::mutex> lock3(m_chunk_unload_set_mutex);


// 	DebugGui::chunk_count_history.push(m_chunks.size());
// 	DebugGui::chunk_load_queue_size_history.push(m_chunk_gen_set.size());
// 	DebugGui::chunk_unload_queue_size_history.push(m_chunk_unload_set.size());




// 	/**************************************************************
// 	 * GENERATE CHUNKS DYNAMICALLY
// 	 **************************************************************/



// 	glm::ivec3 playerChunk = glm::ivec3(m_player.position()) / CHUNK_SIZE;
// 	glm::ivec3 nextPlayerChunk = glm::ivec3(nextPlayerPosition) / CHUNK_SIZE;
// 	glm::ivec3 playerChunkDiff = playerChunk - nextPlayerChunk;

// 	if (playerChunkDiff == glm::ivec3(0)) return;


// 	/**************************************************************
// 	 * LOAD CHUNKS
// 	 **************************************************************/
// 	for(int x = -LOAD_DISTANCE; x < LOAD_DISTANCE; x++)
// 	{
// 		for(int y = -LOAD_DISTANCE; y < LOAD_DISTANCE; y++)
// 		{
// 			if (y + nextPlayerChunk.y < 0 || (y + nextPlayerChunk.y) * CHUNK_SIZE > WORLD_Y_MAX) continue;
// 			for(int z = -LOAD_DISTANCE; z < LOAD_DISTANCE; z++)
// 			{
// 				glm::ivec3 chunkPos = glm::ivec3(x, y, z) + nextPlayerChunk;

// 				if(!m_chunks.contains(chunkPos))
// 				{
// 					// LOG_INFO("Adding chunk to queue: " << chunkPos.x << " " << chunkPos.y << " " << chunkPos.z);

// 					if(m_chunk_gen_set.contains(chunkPos))
// 						continue;
// 					m_chunk_gen_set.insert(chunkPos);

// 					m_chunk_futures.push(m_threadPool.submit([this, chunkPos]()
// 					{
// 						/**************************************************************
// 						 * CHUNK LOADING FUNCTION
// 						 **************************************************************/
// 						Chunk chunk = m_worldGenerator.generateChunk(chunkPos.x, chunkPos.y, chunkPos.z);
// 						chunk.setMeshID(0);
// 						{
// 							std::lock_guard<std::mutex> lock(m_chunks_mutex);
// 							std::lock_guard<std::mutex> lock2(m_chunk_gen_set_mutex);

// 							if (!m_chunk_gen_set.contains(chunkPos))
// 								return;
// 							m_chunk_gen_set.erase(chunkPos);
// 							// auto [it, success] = m_chunks.insert(std::make_pair(chunkPos, std::move(chunk)));
// 							m_chunks.insert(std::make_pair(chunkPos, std::move(chunk)));
// 						}
// 					}));
// 				}
// 			}
// 		}
// 	}

// 	/*// for(int x = -RENDER_DISTANCE; x < RENDER_DISTANCE; x++)
// 	// {
// 	// 	for(int y = -RENDER_DISTANCE; y < RENDER_DISTANCE; y++)
// 	// 	{
// 	// 		for(int z = -RENDER_DISTANCE; z < RENDER_DISTANCE; z++)
// 	// 		{
// 	// 			//create a relative position
// 	// 			glm::ivec3 chunkPos = glm::ivec3(x, y, z) - playerChunkDiff;
// 	// 			// if (chunkPos.y < 0) continue;
// 	// 			// if (chunkPos.y * CHUNK_SIZE > WORLD_Y_MAX) continue;


// 	// 			//check if position is outside the render distance (0, 0, 0 would be the current player position)
// 	// 			if (chunkPos.x < -RENDER_DISTANCE || chunkPos.x > RENDER_DISTANCE
// 	// 				|| chunkPos.y < -RENDER_DISTANCE || chunkPos.y > RENDER_DISTANCE
// 	// 				|| chunkPos.z < -RENDER_DISTANCE || chunkPos.z > RENDER_DISTANCE)
// 	// 			{

// 	// 				chunkPos += playerChunk; //get the real position

// 	// 				std::unique_lock<std::mutex> lock(m_chunks_mutex);
// 	// 				if(m_chunks.contains(chunkPos))
// 	// 				{
// 	// 					std::lock_guard<std::mutex> lock2(m_chunk_unload_queue_mutex);
// 	// 					LOG_INFO("Adding chunk to unload queue: " << chunkPos.x << " " << chunkPos.y << " " << chunkPos.z);
// 	// 					m_chunk_unload_queue.push(chunkPos);
// 	// 				}
// 	// 			}

// 	// 		}
// 	// 	}
// 	// }*/

// 	for (auto & [pos, chunk] : m_chunks)
// 	{
// 		/**************************************************************
// 		 * UNLOAD CHUNKS
// 		 **************************************************************/
// 		float distance = glm::distance(glm::vec3(pos), glm::vec3(playerChunk));
// 		const glm::ivec3 chunkPos = pos;
// 		if (distance > LOAD_DISTANCE)
// 		{
// 			// LOG_INFO("Adding chunk to unload queue: " << pos.x << " " << pos.y << " " << pos.z);

// 			// if(m_chunk_unload_set.contains(pos))
// 			// 	continue;
// 			// m_chunk_unload_set.insert(pos);
// 			m_chunk_gen_set.erase(pos);

// 			m_chunk_futures.push(m_threadPool.submit([this, chunkPos]()
// 			{
// 				/**************************************************************
// 				 * CHUNK UNLOADING FUNCTION
// 				 **************************************************************/
// 				std::unique_lock<std::mutex> lock(m_chunks_mutex);

// 				// LOG_INFO("Unloading chunk: " << chunkPos.x << " " << chunkPos.y << " " << chunkPos.z);

// 				uint64_t mesh_id = m_chunks.at(chunkPos).getMeshID();
// 				m_chunks.erase(chunkPos);
// 				lock.unlock();

// 				m_worldScene.removeMesh(mesh_id);
// 				m_vulkanAPI.destroyMesh(mesh_id);
// 			}));
// 		}
// 		/**************************************************************
// 		* RENDER CHUNKS
// 		****************************************************************/
// 		// else if (distance <= RENDER_DISTANCE && chunk.getMeshID() == VulkanAPI::no_mesh_id)
// 		// {
// 		// 	// LOG_INFO("Adding chunk to load queue: " << pos.x << " " << pos.y << " " << pos.z);
// 		// 	// if (m_chunk_render_set.contains(pos) || m_chunk_unload_set.contains(pos))
// 		// 		// continue;
// 		// 	m_chunk_futures.push(m_threadPool.submit([this, chunkPos]()
// 		// 	{
// 		// 		/**************************************************************
// 		// 		 * CALCULATE MESH FUNCTION
// 		// 		 **************************************************************/
// 		// 		std::pair<Chunk *, uint64_t> chunk_pair, z_pos, z_neg, x_pos, x_neg, y_pos, y_neg;
// 		// 		{
// 		// 			std::lock_guard<std::mutex> lock(m_chunks_mutex);
// 		// 			z_pos.first = m_chunks.contains(chunkPos + glm::ivec3(0, 0, 1)) ? &m_chunks.at(chunkPos + glm::ivec3(0, 0, 1)) : nullptr;
// 		// 			z_neg.first = m_chunks.contains(chunkPos + glm::ivec3(0, 0, -1)) ? &m_chunks.at(chunkPos + glm::ivec3(0, 0, -1)) : nullptr;
// 		// 			x_pos.first = m_chunks.contains(chunkPos + glm::ivec3(1, 0, 0)) ? &m_chunks.at(chunkPos + glm::ivec3(1, 0, 0)) : nullptr;
// 		// 			x_neg.first = m_chunks.contains(chunkPos + glm::ivec3(-1, 0, 0)) ? &m_chunks.at(chunkPos + glm::ivec3(-1, 0, 0)) : nullptr;
// 		// 			y_pos.first = m_chunks.contains(chunkPos + glm::ivec3(0, 1, 0)) ? &m_chunks.at(chunkPos + glm::ivec3(0, 1, 0)) : nullptr;
// 		// 			y_neg.first = m_chunks.contains(chunkPos + glm::ivec3(0, -1, 0)) ? &m_chunks.at(chunkPos + glm::ivec3(0, -1, 0)) : nullptr;
// 		// 			auto it = m_chunks.find(chunkPos);
// 		// 			if (it == m_chunks.end())
// 		// 				return;
// 		// 			chunk_pair.first = &it->second;
// 		// 		}

// 		// 		uint64_t mesh_id = m_vulkanAPI.createMesh(*chunk_pair.first, x_pos.first, x_neg.first, y_pos.first, y_neg.first, z_pos.first, z_neg.first);
// 		// 		chunk_pair.first->setMeshID(mesh_id);
// 		// 		if(mesh_id != VulkanAPI::no_mesh_id)
// 		// 			m_worldScene.addMeshData(mesh_id, glm::vec3(chunkPos * CHUNK_SIZE));
// 		// 	}));
// 		// }
// 	}
// }

void World::update(glm::dvec3 nextPlayerPosition)
{
	// if (playerPosition.length() == 0.0f) return;

	// addChunksToLoadUnloadQueue(nextPlayerPosition);
	addColumnToLoadUnloadQueue(nextPlayerPosition);

	{
		// while(!m_futures.empty())
		// {
		// 	m_futures.begin()->second.get();
		// 	m_futures.erase(m_futures.begin());
		// }
		// LOG_DEBUG("Waiting for futures");
		// LOG_DEBUG("current id : " << m_future_id);
		std::lock_guard<std::mutex> lock(m_finished_futures_mutex);
		while(!m_finished_futures.empty())
		{
			uint64_t id = m_finished_futures.front();
			m_finished_futures.pop();
			auto & future = m_futures.at(id);
			future.get();
			m_futures.erase(id);
		}
	}
	// if (m_chunk_gen_set.size() > 0 || m_chunk_unload_set.size() > 0 || m_chunk_render_set.size() > 0)
	// {
	// 	LOG_INFO("Chunk gen: " << m_chunk_gen_set.size() << " Chunk unload: " << m_chunk_unload_set.size() << " Chunk render: " << m_chunk_render_set.size());
	// }
	// doChunkLoadUnload(20);

	m_player.setPosition(nextPlayerPosition);
	// LOG_DEBUG("NEXT ITERATION");
}
