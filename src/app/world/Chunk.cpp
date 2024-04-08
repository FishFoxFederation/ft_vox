#include "Chunk.hpp"
#include "logger.hpp"

Chunk::Chunk(glm::ivec3 position)
: position(position), m_mesh_id(0)
{
	(void)position;
	// LOG_INFO("Chunk created at position: " << position.x << " " << position.y << " " << position.z);
	for(auto & block : m_blocks)
		block = BlockID::Air;
}

Chunk::Chunk(const Chunk & other)
:	position(other.position),
	m_mesh_id(other.m_mesh_id),
	m_blocks(other.m_blocks)
{
}

Chunk::Chunk(Chunk && other)
:	position(other.position),
	m_mesh_id(other.m_mesh_id),
	m_blocks(std::move(other.m_blocks))
{
}

Chunk::~Chunk()
{

}

BlockID Chunk::getBlock(const int & x, const int & y, const int & z) const
{
	int index = toIndex(x, y, z);

	return m_blocks[index];
}

void Chunk::setBlock(const int & x, const int & y, const int & z, BlockID block)
{
	int index = toIndex(x, y, z);

	m_blocks[index] = block;
	//REGENERATE MESH HERE
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
	return x + y * CHUNK_SIZE + z * CHUNK_SIZE * CHUNK_SIZE;
}

glm::ivec3	Chunk::toCoord(const int & index)
{
	int x = index % CHUNK_SIZE;
	int y = (index / CHUNK_SIZE) % CHUNK_SIZE;
	int z = index / (CHUNK_SIZE * CHUNK_SIZE);
	return glm::ivec3(x, y, z);
}
