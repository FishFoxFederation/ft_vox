#include "WorldGenerator.hpp"

#include <cmath>

WorldGenerator::WorldGenerator()
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
				// if (blockZ + z * CHUNK_SIZE < 128)
				// 	chunk.setBlock(blockX, blockY, blockZ, Block::Grass);
				// else if (blockZ + z * CHUNK_SIZE < 64)
				// 	chunk.setBlock(blockX, blockY, blockZ, Block::Stone);
				// else
				// 	chunk.setBlock(blockX, blockY, blockZ, Block::Air);
				int x = blockX - 8;
				int y = blockY - 8;
				int z = blockZ - 8;
				if (sqrt(x*x + y*y + z*z) < 8.0f)
					chunk.setBlock(blockX, blockY, blockZ, Block::Grass);
				else
					chunk.setBlock(blockX, blockY, blockZ, Block::Air);
			}
		}
	}
	return chunk;
}
