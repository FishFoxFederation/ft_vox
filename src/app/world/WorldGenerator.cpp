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
				Block block_type = y * CHUNK_SIZE + blockY < 50 ? Block::Stone : Block::Grass;
				int _x = blockX - 8;
				int _y = blockY - 8;
				int _z = blockZ - 8;
				if (sqrt(_x*_x + _y*_y + _z*_z) < 8.0f)
					chunk.setBlock(blockX, blockY, blockZ, block_type);
				else
					chunk.setBlock(blockX, blockY, blockZ, Block::Air);
			}
		}
	}
	return chunk;
}
