#pragma once

#include <unordered_map>
#include <array>
#include <limits>

#include "ecs_FWD.hpp"

#include "Manager.hpp"
#include "ComponentStorage.hpp"
#include "SparseSet.hpp"

namespace ecs
{
	template <ValidEntity EntityType, size_t pool_size>
	class ViewIterator
	{
	public:
		typedef SparseSet<EntityType> entitySet;
		typedef entitySet::iterator entityIterator;
		typedef std::shared_ptr<entitySet> SparseSetPtr;
		typedef std::array<SparseSetPtr, pool_size> SparseSetArray;

		ViewIterator(SparseSetArray & sparseSets, SparseSetPtr leadSet, entityIterator leadIter = entityIterator())
			: m_sparseSets{sparseSets}, m_leadSet(leadSet), m_lead_iterator(leadIter)
		{
			seek_next();
		}

		~ViewIterator(){};

		ViewIterator &	operator++()
		{
			m_lead_iterator++;
			seek_next();
			return *this;
		}

		ViewIterator	operator++(int)
		{
			ViewIterator temp = *this;
			++(*this);
			return temp;
		}

		ViewIterator &	operator--()
		{
			--m_lead_iterator;
			seek_prev();
			return *this;
		}

		ViewIterator	operator--(int)
		{
			ViewIterator temp = *this;
			--(*this);
			return temp;
		}

		bool			operator==(const ViewIterator & other) const
		{
			return (m_lead_iterator == other.m_lead_iterator);
		}

		bool			operator!=(const ViewIterator & other) const
		{
			return (!(*this == other));
		}

		EntityType		operator*()
		{
			return *m_lead_iterator;
		}
		
	private:
		

		bool all_of(EntityType entity)
		{
			for (auto & set : m_sparseSets)
			{
				if (!set->contains(entity))
					return false;
			}
			return true;
		}

		void seek_next()
		{
			for(; m_lead_iterator != m_leadSet->end() && !all_of(*m_lead_iterator); ++m_lead_iterator)
			{
			}
		}

		void seek_prev()
		{
			for(; m_lead_iterator != m_leadSet->begin() && !all_of(*m_lead_iterator); --m_lead_iterator)
			{
			}
		}

		SparseSetArray m_sparseSets;
		SparseSetPtr m_leadSet;
		entityIterator m_lead_iterator;
	};

	template <ValidEntity EntityType, typename... ComponentTypes>
	class View
	{
	public:
		typedef SparseSet<EntityType> entitySet;
		typedef IndexOf<ComponentTypes...> index;
		typedef std::array<std::shared_ptr<entitySet>, sizeof... (ComponentTypes)> entitySetArray;
		typedef ViewIterator<EntityType, sizeof... (ComponentTypes)> iterator;

		template <size_t manager_size>
		View( Manager<EntityType, manager_size> & storage)
		{
			//mmmmm spicy
			auto func = [&]<typename ComponentType>()
			{
				using ComponentSet = ComponentStorage<EntityType, ComponentType>;
				std::shared_ptr<ComponentSet> set = storage.template getSetPtr<ComponentType>();

				m_sparseSets[index::template get<ComponentType>()] = set;
			};
			(func.template operator()<ComponentTypes>(), ...);

			size_t min = std::numeric_limits<size_t>::max();
			for (size_t i = 0; i < m_sparseSets.size(); i++)
			{
				auto set = m_sparseSets[i];
				if (set->size() < min)
				{
					min = set->size();
					m_lead = i;
				}
			}
		}

		View(){};

		~View()
		{			
		}

		//begin and end
		iterator begin()
		{
			//give the iterator a lead set and the rest of the sets
			return iterator(m_sparseSets, m_sparseSets[m_lead], m_sparseSets[m_lead]->begin());
		}

		iterator end()
		{
			return iterator(m_sparseSets, m_sparseSets[m_lead], m_sparseSets[m_lead]->end());
		}
	private:
		entitySetArray m_sparseSets;
		size_t m_lead = 0;
	};

};
