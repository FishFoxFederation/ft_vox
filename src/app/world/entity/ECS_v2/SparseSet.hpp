#pragma once

#include "ECS_CONSTANTS.hpp"
#include <array>

namespace ecs
{

	template <typename ComponentType>
	class SparseSet
	{
	public:
		SparseSet() = default;
		~SparseSet() = default;
		typedef std::array<ComponentType, MAX_ENTITIES>::iterator iterator;
		typedef std::array<ComponentType, MAX_ENTITIES>::const_iterator const_iterator;

		SparseSet(SparseSet & other) = default;
		SparseSet(SparseSet && other) = default;
		SparseSet & operator=(SparseSet & other) = default;
		SparseSet & operator=(SparseSet && other) = default;

		void insert(entityType entity, ComponentType component);
		void remove(entityType entity);

		ComponentType &			get(entityType entity);
		const ComponentType &	get(entityType entity) const;

		bool	contains(entityType entity) const;
		size_t	size() const;

		iterator		begin();
		const_iterator	begin() const;
		iterator		end();
		const_iterator	end() const;
	private:
		std::array<T, MAX_ENTITIES> m_dense;
		std::array<entityType, MAX_ENTITIES> m_sparse;

		size_t m_dense_size = 0;
	};
}
