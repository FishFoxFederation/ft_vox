#pragma once

#include "ECS.hpp"



namespace ecs
{
	template <typename... ComponentTypes>
	class viewIterator
	{
	public:
		viewIterator();
		~viewIterator();

		viewIterator & operator++();
		viewIterator operator++(int);
		bool operator==(const viewIterator & other) const;
		bool operator!=(const viewIterator & other) const;

	private:

	};

	template <typename... ComponentTypes>
	class View
	{
	public:
		View(ECS & ecs);

		
	private:
		ECS & m_ecs;
		std::tuple<std::shared_ptr<SparseSet<ComponentTypes>>...> m_components;
	};
};
