#include "ecs.hpp"
#include <cassert>
#include <iostream>
#include "test.hpp"

struct Position
{
	float x;
	float y;
};

struct Velocity
{
	float x;
	int speed;
	float y;
};

const size_t entities_size = 100;

void test_component_set()
{
	ecs::ComponentStorage<ecs::entity, Position> set;

	Position pos = { 1.0f, 2.0f };

	ecs::entity entity = 0;

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

	for (auto & pos : set)
		std::cout << " Position: " << pos.x << ", " << pos.y << std::endl;

	set.remove(entity);
	std::cout << "after remove" << std::endl;

	for (auto & pos : set)
		std::cout << " Position: " << pos.x << ", " << pos.y << std::endl;
}

void test_ecs_entity()
{
	ecs::Manager<ecs::entity> ecs;

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
	
	std::vector<ecs::entity> entities;

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
}

void test_ecs_component()
{
	ecs::Manager<> ecs;

	auto [success, entity ] = ecs.createEntity();

	ecs.addComponentToEntity<Position>(entity, { 1.0f, 2.0f });

	auto component = ecs.get<Position>(entity);

	if (component.x != 1.0f || component.y != 2.0f)
		throw std::logic_error("Component values are not correct");
	
	ecs.removeComponentFromEntity<Position>(entity);

	try {
		auto component = ecs.get<Position>(entity);
		component.x += 2.0f;
	} catch (ecs::Manager<>::ComponentDoesNotExist & e) {
		std::cout << e.what() << std::endl;
	}
}

void test_ecs_view()
{

}

int main()
{
	try {
	test_component_set();
	test_ecs_entity();
	test_ecs_component();
	} catch (std::exception & e) {
		std::cerr << e.what() << std::endl;
	}

}
