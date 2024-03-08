#include "World.hpp"

World::World()
{
	//generate chunks 4 distance from the player
	// for(int x = -2; x < 2; x++)
	// {
	// 	for(int y = -2; y < 2; y++)
	// 	{
	// 		for(int z = -2; z < 2; z++)
	// 		{
	// 			m_chunks.insert(std::make_pair(glm::ivec3(x, y, z), m_worldGenerator.generateChunk(x, y, z)));
	// 			m_visible_chunks.insert(glm::ivec3(x, y, z));
	// 		}
	// 	}
	// }
}

World::~World()
{
}

void World::update(glm::vec3 playerPosition)
{
	(void)playerPosition;
}
