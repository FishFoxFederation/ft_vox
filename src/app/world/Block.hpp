#pragma once

#include "define.hpp"

typedef int_fast8_t	BlockType;
//more to come soon

enum class Block : BlockType
{
	Air = 0,
	Grass = 1,
	Dirt = 2,
	Stone = 3,
	Sand = 4,
	//more to come soon
};
