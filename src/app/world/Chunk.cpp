#include "Chunk.hpp"
#include "logger.hpp"

Chunk::Chunk(glm::ivec3 position)
: m_position(position)
{
	(void)m_position;
	// LOG_INFO("Chunk created at position: " << m_position.x << " " << m_position.y << " " << m_position.z);
	for(auto & block : m_blocks)
		block = Block::Air;
}

Chunk::~Chunk()
{

}

Block Chunk::getBlock(const int & x, const int & y, const int & z) const
{
	int index = toIndex(x, y, z);

	return m_blocks[index];
}

void Chunk::setBlock(const int & x, const int & y, const int & z, Block block)
{
	int index = toIndex(x, y, z);

	m_blocks[index] = block;
	//REGENERATE MESH HERE
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
