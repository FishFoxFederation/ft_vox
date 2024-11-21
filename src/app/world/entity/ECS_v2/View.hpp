#pragma once

#include <unordered_map>
#include <array>

#include "Storage.hpp"
#include "SparseSet.hpp"

namespace ecs
{
	template <typename EntityType, typename... ComponentTypes>
	class viewIterator
	{
	public:
		viewIterator();
		~viewIterator();

		viewIterator &	operator++();
		viewIterator	operator++(int);
		viewIterator &	operator--();
		viewIterator	operator--(int);
		bool			operator==(const viewIterator & other) const;
		bool			operator!=(const viewIterator & other) const;
		
	private:
	};

	template <typename EntityType, typename... ComponentTypes>
	class View
	{
	public:
		typedef viewIterator<EntityType, ComponentTypes...> iterator;
		template <size_t storage_size>
		View(ecs::Storage<EntityType, storage_size> & storage)
		{
			auto func = [&]<typename ComponentType>()
			{
				using SparseSetType = SparseSet<EntityType, ComponentType>;
				auto set = storage.getSetPtr<ComponentType>();
				m_sparseSets.insert({std::type_index(typeid(ComponentType)), set});
			};
			(func.template operator()<ComponentTypes>(), ...);
		}

		~View()
		{			
		}

		//begin and end
		iterator begin()
		{
			//find the smallest set
		}
	private:
		typedef std::tuple<std::pair<ComponentTypes, size_t> ...> indices_tuple;
		typedef std::unordered_map<std::type_index, std::shared_ptr<void>> SparseSets_storage;
		SparseSets_storage m_sparseSets;
	};
};
