#include "World.hpp"

World::World(
	WorldScene & WorldScene,
	VulkanAPI & vulkanAPI,
	ThreadPool & threadPool
):
	m_map(WorldScene, vulkanAPI, threadPool)
{
}

World::~World()
{
}

void World::updateBlock(glm::dvec3 position)
{
	m_map.update(position);
}

void World::updateEntities()
{
}
