#pragma once

#include <vector>

#include "ecs_Exceptions.hpp"
#include "SparseSet.hpp"

namespace ecs
{
	template <ValidEntity EntityType, typename ComponentType>
	class ComponentStorage : public SparseSet<EntityType>
	{
	public:
		typedef std::vector<ComponentType> container_type;
		typedef SparseSet<EntityType> base_type;
		typedef typename container_type::iterator iterator;
		typedef typename container_type::const_iterator const_iterator;
		typedef typename container_type::reverse_iterator reverse_iterator;
		typedef typename container_type::const_reverse_iterator const_reverse_iterator;

		ComponentStorage(){};
		~ComponentStorage(){};

		ComponentStorage(ComponentStorage & other) = default;
		ComponentStorage(ComponentStorage && other) = default;
		ComponentStorage & operator=(ComponentStorage & other) = default;
		ComponentStorage & operator=(ComponentStorage && other) = default;

		void insert(EntityType entity, ComponentType component)
		{
			if (base_type::contains(entity))
				return;
			base_type::insert(entity);
			m_components.push_back(component);
		}

		void remove(EntityType entity)
		{
			if (!base_type::contains(entity))
				return;
			auto index = base_type::remove(entity);
			std::swap(m_components[index], m_components.back());
			m_components.pop_back();
		}

		ComponentType & get(EntityType entity)
		{
			if (!base_type::contains(entity))
				std::out_of_range("Entity does not have component");
			return m_components[base_type::index(entity)];
		}

		const ComponentType & get(EntityType entity) const
		{
			if (!base_type::contains(entity))
				std::out_of_range("Entity does not have component");
			return m_components[base_type::index(entity)];
		}

		ComponentType * tryGet(EntityType entity)
		{
			if (!base_type::contains(entity))
				return nullptr;
			return &m_components[base_type::index(entity)];
		}

		const ComponentType * tryGet(EntityType entity) const
		{
			if (!base_type::contains(entity))
				return nullptr;
			return &m_components[base_type::index(entity)];
		}

		size_t size() const
		{
			return m_components.size();
		}

		iterator		begin()			{ return m_components.begin(); }
		const_iterator	begin() const	{ return m_components.begin(); }
		iterator		end()			{ return m_components.end(); }
		const_iterator	end() const		{ return m_components.end(); }

		reverse_iterator		rbegin()		{ return m_components.rbegin(); }
		const_reverse_iterator	rbegin() const	{ return m_components.rbegin(); }
		reverse_iterator		rend()			{ return m_components.rend(); }
		const_reverse_iterator	rend()	 const	{ return m_components.rend(); }
	private:
		container_type m_components;
	};
}
