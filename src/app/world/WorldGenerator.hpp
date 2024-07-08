#pragma once

#include <memory>
#include "define.hpp"

#include "Perlin.hpp"
#include "Chunk.hpp"
#include "Block.hpp"
#include "logger.hpp"
#include "DebugGui.hpp"

class WorldGenerator
{
public:
	WorldGenerator();
	~WorldGenerator();

	std::shared_ptr<Chunk>	generateChunkColumn(const int & x, const int & z);
	std::shared_ptr<Chunk>	generateChunk(const int & x, const int & y, const int & z);
	std::shared_ptr<Chunk>	generateFullChunk(const int & x, const int & y, const int & z);
	std::shared_ptr<Chunk>	generateChunkColumn(const int & x, const int & z, std::shared_ptr<Chunk> chunk);
	std::shared_ptr<Chunk>	generateChunk(const int & x, const int & y, const int & z, std::shared_ptr<Chunk> chunk);
	std::shared_ptr<Chunk>	generateFullChunk(const int & x, const int & y, const int & z, std::shared_ptr<Chunk> chunk);
	// double	m_avg = 0;
	// int		m_called = 0;
	// float	m_max = -1;
	// float 	m_min = 1;
private:
	Perlin	m_relief_perlin;
	Perlin	m_cave_perlin;

	BlockID generateCaveBlock(glm::ivec3 position);
	BlockID generateReliefBlock(glm::ivec3 position);
	float	generateReliefValue(glm::ivec2 position);
};
