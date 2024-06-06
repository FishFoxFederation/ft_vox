#include "ClientWorld.hpp"
#include "DebugGui.hpp"

ClientWorld::ClientWorld(
	WorldScene & WorldScene,
	VulkanAPI & vulkanAPI,
	ThreadPool & threadPool,
	uint64_t my_player_id
)
:	m_worldScene(WorldScene),
	m_vulkanAPI(vulkanAPI),
	m_threadPool(threadPool),
	// m_players(),
	m_future_id(0)
{
	m_my_player_id = my_player_id;
	addPlayer(m_my_player_id, glm::dvec3(0.0, 220.0, 0.0));
}

ClientWorld::~ClientWorld()
{
	waitForFutures();
}

void ClientWorld::updateBlock(glm::dvec3 position)
{
	updateChunks(position);
	waitForFinishedFutures();
}

// void ClientWorld::update(glm::dvec3 nextPlayerPosition)
// {
// 	updateChunks(nextPlayerPosition);
// 	waitForFinishedFutures();
// }

void ClientWorld::loadChunks(const glm::vec3 & playerPosition)
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
				ret.first->second.status.lock();

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
						// m_chunks.at(glm::ivec3(chunk.x(), chunk.y() , chunk.z())).status.unlock();
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

void ClientWorld::loadChunks(const std::vector<glm::vec3> & playerPositions)
{
	for (auto playerPosition : playerPositions)
		loadChunks(playerPosition);
}

void ClientWorld::unloadChunks(const std::vector<glm::vec3> & playerPositions)
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
				it->second.status.lock();

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

void ClientWorld::unloadChunks(const glm::vec3 & playerPosition)
{
	unloadChunks(std::vector<glm::vec3>{playerPosition});
}

void ClientWorld::meshChunks(const glm::vec3 & playerPosition)
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

void ClientWorld::meshChunk(const glm::ivec2 & chunkPos2D)
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
			if(!m_chunks.contains(chunkPos) || !m_chunks.at(chunkPos).status.isShareLockable())
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
		std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

		//create all mesh data needed ( pointers to neighbors basically )
		// CreateMeshData mesh_data(chunkPos3D, {1, 1, 1}, m_chunks);
		Chunk & chunk = *mesh_data.getCenterChunk();

		uint64_t old_mesh_scene_id;

		//destroy old mesh if it exists
		old_mesh_scene_id = chunk.getMeshID();


		mesh_data.create(); //CPU intensive task to create the mesh
		//storing mesh in the GPU
		uint64_t mesh_id = m_vulkanAPI.storeMesh(
			mesh_data.vertices.data(),
			mesh_data.vertices.size(),
			sizeof(BlockVertex),
			mesh_data.indices.data(),
			mesh_data.indices.size()
		);

		//adding mesh id to the scene so it is rendered
		if(mesh_id != IdList<uint64_t, Mesh>::invalid_id)
		{
			uint64_t mesh_scene_id = m_worldScene.chunk_mesh_list.insert({
				mesh_id,
				Transform(glm::vec3(chunkPos3D * CHUNK_SIZE_IVEC3)).model()
			});
			chunk.setMeshID(mesh_scene_id);
		}

		if (m_worldScene.chunk_mesh_list.contains(old_mesh_scene_id))
		{
			uint64_t mesh_id = m_worldScene.chunk_mesh_list.get(old_mesh_scene_id).id;
			m_worldScene.chunk_mesh_list.erase(old_mesh_scene_id);
			m_vulkanAPI.destroyMesh(mesh_id);
		}

		{
			std::lock_guard<std::mutex> lock(m_finished_futures_mutex);
			m_finished_futures.push(current_id);
			// LOG_DEBUG("Chunk meshed: " << chunkPos3D.x << " " << chunkPos3D.z);
		}

		mesh_data.unlock();

		std::chrono::duration time_elapsed = std::chrono::steady_clock::now() - start;
		DebugGui::chunk_render_time_history.push(std::chrono::duration_cast<std::chrono::microseconds>(time_elapsed).count());
	});
	m_futures.insert(std::make_pair(current_id, std::move(future)));
}

void ClientWorld::updateChunks(const glm::vec3 & playerPosition)
{
	// static std::chrono::steady_clock::time_point last_update = std::chrono::steady_clock::now();
	std::lock_guard<std::mutex> lock(m_chunks_mutex);
	std::lock_guard<std::mutex> lock4(m_visible_chunks_mutex);
	std::lock_guard<std::mutex> lock5(m_unload_set_mutex);
	loadChunks(playerPosition);
	unloadChunks(playerPosition);
	meshChunks(playerPosition);
	doBlockSets();
}

void ClientWorld::doBlockSets()
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
				Chunk & chunk = m_chunks.at(glm::ivec3(chunk_position.x, 0, chunk_position.z));
				chunk.status.lock();
				chunk.setBlock(block_chunk_position, block_id);
				chunk.status.unlock();

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
	m_futures.insert(std::make_pair(current_id, std::move(future)));
}

void ClientWorld::waitForFinishedFutures()
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

void ClientWorld::waitForFutures()
{
	while(!m_futures.empty())
	{
		m_futures.begin()->second.get();
		m_futures.erase(m_futures.begin());
	}
}



void ClientWorld::updateEntities()
{
}

void ClientWorld::applyPlayerMovement(const uint64_t & player_id, const glm::dvec3 & displacement)
{
	std::shared_ptr<Player> player = std::dynamic_pointer_cast<Player>(m_players.at(player_id));
	std::lock_guard<std::mutex> lock(player->mutex);

	// apply displacement
	player->transform.position += displacement;

	DebugGui::player_position = player->transform.position;

	{ // update player mesh
		auto world_scene_lock = m_worldScene.entity_mesh_list.lock();
		m_worldScene.entity_mesh_list.at(player_id).model = Transform(
			player->transform.position + player->hitbox.position,
			glm::vec3(0.0f),
			player->hitbox.size
		).model();
	}
}

void ClientWorld::updatePlayerPosition(const uint64_t & player_id, const glm::dvec3 & position)
{
	std::shared_ptr<Player> player = std::dynamic_pointer_cast<Player>(m_players.at(player_id));
	std::lock_guard<std::mutex> lock(player->mutex);

	// apply displacement
	player->transform.position = position;

	DebugGui::player_position = player->transform.position;

	{ // update player mesh
		auto world_scene_lock = m_worldScene.entity_mesh_list.lock();
		m_worldScene.entity_mesh_list.at(player_id).model = Transform(
			player->transform.position + player->hitbox.position,
			glm::vec3(0.0f),
			player->hitbox.size
		).model();
	}
	if (player_id == m_my_player_id)
	{
		DebugGui::player_position = player->transform.position;

		// update camera
		m_worldScene.camera() = player->camera();
	}
}


std::pair<glm::dvec3, glm::dvec3> ClientWorld::calculatePlayerMovement(
	const uint64_t player_id,
	const int8_t forward,
	const int8_t backward,
	const int8_t left,
	const int8_t right,
	const int8_t up,
	const int8_t down,
	const double delta_time_second
)
{
	std::shared_ptr<Player> player;
	{
		std::lock_guard<std::mutex> lock(m_players_mutex);
		player = m_players.at(player_id);
	}
	std::lock_guard<std::mutex> lock(player->mutex);

	std::pair<glm::dvec3, glm::dvec3> result;

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
	move = player->getTransformedMovement(move);


	glm::dvec3 displacement;

	// if player is walking
	if (player->isFlying() == false)
	{
		double acc = DebugGui::acceleration;
		double ground_friction = DebugGui::ground_friction;
		double air_friction = DebugGui::air_friction;
		glm::dvec3 friction = glm::dvec3(ground_friction, air_friction, ground_friction);

		double jump_force = DebugGui::jump_force;
		double gravity = -DebugGui::gravity;

		if (up && player->canJump())
		{
			player->startJump();
			player->velocity.y = jump_force;
		}

		player->velocity.y += gravity * delta_time_second;

		player->velocity += move * acc * delta_time_second;

		displacement = player->velocity * delta_time_second;

		player->velocity *= (1.0 - glm::min(delta_time_second * friction, 1.0));
	}
	else // if player is flying
	{
		double acc = 100.0;
		double drag = 10.0;

		move.y = up - down;

		player->velocity += move * acc * delta_time_second;

		displacement = player->velocity * delta_time_second;

		player->velocity *= (1.0 - glm::min(delta_time_second * drag, 1.0));

	}

	if (player->shouldCollide())
	{
		const glm::dvec3 move_x = {displacement.x, 0.0, 0.0};
		const glm::dvec3 move_y = {0.0, displacement.y, 0.0};
		const glm::dvec3 move_z = {0.0, 0.0, displacement.z};
		const glm::dvec3 move_xz = {displacement.x, 0.0, displacement.z};
		const glm::dvec3 move_yz = {0.0, displacement.y, displacement.z};
		const glm::dvec3 move_xy = {displacement.x, displacement.y, 0.0};

		bool collision_x = hitboxCollisionWithBlock(player->hitbox, player->transform.position + move_x);
		bool collision_y = hitboxCollisionWithBlock(player->hitbox, player->transform.position + move_y);
		bool collision_z = hitboxCollisionWithBlock(player->hitbox, player->transform.position + move_z);
		bool collision_xz = hitboxCollisionWithBlock(player->hitbox, player->transform.position + move_xz);
		bool collision_yz = hitboxCollisionWithBlock(player->hitbox, player->transform.position + move_yz);
		bool collision_xy = hitboxCollisionWithBlock(player->hitbox, player->transform.position + move_xy);

		// edge case when the player is perfectly aligned with the corner of a block
		if (!collision_x && !collision_z && collision_xz)
		{
			// artificially set the collision on the axis with the smallest displacement
			if (std::abs(displacement.x) > std::abs(displacement.z))
				collision_z = true;
			else
				collision_x = true;
		}
		if ((!collision_x && !collision_y && collision_xy) || (!collision_y && !collision_z && collision_yz))
		{
			collision_y = true;
		}

		if (collision_x)
		{
			displacement.x = 0.0;
			player->velocity.x = 0.0;
		}
		if (collision_y)
		{
			displacement.y = -(player->transform.position.y - glm::floor(player->transform.position.y));
			player->velocity.y = 0.0;
		}
		if (collision_z)
		{
			displacement.z = 0.0;
			player->velocity.z = 0.0;
		}
	}

	DebugGui::player_velocity_vec = player->velocity;
	DebugGui::player_velocity = glm::length(player->velocity);

	result.first = player->transform.position;
	result.second = displacement;
	return result;
}

void ClientWorld::updatePlayerTargetBlock(
	const uint64_t player_id
)
{
	std::shared_ptr<Player> player = m_players.at(player_id);
	std::lock_guard<std::mutex> guard(player->mutex);

	glm::dvec3 position = player->transform.position + player->eyePosition();
	glm::dvec3 direction = player->direction();

	RayCastOnBlockResult raycast = rayCastOnBlock(position, direction, 5.0);

	std::optional<glm::vec3> target_block = raycast.hit && !raycast.inside_block ? std::make_optional(raycast.block_position) : std::nullopt;

	m_worldScene.setTargetBlock(target_block);

	player->targeted_block = raycast;
}

std::pair<bool, glm::vec3> ClientWorld::playerAttack(
	const uint64_t player_id,
	bool attack
)
{
	std::shared_ptr<Player> player = m_players.at(player_id);
	std::lock_guard<std::mutex> guard(player->mutex);

	if (!attack || !player->canAttack())
		return {false, glm::vec3(0.0)};
	player->startAttack();

	if (player->targeted_block.hit)
	{
		// LOG_DEBUG("Block hit: "
		// 	<< player->targeted_block.block_position.x << " " << player->targeted_block.block_position.y << " " << player->targeted_block.block_position.z
		// 	<< " = " << int(player->targeted_block.block)
		// );

		// std::lock_guard<std::mutex> lock(m_blocks_to_set_mutex);
		// m_blocks_to_set.push({player->targeted_block.block_position, Block::Air.id});
		return std::make_pair(true, player->targeted_block.block_position);
	}
	// else
	// {
	// 	LOG_DEBUG("No block hit");
	// }
	return std::make_pair(false, glm::vec3(0.0));
}

std::pair<bool, glm::vec3> ClientWorld::playerUse(
	const uint64_t player_id,
	bool use
)
{
	std::shared_ptr<Player> player = m_players.at(player_id);
	std::lock_guard<std::mutex> guard(player->mutex);

	if (!use || !player->canUse())
		return {false, glm::vec3(0.0)};
	player->startUse();

	if (player->targeted_block.hit)
	{
		glm::vec3 block_placed_position = player->targeted_block.block_position + player->targeted_block.normal;

		// check collision with player
		if (isColliding(
			player->hitbox,
			player->transform.position,
			Block::getData(player->targeted_block.block).hitbox,
			block_placed_position
		))
			return {false, glm::vec3(0.0)};

		// std::lock_guard<std::mutex> lock(m_blocks_to_set_mutex);
		// m_blocks_to_set.push({block_placed_position, Block::Stone.id});
		return {true, block_placed_position};
	}
	return {false, glm::vec3(0.0)};
}

void ClientWorld::updatePlayerCamera(
	const uint64_t player_id,
	const double x_offset,
	const double y_offset
)
{
	std::shared_ptr<Player> player = m_players.at(player_id);
	std::unique_lock<std::mutex> guard(player->mutex);

	player->moveDirection(x_offset, y_offset);
}

void ClientWorld::updatePlayer(
	const uint64_t player_id,
	std::function<void(Player &)> update
)
{
	std::shared_ptr<Player> player = m_players.at(player_id);
	std::lock_guard<std::mutex> lock(player->mutex);
	update(*player);
}

void ClientWorld::createMob()
{
	std::lock_guard<std::mutex> lock(m_mobs_mutex);

	std::shared_ptr<Mob> mob = std::make_shared<Mob>();
	mob->transform.position = glm::dvec3(0.0, 220.0, 0.0);
	uint64_t mob_id = m_mob_id++;
	m_mobs.insert(std::make_pair(mob_id, mob));

	{
		// auto world_scene_lock = m_worldScene.entity_mesh_list.lock();
		m_worldScene.entity_mesh_list.insert(
			mob_id,
			{
				m_vulkanAPI.cube_mesh_id,
				Transform(
					mob->transform.position + mob->hitbox.position,
					glm::vec3(0.0f),
					mob->hitbox.size
				).model()
			}
		);
	}
}

void ClientWorld::updateMobs(
	const double delta_time_second
)
{
	std::shared_ptr<Player> player;
	{
		std::lock_guard<std::mutex> lock1(m_players_mutex);
		player = m_players.at(m_my_player_id);
	}
	std::lock_guard<std::mutex> lock2(player->mutex);



	std::lock_guard<std::mutex> lock(m_mobs_mutex);
	for (auto & [id, mob] : m_mobs)
	{
		std::lock_guard<std::mutex> lock(mob->mutex);

		{
			glm::dvec3 chunk_position3D = getChunkPosition(mob->transform.position);
			glm::ivec2 chunk_position2D = glm::ivec2(chunk_position3D.x, chunk_position3D.z);
			std::lock_guard<std::mutex> lock(m_chunks_mutex);
			if (m_loaded_chunks.contains(chunk_position2D) == false)
				continue;
		}

		mob->target_position = player->transform.position;

		// determine if mob is on the ground or in the air and detect
		bool on_ground = hitboxCollisionWithBlock(mob->feet, mob->transform.position);
		if (on_ground && !mob->on_ground) // mob just landed
		{
			mob->jump_remaining = 1;
			mob->jumping = false;
		}
		if (!on_ground && mob->on_ground) // mob just started falling
		{
			mob->startFall();
		}
		mob->on_ground = on_ground;

		glm::dvec3 diff = mob->target_position - mob->transform.position;

		// get the movement vector
		glm::dvec3 move{diff.x, 0.0, diff.z};
		// normalize the move vector to prevent faster diagonal movement
		// but without the y component because we don't want to slow down the mob when moving jumping
		if (glm::length(move) > 0.0)
			move = glm::normalize(move);

		double acc = 30.0;
		double ground_friction = 10.0;
		double air_friction = 0.8;
		glm::dvec3 friction = glm::dvec3(ground_friction, air_friction, ground_friction);

		double jump_force = 9.0;
		double gravity = -25.0;

		if (mob->should_jump && mob->canJump())
		{
			mob->startJump();
			mob->velocity.y = jump_force;
			mob->should_jump = false;
		}

		mob->velocity.y += gravity * delta_time_second;

		mob->velocity += move * acc * delta_time_second;

		glm::dvec3 displacement = mob->velocity * delta_time_second;

		mob->velocity *= (1.0 - glm::min(delta_time_second * friction, 1.0));

		// collision detection
		const glm::dvec3 move_x = {displacement.x, 0.0, 0.0};
		const glm::dvec3 move_y = {0.0, displacement.y, 0.0};
		const glm::dvec3 move_z = {0.0, 0.0, displacement.z};
		const glm::dvec3 move_xz = {displacement.x, 0.0, displacement.z};
		const glm::dvec3 move_yz = {0.0, displacement.y, displacement.z};
		const glm::dvec3 move_xy = {displacement.x, displacement.y, 0.0};

		bool collision_x = hitboxCollisionWithBlock(mob->hitbox, mob->transform.position + move_x);
		bool collision_y = hitboxCollisionWithBlock(mob->hitbox, mob->transform.position + move_y);
		bool collision_z = hitboxCollisionWithBlock(mob->hitbox, mob->transform.position + move_z);
		bool collision_xz = hitboxCollisionWithBlock(mob->hitbox, mob->transform.position + move_xz);
		bool collision_yz = hitboxCollisionWithBlock(mob->hitbox, mob->transform.position + move_yz);
		bool collision_xy = hitboxCollisionWithBlock(mob->hitbox, mob->transform.position + move_xy);
		// edge case when the mob is perfectly aligned with the corner of a block
		if (!collision_x && !collision_z && collision_xz)
		{
			// artificially set the collision on the axis with the smallest displacement
			if (std::abs(displacement.x) > std::abs(displacement.z))
				collision_z = true;
			else
				collision_x = true;
		}
		if ((!collision_x && !collision_y && collision_xy) || (!collision_y && !collision_z && collision_yz))
		{
			collision_y = true;
		}

		if (collision_x)
		{
			displacement.x = 0.0;
			mob->velocity.x = 0.0;
			mob->should_jump = true;
		}
		if (collision_y)
		{
			displacement.y = -(mob->transform.position.y - glm::floor(mob->transform.position.y));
			mob->velocity.y = 0.0;
		}
		if (collision_z)
		{
			displacement.z = 0.0;
			mob->velocity.z = 0.0;
			mob->should_jump = true;
		}

		mob->transform.position += displacement;

		{ // update mob mesh
			auto world_scene_lock = m_worldScene.entity_mesh_list.lock();
			m_worldScene.entity_mesh_list.at(id).model = Transform(
				mob->transform.position + mob->hitbox.position,
				glm::vec3(0.0f),
				mob->hitbox.size
			).model();
		}
	}
}

Camera ClientWorld::getCamera(const uint64_t player_id)
{
	std::shared_ptr<Player> player = m_players.at(player_id);
	std::lock_guard<std::mutex> lock(player->mutex);
	return player->camera();
}

glm::dvec3 ClientWorld::getPlayerPosition(const uint64_t player_id)
{
	std::shared_ptr<Player> player = m_players.at(player_id);
	std::lock_guard<std::mutex> lock(player->mutex);
	return player->transform.position;
}

bool ClientWorld::hitboxCollisionWithBlock(const HitBox & hitbox, const glm::dvec3 & position)
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

				// LOG_DEBUG("Lock m_chunks_mutex");
				// double start = std::chrono::steady_clock::now().time_since_epoch().count();
				std::lock_guard<std::mutex> lock(m_chunks_mutex);
				// double end = std::chrono::steady_clock::now().time_since_epoch().count();
				// LOG_DEBUG("Lock m_chunks_mutex done in " << (end - start) / 1e6 << " ms");
				if (m_loaded_chunks.contains(chunk_position2D))
				{
					Chunk & chunk = m_chunks.at(glm::ivec3(chunk_position.x, 0, chunk_position.z));

					chunk.status.lock_shared();
					BlockID block_id = chunk.getBlock(block_chunk_position);
					chunk.status.unlock_shared();

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

RayCastOnBlockResult ClientWorld::rayCastOnBlock(
	const glm::vec3 & origin,
	const glm::vec3 & direction,
	const double max_distance
)
{
	glm::vec3 position = origin;
	glm::vec3 dir = glm::normalize(direction);
	glm::vec3 block_position = glm::floor(position);

	// step is sign of the direction (used to step through the block grid)
	glm::vec3 step = glm::sign(dir);
	// delta is the distance between blocks following the direction vector
	glm::vec3 delta{
		glm::length(dir * glm::abs(1.0f / dir.x)),
		glm::length(dir * glm::abs(1.0f / dir.y)),
		glm::length(dir * glm::abs(1.0f / dir.z))
	};
	// side_dist is the distance from the current position to the next block
	glm::vec3 side_dist{
		(dir.x > 0.0f ? (block_position.x + 1.0f - position.x) : (position.x - block_position.x)) * delta.x,
		(dir.y > 0.0f ? (block_position.y + 1.0f - position.y) : (position.y - block_position.y)) * delta.y,
		(dir.z > 0.0f ? (block_position.z + 1.0f - position.z) : (position.z - block_position.z)) * delta.z
	};

	int axis = -1;
	float min_side_dist = 0.0f;
	while (min_side_dist < max_distance)
	{
		glm::vec3 block_chunk_position = getBlockChunkPosition(block_position);
		glm::vec3 chunk_position = getChunkPosition(block_position);
		glm::ivec2 chunk_position2D = glm::ivec2(chunk_position.x, chunk_position.z);
		{
			std::lock_guard<std::mutex> lock(m_chunks_mutex);
			if (m_loaded_chunks.contains(chunk_position2D))
			{
				Chunk & chunk = m_chunks.at(glm::ivec3(chunk_position.x, 0, chunk_position.z));

				chunk.status.lock_shared();
				BlockID block_id = chunk.getBlock(block_chunk_position);
				chunk.status.unlock_shared();

				if (Block::hasProperty(block_id, BLOCK_PROPERTY_SOLID))
				{
					// for now treat all blocks as cubes
					glm::vec3 normal = glm::vec3(0.0f);
					bool inside_block = false;
					if (axis != -1)
						normal[axis] = -step[axis];
					else
					{
						inside_block = true;
					}

					const glm::vec3 hit_position = position + dir * min_side_dist;

					return {
						true,
						block_position,
						normal,
						hit_position,
						block_id,
						inside_block
					};
				}
			}
		}

		// find the axis with the smallest side_dist
		axis = 0;
		if (side_dist.y < side_dist.x)
		{
			axis = 1;
			if (side_dist.z < side_dist.y)
				axis = 2;
		}
		else if (side_dist.z < side_dist.x)
		{
			axis = 2;
		}

		// save the minimum side_dist
		min_side_dist = side_dist[axis];

		// increment the side_dist
		side_dist[axis] += delta[axis];

		// increment the block position
		block_position[axis] += step[axis];
	}

	return {
		false,
		glm::vec3(0.0f),
		glm::vec3(0.0f),
		glm::vec3(0.0f),
		BlockID::Air,
		false
	};
}

void ClientWorld::addPlayer(const uint64_t player_id, const glm::dvec3 & position)
{
	std::shared_ptr<Player> player = std::make_shared<Player>();
	player->transform.position = position;
	m_players.insert(std::make_pair(player_id, player));

	{
		// auto world_scene_lock = m_worldScene.entity_mesh_list.lock();
		m_worldScene.entity_mesh_list.insert(
			player_id,
			{
				m_vulkanAPI.cube_mesh_id,
				Transform(
					player->transform.position + player->hitbox.position,
					glm::vec3(0.0f),
					player->hitbox.size
				).model()
			}
		);
	}
}

void ClientWorld::removePlayer(const uint64_t player_id)
{
	std::shared_ptr<Player> player = m_players.at(player_id);
	std::lock_guard<std::mutex> lock(player->mutex);

	m_players.erase(player_id);
	{
		// auto world_scene_lock = m_worldScene.entity_mesh_list.lock();
		m_worldScene.entity_mesh_list.erase(player_id);
	}
}

void ClientWorld::modifyBlock(const glm::vec3 & position, const BlockID & block_id)
{
	std::lock_guard<std::mutex> lock(m_blocks_to_set_mutex);
	m_blocks_to_set.push({position, block_id});
}
