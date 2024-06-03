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
