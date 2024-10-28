#pragma once


#include "define.hpp"

#include "Block.hpp"

class StructuresInfo;

typedef std::vector<std::vector<std::vector<BlockInfo::Type>>> StructureBlocks;

class StructureInfo
{
public:
	enum class Type : uint16_t
	{
		Tree,
		None
	} const id = Type::None;

	StructureInfo() = delete;
	StructureInfo(const Type & type, const glm::ivec3 & size, const std::vector<std::vector<std::vector<BlockInfo::Type>>> & blocks) :
		id(type),
		size(size),
		blocks(blocks)
	{

	}


	const glm::ivec3 size = { 0, 0, 0 };
	BlockInfo::Type getBlock(const glm::ivec3 & pos) const
	{
		return blocks[pos.y][pos.z][pos.x];
	}
private:
	const StructureBlocks blocks;
};

class StructuresInfo
{
public:

	static const StructuresInfo & getInstance()
	{
		static StructuresInfo instance;
		return instance;
	}

	const StructureInfo & get(const StructureInfo::Type id) const
	{
		return m_infos[static_cast<size_t>(id)];
	}

private:

	std::vector<StructureInfo> m_infos;

	StructuresInfo();
};

extern const StructuresInfo & g_structures_info;
