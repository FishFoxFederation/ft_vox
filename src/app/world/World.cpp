#include "World.hpp"

World::World(
	WorldScene & WorldScene,
	VulkanAPI & vulkanAPI,
	ThreadPool & threadPool
)
:	m_worldScene(WorldScene),
	m_vulkanAPI(vulkanAPI),
	m_threadPool(threadPool),
	m_entities(),
	m_player(std::make_shared<Player>()),
	m_future_id(0)
{
	m_player->transform.position = glm::dvec3(0.0, 220.0, 0.0);
}

World::~World()
{
	waitForFutures();
}

void World::updateBlock(glm::dvec3 position)
{
	updateChunks(position);
	waitForFinishedFutures();
}

// void World::update(glm::dvec3 nextPlayerPosition)
// {
// 	updateChunks(nextPlayerPosition);
// 	waitForFinishedFutures();
// }

void World::updateEntities()
{
}

void World::loadChunks(const glm::vec3 & playerPosition)
{
	glm::ivec3 playerChunk3D = glm::ivec3(playerPosition) / CHUNK_SIZE_IVEC3;
	glm::ivec2 playerChunk2D = glm::ivec2(playerChunk3D.x, playerChunk3D.z);
	//here coords are in 2D because we are working with chunk columns
	//when we need specific 3D coords we always use 0 Y
	for(int x = -LOAD_DISTANCE; x < LOAD_DISTANCE; x++)
	{
		for(int z = -LOAD_DISTANCE; z < LOAD_DISTANCE; z++)
		{
			//transform the relative position to the real position
			glm::ivec2 chunkPos2D = glm::ivec2(x, z) + playerChunk2D;
			glm::ivec3 chunkPos3D = glm::ivec3(chunkPos2D.x, 0, chunkPos2D.y);
			if(!m_loaded_chunks.contains(chunkPos2D))
			{
				auto it = m_chunks.find(glm::ivec3(chunkPos2D.x, 0, chunkPos2D.y));
				if (it != m_chunks.end())
					continue;
				auto ret = m_chunks.insert(std::make_pair(chunkPos3D, Chunk(chunkPos3D)));
				ret.first->second.status.addWriter();

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
						m_loaded_chunks.insert(chunkPos2D);
						m_chunks.at(glm::ivec3(chunk.x(), chunk.y() , chunk.z())) = std::move(chunk);
						//line under is commented because the new chunk that is being moved in has a blank status
						// m_chunks.at(glm::ivec3(chunk.x(), chunk.y() , chunk.z())).status.removeWriter();
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
}

void World::loadChunks(const std::vector<glm::vec3> & playerPositions)
{
	for (auto playerPosition : playerPositions)
		loadChunks(playerPosition);
}

void World::unloadChunks(const std::vector<glm::vec3> & playerPositions)
{
	std::vector<glm::ivec2> playerChunks2D;
	for (auto playerPosition : playerPositions)
	{
		glm::ivec3 playerChunk3D = glm::ivec3(playerPosition) / CHUNK_SIZE_IVEC3;
		glm::ivec2 playerChunk2D = glm::ivec2(playerChunk3D.x, playerChunk3D.z);
		playerChunks2D.push_back(playerChunk2D);
	}

	for (auto & chunkPos2D : m_loaded_chunks)
	{
		if (m_unload_set.contains(chunkPos2D))
			continue;
		const glm::ivec3 chunkPos3D = glm::ivec3(chunkPos2D.x, 0, chunkPos2D.y);
		bool to_unload = true;
		for (auto & playerChunk2D : playerChunks2D)
		{
			//distance between the chunk and the player (in 2D space
			float distanceX = std::abs(chunkPos2D.x - playerChunk2D.x);
			float distanceZ = std::abs(chunkPos2D.y - playerChunk2D.y);
			if (distanceX <= LOAD_DISTANCE && distanceZ <= LOAD_DISTANCE)
			{
				to_unload = false;
				break;
			}
		}

		if (to_unload)
		{
			// LOG_DEBUG("Unloading chunk: " << chunkPos2D.x << " " << chunkPos2D.y);
			uint64_t current_id = m_future_id++;
			m_unload_set.insert(chunkPos2D);
			std::future<void> future = m_threadPool.submit([this, chunkPos2D, chunkPos3D, current_id]()
			{
				/**************************************************************
				 * CHUNK UNLOADING FUNCTION
				 **************************************************************/
				std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
				uint64_t mesh_id;
				std::unordered_map<glm::ivec3, Chunk>::iterator it;

				{
					std::lock_guard<std::mutex> lock(m_chunks_mutex);
					it = m_chunks.find(chunkPos3D);
					if (it == m_chunks.end())
						return;
				}

				//will block and wait
				it->second.status.addWriter();

				{
					std::lock_guard<std::mutex> lock(m_chunks_mutex);
					std::lock_guard<std::mutex> lock2(m_visible_chunks_mutex);
					std::lock_guard<std::mutex> lock3(m_unload_set_mutex);

					mesh_id = m_chunks.at(chunkPos3D).getMeshID();
					m_chunks.erase(chunkPos3D);
					m_loaded_chunks.erase(chunkPos2D);
					m_visible_chunks.erase(chunkPos2D);
					m_unload_set.erase(chunkPos2D);
				}

				m_worldScene.chunk_mesh_list.remove(mesh_id);
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
	}
}

void World::unloadChunks(const glm::vec3 & playerPosition)
{
	unloadChunks(std::vector<glm::vec3>{playerPosition});
}

void World::meshChunks(const glm::vec3 & playerPosition)
{
	glm::ivec3 playerChunk3D = glm::ivec3(playerPosition) / CHUNK_SIZE_IVEC3;
	glm::ivec2 playerChunk2D = glm::ivec2(playerChunk3D.x, playerChunk3D.z);
	for(auto chunkPos2D : m_loaded_chunks)
	{
		float distanceX = std::abs(chunkPos2D.x - playerChunk2D.x);
		float distanceZ = std::abs(chunkPos2D.y - playerChunk2D.y);

		if (distanceX < RENDER_DISTANCE && distanceZ < RENDER_DISTANCE
			&& !m_visible_chunks.contains(chunkPos2D))
		{
			glm::ivec3 chunkPos3D = glm::ivec3(chunkPos2D.x, 0, chunkPos2D.y);
			/********
			 * CHECKING IF NEIGHBOURS EXIST AND ARE AVAILABLE
			********/
			bool unavailable_neighbours = false;
			for(int x = -1; x < 2; x++)
			{
				for(int z = -1; z < 2; z++)
				{
					glm::ivec3 chunkPos = glm::ivec3(x, 0, z) + glm::ivec3(chunkPos2D.x, 0, chunkPos2D.y);
					if(!m_chunks.contains(chunkPos) || !m_chunks.at(chunkPos).status.isReadable())
					{
						unavailable_neighbours = true;
						break;
					}
				}
			}
			if (unavailable_neighbours)
				continue;
			//this is possible and thread safe to test if they are readable and then to modify their statuses
			//only because we have the guarantee that not other task will try to write to them
			//since task dispatching is done in order

			m_visible_chunks.insert(chunkPos2D);

			//The constructor will mark the chunk as being read
			CreateMeshData mesh_data(chunkPos3D, {1, 1, 1}, m_chunks);

			/********
			* PUSHING TASK TO THREAD POOL
			********/
			uint64_t current_id = m_future_id++;
			std::future<void> future = m_threadPool.submit([this, chunkPos3D, current_id, mesh_data = std::move(mesh_data)]() mutable
			{
				/**************************************************************
				 * CALCULATE MESH FUNCTION
				 **************************************************************/
				// LOG_DEBUG("Meshing chunk: " << pos2D.x << " " << pos2D.y);
				std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

				//create all mesh data needed ( pointers to neighbors basically )
				// CreateMeshData mesh_data(chunkPos3D, {1, 1, 1}, m_chunks);
				Chunk & chunk = *mesh_data.getCenterChunk();

				mesh_data.create(); //CPU intensive task to create the mesh
				//storing mesh in the GPU
				uint64_t mesh_id = m_vulkanAPI.storeMesh(
					mesh_data.vertices.data(),
					mesh_data.vertices.size(),
					sizeof(BlockVertex),
					mesh_data.indices.data(),
					mesh_data.indices.size()
				);

				chunk.setMeshID(mesh_id);
				//adding mesh id to the scene so it is rendered
				if(mesh_id != VulkanAPI::no_mesh_id)
					m_worldScene.chunk_mesh_list.add(mesh_id, glm::vec3(chunkPos3D * CHUNK_SIZE_IVEC3));

				{
					std::lock_guard<std::mutex> lock(m_finished_futures_mutex);
					m_finished_futures.push(current_id);
					LOG_DEBUG("Chunk meshed: " << chunkPos3D.x << " " << chunkPos3D.z);

				}

				std::chrono::duration time_elapsed = std::chrono::steady_clock::now() - start;
				DebugGui::chunk_render_time_history.push(std::chrono::duration_cast<std::chrono::microseconds>(time_elapsed).count());
			});
			m_futures.insert(std::make_pair(current_id, std::move(future)));
		}
	}
}

void World::updateChunks(const glm::vec3 & playerPosition)
{
	// static std::chrono::steady_clock::time_point last_update = std::chrono::steady_clock::now();
	std::lock_guard<std::mutex> lock(m_chunks_mutex);
	std::lock_guard<std::mutex> lock4(m_visible_chunks_mutex);
	std::lock_guard<std::mutex> lock5(m_unload_set_mutex);
	loadChunks(playerPosition);
	unloadChunks(playerPosition);
	meshChunks(playerPosition);
}

void World::waitForFinishedFutures()
{
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

void World::waitForFutures()
{
	while(!m_futures.empty())
	{
		m_futures.begin()->second.get();
		m_futures.erase(m_futures.begin());
	}
}

glm::vec3 World::getBlockChunkPosition(const glm::vec3 & position)
{
	glm::vec3 block_chunk_position = glm::ivec3(position) % CHUNK_SIZE_IVEC3;
	if (block_chunk_position.x < 0) block_chunk_position.x += CHUNK_X_SIZE;
	if (block_chunk_position.y < 0) block_chunk_position.y += CHUNK_Y_SIZE;
	if (block_chunk_position.z < 0) block_chunk_position.z += CHUNK_Z_SIZE;

	return block_chunk_position;
}

glm::vec3 World::getChunkPosition(const glm::vec3 & position)
{
	return glm::floor(position / CHUNK_SIZE_VEC3);
}

void World::updatePlayer(
	const glm::dvec3 & move,
	const glm::dvec2 & look
)
{
	std::lock_guard<std::mutex> lock(m_player_mutex);

	// glm::dvec3 position = m_player->transform.position;
	// glm::dvec3 displacement = m_player->getDisplacement(move);

	// for (int i = 0; i < 3; i++)
	// {
	// 	glm::dvec3 new_position = position;
	// 	new_position[i] += displacement[i];

	// 	glm::vec3 block_position = glm::floor(new_position);

	// 	glm::vec3 block_chunk_position = glm::ivec3(block_position) % CHUNK_SIZE_IVEC3;
	// 	if (block_chunk_position.x < 0) block_chunk_position.x += CHUNK_X_SIZE;
	// 	if (block_chunk_position.y < 0) block_chunk_position.y += CHUNK_Y_SIZE;
	// 	if (block_chunk_position.z < 0) block_chunk_position.z += CHUNK_Z_SIZE;

	// 	glm::vec3 chunk_position = glm::floor(block_position / CHUNK_SIZE_VEC3);
	// 	glm::ivec2 chunk_position2D = glm::ivec2(chunk_position.x, chunk_position.z);

	// 	{
	// 		std::lock_guard<std::mutex> lock(m_chunks_mutex);
	// 		if (m_loaded_chunks.contains(chunk_position2D))
	// 		{
	// 			Chunk & chunk = m_chunks.at(glm::ivec3(chunk_position.x, 0, chunk_position.z));
	// 			chunk.status.addReader();

	// 			BlockID block_id = chunk.getBlock(block_chunk_position.x, block_chunk_position.y, block_chunk_position.z);
	// 			if (Block::hasProperty(block_id, BLOCK_PROPERTY_SOLID))
	// 			{
	// 				displacement[i] = 0.0;
	// 			}

	// 			chunk.status.removeReader();
	// 		}
	// 	}
	// }

	Transform transform = m_player->transform;
	glm::dvec3 displacement = m_player->getDisplacement(move);

	glm::dvec3 new_position = transform.position + displacement;
	Transform new_transform = transform;
	new_transform.position = new_position;

	// glm::vec3 block_position = glm::floor(new_position);
	// glm::vec3 block_chunk_position = getBlockChunkPosition(block_position);
	// glm::vec3 chunk_position = getChunkPosition(block_position);
	// glm::ivec2 chunk_position2D = glm::ivec2(chunk_position.x, chunk_position.z);

	{
		std::lock_guard<std::mutex> lock(m_chunks_mutex);

		for (int x = -1; x <= 1; x++)
		{
		for (int z = -1; z <= 1; z++)
		{
		for (int y = -1; y <= 2; y++)
		{
			glm::vec3 block_position = glm::floor(glm::dvec3(new_position.x + x, new_position.y + y, new_position.z + z));
			glm::vec3 block_chunk_position = getBlockChunkPosition(block_position);
			glm::vec3 chunk_position = getChunkPosition(block_position);
			glm::ivec2 chunk_position2D = glm::ivec2(chunk_position.x, chunk_position.z);

			if (m_loaded_chunks.contains(chunk_position2D))
			{
				Chunk & chunk = m_chunks.at(glm::ivec3(chunk_position.x, 0, chunk_position.z));
				chunk.status.addReader();

				BlockID block_id = chunk.getBlock(block_chunk_position);
				if (Block::hasProperty(block_id, BLOCK_PROPERTY_SOLID))
				{
					if (isColliding(
						m_player->hitbox.transform(new_transform.model()),
						Block::getData(block_id).hitbox.transform(glm::translate(glm::mat4(1.0), block_position))
					))
					{
						displacement = glm::dvec3(0.0);
					}
					// else
					// {
					// 	LOG_DEBUG("No collision detected");
					// }
				}

				chunk.status.removeReader();
			}
		}
		}
		}

	}


	m_player->movePosition(displacement);
	m_player->moveDirection(look.x, look.y);

	DebugGui::player_position = m_player->transform.position;
}

Camera World::getCamera()
{
	std::lock_guard<std::mutex> lock(m_player_mutex);
	return m_player->camera();
}

glm::dvec3 World::getPlayerPosition()
{
	std::lock_guard<std::mutex> lock(m_player_mutex);
	return m_player->transform.position;
}
