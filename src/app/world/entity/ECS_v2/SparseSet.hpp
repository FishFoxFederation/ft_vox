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
	};
	
	template <ValidEntity entityType>
	class SparseSet
	{
	public:
		typedef entityType node_type;
		typedef std::vector<node_type> dense_type;
		typedef std::vector<size_t> sparse_type;

		typedef dense_type::iterator					iterator;
		typedef dense_type::const_iterator				const_iterator;
		typedef dense_type::reverse_iterator			reverse_iterator;
		typedef dense_type::const_reverse_iterator		const_reverse_iterator;

		SparseSet()	{};
		SparseSet(size_t size)
			: m_dense(size), m_sparse(size) {};
		virtual ~SparseSet() = default;

		SparseSet(SparseSet & other) = default;
		SparseSet(SparseSet && other) = default;
		SparseSet & operator=(SparseSet & other) = default;
		SparseSet & operator=(SparseSet && other) = default;

		size_t insert(entityType entity)
		{
			auto entity_index = ecs::Manager<entityType>::getEntityIndex(entity);
			if (contains(entity))
				return 0;

			m_dense.push_back({entity});

			if (m_sparse.size() <= entity_index)
				m_sparse.resize(entity_index + 1);

			auto dense_index = m_dense.size() - 1;
			m_sparse[entity_index] = dense_index;
			return dense_index;
		}

		virtual size_t remove(entityType entity)
		{
			if (!contains(entity) || m_dense.size() == 0)
				return 0;
			size_t entity_index = ecs::Manager<entityType>::getEntityIndex(entity);
			size_t dense_index = m_sparse[entity_index];

			if (m_dense.size() > 1)
			{
				//swap with last element and then destroy
				entityType last_entity = m_dense.back();
				size_t last_entity_index = ecs::Manager<entityType>::getEntityIndex(last_entity);
				//update sparse array before swapping
				m_sparse[last_entity_index] = dense_index;

				std::swap(m_dense[dense_index], m_dense.back());
			}
			m_dense.pop_back();
			return dense_index;
		}

		// ComponentType &			get(entityType entity)
		// {
		// 	if (!contains(entity))
		// 		throw std::logic_error("Entity does not have component");

		// 	size_t dense_index = m_sparse[entity];
		// 	return m_dense[dense_index].component;
		// }

		// const ComponentType &	get(entityType entity) const
		// {
		// 	if (!contains(entity))
		// 		throw std::logic_error("Entity does not have component");

		// 	size_t dense_index = m_sparse[entity];
		// 	return m_dense[dense_index].component;
		// }

		// ComponentType *			tryGet(entityType entity)
		// {
		// 	if (!contains(entity))
		// 		return nullptr;

		// 	size_t dense_index = m_sparse[entity];
		// 	return &m_dense[dense_index].component;
		// }

		// const ComponentType *	tryGet(entityType entity) const
		// {
		// 	if (!contains(entity))
		// 		return nullptr;

		// 	size_t dense_index = m_sparse[entity];
		// 	return &m_dense[dense_index].component;
		// }

		size_t index(entityType entity) const
		{
			size_t entity_index = ecs::Manager<entityType>::getEntityIndex(entity);
			return m_sparse[entity_index];
		}

		bool	contains(entityType entity) const
		{
			auto entity_index = ecs::Manager<entityType>::getEntityIndex(entity);
			//check that entity could be in sparse array
			//then check that it could be in dense array
			//then check that the entity is the same

			//we check the whole entity in the dense array and not just the index because
			//the index could be the same be the entity metadata ( version ) could be different 
			return (
				m_sparse.size() > entity_index
				&& m_sparse[entity_index] < m_dense.size()
				&& m_dense[m_sparse[entity_index]] == entity);
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
