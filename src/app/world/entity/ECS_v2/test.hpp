#pragma once

#include <tuple>

template <typename EntityType, typename ComponentType>
class bar
{
public:
	ComponentType component;
};

template <typename EntityType, typename... ComponentTypes>
class Test
{
public:
	Test()
	{
		auto lambda = []<typename ComponentType>()
		{
			std::cout << "ComponentType: " << typeid(ComponentType).name() << std::endl;
		};

		(lambda.template operator()<ComponentTypes>(), ...);
	}

	typedef std::tuple<bar<EntityType, ComponentTypes>...> test_tuple;
private:
};
