#pragma once

#include "ECS_CONSTANTS.hpp"
#include <array>

namespace ecs
{

	template <typename ComponentType, std::size_t size>
	class SparseSet
	{
	public:
		SparseSet() = default;
		~SparseSet() = default;
		typedef std::array<ComponentType, size>::iterator iterator;
		typedef std::array<ComponentType, size>::const_iterator const_iterator;

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
		std::array<T, size> m_dense;
		std::array<entityType, size> m_sparse;

		size_t m_dense_size = 0;
	};
}
