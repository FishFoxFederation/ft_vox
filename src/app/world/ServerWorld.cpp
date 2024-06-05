#include "ServerWorld.hpp"

ServerWorld::ServerWorld()
{
	this->m_chunks.insert({glm::ivec3(0, 0, 0), m_world_generator.generateChunkColumn(0, 0)});
	this->m_chunks.insert({glm::ivec3(0, 0, 1), m_world_generator.generateChunkColumn(0, 1)});
	this->m_chunks.insert({glm::ivec3(1, 0, 0), m_world_generator.generateChunkColumn(1, 0)});
	this->m_chunks.insert({glm::ivec3(1, 0, 1), m_world_generator.generateChunkColumn(1, 1)});
}

ServerWorld::~ServerWorld()
{
}

Chunk & ServerWorld::getChunk(const glm::ivec3 & chunk_position)
{
	std::lock_guard<std::mutex> lock(m_chunks_mutex);
	return m_chunks.at(chunk_position);
}

const Chunk & ServerWorld::getChunk(const glm::ivec3 & chunk_position) const
{
	std::lock_guard<std::mutex> lock(m_chunks_mutex);
	return m_chunks.at(chunk_position);
}

void ServerWorld::setBlock(const glm::vec3 & position, BlockID block)
{
	glm::ivec3 chunk_position = getChunkPosition(position);
	glm::ivec3 block_chunk_position = getBlockChunkPosition(position);

	Chunk & chunk = getChunk(chunk_position);
	std::lock_guard<Status> lock(chunk.status);
	chunk.setBlock(block_chunk_position, block);
}
