#pragma once

#include "CreateMeshData.hpp"

#include <glm/vec3.hpp>
#include <unordered_map>

namespace std
{
	template<>
	struct hash<glm::ivec3>
	{
		std::size_t operator()(const glm::ivec3 & k) const
		{
			return std::hash<int>()(k.x) ^ std::hash<int>()(k.y) ^ std::hash<int>()(k.z);
		}
	};
}
