#include "Item.hpp"

ItemsInfo & g_items_info = ItemsInfo::getInstance();

ItemsInfo::ItemsInfo():
	m_info({
		{
			.type = ItemInfo::Type::GrassBlock,
			.block_id = BlockInfo::Type::Grass
		},
		{
			.type = ItemInfo::Type::DirtBlock,
			.block_id = BlockInfo::Type::Dirt
		},
		{
			.type = ItemInfo::Type::StoneBlock,
			.block_id = BlockInfo::Type::Stone
		},
		{
			.type = ItemInfo::Type::WaterBlock,
			.block_id = BlockInfo::Type::Water
		},
		{
			.type = ItemInfo::Type::GlassBlock,
			.block_id = BlockInfo::Type::Glass
		},
		{
			.type = ItemInfo::Type::LightBlock,
			.block_id = BlockInfo::Type::Light
		},
		{
			.type = ItemInfo::Type::WoodBlock,
			.block_id = BlockInfo::Type::Wood
		},
		{
			.type = ItemInfo::Type::LeavesBlock,
			.block_id = BlockInfo::Type::Leaves
		}
	})
{
}

