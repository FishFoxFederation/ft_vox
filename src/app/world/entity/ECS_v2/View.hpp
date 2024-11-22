#pragma once

#include <unordered_map>
#include <array>

#include "Manager.hpp"
#include "ComponentStorage.hpp"
#include "SparseSet.hpp"

namespace ecs
{
	template <ValidEntity EntityType, size_t pool_size>
	class viewIterator
	{
	public:
		viewIterator();
		~viewIterator();

		viewIterator &	operator++()
		{
			seek_next();
			return *this;
		}

		viewIterator	operator++(int)
		{
			viewIterator temp = *this;
			seek_next();
			return temp;
		}

		viewIterator &	operator--()
		{
			seek_prev();
			return *this;
		}

		viewIterator	operator--(int)
		{
			viewIterator temp = *this;
			seek_prev();
			return temp;
		}

		bool			operator==(const viewIterator & other) const
		{
			return (*m_lead_iterator == *other.m_lead_iterator);
		}

		bool			operator!=(const viewIterator & other) const
		{
			return (!(*this == other));
		}

		EntityType		operator*()
		{
			return *m_lead_iterator;
		}
		
	private:
		typedef SparseSet<EntityType> entitySet;
		typedef entitySet::iterator entityIterator;
		typedef std::shared_ptr<entitySet> SparseSetPtr;
		typedef std::array<SparseSetPtr, pool_size> SparseSetArray;

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
		typedef viewIterator<EntityType, sizeof... (ComponentTypes)> iterator;

		// template <size_t storage_size>
		// View(ecs::Manager<EntityType, storage_size> & storage)
		// {
		// 	auto func = [&]<typename ComponentType>()
		// 	{
		// 		using ComponentSet = ComponentStorage<EntityType, ComponentType>;
		// 		std::shared_ptr<ComponentSet> set = storage.getSetPtr<ComponentType>();

		// 		m_sparseSets.insert({std::type_index(typeid(ComponentType)), set});
		// 	};
		// 	(func.template operator()<ComponentTypes>(), ...);
		// }
		View(){};
		~View()
		{			
		}

		// template<typename ComponentType>
		// ComponentType & get(EntityType entity)
		// {
		// 	using ComponentSetType = ComponentStorage<EntityType, ComponentType>;
		// 	auto it = m_sparseSets.find(std::type_index(typeid(ComponentType)));
		// 	if (it == m_sparseSets.end())
		// 		throw std::out_of_range("Component does not exist in view");
			
		// 	return it->second->get(entity);
		// }

		// std::tuple<ComponentType &...> get(EntityType entity)
		// {
		// 	return {get<ComponentTypes>(entity)...};
		// }

		//begin and end
		iterator begin()
		{
			//give the iterator a lead set and the rest of the sets
		}
	private:
		std::unordered_map<std::type_index, std::shared_ptr<entitySet>> m_sparseSets;
	};

};
