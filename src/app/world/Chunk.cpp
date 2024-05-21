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
	status(other.status),
	m_mesh_id(other.m_mesh_id),
	m_blocks(std::move(other.m_blocks))
{
}

Chunk & Chunk::operator=(const Chunk && other)
{
	m_mesh_id = other.m_mesh_id;
	m_blocks = std::move(other.m_blocks);
	status = other.status;
	return *this;
}

Chunk::~Chunk()
{
	if (!status.isLocked())
	{
		status.lock();
	}
}

BlockID Chunk::getBlock(const int & x, const int & y, const int & z) const
{
	int index = toIndex(x, y, z);

	return m_blocks[index];
}

BlockID Chunk::getBlock(const glm::vec3 & position) const
{
	return getBlock((int)position.x, (int)position.y, (int)position.z);
}

void Chunk::setBlock(const int & x, const int & y, const int & z, BlockID block)
{
	int index = toIndex(x, y, z);

	m_blocks[index] = block;
	//REGENERATE MESH HERE
}

void Chunk::setBlock(const glm::vec3 & position, BlockID block)
{
	setBlock((int)position.x, (int)position.y, (int)position.z, block);
}

const uint64_t & Chunk::getMeshID() const
{
	return m_mesh_id;
}

void Chunk::setMeshID(const uint64_t & mesh_id)
{
	m_mesh_id = mesh_id;
}

int Chunk::toIndex(const int & x, const int & y, const int & z)
{
	return x + y * CHUNK_X_SIZE + z * CHUNK_X_SIZE * CHUNK_Y_SIZE;
}

glm::ivec3	Chunk::toCoord(const int & index)
{
	int x = index % CHUNK_X_SIZE;
	int y = (index - x) % CHUNK_Y_SIZE;
	int z = (index - x - y) % CHUNK_Z_SIZE;
	return glm::ivec3(x, y, z);
}
