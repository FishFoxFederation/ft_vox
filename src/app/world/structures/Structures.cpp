#include "Structures.hpp"

StructuresInfo & g_structures_info = StructuresInfo::getInstance();

typedef BlockInfo::Type BT;

StructuresInfo::StructuresInfo():
	m_infos({
		StructureInfo(
			StructureInfo::Type::Tree,
			{ 3, 6, 3},
			{
				{
					{BT::Air, BT::Air, BT::Air},//air air air
					{BT::Air, BT::Wood, BT::Air},// air wood air
					{BT::Air, BT::Air, BT::Air}// air air air
				},
				{
					{BT::Air, BT::Air, BT::Air},// air air air
					{BT::Air, BT::Wood, BT::Air},// air wood air
					{BT::Air, BT::Air, BT::Air}// air air air
				},
				{
					{BT::Air, BT::Air, BT::Air},// air air air
					{BT::Air, BT::Wood, BT::Air},// air wood air
					{BT::Air, BT::Air, BT::Air}// air air air
				},
				{
					{BT::Leaves, BT::Leaves, BT::Leaves},
					{BT::Leaves, BT::Wood, BT::Leaves},
					{BT::Leaves, BT::Leaves, BT::Leaves}
				},
				{
					{BT::Leaves, BT::Leaves, BT::Leaves},
					{BT::Leaves, BT::Wood, BT::Leaves},
					{BT::Leaves, BT::Leaves, BT::Leaves}
				},
				{
					{BT::Air, BT::Leaves, BT::Air},
					{BT::Leaves, BT::Leaves, BT::Leaves},
					{BT::Air, BT::Leaves, BT::Air}
				}
			}
		)
	})
{

}
