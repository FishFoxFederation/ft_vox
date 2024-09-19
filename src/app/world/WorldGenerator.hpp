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
using enum Chunk::genLevel;
public:
	struct genInfo
	{
		Chunk::genLevel oldLevel;
		Chunk::genLevel level;
		glm::ivec3 zoneSize;
		glm::ivec3 zoneStart;
	};
	constexpr static std::array<glm::ivec3, 2> ZONE_SIZES = {
		glm::ivec3(3, 3, 3),
		glm::ivec3(5, 5, 5)
	};
	constexpr static int MAX_TICKET_LEVEL = TICKET_LEVEL_INACTIVE + 2;

	//gen level 0 if full chunkGeneration


	WorldGenerator();

	~WorldGenerator();

	/**
	 * @brief get the generation info for the given ticket level and chunk position
	 * 
	 * @param ticket_level
	 * @param old_level
	 * @param chunkPos3D
	 * @return a genInfo struct
	 */
	static genInfo getGenInfo(int ticket_level, Chunk::genLevel old_level, glm::ivec3 chunkPos3D);

	/**
	 * @brief will do a generation pass to the chunks
	 * 
	 * @param info the struct returned by the getGenInfo function
	 * @param chunks the chunks to generate
	 * 
	 * @warning the chunks MUST be locked before calling and unlocked after
	 */
	void 					generate(genInfo info, ChunkMap & chunks);

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
