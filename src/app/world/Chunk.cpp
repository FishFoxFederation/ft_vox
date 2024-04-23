#include "Chunk.hpp"
#include "logger.hpp"

Chunk::Chunk(glm::ivec3 position)
: position(position), m_mesh_id(0)
{
	(void)position;
	// LOG_INFO("Chunk created at position: " << position.x << " " << position.y << " " << position.z);
	for(int i = 0; i < BLOCKS_PER_CHUNK; i++)
		m_blocks[i] = BlockID::Air;
}

// Chunk::Chunk(const Chunk & other)
// :	position(other.position),
// 	m_mesh_id(other.m_mesh_id),
// 	m_blocks(other.m_blocks)
// {
// }

Chunk::Chunk(Chunk && other)
:	position(other.position),
	m_mesh_id(other.m_mesh_id),
	m_blocks(std::move(other.m_blocks))
{
}

Chunk & Chunk::operator=(const Chunk && other)
{
	m_mesh_id = other.m_mesh_id;
	m_blocks = std::move(other.m_blocks);
	return *this;
}

Chunk::~Chunk()
{

}

BlockID Chunk::getBlock(const glm::ivec3 & position) const
{
	int index = toIndex(position);

	return m_blocks[index];
}

void Chunk::setBlock(const glm::ivec3 & position, BlockID block)
{
	int index = toIndex(position);

	m_blocks[index] = block;
}

const uint64_t & Chunk::getMeshID() const
{
	return m_mesh_id;
}

void Chunk::setMeshID(const uint64_t & mesh_id)
{
	m_mesh_id = mesh_id;
}


int Chunk::toIndex(const glm::ivec3 & pos)
{
	return pos.x + pos.y * CHUNK_X_SIZE + pos.z * CHUNK_X_SIZE * CHUNK_Y_SIZE;
}

glm::ivec3	Chunk::toCoord(const int & index)
{
	int x = index % CHUNK_X_SIZE;
	int y = (index - x) % CHUNK_Y_SIZE;
	int z = (index - x - y) % CHUNK_Z_SIZE;
	return glm::ivec3(x, y, z);
}

void Chunk::ChunkStatus::addWorking()
{
	std::lock_guard<std::mutex> lock(m_mutex);
	m_flags |= ChunkStatus::WORKING;
	m_counter++;
}

void Chunk::ChunkStatus::removeWorking()
{
	std::lock_guard<std::mutex> lock(m_mutex);
	m_counter--;
	if (m_counter == 0)
	{
		m_flags ^= ChunkStatus::WORKING;
		m_cv.notify_all();
	}
}

void Chunk::ChunkStatus::setFlag(const uint64_t & flag)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	m_flags |= flag;
}

void Chunk::ChunkStatus::clearFlag(const uint64_t & flag)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	m_flags ^= flag;
	if (m_flags == ChunkStatus::CLEAR)
		m_cv.notify_all();
}

void Chunk::ChunkStatus::waitForClear()
{
	std::condition_variable cv;
	std::unique_lock<std::mutex> lock(m_mutex);
	m_cv.wait(lock, [&] {return m_flags == ChunkStatus::CLEAR; });
	return;
}

bool Chunk::ChunkStatus::isSet(const uint64_t & flag)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	return m_flags & flag;
}

bool Chunk::ChunkStatus::isClear()
{
	std::lock_guard<std::mutex> lock(m_mutex);
	return m_flags == ChunkStatus::CLEAR;
}
