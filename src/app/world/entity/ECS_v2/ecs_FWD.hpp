#pragma once

#include "ecs_CONSTANTS.hpp"
#include "ecs_utils.hpp"

namespace ecs
{
	template <ValidEntity EntityType, typename... ComponentTypes>
	class View;

	template <ValidEntity EntityType, size_t pool_size>
	class ViewIterator;

	template <ValidEntity entityType = ecs::entity>
	class Manager;
}
