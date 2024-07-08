#include "WorldGenerator.hpp"

#include <cmath>

WorldGenerator::WorldGenerator()
: m_relief_perlin(1, 3, 1, 0.5, 2),
  m_cave_perlin(1, 4, 1, 0.5, 2)
{

}

WorldGenerator::~WorldGenerator()
{

}

std::shared_ptr<Chunk> WorldGenerator::generateFullChunk(const int & x, const int & y, const int & z)
{
	return generateFullChunk(x, y, z, nullptr);
}

std::shared_ptr<Chunk> WorldGenerator::generateFullChunk(const int & x, const int & y, const int & z, std::shared_ptr<Chunk> chunk)
{
	if (chunk == nullptr)
		chunk = std::make_shared<Chunk>(glm::ivec3(x, y, z));
	(void)x, (void)y;

	for(int blockX = 0; blockX < CHUNK_X_SIZE; blockX++)
	{
		for(int blockY = 0; blockY < CHUNK_Y_SIZE; blockY++)
		{
			for(int blockZ = 0; blockZ < CHUNK_Z_SIZE; blockZ++)
			{
				chunk->setBlock(blockX, blockY, blockZ, BlockID::Stone);
			}
		}
	}
	chunk->setGenerated(true);
	return chunk;
}

std::shared_ptr<Chunk> WorldGenerator::generateChunkColumn(const int & x, const int & z)
{
	return generateChunkColumn(x, z, nullptr);
}

std::shared_ptr<Chunk> WorldGenerator::generateChunkColumn(const int & x, const int & z, std::shared_ptr<Chunk> column)
{
	if (column == nullptr)
		column = std::make_shared<Chunk>(glm::ivec3(x, 0, z));
	// chunk = std::make_shared<Chunk>(glm::ivec3(x, y, z));


	for(int blockX = 0; blockX < CHUNK_X_SIZE; blockX++)
	{
		for(int blockZ = 0; blockZ < CHUNK_Z_SIZE; blockZ++)
		{
			//generate the relief value for the whole column
			float value = generateReliefValue(glm::ivec2(
				blockX + x * CHUNK_X_SIZE,
				blockZ + z * CHUNK_Z_SIZE
			));

			for(int blockY = 0; blockY < CHUNK_Y_SIZE; blockY++)
			{

				glm::ivec3 position = glm::ivec3(
					blockX + x * CHUNK_X_SIZE,
					blockY,
					blockZ + z * CHUNK_Z_SIZE
				);
				BlockID to_set;

				//check to see wether above or below the relief value
				if (value + 128 > position.y)
					to_set = BlockID::Stone;
				else if (value + 133 > position.y)
					to_set = BlockID::Grass;
				else
					to_set = BlockID::Air;

				//if below relief value try to carve a cave
				if (to_set != BlockID::Air && generateCaveBlock(position) == BlockID::Air)
					to_set = BlockID::Air;
				// try {
				column->setBlock(blockX, blockY, blockZ, to_set);
				// } catch (std::exception & e)
				// {
					// LOG_ERROR("EXCEPTION: " << e.what());
				// }
			}
		}
	}
	column->setGenerated(true);
	return column;
}

std::shared_ptr<Chunk> WorldGenerator::generateChunk(const int & x, const int & y, const int & z, std::shared_ptr<Chunk> chunk)
{
	if (chunk == nullptr)
		chunk = std::make_shared<Chunk>(glm::ivec3(x, y, z));
	(void)x, (void)y;

	for(int blockX = 0; blockX < CHUNK_X_SIZE; blockX++)
	{
		for(int blockY = 0; blockY < CHUNK_Y_SIZE; blockY++)
		{
			for(int blockZ = 0; blockZ < CHUNK_Z_SIZE; blockZ++)
			{
				glm::ivec3 position = glm::ivec3(
					blockX + x * CHUNK_X_SIZE,
					blockY + y * CHUNK_Y_SIZE,
					blockZ + z * CHUNK_Z_SIZE
				);
				BlockID to_set;

				// if( position.y == 128)
				// 	to_set = Block::Air;
				// else
				// {
					to_set = generateReliefBlock(position);
					if (to_set != BlockID::Air && generateCaveBlock(position) == BlockID::Air)
						to_set = BlockID::Air;
				// }
				// if (to_set != Block::Air && position.y > 128)
				// {
				// 	to_set = Block::Grass;
				// }
				chunk->setBlock(blockX, blockY, blockZ, to_set);
			}
		}
	}
	chunk->setGenerated(true);
	return chunk;
}

BlockID WorldGenerator::generateReliefBlock(glm::ivec3 position)
{
	float value = generateReliefValue(glm::ivec2(position.x, position.z));

	if (value + 128 > position.y)
		return BlockID::Stone;
	// else if (value > -0.05f && value < 0.05f)
		// chunk.setBlock(blockX, blockY, blockZ, Block::Air);
	else if (value + 133 > position.y)
		return BlockID::Grass;
	else
		return BlockID::Air;
}

float WorldGenerator::generateReliefValue(glm::ivec2 position)
{
	float value = m_relief_perlin.noise(glm::vec2(
		position.x * 0.01f,
		position.y * 0.01f
	));

	value = ((value + 1) / 2) * 128;

	return value;
}

BlockID WorldGenerator::generateCaveBlock(glm::ivec3 position)
{


	//spaghetti caves
	float valueA = m_cave_perlin.noise(glm::vec3(
		position.x * 0.01f,
		position.y * 0.016f,
		position.z * 0.01f
	));

	float valueB = m_cave_perlin.noise(glm::vec3(
		position.x * 0.01f,
		(position.y + 42) * 0.016f,
		position.z * 0.01f
	));

	float threshold = 0.002f;
	float value = valueA * valueA + valueB * valueB;
	// value /= threshold;

	// m_avg += value;
	// if (value > m_max)
	// 	m_max = value;
	// if (value < m_min)
	// 	m_min = value;
	// m_called++;

	if (value < threshold)
		return BlockID::Air;

	// else try to create cheese cave

	valueA = (valueA + 1) / 2;
	valueA += 0.01f;

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

	if (valueA > 0 && valueA < edge)
		return BlockID::Air;
	else
		return BlockID::Stone;

	return BlockID::Stone;
}
