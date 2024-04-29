#pragma once

#include "define.hpp"

#include "List.hpp"
#include "Map.hpp"
#include "Entity.hpp"

#include <unordered_map>

class World
{

public:

	World(
		WorldScene & WorldScene,
		VulkanAPI & vulkanAPI,
		ThreadPool & threadPool
	);
	~World();

	World(World & other) = delete;
	World(World && other) = delete;
	World & operator=(World & other) = delete;
	World & operator=(World && other) = delete;

	void updateBlock(glm::dvec3 position);
	void updateEntities();

private:

	Map m_map;

	IdList<uint64_t, std::shared_ptr<Entity>> m_entities;

	std::shared_ptr<Entity> m_player;

};

class ClientWorld
{

};
