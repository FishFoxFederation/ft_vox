#include "ServerWorld.hpp"

ServerWorld::ServerWorld()
{
	std::shared_ptr<Chunk> chunk;

	std::lock_guard<std::mutex> lock(m_chunks_mutex);

	chunk = std::make_shared<Chunk>(m_world_generator.generateChunkColumn(0, 0));
	this->m_chunks.insert({glm::ivec3(0, 0, 0), chunk});

	chunk = std::make_shared<Chunk>(m_world_generator.generateChunkColumn(0, 1));
	this->m_chunks.insert({glm::ivec3(0, 0, 1), chunk});

	chunk = std::make_shared<Chunk>(m_world_generator.generateChunkColumn(1, 0));
	this->m_chunks.insert({glm::ivec3(1, 0, 0), chunk});

	chunk = std::make_shared<Chunk>(m_world_generator.generateChunkColumn(1, 1));
	this->m_chunks.insert({glm::ivec3(1, 0, 1), chunk});
}

ServerWorld::~ServerWorld()
{
}

std::shared_ptr<Chunk> ServerWorld::getAndLoadChunk(const glm::ivec3 & chunk_position)
{
	std::shared_ptr<Chunk> chunk = getChunk(chunk_position);

	if (chunk == nullptr)
	{
		loadChunk(chunk_position);
		chunk = getChunk(chunk_position);
	}
	
	return chunk;
}

void ServerWorld::setBlock(const glm::vec3 & position, BlockID block)
{
	glm::ivec3 chunk_position = getChunkPosition(position);
	glm::ivec3 block_chunk_position = getBlockChunkPosition(position);

	std::shared_ptr<Chunk> chunk = getChunk(chunk_position);
	if (chunk == nullptr)
		return;
	std::lock_guard<Status> lock(chunk->status);
	chunk->setBlock(block_chunk_position, block);
}

void ServerWorld::loadChunk(const glm::ivec3 & chunk_position)
{
	std::shared_ptr<Chunk> chunk = std::make_shared<Chunk>(m_world_generator.generateChunkColumn(chunk_position.x, chunk_position.z));
	{
		std::lock_guard<std::mutex> lock(m_chunks_mutex);
		m_chunks.insert({chunk_position, std::move(chunk)});
	}
}

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
			std::shared_ptr<Chunk> chunk = getAndLoadChunk(chunk_position);
			data.chunks_to_load.push_back(chunk);
		}
	}
	
	return data;
}
