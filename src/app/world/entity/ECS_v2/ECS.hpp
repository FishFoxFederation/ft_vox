#pragma once

#include <typeindex>
#include <unordered_map>
#include <memory>
#include <exception>
#include <string>
#include <functional>
#include <tuple>
#include <utility>
#include <iostream>

#include "ECS_CONSTANTS.hpp"
#include "ECS_FWD.hpp"

#include "SparseSet.hpp"
#include "View.hpp"

namespace ecs
{

	/**
	 * @brief 
	 * 
	 * @tparam size 
	 */
	template <size_t size = MAX_ENTITIES>
	class ECS
	{
	public:
		ECS()
		{};
		~ECS(){};

		ECS(ECS & other) = delete;
		ECS(ECS && other) = delete;
		ECS & operator=(ECS & other) = delete;
		ECS & operator=(ECS && other) = delete;

		/*************************************************************\
		 * 	ENTITY
		\*************************************************************/
		std::pair<bool, entityType>		createEntity()
		{
			if (m_entities.size() == MAX_ENTITIES)
				return {false, 0};
			//check if there are any dead entities to reuse
			if (m_dead_entity_head != 0)
			{
				entityType dead_entity = m_dead_entity_head;
				auto dead_index = getEntityIndex(dead_entity);

				//set the new dead entity head to the next dead entity
				//see doc for more info
				m_dead_entity_head = m_entities[dead_index];

				IncreaseEntityVersion(dead_entity);
				m_entities[dead_index] = dead_entity;
				return {true, dead_entity};
			}
			else
			{
				auto new_entity = createNewEntity();
				m_entities.push_back(new_entity);
				return {true, new_entity};
			}
		}

		void			removeEntity(entityType entity)
		{
			if (!isAlive(entity))
				throw EntityDoesNotExist(entity);
			
			auto index = getEntityIndex(entity);
			
			std::swap(m_entities[index], m_dead_entity_head);

			//todo remove entity from all components
		}

		bool 			isAlive(entityType entity) const
		{
			auto index = getEntityIndex(entity);

			if (index >= m_entities.size())
				return false;
			
			if (m_entities[index] != entity)
				return false;
			return true;
		}

		/*************************************************************\
		 * 	COMPONENTS
		\*************************************************************/
		//add component
		template <typename ComponentType>
		void addComponent()
		{
			const std::type_index type = std::type_index(typeid(ComponentType));
			if (m_components.contains(type))
				throw ComponentAlreadyExists<ComponentType>();

			m_components[type] = std::make_shared<SparseSet<ComponentType>>();
		}

		//remove component
		template <typename ComponentType>
		void removeComponent()
		{
			const std::type_index type = std::type_index(typeid(ComponentType));
			if (!m_components.contains(type))
				throw ComponentDoesNotExist<ComponentType>();

			m_components.erase(type);
		}

		
		/*************************************************************\
		 * 	COMPONENT-ENTITY RELATION
		\*************************************************************/
		//add component to entity
		template <typename ComponentType>
		void addComponentToEntity(entityType entity)
		{
		}

		//remove component from entity
		template <typename ComponentType>
		void removeComponentFromEntity(entityType entity)
		{
		}

		//get component from entity
		template <typename ComponentType>
		ComponentType & getComponentFromEntity(entityType entity)
		{
		}

		/*************************************************************\
		 * 	VIEWS
		\*************************************************************/
		template <typename... ComponentTypes>
		View<ComponentTypes...> view()
		{
		}

		/*********************************\
		 * 	EXCEPTIONS
		\*********************************/
		class EntityDoesNotExist : public std::exception
		{
		public:
			EntityDoesNotExist(entityType entity)
				: m_entity(entity) {}
			const char * what() const noexcept override { return std::string("Entity does not exist " + std::to_string(m_entity)).c_str(); }
			entityType getEntity() const { return m_entity; }
		private:
			entityType m_entity;
		};

		class EntityAlreadyExists : public std::exception
		{
		public:
			EntityAlreadyExists(entityType entity)
				: m_entity(entity) {}
			const char * what() const noexcept override { return std::string("Entity already exists " + std::to_string(m_entity)).c_str(); }
			entityType getEntity() const { return m_entity; }
		private:
			entityType m_entity;

		};

		template <typename ComponentType>
		class ComponentDoesNotExist : public std::exception
		{
		public:
			ComponentDoesNotExist()
				: m_type(typeid(ComponentType)) {}
			const char * what() const noexcept override
			{
				std::string message = "Component does not exist ";
				message += m_type.name();
				return message.c_str();
			}
			const std::type_index & getType() const { return m_type; }
		private:
			std::type_index m_type;
		};

		template <typename ComponentType>
		class ComponentAlreadyExists : public std::exception
		{
		public:
			ComponentAlreadyExists()
				: m_type(typeid(ComponentType)) {}
			const char * what() const noexcept override
			{
				std::string message = "Component already exists ";
				message += m_type.name();
				return message.c_str();
			}
			const std::type_index & getType() const { return m_type; }
		private:
			std::type_index m_type;
		};

	private:
		std::vector<entityType> m_entities;

		entityType m_dead_entity_head = 0;

		std::unordered_map<std::type_index, std::shared_ptr<void>> m_components;

		uint32_t getEntityVersion(entityType entity) const
		{
			return entity & 0xFFF;
		}

		uint32_t getEntityIndex(entityType entity) const
		{
			return entity >> 3;
		}

		void IncreaseEntityVersion(entityType & entity)
		{
			entity += (getEntityVersion(entity) + 1) & 0xFFF;
		}

		entityType createNewEntity()
		{
			return m_entities.size() << 3 | 1;
		}
	};
}
