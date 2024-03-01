#pragma once

#include "define.hpp"

#include <glm/vec3.hpp>
#include <unordered_map>
#include <unordered_set>

#include "Block.hpp"
#include "Chunk.hpp"
#include "WorldGenerator.hpp"
#include "hashes.hpp"

#define CHUNKS_PER_WORLD 16

/**
 * @brief 
 * 
 */
class World
{
public:
	World();
	~World();

	void update(glm::vec3 playerPosition);
private:
	WorldGenerator m_worldGenerator;
	std::unordered_map<glm::ivec3, Chunk>	m_chunks;
	std::unordered_set<glm::ivec3>			m_visible_chunks;
};
