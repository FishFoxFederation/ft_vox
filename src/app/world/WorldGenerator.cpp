#include "WorldGenerator.hpp"

#include <cmath>

WorldGenerator::WorldGenerator()
: m_relief_perlin(1, 3, 1, 0.3, 2),
  m_cave_perlin(1, 2, 1, 0.7, 2)
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
				glm::ivec3 position = glm::ivec3(
					blockX + x * CHUNK_SIZE,
					blockY + y * CHUNK_SIZE,
					blockZ + z * CHUNK_SIZE
				);
				Block to_set;

				if( position.y == 128)
					to_set = Block::Air;
				else
				{
					to_set = generateReliefBlock(position);
					if (to_set != Block::Air && generateCaveBlock(position) == Block::Air)
						to_set = Block::Air;
				}
				chunk.setBlock(blockX, blockY, blockZ, to_set);
			}
		}
	}
	return chunk;
}

Block WorldGenerator::generateReliefBlock(glm::ivec3 position)
{
	float value = m_relief_perlin.noise(glm::vec2(
		position.x * 0.01f,
		position.z * 0.01f
	));

	m_avg += value;
	if (value > m_max)
		m_max = value;
	if (value < m_min)
		m_min = value;
	m_called++;

	value = ((value + 1) / 2) * 128;
	if (value + 110 > position.y)
		return Block::Grass;
	// else if (value > -0.05f && value < 0.05f)
		// chunk.setBlock(blockX, blockY, blockZ, Block::Air);
	else
		return Block::Air;
}

Block WorldGenerator::generateCaveBlock(glm::ivec3 position)
{
	float value = m_cave_perlin.noise(glm::vec3(
		position.x * 0.01f,
		position.y * 0.01f,
		position.z * 0.01f
	));
	// value += 0.01f;

	//bias value to have less chance of a cave the higher the y value
	//for now the transition layer will be between 110 and 128

	float edge = 0.05f;

	if (position.y > 100 && position.y < 140)
		edge = 0.025f;
	else if (position.y >= 140 && position.y < 141)
		edge = 0.02f;
	else if (position.y >= 141 && position.y < 142)
		edge = 0.015f;
	else if (position.y >= 142 && position.y < 143)
		edge = 0.01f;
	else if (position.y >= 143)
		edge = 0;

	if (value > 0 && value < edge)
		return Block::Air;
	else
		return Block::Grass;
}
