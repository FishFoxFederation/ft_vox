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
	 * @brief This class is the main class of the ECS system
	 * you can use it to create entitites, add component to entities, fetch components from entities and remove them
	 * 
	 * 
	 * @details Go see the Entity page for more info on how the entities are stored
	 * @tparam entityType the internal type used for entities id, must be integral and unsigned, default uint32_t
	 */
	template <ValidEntity entityType = ecs::entity>
	class Manager
	{
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

		/**
		 * @brief Create a new entity
		 * 
		 * @retval std::pair<TRUE, entityType> if the entity was created successfully
		 * @retval std::pair<FALSE, undefined> if the entity could not be created
		 */
		std::pair<bool, entityType>		createEntity()
		{
			if (m_entities.size() == ecs::MAX_ENTITIES)
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

		/**
		 * @brief Remove an entity from the manager
		 * 
		 * @warning Undefined behavior if the entity does not exist
		 * 
		 * @param entity 
		 */
		void			removeEntity(entityType entity)
		{
			if (!isAlive(entity))
				throw EntityDoesNotExist(entity);
			
			auto index = getEntityIndex(entity);
			
			std::swap(m_entities[index], m_dead_entity_head);

			//temporary would like to find a way to have it in O(1)
			for (auto & [type, set] : m_components)
				set->remove(entity);
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

		/**
		 * @brief Get the version of the entity, see the entity doc for more info
		 * 
		 * @param entity 
		 * @return uint32_t 
		 */
		static constexpr uint32_t getEntityVersion(const entityType & entity)
		{
			return entity & 0xF;
		}

		/**
		 * @brief Get the index of the entity in the entity array, see the entity doc for more info
		 * 
		 * @param entity 
		 * @return uint32_t 
		 */
		static constexpr uint32_t getEntityIndex(const entityType & entity)
		{
			return entity >> 4;
		}

		/*************************************************************\
		 * 	COMPONENT-ENTITY RELATION
		\*************************************************************/
		
		/**
		 * @brief add a component to an entity
		 * 
		 * @tparam ComponentType 
		 * @param entity 
		 * @param component 
		 */
		template <typename ComponentType>
		void add(entityType entity, ComponentType component)
		{
			using ComponentSetType = ComponentStorage<entityType, ComponentType>;
			ComponentSetType & set = getSet<ComponentType>();

			set.insert(entity, component);
		}

		/**
		 * @brief add multiple components to an entity
		 * 
		 * @tparam ComponentTypes 
		 * @param entity 
		 * @param components 
		 */
		template <typename... ComponentTypes>
		void add(entityType entity, ComponentTypes... components)
		{
			(add<ComponentTypes>(entity, components), ...);
		}

		/**
		 * @brief remove any components from an entity
		 * 
		 * @tparam ComponentTypes 
		 * @param entity 
		 */
		template <typename... ComponentTypes>
		void remove(entityType entity)
		{
			(_remove_component<ComponentTypes>(entity), ...);
		}

		/**
		 * @brief get a component attached to an entity
		 * 
		 * @warning may throw if the entity does not have the component
		 *  prefer using tryGetComponent
		 * 
		 * @tparam ComponentType 
		 * @param entity 
		 * @return ComponentType& 
		 */
		template <typename ComponentType>
		ComponentType & getComponent(entityType entity)
		{
			using ComponentSetType = ComponentStorage<entityType, ComponentType>;
			ComponentSetType & set = getSet<ComponentType>();

			return set.get(entity);
		}

		/**
		 * @brief Get multiple components attached to an entity as a tuple
		 * 
		 * @warning may throw if the entity does not have one of the components, prefer using
		 * tryGetComponents
		 * 
		 * you can use @code auto [comp1, comp2] = getComponents<Comp1, Comp2>(entity); @endcode  
		 * @tparam ComponentTypes 
		 * @param entity 
		 * @return std::tuple<ComponentTypes &...> 
		 */
		template <typename... ComponentTypes>
		std::tuple<ComponentTypes &...> getComponents(entityType entity)
		{
			return {getComponent<ComponentTypes>(entity)...};
		}

		/**
		 * @brief Try to get a component attached to an entity
		 * 
		 * @tparam ComponentType 
		 * @param entity 
		 * @return ComponentType *
		 */
		template <typename ComponentType>
		ComponentType * tryGetComponent(entityType entity)
		{
			using ComponentSetType = ComponentStorage<entityType, ComponentType>;
			ComponentSetType & set = getSet<ComponentType>();

			return set.tryGet(entity);
		}

		/**
		 * @brief try to get multiple components attached to an entity as a tuple
		 * 
		 * @tparam ComponentTypes 
		 * @param entity 
		 * @return std::tuple<ComponentTypes *...> 
		 */
		template <typename... ComponentTypes>
		std::tuple<ComponentTypes *...> tryGetComponents(entityType entity)
		{
			return std::make_tuple(tryGetComponent<ComponentTypes>(entity)...);
		}
		
		/*************************************************************\
		 * 	SINGLETONS
		\*************************************************************/
		template <typename T>
		T & getSingleton()
		{
			auto id = std::type_index(typeid(T));

			if (!m_singletons.contains(id))
				addSingleton<T>(T());
			return *std::static_pointer_cast<T>(m_singletons.at(id));
		}

		template <typename T>
		void addSingleton(const T & singleton)
		{
			auto id = std::type_index(typeid(T));

			// if (m_singletons.contains(id))
				// throw ComponentAlreadyExists<T>();
			m_singletons.insert({id, std::make_shared<T>(singleton)});
		}

		/*************************************************************\
		 * 	VIEWS
		\*************************************************************/
		/**
		 * @brief get an iterable view of entities that have all the components
		 * listed in the tparams
		 * 
		 * @tparam ComponentTypes 
		 * @return View<entityType, ComponentTypes...> 
		 */
		template <typename... ComponentTypes>
		View<entityType, ComponentTypes...> view()
		{
			return View<entityType, ComponentTypes...>(*this);
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
		std::unordered_map<std::type_index, std::shared_ptr<void>> m_singletons;

		/*************************************************\
		 * 	ENTITY UTILS
		\*************************************************/

		/**
		 * @brief Increase the version of the entity, see the entity doc for more info
		 * 
		 * @param entity 
		 */
		void IncreaseEntityVersion(entityType & entity)
		{
			entity += (getEntityVersion(entity) + 1) & 0xF;
		}
		
		/**
		 * @brief Create a new entity with a version of 1 and an appropriate index
		 * 
		 * @return entityType 
		 */
		entityType createNewEntity()
		{
			return m_entities.size() << 4 | 1;
		}

		/*************************************************\
		 * 	COMPONENT UTILS
		\*************************************************/
		template <typename ComponentType>
		componentSet<ComponentType> & getSet()
		{
			return *getSetPtr<ComponentType>();
		}

		/**
		 * @brief Get a ptr to a component set, if it does not exist create it
		 * used throughout the manager to garanty access to a component set
		 * 
		 * @tparam ComponentType 
		 * @return std::shared_ptr<componentSet<ComponentType>> 
		 */
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

		/**
		 * @brief Remove a component from an entity
		 * 
		 * @tparam ComponentType 
		 * @param entity 
		 */
		template <typename ComponentType>
		void _remove_component(entityType entity)
		{
			using ComponentSetType = ComponentStorage<entityType, ComponentType>;
			ComponentSetType & set = getSet<ComponentType>();

			set.remove(entity);
		}
	};
}
