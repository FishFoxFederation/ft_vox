#pragma once

#include "CreateMeshData.hpp"

#include <glm/vec3.hpp>
#include <glm/vec2.hpp>

namespace std
{
	template<>
	struct hash<glm::ivec3>
	{
		std::size_t operator()(const glm::ivec3 & v) const
		{
			static const unsigned int m = 0x5bd1e995U;

			unsigned int hash = 0;
			unsigned int k = v.x;

			//first vector element
			k *= m;
			k ^= k >> 24;
			k *= m;
			hash *= m;
			hash ^= k;
			//second vector element
			k = v.y;
			k *= m;
			k ^= k >> 24;
			k *= m;
			hash *= m;
			hash ^= k;
			//third vector element
			k = v.z;
			k *= m;
			k ^= k >> 24;
			k *= m;
			hash *= m;
			hash ^= k;

			hash ^= hash >> 13;
			hash *= m;
			hash ^= hash >> 15;
			return hash;
		}
	};

	template<>
	struct hash<glm::ivec2>
	{
		std::size_t operator()(const glm::ivec2 & v) const
		{
			static const unsigned int m = 0x5bd1e995U;

			unsigned int hash = 0;
			unsigned int k = v.x;

			//first vector element
			k *= m;
			k ^= k >> 24;
			k *= m;
			hash *= m;
			hash ^= k;
			//second vector element
			k = v.y;
			k *= m;
			k ^= k >> 24;
			k *= m;
			hash *= m;
			hash ^= k;

			hash ^= hash >> 13;
			hash *= m;
			hash ^= hash >> 15;
			return hash;
		}
	};
}
