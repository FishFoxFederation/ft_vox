#pragma once 

#include <concepts>
#include <functional>
#include <tuple>
#include "ecs_CONSTANTS.hpp"

namespace ecs
{
	template <typename I>
	concept ValidEntity = std::integral<I> && std::unsigned_integral<I>;

}
