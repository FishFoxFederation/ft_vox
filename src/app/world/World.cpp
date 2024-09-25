#include "World.hpp"

World::World()
{
}

World::~World()
{
}

glm::vec3 World::getBlockChunkPosition(const glm::vec3 & position)
{
	// glm::vec3 block_chunk_position = glm::ivec3(position) % CHUNK_SIZE_IVEC3;
	// if (block_chunk_position.x < 0) block_chunk_position.x += CHUNK_X_SIZE;
	// if (block_chunk_position.y < 0) block_chunk_position.y += CHUNK_Y_SIZE;
	// if (block_chunk_position.z < 0) block_chunk_position.z += CHUNK_Z_SIZE;

	// return block_chunk_position;

	// use the function from Chunk.hpp
	return ::getBlockChunkPos(glm::ivec3(position));
}

glm::vec3 World::getChunkPosition(const glm::vec3 & position)
{
	// return glm::floor(position / CHUNK_SIZE_VEC3);

	// use the function from Chunk.hpp
	return ::getChunkPos(glm::ivec3(position));
}

std::shared_ptr<Chunk> World::getChunk(const glm::ivec3 & position) const
{
	std::lock_guard lock(m_chunks_mutex);
	LockMark(m_chunks_mutex);
	return getChunkNoLock(position);
}

std::shared_ptr<Chunk> World::getChunkNoLock(const glm::ivec3 & position) const
{
	auto it = m_chunks.find(position);
	if (it != m_chunks.end())
		return it->second;
	return nullptr;
}

void World::insertChunk(const glm::ivec3 & position, std::shared_ptr<Chunk> chunk)
{
	std::lock_guard lock(m_chunks_mutex);
	auto ret =	m_chunks.insert(std::make_pair(position, chunk));
	if (!ret.second)
	{
		LOG_ERROR("Failed to insert chunk");
	}

	if (!m_chunks.contains(position))
	{
		LOG_CRITICAL("IDK WTF");
	}
}

void World::insertChunkNoLock(const glm::ivec3 & position, std::shared_ptr<Chunk> chunk)
{
	auto ret =	m_chunks.insert(std::make_pair(position, chunk));
	if (!ret.second)
	{
		LOG_ERROR("Failed to insert chunk");
	}

	if (!m_chunks.contains(position))
	{
		LOG_CRITICAL("IDK WTF");
	}
}

// void World::waitForFinishedFutures()
// {
// 	ZoneScoped;
// 	std::lock_guard lock(m_finished_futures_mutex);
// 	while(!m_finished_futures.empty())
// 	{
// 		uint64_t id = m_finished_futures.front();
// 		m_finished_futures.pop();
// 		auto & future = m_futures.at(id);
// 		future.get();
// 		m_futures.erase(id);
// 	}
// }

// void World::waitForFutures()
// {
// 	while(!m_futures.empty())
// 	{
// 		m_futures.begin()->second.get();
// 		m_futures.erase(m_futures.begin());
// 	}
// }

void World::updateSkyLight(const glm::ivec3 & block_position)
{
	const glm::ivec3 start_chunk_pos = getChunkPos(block_position);
	std::shared_ptr<Chunk> start_chunk = getChunk(start_chunk_pos);
	if (start_chunk == nullptr)
	{
		LOG_WARNING("World::updateSkyLight: start_chunk is nullptr.");
		return;
	}

	constexpr std::array<glm::ivec3, 6> NEIGHBOR_OFFSETS = {
		glm::ivec3(1, 0, 0),
		glm::ivec3(-1, 0, 0),
		glm::ivec3(0, 1, 0),
		glm::ivec3(0, -1, 0),
		glm::ivec3(0, 0, 1),
		glm::ivec3(0, 0, -1)
	};

	start_chunk->status.lock_shared();
	for (int x = 0; x < CHUNK_X_SIZE; x++)
	{
		for (int y = 0; y < CHUNK_Y_SIZE; y++)
		{
			for (int z = 0; z < CHUNK_Z_SIZE; z++)
			{
				start_chunk->setSkyLight(x, y, z, 0);
			}
		}
	}

	std::queue<glm::ivec3> light_queue;
	for (int x = 0; x < CHUNK_X_SIZE; x++)
	{
		for (int z = 0; z < CHUNK_Z_SIZE; z++)
		{
			const glm::ivec3 block_chunk_pos = glm::ivec3(x, CHUNK_Y_SIZE - 1, z);
			const glm::ivec3 block_world_pos = start_chunk_pos * CHUNK_SIZE_IVEC3 + block_chunk_pos;
			const BlockID block_id = start_chunk->getBlock(block_chunk_pos);

			if (!Block::hasProperty(block_id, BLOCK_PROPERTY_OPAQUE))
			{
				const int absorbed_light = Block::getData(block_id).absorb_light;
				const int new_light = 15 - absorbed_light;

				start_chunk->setSkyLight(block_chunk_pos, new_light);
				light_queue.push(block_world_pos);
			}
		}
	}
	start_chunk->status.unlock_shared();

	std::unordered_set<glm::ivec3> chunks_that_should_be_remeshed;
	while (!light_queue.empty())
	{
		if (light_queue.size() > BLOCKS_PER_CHUNK)
		{
			LOG_WARNING("World::updateSkyLight: light_queue size > BLOCKS_PER_CHUNK (" << BLOCKS_PER_CHUNK << ")");
			break;
		}

		const glm::ivec3 current_block_world_pos = light_queue.front();
		light_queue.pop();

		const glm::ivec3 chunk_pos = getChunkPos(current_block_world_pos);
		const glm::ivec3 block_chunk_pos = getBlockChunkPos(current_block_world_pos);
		const std::shared_ptr<Chunk> chunk = getChunk(chunk_pos);
		if (chunk == nullptr)
		{
			LOG_WARNING("World::updateSkyLight: visited chunk is nullptr.");
			continue;
		}

		chunk->status.lock_shared();
		const BlockID current_id = chunk->getBlock(block_chunk_pos);
		// not const because it can change when looking at neighbors
		int current_light = chunk->getSkyLight(block_chunk_pos);
		chunk->status.unlock_shared();
		const int current_absorb_light = Block::getData(current_id).absorb_light;

		for (int i = 0; i < 6; i++)
		{
			const glm::ivec3 neighbor_world_pos = current_block_world_pos + NEIGHBOR_OFFSETS[i];
			const glm::ivec3 neighbor_chunk_pos = getChunkPos(neighbor_world_pos);
			const glm::ivec3 neighbor_block_chunk_pos = getBlockChunkPos(neighbor_world_pos);

			if (neighbor_chunk_pos.y != 0)
				continue;

			std::shared_ptr<Chunk> neighbor_chunk = getChunk(neighbor_chunk_pos);
			if (neighbor_chunk != nullptr)
			{
				neighbor_chunk->status.lock_shared();
				const BlockID neighbor_id = neighbor_chunk->getBlock(neighbor_block_chunk_pos);
				const int neighbor_light = neighbor_chunk->getSkyLight(neighbor_block_chunk_pos);

				if (!Block::hasProperty(neighbor_id, BLOCK_PROPERTY_OPAQUE))
				{
					const int neighbor_absorbed_light = Block::getData(neighbor_id).absorb_light;

					/*
					 * https://minecraft.fandom.com/wiki/Light#Sky_light
					 *
					 * When sky light of a level of 15 spreads down through a transparent block, the level remains unchanged.
					 * When it spreads horizontally or upward, it reduces 1 light level.
					 * However, when it spreads through a light-filtering block, it does not follow the above two rules and attenuates specific light levels.
					 * Sky light with a level less than 15 spreads as block light - when it propagates to adjacent (including top and bottom, six blocks in total) blocks,
					 * it is attenuated until it is 0.
					 *
					 * This implementation reduces the light level by 1 even when passing through a light-filtering block.
					*/

					int neighbor_new_light = current_light
						- (neighbor_absorbed_light > 0) * neighbor_absorbed_light
						- (i != 3 || current_light != 15) * 1; // i == 3 is when the current light spreads down

					int current_new_light = neighbor_light
						- (current_absorb_light > 0) * current_absorb_light
						- (i != 2 || neighbor_light != 15) * 1; // i == 2 is when the neighbor light spreads down


					if (neighbor_new_light > neighbor_light)
					{
						neighbor_chunk->setSkyLight(neighbor_block_chunk_pos, neighbor_new_light);
						light_queue.push(neighbor_world_pos);
						// remesh the neighbor chunk
						neighbor_chunk->setMeshed(false);
					}
					else if (current_new_light > current_light)
					{
						chunk->setSkyLight(block_chunk_pos, current_new_light);
						light_queue.push(current_block_world_pos);
						// update the local variable current_light for the next neighbors
						current_light = current_new_light;
					}
				}
				neighbor_chunk->status.unlock_shared();
			}
			else
			{
				LOG_WARNING("World::updateSkyLight: neighbor_chunk  is nullptr.");
			}
		}
	}
}

void World::updateBlockLight(const glm::ivec3 & block_position)
{

}
