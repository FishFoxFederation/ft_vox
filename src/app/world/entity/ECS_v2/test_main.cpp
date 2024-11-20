#include "SparseSet.hpp"
#include "ECS.hpp"
#include <cassert>
#include <iostream>

struct Position
{
	float x;
	float y;
};

const size_t entities_size = 100;

void test_sparse_set()
{
	ecs::SparseSet<Position> set;

	Position pos = { 1.0f, 2.0f };

	ecs::entityType entity = 0;

	set.insert(entity, pos);

	Position & pos_ref = set.get(entity);

	pos_ref.x = 3.0f;

	try {
		set.insert(entity, pos);
	} catch (std::exception & e) {
	}

	set.insert(1, { 2.0f, 3.0f });
	set.insert(2, { 3.0f, 4.0f });
	set.insert(3, { 4.0f, 5.0f });

	if (set.size() != 4)
		throw std::logic_error("SparseSet size is not 4");

	for (auto & [entity, pos] : set)
		std::cout << "Entity: " << entity << " Position: " << pos.x << ", " << pos.y << std::endl;

	set.remove(entity);
	std::cout << "after remove" << std::endl;

	for (auto & [entity, pos] : set)
		std::cout << "Entity: " << entity << " Position: " << pos.x << ", " << pos.y << std::endl;
}

void test_ecs()
{
	ecs::ECS<100> ecs;

	auto [success, entity] = ecs.createEntity();

	if (!success)
		throw std::logic_error("Entity creation failed");

	if (!ecs.isAlive(entity))
		throw std::logic_error("Entity is not alive");

	ecs.removeEntity(entity);

	if (ecs.isAlive(entity))
		throw std::logic_error("Entity is alive");

	auto [success2, entity2] = ecs.createEntity();
	if (entity2 == entity)
		throw std::logic_error("Entity is not unique");
	
	std::vector<ecs::entityType> entities;

	for(int i = 0; i < 50; i++)
	{
		auto [success, entity] = ecs.createEntity();
		if (!success)
			throw std::logic_error("Entity creation failed");
		entities.push_back(entity);
	}
	for(auto entity : entities)
	{
		ecs.removeEntity(entity);
	}
	for(auto entity : entities)
	{
		if (ecs.isAlive(entity))
			throw std::logic_error("Entity is alive");
	}
	entities.clear();
	for(int i = 0; i < 50; i++)
	{
		auto [success, entity] = ecs.createEntity();
		if (!success)
			throw std::logic_error("Entity creation failed");
		entities.push_back(entity);
	}

	ecs.addComponent<Position>();
	ecs.removeComponent<Position>();
}

int main()
{
	try {
	test_sparse_set();
	test_ecs();
	} catch (std::exception & e) {
		std::cerr << e.what() << std::endl;
	}
}
