#include "World.hpp"

World::World()
{
}

World::~World()
{
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

std::shared_ptr<Chunk> World::getChunk(const glm::ivec3 & position) const
{
	std::lock_guard lock(m_chunks_mutex);
	LockMark(m_chunks_mutex);
	getChunkNoLock(position);
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
