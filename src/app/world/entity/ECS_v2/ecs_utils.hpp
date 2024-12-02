#pragma once 

#include <concepts>
#include <functional>
#include <tuple>
#include "ecs_CONSTANTS.hpp"

namespace ecs
{
	template <typename I>
	concept ValidEntity = std::integral<I> && std::unsigned_integral<I>;

	template <typename T, typename... Ts>
	concept is_any = std::disjunction_v<std::is_same<T, Ts>...>;

	template <typename... Args>
	struct IndexOf
	{
		template <typename T> requires is_any<T, Args...>
		static size_t get()
		{
			static size_t local_index = index++;

			return local_index;
		}
	private:
		static size_t index;
	};

	template <typename... Args>
	size_t IndexOf<Args...>::index = 0;
}
