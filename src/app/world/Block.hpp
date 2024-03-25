#pragma once

#include "define.hpp"

#include <string>
#include <vector>

typedef int_fast8_t	BlockType;
typedef uint64_t BlockProperties;
typedef uint32_t TextureID;
//more to come soon

enum class BlockID : BlockType
{
	Air,
	Grass,
	Dirt,
	Stone
};

// Block properties
#define BLOCK_PROPERTY_NONE		0U		// default
#define BLOCK_PROPERTY_SOLID	1U		// solid block (can be walked on)
#define BLOCK_PROPERTY_OPAQUE	1U << 1 // opaque block (cannot be seen through)
#define BLOCK_PROPERTY_CUBE		1U << 2 // cube block (is a standard cube shape)
#define BLOCK_PROPERTY_LIGHT	1U << 2 // light block (emits light)

struct TextureData
{
	const TextureID top;
	const TextureID bottom;
	const TextureID left;
	const TextureID right;
	const TextureID front;
	const TextureID back;
};

struct Data
{
	const BlockID id;
	const TextureData texture;
	const BlockProperties properties;
};

struct Block
{

	static inline const std::vector<std::string> texture_names = {
		"assets/textures/grass_top.jpg", // 0
		"assets/textures/grass_top.jpg", // 1
		"assets/textures/grass_side.jpg", // 2
		"assets/textures/dirt.jpg", // 3
		"assets/textures/stone.jpg" // 4
	};

	static const Data & getData(const BlockID id)
	{
		switch (id)
		{
		case BlockID::Air: return Air;
		case BlockID::Grass: return Grass;
		case BlockID::Dirt: return Dirt;
		case BlockID::Stone: return Stone;
		default: return Air;
		}
	}

	static inline const Data Air = {
		.id = BlockID::Air,
		.texture = {
			.top = 0,
			.bottom = 0,
			.left = 0,
			.right = 0,
			.front = 0,
			.back = 0
		},
		.properties = BLOCK_PROPERTY_NONE
	};
	static inline const Data Grass = {
		.id = BlockID::Grass,
		.texture = {
			.top = 1, // grass_top
			.bottom = 3, // dirt
			.left = 2, // grass_side
			.right = 2, // grass_side
			.front = 2, // grass_side
			.back = 2 // grass_side
		},
		.properties =
			BLOCK_PROPERTY_SOLID
			| BLOCK_PROPERTY_OPAQUE
			| BLOCK_PROPERTY_CUBE
	};
	static inline const Data Dirt = {
		.id = BlockID::Dirt,
		.texture = {
			.top = 3, // dirt
			.bottom = 3, // dirt
			.left = 3, // dirt
			.right = 3, // dirt
			.front = 3, // dirt
			.back = 3 // dirt
		},
		.properties =
			BLOCK_PROPERTY_SOLID
			| BLOCK_PROPERTY_OPAQUE
			| BLOCK_PROPERTY_CUBE
	};
	static inline const Data Stone = {
		.id = BlockID::Stone,
		.texture = {
			.top = 4, // stone
			.bottom = 4, // stone
			.left = 4, // stone
			.right = 4, // stone
			.front = 4, // stone
			.back = 4 // stone
		},
		.properties =
			BLOCK_PROPERTY_SOLID
			| BLOCK_PROPERTY_OPAQUE
			| BLOCK_PROPERTY_CUBE
	};

};
