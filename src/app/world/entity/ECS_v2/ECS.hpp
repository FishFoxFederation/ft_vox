#pragma once

#include <typeindex>
#include <unordered_map>
#include <memory>
#include <exception>
#include <string>

#include "ECS_CONSTANTS.hpp"


#include "SparseSet.hpp"

namespace ecs
{

	template <size_t size = MAX_ENTITIES>
	class ECS
	{
	public:
		ECS();
		~ECS();

		ECS(ECS & other) = delete;
		ECS(ECS && other) = delete;
		ECS & operator=(ECS & other) = delete;
		ECS & operator=(ECS && other) = delete;

		entityType		createEntity();
		void			removeEntity(entityType entity);
		bool 			isAlive(entityType entity) const;

		//add component
		template <typename ComponentType>
		void addComponent()
		{
			const std::type_index type = std::type_index(typeid(ComponentType));
			if (m_components.contains(type))
				throw ComponentAlreadyExists();

			m_components[type] = std::make_shared<SparseSet<ComponentType, size>>();
		}

		//remove component
		template <typename ComponentType>
		void removeComponent()
		{
			const std::type_index type = std::type_index(typeid(ComponentType));
			if (!m_components.contains(type))
				throw ComponentDoesNotExist();

			m_components.erase(type);
		}

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


		//add system

		//remove system

		//update systems



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

		class ComponentDoesNotExist : public std::exception
		{
		public:
			ComponentDoesNotExist(const std::type_index & type)
				: m_type(type) {}
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
			ComponentAlreadyExists(const std::type_index & type)
				: m_type(type) {}
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
		std::array<entityType, MAX_ENTITIES> m_entities;

		std::unordered_map<std::type_index, std::shared_ptr<void>> m_components;
	};
}
