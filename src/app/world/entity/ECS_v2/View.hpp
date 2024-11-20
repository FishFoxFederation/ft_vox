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

		viewIterator &	operator++();
		viewIterator	operator++(int);
		viewIterator &	operator--();
		viewIterator	operator--(int);
		bool			operator==(const viewIterator & other) const;
		bool			operator!=(const viewIterator & other) const;

	private:

	};

	template <typename... ComponentTypes>
	class View
	{
	public:
		View();
		// View(ECS & ecs) {};

		viewIterator<ComponentTypes...>	begin();
		viewIterator<ComponentTypes...>	end();
	private:
	};
};
