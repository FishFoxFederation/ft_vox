#include "ServerWorld.hpp"

void ServerWorld::updateBlocks()
{
	ZoneScopedN("Update Blocks");
	std::lock_guard lock(m_block_updates_mutex);
	//do random block ticks

	while (!m_block_updates.empty())
	{
		auto block_update = m_block_updates.front();
		m_block_updates.pop();

		switch (block_update.type)
		{
			case BlockUpdateData::Type::PLACE:
			{
				placeBlock(block_update.position, block_update.block);
				break;
			}
			case BlockUpdateData::Type::DESTROY:
			{
				placeBlock(block_update.position, BlockInfo::Type::Air);
				break;
			}
			case BlockUpdateData::Type::UPDATE:
			{
				break;
			}
			case BlockUpdateData::Type::RANDOM:
			{
				break;
			}

		}
	}
}

void ServerWorld::addBlockUpdate(const BlockUpdateData & data)
{
	std::lock_guard lock(m_block_updates_mutex);
	m_block_updates.push(data);
}

// std::shared_ptr<Chunk> ServerWorld::getAndLoadChunk(const glm::ivec3 & chunk_position)
// {
// 	std::shared_ptr<Chunk> chunk = getChunk(chunk_position);

// 	if (chunk == nullptr)
// 	{
// 		loadChunk(chunk_position);
// 		chunk = getChunk(chunk_position);
// 	}

// 	return chunk;
// }

void ServerWorld::placeBlock(const glm::vec3 & position, BlockInfo::Type block)
{
	glm::ivec3 chunk_position = getChunkPosition(position);
	glm::ivec3 block_chunk_position = getBlockChunkPosition(position);

	std::shared_ptr<Chunk> chunk = getChunk(chunk_position);
	if (chunk == nullptr)
		return;

	auto packet = std::make_shared<BlockActionPacket>(block, position, BlockActionPacket::Action::PLACE);
	{
		std::lock_guard lock(chunk->status);
		chunk->setBlock(block_chunk_position, block);
		for (auto id : chunk->observing_player_ids)
		{
			packet->SetConnectionId(m_player_to_connection_id.at(id));
			m_server.send(packet);
		}
	}
	{ // Add the world position of the block to the light update queue
		std::lock_guard light_lock(m_block_light_update_mutex);
		m_block_light_update.push(position);
	}
}

void ServerWorld::setBlock(const glm::vec3 & position, BlockInfo::Type block)
{
	glm::ivec3 chunk_position = getChunkPosition(position);
	glm::ivec3 block_chunk_position = getBlockChunkPosition(position);

	std::shared_ptr<Chunk> chunk = getChunk(chunk_position);
	if (chunk == nullptr)
		return;
	std::lock_guard lock(chunk->status);
	chunk->setBlock(block_chunk_position, block);
}

// void ServerWorld::loadChunk(const glm::ivec3 & chunk_position)
// {
// 	std::shared_ptr<Chunk> chunk = m_world_generator.generateChunkColumn(chunk_position.x, chunk_position.z);
// 	{
// 		std::lock_guard lock(m_chunks_mutex);
// 		m_chunks.insert({chunk_position, std::move(chunk)});
// 	}
// }

ServerWorld::ChunkLoadUnloadData ServerWorld::getChunksToUnload(
	const glm::vec3 & old_player_position,
	const glm::vec3 & new_player_position)
{
	ChunkLoadUnloadData data;

	glm::ivec3 new_player_chunk_position = getChunkPosition(new_player_position);
	glm::ivec3 old_player_chunk_position = getChunkPosition(old_player_position);

	new_player_chunk_position.y = 0;
	old_player_chunk_position.y = 0;
	// glm::ivec3 chunk_direction = new_player_chunk_position - old_player_chunk_position;

	std::unordered_set<glm::ivec3> old_chunks_in_range;
	std::unordered_set<glm::ivec3> new_chunks_in_range;


	for(int x = -SERVER_LOAD_DISTANCE; x <= SERVER_LOAD_DISTANCE; x++)
	{
		for(int z = -SERVER_LOAD_DISTANCE; z <= SERVER_LOAD_DISTANCE; z++)
		{
			glm::ivec3 chunk_position = old_player_chunk_position + glm::ivec3(x, 0, z);
			float distance = glm::distance(glm::vec2(chunk_position.x, chunk_position.z), glm::vec2(old_player_chunk_position.x, old_player_chunk_position.z));
			if (distance < SERVER_LOAD_DISTANCE)
				old_chunks_in_range.insert(chunk_position);
		}
	}

	for(int x = -SERVER_LOAD_DISTANCE; x <= SERVER_LOAD_DISTANCE; x++)
	{
		for(int z = -SERVER_LOAD_DISTANCE; z <= SERVER_LOAD_DISTANCE; z++)
		{
			glm::ivec3 chunk_position = new_player_chunk_position + glm::ivec3(x, 0, z);
			float distance = glm::distance(glm::vec2(chunk_position.x, chunk_position.z), glm::vec2(new_player_chunk_position.x, new_player_chunk_position.z));
			if (distance < SERVER_LOAD_DISTANCE)
				new_chunks_in_range.insert(chunk_position);
		}
	}

	for (auto chunk_position : old_chunks_in_range)
	{
		if (!new_chunks_in_range.contains(chunk_position))
			data.chunks_to_unload.push_back(chunk_position);
	}

	for (auto chunk_position : new_chunks_in_range)
	{
		if (!old_chunks_in_range.contains(chunk_position))
		{
			std::shared_ptr<Chunk> chunk = getChunkNoLock(chunk_position);
			data.chunks_to_load.push_back(chunk);
		}
	}

	return data;
}

ServerWorld::ChunkLoadUnloadData ServerWorld::updateChunkObservations(uint64_t player_id)
{
	ChunkLoadUnloadData data;

	bool first_time = !m_last_tick_player_positions.contains(player_id);
	if (first_time)
		m_last_tick_player_positions.insert({player_id, m_current_tick_player_positions.at(player_id)});
	glm::ivec3 new_player_chunk_position = getChunkPosition(m_current_tick_player_positions.at(player_id));
	glm::ivec3 old_player_chunk_position = getChunkPosition(m_last_tick_player_positions.at(player_id));

	new_player_chunk_position.y = 0;
	old_player_chunk_position.y = 0;
	if (!first_time && new_player_chunk_position == old_player_chunk_position)
		return data;

	std::unordered_set<glm::ivec3> old_chunks_in_range;
	std::unordered_set<glm::ivec3> new_chunks_in_range;


	//fill old_chunks_in_range
	if(!first_time)
	{
		for(int x = -SERVER_LOAD_DISTANCE; x <= SERVER_LOAD_DISTANCE; x++)
		{
			for(int z = -SERVER_LOAD_DISTANCE; z <= SERVER_LOAD_DISTANCE; z++)
			{
				glm::ivec3 chunk_position = old_player_chunk_position + glm::ivec3(x, 0, z);
				old_chunks_in_range.insert(chunk_position);
			}
		}
	}

	//fill new_chunks_in_range
	for(int x = -SERVER_LOAD_DISTANCE; x <= SERVER_LOAD_DISTANCE; x++)
	{
		for(int z = -SERVER_LOAD_DISTANCE; z <= SERVER_LOAD_DISTANCE; z++)
		{
			glm::ivec3 chunk_position = new_player_chunk_position + glm::ivec3(x, 0, z);
			new_chunks_in_range.insert(chunk_position);
		}
	}

	for (auto chunk_position : old_chunks_in_range)
	{
		//if chunk is now too far away
		if (!new_chunks_in_range.contains(chunk_position))
		{
			std::shared_ptr<Chunk> chunk = getChunk(chunk_position);
			chunk->status.lock();
			chunk->observing_player_ids.erase(player_id);
			chunk->status.unlock();
			data.chunks_to_unload.push_back(chunk_position);
		}
	}

	for (auto chunk_position : new_chunks_in_range)
	{
		//if chunk is now in range it is new
		if (!old_chunks_in_range.contains(chunk_position))
		{
			std::shared_ptr<Chunk> chunk = getChunk(chunk_position);
			if (chunk == nullptr)
			{
				LOG_CRITICAL("Chunk is nullptr pos:" << chunk_position.x << " " << chunk_position.y << " " << chunk_position.z);
			}
			chunk->observing_player_ids.insert(player_id);
			data.chunks_to_load.push_back(chunk);
		}
	}

	return data;
}

void ServerWorld::removeChunkObservations(std::shared_ptr<Player> player)
{
	uint64_t player_id = player->player_id;
	glm::ivec3 player_chunk_position = getChunkPosition(player->transform.position);
	player_chunk_position.y = 0;
	std::shared_ptr<Chunk> chunk = nullptr;
	for(int x = -SERVER_LOAD_DISTANCE; x <= SERVER_LOAD_DISTANCE; x++)
	{
		for(int z = -SERVER_LOAD_DISTANCE; z <= SERVER_LOAD_DISTANCE; z++)
		{
			glm::ivec3 chunk_position = player_chunk_position + glm::ivec3(x, 0, z);
			chunk = getChunk(chunk_position);
			if (chunk == nullptr) continue;
			std::lock_guard(chunk->status);
			chunk->observing_player_ids.erase(player_id);
		}
	}
}

ChunkMap ServerWorld::getChunkZone(glm::ivec3 zoneStart, glm::ivec3 zoneSize)
{
	std::unordered_map<glm::ivec3, std::shared_ptr<Chunk>> chunks;
	for(int x = 0; x < zoneSize.x; x++)
	{
		for(int y = 0; y < zoneSize.z; y++)
		{
			glm::ivec3 chunk_position = zoneStart + glm::ivec3(x, 0, y);
			std::shared_ptr<Chunk> chunk = getChunkNoLock(chunk_position);
			if (chunk == nullptr)
			{
				chunk = std::make_shared<Chunk>(chunk_position);
				m_chunks.insert({chunk_position, chunk});
				chunk->status.lock();
			}
			else
				chunk->status.lock();
			chunks.insert({chunk_position, chunk});
		}
	}
	return chunks;
}

// uint64_t ServerWorld::asyncGenChunk(const glm::ivec3 & chunkPos3D, Chunk::genLevel gen_level , Chunk::genLevel current_gen_level = Chunk::genLevel::EMPTY)
// {
// 	ChunkMap chunksToGen;
// 	WorldGenerator::genInfo info = m_world_generator.getGenInfo(gen_level, current_gen_level, chunkPos3D);

// 	chunksToGen = getChunkZone(info.zoneStart, info.zoneSize);

// 	return m_threadPool.submit([this, info, chunksToGen] () mutable
// 	{
// 		ZoneScopedN("Generate Chunk");
// 		// m_world_generator.generateChunkColumn(chunkPos3D.x, chunkPos3D.z, chunk);
// 		m_world_generator.generate(info, chunksToGen);

// 		for (auto & [chunk_position, chunk] : chunksToGen)
// 			chunk->status.unlock();
// 	});
// }

void ServerWorld::doChunkGens(WorldGenerator::ChunkGenList & chunks_to_gen)
{
	m_chunk_gen_data.graph = m_world_generator.getGenerationGraph(chunks_to_gen);
	LOG_INFO("Starting chunk gen");
	m_chunk_gen_data.future = m_executor.run(m_chunk_gen_data.graph);
}

void ServerWorld::updateLights()
{
	/*
	 * This function is the same for the server and the client.
	 * TODO: decide if it should be moved to the World class
	 *
	 * If this comment is still here and the functions are different,
	 * ask me why the fuck it is the case :)
	 */
	for (;;)
	{
		glm::ivec3 position;
		{
			std::lock_guard light_update_lock(m_block_light_update_mutex);
			if (m_block_light_update.empty())
				break;

			position = m_block_light_update.front();
			m_block_light_update.pop();
		}

		// This check is also present in the updateLight functions below
		// TODO: decide where it should be kept
		const glm::ivec3 chunk_position = getChunkPosition(position);
		std::shared_ptr<Chunk> chunk = getChunk(chunk_position);
		if (chunk == nullptr)
			continue;

		updateSkyLight(position);
		updateBlockLight(position);
	}
}
