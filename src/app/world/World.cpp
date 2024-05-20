#include "World.hpp"

World::World(
	WorldScene & WorldScene,
	VulkanAPI & vulkanAPI,
	ThreadPool & threadPool
)
:	m_worldScene(WorldScene),
	m_vulkanAPI(vulkanAPI),
	m_threadPool(threadPool),
	m_players(),
	m_future_id(0)
{
	std::shared_ptr<Player> my_player = std::make_shared<Player>();
	my_player->transform.position = glm::dvec3(0.0, 220.0, 0.0);

	m_my_player_id = m_players.insert(my_player);

	m_worldScene.entity_mesh_list.insert(
		m_my_player_id, {m_vulkanAPI.cube_mesh_id, {}}
	);
}

World::~World()
{
	waitForFutures();
}

void World::updateBlock(glm::dvec3 position)
{
	updateChunks(position);
	LOG_DEBUG("UPDATED CHUNKS");
	waitForFinishedFutures();
	LOG_DEBUG("WAITED FOR FUTURES");
}

// void World::update(glm::dvec3 nextPlayerPosition)
// {
// 	updateChunks(nextPlayerPosition);
// 	waitForFinishedFutures();
// }

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
				uint64_t mesh_scene_id, mesh_id;
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

					mesh_scene_id = m_chunks.at(chunkPos3D).getMeshID();
					m_chunks.erase(chunkPos3D);
					m_loaded_chunks.erase(chunkPos2D);
					m_visible_chunks.erase(chunkPos2D);
					m_unload_set.erase(chunkPos2D);
				}

				if (m_worldScene.chunk_mesh_list.contains(mesh_scene_id))
				{
					mesh_id = m_worldScene.chunk_mesh_list.get(mesh_scene_id).id;
					m_worldScene.chunk_mesh_list.erase(mesh_scene_id);
					m_vulkanAPI.destroyMesh(mesh_id);
				}

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
			meshChunk(chunkPos2D);
		}
	}
}

void World::meshChunk(const glm::ivec2 & chunkPos2D)
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
		return;
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
		// std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

		//create all mesh data needed ( pointers to neighbors basically )
		// CreateMeshData mesh_data(chunkPos3D, {1, 1, 1}, m_chunks);
		LOG_DEBUG(__LINE__);
		Chunk & chunk = *mesh_data.getCenterChunk();
		LOG_DEBUG(__LINE__);

		uint64_t old_mesh_scene_id;

		//destroy old mesh if it exists
		old_mesh_scene_id = chunk.getMeshID();
		LOG_DEBUG(__LINE__);


		mesh_data.create(); //CPU intensive task to create the mesh
		//storing mesh in the GPU
		LOG_DEBUG(__LINE__);
		uint64_t mesh_id = m_vulkanAPI.storeMesh(
			mesh_data.vertices.data(),
			mesh_data.vertices.size(),
			sizeof(BlockVertex),
			mesh_data.indices.data(),
			mesh_data.indices.size()
		);
		LOG_DEBUG(__LINE__);

		//adding mesh id to the scene so it is rendered
		if(mesh_id != IdList<uint64_t, Mesh>::invalid_id)
		{
			uint64_t mesh_scene_id = m_worldScene.chunk_mesh_list.insert({
				mesh_id,
				Transform(glm::vec3(chunkPos3D * CHUNK_SIZE_IVEC3)).model()
			});
			chunk.setMeshID(mesh_scene_id);
		}
		LOG_DEBUG(__LINE__);

		if (m_worldScene.chunk_mesh_list.contains(old_mesh_scene_id))
		{
			uint64_t mesh_id = m_worldScene.chunk_mesh_list.get(old_mesh_scene_id).id;
			m_worldScene.chunk_mesh_list.erase(old_mesh_scene_id);
			m_vulkanAPI.destroyMesh(mesh_id);
		}
		LOG_DEBUG(__LINE__);

		{
			std::lock_guard<std::mutex> lock(m_finished_futures_mutex);
			m_finished_futures.push(current_id);
			LOG_DEBUG("Chunk meshed: " << chunkPos3D.x << " " << chunkPos3D.z);

		}
		LOG_DEBUG(__LINE__);

		// std::chrono::duration time_elapsed = std::chrono::steady_clock::now() - start;
		// DebugGui::chunk_render_time_history.push(std::chrono::duration_cast<std::chrono::microseconds>(time_elapsed).count());
	});
	m_futures.insert(std::make_pair(current_id, std::move(future)));
}

void World::updateChunks(const glm::vec3 & playerPosition)
{
	// static std::chrono::steady_clock::time_point last_update = std::chrono::steady_clock::now();
	std::lock_guard<std::mutex> lock(m_chunks_mutex);
	std::lock_guard<std::mutex> lock4(m_visible_chunks_mutex);
	std::lock_guard<std::mutex> lock5(m_unload_set_mutex);
	loadChunks(playerPosition);
	LOG_DEBUG("LOADED CHUNKS");
	unloadChunks(playerPosition);
	LOG_DEBUG("UNLOADED CHUNKS");
	meshChunks(playerPosition);
	LOG_DEBUG("MESHED CHUNKS");
	doBlockSets();
}

void World::doBlockSets()
{
	{
		std::lock_guard<std::mutex> lock(m_blocks_to_set_mutex);
		if (m_blocks_to_set.empty())
			return;
	}
	uint64_t current_id = m_future_id++;
	std::future<void> future = m_threadPool.submit([this, current_id]()
	{
		std::lock_guard<std::mutex> lock(m_chunks_mutex);
		std::lock_guard<std::mutex> lock2(m_visible_chunks_mutex);
		std::lock_guard<std::mutex> lock3(m_blocks_to_set_mutex);
		while(!m_blocks_to_set.empty())
		{
			auto [position, block_id] = m_blocks_to_set.front();
			m_blocks_to_set.pop();
			glm::vec3 block_chunk_position = getBlockChunkPosition(position);
			glm::vec3 chunk_position = getChunkPosition(position);
			glm::ivec2 chunk_position2D = glm::ivec2(chunk_position.x, chunk_position.z);

			if (m_loaded_chunks.contains(chunk_position2D))
			{
				LOG_DEBUG("BEFORE AT");
				Chunk & chunk = m_chunks.at(glm::ivec3(chunk_position.x, 0, chunk_position.z));
				LOG_DEBUG("AFTER AT");
			// 	if (chunk.status.hasReaders())
			// 		LOG_DEBUG("Chunk has readers");
			// 	if (chunk.status.hasWriters())
			// 		LOG_DEBUG("Chunk has writers");
			// if (chunk.status.tryAddWriter() == false)
			// {
			// 	LOG_DEBUG("Chunk is busy");
			// 	return;
			// }
				chunk.status.addWriter();
				chunk.setBlock(block_chunk_position, block_id);
				chunk.status.removeWriter();

				m_visible_chunks.erase(chunk_position2D);
				if (block_chunk_position.x == 0)
					m_visible_chunks.erase(glm::ivec2(chunk_position2D.x - 1, chunk_position2D.y));
				if (block_chunk_position.x == CHUNK_X_SIZE - 1)
					m_visible_chunks.erase(glm::ivec2(chunk_position2D.x + 1, chunk_position2D.y));
				if (block_chunk_position.z == 0)
					m_visible_chunks.erase(glm::ivec2(chunk_position2D.x, chunk_position2D.y - 1));
				if (block_chunk_position.z == CHUNK_Z_SIZE - 1)
					m_visible_chunks.erase(glm::ivec2(chunk_position2D.x, chunk_position2D.y + 1));
			}
		}

		{
			std::lock_guard<std::mutex> lock(m_finished_futures_mutex);
			m_finished_futures.push(current_id);
		}
	});

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

void World::updateEntities()
{
}

void World::updatePlayerPosition(
	const uint64_t player_id,
	const int8_t forward,
	const int8_t backward,
	const int8_t left,
	const int8_t right,
	const int8_t up,
	const int8_t down,
	const double delta_time
)
{
	std::shared_ptr<Player> player = std::dynamic_pointer_cast<Player>(m_players.get(player_id));
	std::lock_guard<std::mutex> lock(player->mutex);


	// determine if player is on the ground or in the air and detect
	bool on_ground = hitboxCollisionWithBlock(player->feet, player->transform.position);
	if (on_ground && !player->on_ground) // player just landed
	{
		player->jump_remaining = 1;
		player->jumping = false;
	}
	if (!on_ground && player->on_ground) // player just started falling
	{
		player->startFall();
	}
	player->on_ground = on_ground;


	// get the movement vector
	glm::dvec3 move(right - left, 0.0, forward - backward);
	// normalize the move vector to prevent faster diagonal movement
	// but without the y component because we don't want to slow down the player when moving jumping
	if (glm::length(move) > 0.0)
		move = glm::normalize(move);
	// transform the move vector to the player's local coordinate system
	glm::dvec3 input_force = player->getTransformedMovement(move);
	// set the y component of the input force
	input_force.y = up - down;


	double speed_factor = 1.0;

	// the input force is now ready for flying
	// but we need to modify it's y component for handling jump when walking
	if (player->gameMode != Player::GameMode::SPECTATOR && !player->flying)
	{
		if (player->on_ground)
		{
			if (up && player->jump_remaining > 0)
			{
				player->velocity.y = player->jump_force;
				player->jump_remaining--;
				player->jumping = true;
			}
		}

		// if not flying, the y component of the input force is ignored
		input_force.y = 0.0;
	}
	else
	{
		speed_factor *= player->fly_speed_factor;
	}

	// apply speed factors
	if (player->sneaking)
		speed_factor *= player->sneak_speed_factor;
	if (player->sprinting)
		speed_factor *= player->sprint_speed_factor;
	if (player->jumping)
		speed_factor *= player->jump_speed_factor;


	player->input_velocity = input_force * player->default_speed * speed_factor;

	// apply gravity
	if (player->shouldFall())
	{
		player->velocity.y += player->gravity * delta_time;
	}

	// check for collision with blocks
	// each axis is checked separately to allow for sliding along walls
	glm::dvec3 displacement = (player->velocity + player->input_velocity) * delta_time;
	if (player->shouldCollide())
	{
		glm::dvec3 move_x = {displacement.x, 0.0, 0.0};
		glm::dvec3 move_y = {0.0, displacement.y, 0.0};
		glm::dvec3 move_z = {0.0, 0.0, displacement.z};
		glm::dvec3 move_xz = {displacement.x, 0.0, displacement.z};

		bool collision_x = hitboxCollisionWithBlock(player->hitbox, player->transform.position + move_x);
		bool collision_y = hitboxCollisionWithBlock(player->hitbox, player->transform.position + move_y);
		bool collision_z = hitboxCollisionWithBlock(player->hitbox, player->transform.position + move_z);
		bool collision_xz = hitboxCollisionWithBlock(player->hitbox, player->transform.position + move_xz);

		// edge case when the player is perfectly aligned with the corner of a block
		if (!collision_x && !collision_z && collision_xz)
		{
			// artificially set the collision on the axis with the smallest displacement
			if (std::abs(displacement.x) > std::abs(displacement.z))
				collision_z = true;
			else
				collision_x = true;
		}

		if (collision_x)
		{
			displacement.x = 0.0;
			player->velocity.x = 0.0;
		}
		if (collision_y)
		{
			displacement.y = 0.0;
			player->velocity.y = 0.0;
		}
		if (collision_z)
		{
			displacement.z = 0.0;
			player->velocity.z = 0.0;
		}
	}

	// apply displacement
	player->transform.position += displacement;


	DebugGui::player_position = player->transform.position;

	{ // update player mesh
		auto world_scene_lock = m_worldScene.entity_mesh_list.lock();
		m_worldScene.entity_mesh_list.at(m_my_player_id).model = Transform(
			player->transform.position + player->hitbox.position,
			glm::vec3(0.0f),
			player->hitbox.size
		).model();
	}
}

void World::playerAttack(
	const uint64_t player_id
)
{
	std::shared_ptr<Player> player = std::dynamic_pointer_cast<Player>(m_players.get(player_id));
	std::lock_guard<std::mutex> lock(player->mutex);

	glm::dvec3 position = player->transform.position + player->eyePosition();
	glm::dvec3 direction = player->direction();
	std::optional<glm::vec3> hit = rayCast(position, direction, 5.0);
	if (hit.has_value())
	{
		glm::vec3 block_position = hit.value();
		glm::vec3 block_chunk_position = getBlockChunkPosition(block_position);
		glm::vec3 chunk_position = getChunkPosition(block_position);
		glm::ivec2 chunk_position2D = glm::ivec2(chunk_position.x, chunk_position.z);

		std::lock_guard<std::mutex> lock(m_chunks_mutex);
		if (m_loaded_chunks.contains(chunk_position2D))
		{
			Chunk & chunk = m_chunks.at(glm::ivec3(chunk_position.x, 0, chunk_position.z));

			chunk.status.addReader();
			BlockID block_id = chunk.getBlock(block_chunk_position);
			chunk.status.removeReader();

			if (Block::hasProperty(block_id, BLOCK_PROPERTY_SOLID))
			{
				LOG_DEBUG("Block hit: " << block_position.x << " " << block_position.y << " " << block_position.z << " = " << int(block_id));

				std::lock_guard<std::mutex> lock(m_blocks_to_set_mutex);
				m_blocks_to_set.push({block_position, Block::Air.id});
			}

		}
	}
	else
	{
		LOG_DEBUG("No block hit");
	}
}

void World::updatePlayer(
	const uint64_t player_id,
	std::function<void(Player &)> update
)
{
	std::shared_ptr<Player> player = std::dynamic_pointer_cast<Player>(m_players.get(player_id));
	std::lock_guard<std::mutex> lock(player->mutex);
	update(*player);
}

Camera World::getCamera(const uint64_t player_id)
{
	std::shared_ptr<Player> player = std::dynamic_pointer_cast<Player>(m_players.get(player_id));
	std::lock_guard<std::mutex> lock(player->mutex);
	return player->camera();
}

glm::dvec3 World::getPlayerPosition(const uint64_t player_id)
{
	std::shared_ptr<Player> player = std::dynamic_pointer_cast<Player>(m_players.get(player_id));
	std::lock_guard<std::mutex> lock(player->mutex);
	return player->transform.position;
}

bool World::hitboxCollisionWithBlock(const HitBox & hitbox, const glm::dvec3 & position)
{
	glm::dvec3 offset = glm::dvec3(0.0);
	for (offset.x = -1; offset.x <= glm::ceil(hitbox.size.x); offset.x++)
	{
		for (offset.z = -1; offset.z <= glm::ceil(hitbox.size.z); offset.z++)
		{
			for (offset.y = -1; offset.y <= glm::ceil(hitbox.size.y); offset.y++)
			{
				glm::vec3 block_position = glm::floor(position + offset);
				glm::vec3 block_chunk_position = getBlockChunkPosition(block_position);
				glm::vec3 chunk_position = getChunkPosition(block_position);
				glm::ivec2 chunk_position2D = glm::ivec2(chunk_position.x, chunk_position.z);

				std::lock_guard<std::mutex> lock(m_chunks_mutex);
				if (m_loaded_chunks.contains(chunk_position2D))
				{
					Chunk & chunk = m_chunks.at(glm::ivec3(chunk_position.x, 0, chunk_position.z));

					chunk.status.addReader();
					BlockID block_id = chunk.getBlock(block_chunk_position);
					chunk.status.removeReader();

					if (Block::hasProperty(block_id, BLOCK_PROPERTY_SOLID))
					{
						HitBox block_hitbox = Block::getData(block_id).hitbox;

						if (isColliding(hitbox, position, block_hitbox, block_position))
						{
							return true;
						}
					}
				}
			}
		}
	}
	return false;
}

std::optional<glm::vec3>  World::rayCast(const glm::vec3 & origin, const glm::vec3 & direction, const double max_distance)
{
	glm::vec3 position = origin;
	glm::vec3 direction_normalized = glm::normalize(direction);

	for (float d = 0.0f; d < max_distance; d += 0.1f)
	{
		glm::vec3 block_position = glm::floor(position);
		glm::vec3 block_chunk_position = getBlockChunkPosition(block_position);
		glm::vec3 chunk_position = getChunkPosition(block_position);
		glm::ivec2 chunk_position2D = glm::ivec2(chunk_position.x, chunk_position.z);

		{
			std::lock_guard<std::mutex> lock(m_chunks_mutex);
			if (m_loaded_chunks.contains(chunk_position2D))
			{
				Chunk & chunk = m_chunks.at(glm::ivec3(chunk_position.x, 0, chunk_position.z));

				chunk.status.addReader();
				BlockID block_id = chunk.getBlock(block_chunk_position);
				chunk.status.removeReader();

				if (Block::hasProperty(block_id, BLOCK_PROPERTY_SOLID))
				{
					// for now treat all blocks as solid cubes
					return block_position;
				}

			}
		}

		position = origin + direction_normalized * d;
	}

	return std::nullopt;
}
