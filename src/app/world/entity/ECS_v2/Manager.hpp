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

#include "ecs_CONSTANTS.hpp"
#include "ecs_FWD.hpp"
#include "ecs_utils.hpp"
#include "ecs_Exceptions.hpp"

#include "ComponentStorage.hpp"
#include "SparseSet.hpp"
#include "View.hpp"

namespace ecs
{

	/**
	 * @brief A simple ECS 
	 * 
	 * @tparam entityType 
	 */
	template <ValidEntity entityType = ecs::entity, size_t max = 5000>
	class Manager
	{
	// friend class View;
	public:
		template <ValidEntity entity, typename... ComponentTypes>
		friend class View;

		typedef SparseSet<entityType> entitySet;
		template <typename ComponentType>
		using componentSet = ComponentStorage<entityType, ComponentType>;

		Manager(){};
		~Manager(){};

		Manager(Manager & other) = delete;
		Manager(Manager && other) = delete;
		Manager & operator=(Manager & other) = delete;
		Manager & operator=(Manager && other) = delete;

		/*************************************************************\
		 * 	ENTITY
		\*************************************************************/
		std::pair<bool, entityType>		createEntity()
		{
			if (m_entities.size() == max)
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
		 * 	COMPONENT-ENTITY RELATION
		\*************************************************************/
		//add component to entity
		template <typename ComponentType>
		void addComponentToEntity(entityType entity, ComponentType component = ComponentType())
		{
			using ComponentSetType = ComponentStorage<entityType, ComponentType>;
			ComponentSetType & set = getSet<ComponentType>();

			set.insert(entity, component);
		}

		//remove component from entity
		template <typename ComponentType>
		void removeComponentFromEntity(entityType entity)
		{
			using ComponentSetType = ComponentStorage<entityType, ComponentType>;
			ComponentSetType & set = getSet<ComponentType>();

			set.remove(entity);
		}

		//get component from entity
		template <typename ComponentType>
		ComponentType & get(entityType entity)
		{
			using ComponentSetType = ComponentStorage<entityType, ComponentType>;
			ComponentSetType & set = getSet<ComponentType>();

			return set.get(entity);
		}

		template <typename... ComponentTypes>
		std::tuple<ComponentTypes &...> getTuple(entityType entity)
		{
			return std::make_tuple(get<ComponentTypes>(entity)...);
		}

		template <typename ComponentType>
		ComponentType & tryGet(entityType entity)
		{
			using ComponentSetType = ComponentStorage<entityType, ComponentType>;
			ComponentSetType & set = getSet<ComponentType>();

			return set.tryGet(entity);
		}

		template <typename... ComponentTypes>
		std::tuple<ComponentTypes &...> tryGetTuple(entityType entity)
		{
			return std::make_tuple(tryGet<ComponentTypes>(entity)...);
		}

		/*************************************************************\
		 * 	VIEWS
		\*************************************************************/
		template <typename... ComponentTypes>
		View<ComponentTypes...> view()
		{
		}

		/*************************************************************\
		 * 	EXCEPTIONS
		\*************************************************************/

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

		class ComponentDoesNotExist : public std::exception
		{
		public:
			template <typename ComponentType>
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

		class ComponentAlreadyExists : public std::exception
		{
		public:
			template <typename ComponentType>
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

		std::unordered_map<std::type_index, std::shared_ptr<entitySet>> m_components;

		/*************************************************\
		 * 	ENTITY UTILS
		\*************************************************/
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

		/*************************************************\
		 * 	COMPONENT UTILS
		\*************************************************/
		template <typename ComponentType>
		componentSet<ComponentType> & getSet()
		{
			return *getSetPtr<ComponentType>();
		}

		template <typename ComponentType>
		std::shared_ptr<componentSet<ComponentType>> getSetPtr()
		{
			using componentSetType = componentSet<ComponentType>;
			const std::type_index type = std::type_index(typeid(ComponentType));

			auto it = m_components.find(type);
			if (it == m_components.end())
			{
				auto ret_pair = m_components.insert({type, std::make_shared<componentSetType>()});
				it = ret_pair.first;
			}
			return std::static_pointer_cast<componentSetType>(it->second);
		}
	};
}
