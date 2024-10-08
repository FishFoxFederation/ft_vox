#pragma once

#include "define.hpp"
#include "Block.hpp"

#include <cstdint>

class Item
{

public:

	enum class Type : uint16_t
	{
		GrassBlock,
		DirtBlock,
		StoneBlock,
		WaterBlock,
		GlassBlock,
		LightBlock,

		None

	} const type = Type::None;

	const BlockID block_id;
	uint64_t mesh_id = 0;

private:

};

struct Items
{
	static inline std::vector<Item> list = {
		{ Item::Type::GrassBlock, BlockID::Grass },
		{ Item::Type::DirtBlock, BlockID::Dirt },
		{ Item::Type::StoneBlock, BlockID::Stone },
		{ Item::Type::WaterBlock, BlockID::Water },
		{ Item::Type::GlassBlock, BlockID::Glass },
		{ Item::Type::LightBlock, BlockID::Light }
	};
};
