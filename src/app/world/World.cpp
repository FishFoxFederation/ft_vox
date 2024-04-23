#include "World.hpp"

World::World(WorldScene & WorldScene, VulkanAPI & vulkanAPI, ThreadPool & threadPool)
: m_worldGenerator()
, m_worldScene(WorldScene)
, m_vulkanAPI(vulkanAPI)
, m_threadPool(threadPool)
, m_player()
{
}

World::~World()
{
}


void World::addColumnToLoadUnloadQueue(const glm::vec3 & nextPlayerPosition)
{
	std::lock_guard<std::mutex> lock(m_chunks_mutex);
	std::lock_guard<std::mutex> lock4(m_visible_columns_mutex);


	glm::ivec3 nextPlayerChunk = glm::ivec3(nextPlayerPosition) / CHUNK_SIZE_IVEC3;

	glm::ivec2 nextPlayerChunk2D = glm::ivec2(nextPlayerChunk.x, nextPlayerChunk.z);


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
					Chunk chunk = m_worldGenerator.generateChunkColumn(chunkPos2D.x, chunkPos2D.y);
					{
						std::lock_guard<std::mutex> lock(m_chunks_mutex);
						m_loaded_columns.insert(chunkPos2D);
						m_chunks.at(glm::ivec3(chunk.x(), chunk.y() , chunk.z())) = std::move(chunk);
					}
					
					{
						std::lock_guard<std::mutex> lock(m_finished_futures_mutex);
						m_finished_futures.push(current_id);
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

void World::waitForFinishedTasks()
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

void World::update(glm::dvec3 nextPlayerPosition)
{

	addColumnToLoadUnloadQueue(nextPlayerPosition);
	waitForFinishedTasks();

	m_player.setPosition(nextPlayerPosition);
}
