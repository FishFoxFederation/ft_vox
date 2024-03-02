#include "WorldGenerator.hpp"

#include <cmath>

WorldGenerator::WorldGenerator()
: m_relief_perlin(0, 1, 1, 1, 2)
{

}

WorldGenerator::~WorldGenerator()
{

}

Chunk WorldGenerator::generateChunk(const int & x, const int & y, const int & z)
{
	Chunk chunk(glm::ivec3(x, y, z));
	(void)x, (void)y;

	for(int blockX = 0; blockX < CHUNK_SIZE; blockX++)
	{
		for(int blockY = 0; blockY < CHUNK_SIZE; blockY++)
		{
			for(int blockZ = 0; blockZ < CHUNK_SIZE; blockZ++)
			{
				float value = m_relief_perlin.noise(glm::vec2(
					(blockX + x * CHUNK_SIZE) * 0.01f,
					(blockZ + z * CHUNK_SIZE) * 0.01f
				));
				value = ((value + 1) / 2) * 256;
				if (blockY + y * CHUNK_SIZE < value)
					chunk.setBlock(blockX, blockY, blockZ, Block::Grass);
				else
					chunk.setBlock(blockX, blockY, blockZ, Block::Air);
			}
		}
	}
	return chunk;
}
