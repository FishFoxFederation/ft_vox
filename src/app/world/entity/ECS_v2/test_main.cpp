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
	float y;
};

struct mesh
{
	int id;
};

struct transform
{
	float x;
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

	ecs.add<Position>(entity, { 1.0f, 2.0f });

	auto component = ecs.getComponent<Position>(entity);

	if (component.x != 1.0f || component.y != 2.0f)
		throw std::logic_error("Component values are not correct");
	
	ecs.remove<Position>(entity);

	try {
		auto component = ecs.getComponent<Position>(entity);
		component.x += 2.0f;
	} catch (ecs::Manager<>::ComponentDoesNotExist & e) {
		std::cout << e.what() << std::endl;
	}
}

void test_ecs_view()
{
	std::cout << "ECS VIEW" << std::endl;
	ecs::Manager<> ecs;
	Position pos = { 1.0f, 2.0f };
	Velocity vel = { 3.0f, 4.0f };
	mesh m = { 5 };
	transform t = { 6.0f, 7.0f };
	auto [success, entity1 ] = ecs.createEntity();
	auto [success2, entity2 ] = ecs.createEntity();
	auto [success3, entity3 ] = ecs.createEntity();
	auto [success4, entity4 ] = ecs.createEntity();
	ecs.add<Position, Velocity, mesh, transform>(entity1, pos, vel, m, t);
	std::cout << "Entity1: " << entity1 << std::endl;
	ecs.add<Position, Velocity, mesh, transform>(entity2, pos, vel, m, t);
	std::cout << "Entity2: " << entity2 << std::endl;
	ecs.add<Position, Velocity>(entity3, pos, vel);
	std::cout << "Entity3: " << entity3 << std::endl;
	ecs.add<Position, Velocity>(entity4, pos, vel);
	std::cout << "Entity4: " << entity4 << std::endl;

	auto view = ecs.view<mesh, transform>();
	for(auto entity : view)
	{
		auto [m , t] = ecs.getComponents<mesh, transform>(entity);
		std::cout << "Entity: " << entity << " Mesh: " << m.id << " Transform: " << t.x << ", " << t.y << std::endl;
	}

	auto view2 = ecs.view<Position, Velocity>();
	for(auto entity : view2)
	{
		auto [pos, vel] = ecs.getComponents<Position, Velocity>(entity);
		std::cout << "Entity: " << entity << " Position: " << pos.x << ", " << pos.y << " Velocity: " << vel.x << ", " << vel.y << std::endl;
	}
}

void test_ecs_remove()
{
	std::cout << "ECS REMOVE" << std::endl;
	ecs::Manager<> ecs;
	Position pos = { 1.0f, 2.0f };
	Velocity vel = { 3.0f, 4.0f };
	mesh m = { 5 };
	transform t = { 6.0f, 7.0f };
	auto [success, entity1 ] = ecs.createEntity();
	auto [success2, entity2 ] = ecs.createEntity();
	auto [success3, entity3 ] = ecs.createEntity();
	auto [success4, entity4 ] = ecs.createEntity();
	ecs.add<Position, Velocity, mesh, transform>(entity1, pos, vel, m, t);
	std::cout << "Entity1: " << entity1 << std::endl;
	ecs.add<Position, Velocity, mesh, transform>(entity2, pos, vel, m, t);
	std::cout << "Entity2: " << entity2 << std::endl;
	ecs.add<Position, Velocity>(entity3, pos, vel);
	std::cout << "Entity3: " << entity3 << std::endl;
	ecs.add<Position, Velocity>(entity4, pos, vel);
	std::cout << "Entity4: " << entity4 << std::endl;

	ecs.remove<Position, Velocity>(entity1);

	auto view = ecs.view<mesh, transform>();
	for(auto entity : view)
	{
		auto [m , t] = ecs.getComponents<mesh, transform>(entity);
		std::cout << "Entity: " << entity << " Mesh: " << m.id << " Transform: " << t.x << ", " << t.y << std::endl;
	}

	auto view2 = ecs.view<Position, Velocity>();
	for(auto entity : view2)
	{
		auto [pos, vel] = ecs.getComponents<Position, Velocity>(entity);
		std::cout << "Entity: " << entity << " Position: " << pos.x << ", " << pos.y << " Velocity: " << vel.x << ", " << vel.y << std::endl;
	}

	ecs.removeEntity(entity2);

	std::cout << "MESH TRANSFORM" << std::endl;
	view = ecs.view<mesh, transform>();
	for(auto entity : view)
	{
		auto [m , t] = ecs.getComponents<mesh, transform>(entity);
		std::cout << "Entity: " << entity << " Mesh: " << m.id << " Transform: " << t.x << ", " << t.y << std::endl;
	}

	std::cout << "POSITION VELOCITY" << std::endl;
	view2 = ecs.view<Position, Velocity>();
	for(auto entity : view2)
	{
		auto [pos, vel] = ecs.getComponents<Position, Velocity>(entity);
		std::cout << "Entity: " << entity << " Position: " << pos.x << ", " << pos.y << " Velocity: " << vel.x << ", " << vel.y << std::endl;
	}
}

int main()
{
	try {
	test_component_set();
	test_ecs_entity();
	test_ecs_component();
	test_ecs_view();
	test_ecs_remove();
	} catch (std::exception & e) {
		std::cerr << e.what() << std::endl;
	}

}
