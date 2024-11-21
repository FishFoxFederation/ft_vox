#pragma once

#include <array>
#include <typeinfo>
#include <iterator>
#include <vector>
#include "ecs_utils.hpp"
#include "ecs_CONSTANTS.hpp"

namespace ecs
{	
	template <ValidEntity entityType, typename ComponentType>
	struct SparseSetNode
	{
		entityType entity;
		ComponentType component;
	};

	
	template <ValidEntity entityType, typename ComponentType>
	class SparseSet
	{
	public:
		typedef SparseSetNode<entityType, ComponentType> node_type;
		typedef std::vector<node_type> dense_type;
		typedef std::vector<size_t> sparse_type;

		typedef dense_type::iterator					iterator;
		typedef dense_type::const_iterator				const_iterator;
		typedef dense_type::reverse_iterator			reverse_iterator;
		typedef dense_type::const_reverse_iterator		const_reverse_iterator;

		SparseSet()	{};
		SparseSet(size_t size)
			: m_dense(size), m_sparse(size) {};
		~SparseSet() = default;

		SparseSet(SparseSet & other) = default;
		SparseSet(SparseSet && other) = default;
		SparseSet & operator=(SparseSet & other) = default;
		SparseSet & operator=(SparseSet && other) = default;

		void insert(entityType entity, ComponentType component)
		{
			if (contains(entity))
				return;

			m_dense.push_back({ entity, component });

			if (m_sparse.size() <= entity)
				m_sparse.resize(entity + 1);

			m_sparse[entity] = m_dense.size() - 1;
		}

		void remove(entityType entity)
		{
			if (!contains(entity) || m_dense.size() == 0)
				return;
			
			size_t dense_index = m_sparse[entity];

			if (m_dense.size() > 1)
			{
				//swap with last element and then destroy
				entityType last_entity = m_dense.back().entity;

				//update sparse array before swapping
				m_sparse[last_entity] = dense_index;

				std::swap(m_dense[dense_index], m_dense.back());
			}
			m_dense.pop_back();
		}

		ComponentType &			get(entityType entity)
		{
			if (!contains(entity))
				throw std::logic_error("Entity does not have component");

			size_t dense_index = m_sparse[entity];
			return m_dense[dense_index].component;
		}

		const ComponentType &	get(entityType entity) const
		{
			if (!contains(entity))
				throw std::logic_error("Entity does not have component");

			size_t dense_index = m_sparse[entity];
			return m_dense[dense_index].component;
		}

		ComponentType *			tryGet(entityType entity)
		{
			if (!contains(entity))
				return nullptr;

			size_t dense_index = m_sparse[entity];
			return &m_dense[dense_index].component;
		}

		const ComponentType *	tryGet(entityType entity) const
		{
			if (!contains(entity))
				return nullptr;

			size_t dense_index = m_sparse[entity];
			return &m_dense[dense_index].component;
		}

		bool	contains(entityType entity) const
		{
			return (
				m_sparse.size() > entity
				&& m_sparse[entity] < m_dense.size()
				&& m_dense[m_sparse[entity]].entity == entity);
		}

		size_t	size() const
		{
			return m_dense.size();
		}
		
		iterator					begin()			{	return m_dense.begin(); }
		const_iterator				begin() const	{	return m_dense.begin(); }
		reverse_iterator			rbegin()		{	return std::make_reverse_iterator(end()); }
		const_reverse_iterator		rbegin() const	{	return std::make_reverse_iterator(end()); }

		iterator					end()			{	return m_dense.end(); }
		const_iterator				end() const		{	return m_dense.end(); }
		reverse_iterator			rend()			{	return std::make_reverse_iterator(begin()); }
		const_reverse_iterator		rend() const	{	return std::make_reverse_iterator(begin()); }

	private:
		/**
		 * @brief the dense array containing the nodes
		 * 
		 */
		dense_type	m_dense = {};

		/**
		 * @brief the sparse array containing the indices of the nodes in the dense array
		 * 
		 */
		sparse_type	m_sparse = {};
	};

}
