#pragma once

#include "define.hpp"

#include <cstdint>
#include <queue>
#include <unordered_map>
#include <typeindex>
#include <vector>
#include <functional>
#include <memory>
#include <set>

// This is a tentative implementation of an Entity Component System
// The goal is to have a system that is easy to use, understand and extend

class ECS
{

public:

	typedef uint64_t Entity;


	ECS()
	{
	}

	~ECS()
	{
	}

	ECS(ECS & other) = delete;
	ECS(ECS && other) = delete;
	ECS & operator=(ECS & other) = delete;
	ECS & operator=(ECS && other) = delete;


	Entity createEntity()
	{
		static Entity id = 0;
		m_entities[id];
		return id++;
	}

	void removeEntity(Entity entity)
	{
		m_entities.erase(entity);
		for (auto & component : m_components)
		{
			component.second.erase(entity);
		}
	}


	template <typename T, typename... Args>
	void addComponent(Entity entity, Args &&... args)
	{
		const std::type_index type = std::type_index(typeid(T));
		m_entities[entity].insert(type);
		m_components[type][entity] = std::make_shared<T>(T{std::forward<Args>(args)...});
	}

	template <typename T>
	T * getComponent(Entity entity)
	{
		const std::type_index type = std::type_index(typeid(T));
		auto it = m_components[type].find(entity);
		if (it == m_components[type].end())
		{
			throw std::runtime_error("Entity does not have component");
		}
		return static_cast<T *>(it->second.get());
	}

	template <typename T>
	void removeComponent(Entity entity)
	{
		const std::type_index type = std::type_index(typeid(T));
		m_entities[entity].erase(type);
		m_components[type].erase(entity);
	}

	/**
	 * @brief This function will call the function func for each entity that has all the components specified
	 * 
	 * @tparam Components The components that the entity must have. They are specified by the function arguments
	 * @param func The function to call
	 */
	template <typename... Components>
	void forEach(void (*func)(Components &...))
	{
		std::vector<std::type_index> types = {std::type_index(typeid(Components))...};
		for (auto & [entity, entity_component_types] : m_entities)
		{
			if (
				std::all_of(types.begin(), types.end(),
					[&entity_component_types](std::type_index type)
					{
						return entity_component_types.contains(type);
					}
				)
			)
			{
				func(*getComponent<Components>(entity)...);
			}
		}
	}



private:

	std::unordered_map<Entity, std::set<std::type_index>> m_entities;

	std::unordered_map<std::type_index, std::unordered_map<Entity, std::shared_ptr<void>>> m_components;

};
