#include "WorldGenerator.hpp"

WorldGenerator::WorldGenerator()
{

}

WorldGenerator::~WorldGenerator()
{

}

Chunk && WorldGenerator::generateChunk(const int & x, const int & y, const int & z)
{
	Chunk chunk;
	(void)x, (void)y;

	for(int blockX = 0; blockX < CHUNK_SIZE; blockX++)
	{
		for(int blockY = 0; blockY < CHUNK_SIZE; blockY++)
		{
			for(int blockZ = 0; blockZ < CHUNK_SIZE; blockZ++)
			{
				if (blockZ + z < 128)
					chunk.setBlock(blockX, blockY, blockZ, Block::Grass);
				else if (blockZ + z < 64)
					chunk.setBlock(blockX, blockY, blockZ, Block::Stone);
				else
					chunk.setBlock(blockX, blockY, blockZ, Block::Air);
			}
		}
	}
}
