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

// Block faces coresponding to the texture array
#define BLOCK_FACE_TOP		0
#define BLOCK_FACE_BOTTOM	1
#define BLOCK_FACE_LEFT		2
#define BLOCK_FACE_RIGHT	3
#define BLOCK_FACE_FRONT	4
#define BLOCK_FACE_BACK		5

struct Data
{
	const BlockID id;
	const TextureID texture[6];
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
		// "assets/textures/debug/white.png",
		// "assets/textures/debug/right.png",
		// "assets/textures/debug/left.png",
		// "assets/textures/debug/front.png",
		// "assets/textures/debug/top.png",
		// "assets/textures/debug/back_bottom.png",
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

	static bool hasProperty(const BlockID id, const BlockProperties property)
	{
		return (getData(id).properties & property) == property;
	}

	static inline const Data Air = {
		.id = BlockID::Air,
		.texture = {
			0,
			0,
			0,
			0,
			0,
			0
		},
		.properties = BLOCK_PROPERTY_NONE
	};
	static inline const Data Grass = {
		.id = BlockID::Grass,
		.texture = {
			1, // grass_top
			3, // dirt
			2, // grass_side
			2, // grass_side
			2, // grass_side
			2  // grass_side
		},
		.properties =
			BLOCK_PROPERTY_SOLID
			| BLOCK_PROPERTY_OPAQUE
			| BLOCK_PROPERTY_CUBE
	};
	static inline const Data Dirt = {
		.id = BlockID::Dirt,
		.texture = {
			3, // dirt
			3, // dirt
			3, // dirt
			3, // dirt
			3, // dirt
			3  // dirt
		},
		.properties =
			BLOCK_PROPERTY_SOLID
			| BLOCK_PROPERTY_OPAQUE
			| BLOCK_PROPERTY_CUBE
	};
	static inline const Data Stone = {
		.id = BlockID::Stone,
		.texture = {
			4, // stone
			4, // stone
			4, // stone
			4, // stone
			4, // stone
			4  // stone
		},
		.properties =
			BLOCK_PROPERTY_SOLID
			| BLOCK_PROPERTY_OPAQUE
			| BLOCK_PROPERTY_CUBE
	};

};
