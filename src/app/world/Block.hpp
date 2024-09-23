#pragma once

#include "define.hpp"
#include "HitBox.hpp"

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
	Stone,
	Water,
	Glass
};

// Block properties
#define BLOCK_PROPERTY_NONE		0U		// default
#define BLOCK_PROPERTY_SOLID	1U		// solid block (can be walked on)
#define BLOCK_PROPERTY_OPAQUE	1U << 1 // opaque block (cannot be seen through)
#define BLOCK_PROPERTY_CUBE		1U << 2 // cube block (is a standard cube shape)
#define BLOCK_PROPERTY_LIGHT	1U << 3 // light block (emits light)
#define BLOCK_PROPERTY_FLUID	1U << 4 // fluid block (flows)

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
	const HitBox hitbox;
	const int emit_light; // light level emitted by the block
	const int absorb_light; // light level absorbed by the block
};

struct Block
{

	static inline const std::vector<std::string> texture_names = {
		"assets/textures/block/grass_top.png", // 0
		"assets/textures/block/grass_top.png", // 1
		"assets/textures/block/grass_side.png", // 2
		"assets/textures/block/dirt.png", // 3
		"assets/textures/block/stone.png", // 4
		"assets/textures/block/water.png", // 5
		"assets/textures/block/glass_clear.png", // 5
		// "assets/textures/block/debug/white.png",
		// "assets/textures/block/debug/right.png",
		// "assets/textures/block/debug/left.png",
		// "assets/textures/block/debug/front.png",
		// "assets/textures/block/debug/top.png",
		// "assets/textures/block/debug/back_bottom.png",
	};

	static const Data & getData(const BlockID id)
	{
		switch (id)
		{
		case BlockID::Air: return Air;
		case BlockID::Grass: return Grass;
		case BlockID::Dirt: return Dirt;
		case BlockID::Stone: return Stone;
		case BlockID::Water: return Water;
		case BlockID::Glass: return Glass;
		default: return Air;
		}
	}

	static bool hasProperty(const BlockID id, const BlockProperties property)
	{
		return (getData(id).properties & property) == property;
	}

	static inline const Data Air = {
		.id = BlockID::Air,
		.texture = { 0, 0, 0, 0, 0, 0 },
		.properties = BLOCK_PROPERTY_NONE,
		.hitbox = {{0, 0, 0}, {1, 1, 1}},
		.emit_light = 0,
		.absorb_light = 0
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
			| BLOCK_PROPERTY_CUBE,
		.hitbox = {{0, 0, 0}, {1, 1, 1}},
		.emit_light = 0,
		.absorb_light = 15
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
			| BLOCK_PROPERTY_CUBE,
		.hitbox = {{0, 0, 0}, {1, 1, 1}},
		.emit_light = 0,
		.absorb_light = 15
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
			| BLOCK_PROPERTY_CUBE,
		.hitbox = {{0, 0, 0}, {1, 1, 1}},
		.emit_light = 0,
		.absorb_light = 15
	};
	static inline const Data Water = {
		.id = BlockID::Water,
		.texture = {
			5, // water
			5, // water
			5, // water
			5, // water
			5, // water
			5  // water
		},
		.properties =
			BLOCK_PROPERTY_FLUID,
		.hitbox = {{0, 0, 0}, {1, 1, 1}},
		.emit_light = 0,
		.absorb_light = 1
	};
	static inline const Data Glass = {
		.id = BlockID::Glass,
		.texture = {
			6, // glass
			6, // glass
			6, // glass
			6, // glass
			6, // glass
			6  // glass
		},
		.properties =
			BLOCK_PROPERTY_SOLID
			| BLOCK_PROPERTY_CUBE,
		.hitbox = {{0, 0, 0}, {1, 1, 1}},
		.emit_light = 0,
		.absorb_light = 0
	};

};
