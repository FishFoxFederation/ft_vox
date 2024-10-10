#include "VulkanAPI.hpp"
#include "Item.hpp"
#include "logger.hpp"

static void createItemCubeMesh(
	std::vector<ItemVertex> & vertices,
	std::vector<uint32_t> & indices
)
{
	// origine is at the bottom left back corner
	// triangle order is clockwise

	// the faces should be in the order: top, bottom, right, left, front, back
	// this is the order that the textures are in the block data

	// top
	const glm::vec3 normal_top = { 0.0f, 1.0f, 0.0f };

	vertices.push_back({ { 0.0f, 1.0f, 0.0f }, normal_top, { 0.0f, 0.0f }, 0 });
	vertices.push_back({ { 1.0f, 1.0f, 0.0f }, normal_top, { 1.0f, 0.0f }, 0 });
	vertices.push_back({ { 1.0f, 1.0f, 1.0f }, normal_top, { 1.0f, 1.0f }, 0 });
	vertices.push_back({ { 0.0f, 1.0f, 1.0f }, normal_top, { 0.0f, 1.0f }, 0 });

	indices.push_back(0);
	indices.push_back(1);
	indices.push_back(2);
	indices.push_back(2);
	indices.push_back(3);
	indices.push_back(0);

	// bottom
	const glm::vec3 normal_bottom = { 0.0f, -1.0f, 0.0f };

	vertices.push_back({ { 0.0f, 0.0f, 0.0f }, normal_bottom, { 0.0f, 0.0f }, 0 });
	vertices.push_back({ { 1.0f, 0.0f, 0.0f }, normal_bottom, { 1.0f, 0.0f }, 0 });
	vertices.push_back({ { 1.0f, 0.0f, 1.0f }, normal_bottom, { 1.0f, 1.0f }, 0 });
	vertices.push_back({ { 0.0f, 0.0f, 1.0f }, normal_bottom, { 0.0f, 1.0f }, 0 });

	indices.push_back(4);
	indices.push_back(6);
	indices.push_back(5);
	indices.push_back(4);
	indices.push_back(7);
	indices.push_back(6);

	// right
	const glm::vec3 normal_right = { 1.0f, 0.0f, 0.0f };

	vertices.push_back({ { 1.0f, 0.0f, 0.0f }, normal_right, { 1.0f, 1.0f }, 0 });
	vertices.push_back({ { 1.0f, 0.0f, 1.0f }, normal_right, { 0.0f, 1.0f }, 0 });
	vertices.push_back({ { 1.0f, 1.0f, 1.0f }, normal_right, { 0.0f, 0.0f }, 0 });
	vertices.push_back({ { 1.0f, 1.0f, 0.0f }, normal_right, { 1.0f, 0.0f }, 0 });

	indices.push_back(8);
	indices.push_back(9);
	indices.push_back(10);
	indices.push_back(10);
	indices.push_back(11);
	indices.push_back(8);

	// left
	const glm::vec3 normal_left = { -1.0f, 0.0f, 0.0f };

	vertices.push_back({ { 0.0f, 0.0f, 0.0f }, normal_left, { 0.0f, 1.0f }, 0 });
	vertices.push_back({ { 0.0f, 0.0f, 1.0f }, normal_left, { 1.0f, 1.0f }, 0 });
	vertices.push_back({ { 0.0f, 1.0f, 1.0f }, normal_left, { 1.0f, 0.0f }, 0 });
	vertices.push_back({ { 0.0f, 1.0f, 0.0f }, normal_left, { 0.0f, 0.0f }, 0 });

	indices.push_back(12);
	indices.push_back(14);
	indices.push_back(13);
	indices.push_back(12);
	indices.push_back(15);
	indices.push_back(14);

	// front
	const glm::vec3 normal_front = { 0.0f, 0.0f, 1.0f };

	vertices.push_back({ { 0.0f, 0.0f, 1.0f }, normal_front, { 0.0f, 1.0f }, 0 });
	vertices.push_back({ { 1.0f, 0.0f, 1.0f }, normal_front, { 1.0f, 1.0f }, 0 });
	vertices.push_back({ { 1.0f, 1.0f, 1.0f }, normal_front, { 1.0f, 0.0f }, 0 });
	vertices.push_back({ { 0.0f, 1.0f, 1.0f }, normal_front, { 0.0f, 0.0f }, 0 });

	indices.push_back(16);
	indices.push_back(18);
	indices.push_back(17);
	indices.push_back(16);
	indices.push_back(19);
	indices.push_back(18);

	// back
	const glm::vec3 normal_back = { 0.0f, 0.0f, -1.0f };

	vertices.push_back({ { 0.0f, 0.0f, 0.0f }, normal_back, { 1.0f, 1.0f }, 0 });
	vertices.push_back({ { 1.0f, 0.0f, 0.0f }, normal_back, { 0.0f, 1.0f }, 0 });
	vertices.push_back({ { 1.0f, 1.0f, 0.0f }, normal_back, { 0.0f, 0.0f }, 0 });
	vertices.push_back({ { 0.0f, 1.0f, 0.0f }, normal_back, { 1.0f, 0.0f }, 0 });

	indices.push_back(20);
	indices.push_back(21);
	indices.push_back(22);
	indices.push_back(22);
	indices.push_back(23);
	indices.push_back(20);
}

static void setTextureIndices(
	std::vector<ItemVertex> & vertices,
	const ItemInfo & itemInfo
)
{
	const BlockInfo &block_data = g_blocks_info.get(itemInfo.block_id);

	for (size_t i = 0; i < 6; i++)
	{
		const uint32_t texture_index = block_data.texture[i];

		for (size_t j = 0; j < 4; j++)
		{
			vertices[i * 4 + j].texture_index = texture_index;
		}
	}
}

void VulkanAPI::createItemMeshes()
{
	for (size_t i = 0; i < g_items_info.count(); i++)
	{
		ItemInfo & itemInfo = g_items_info.get(i);

		std::vector<ItemVertex> vertices;
		std::vector<uint32_t> indices;

		createItemCubeMesh(vertices, indices);
		setTextureIndices(vertices, itemInfo);

		itemInfo.mesh_id = storeMesh(
			vertices.data(),
			vertices.size(),
			sizeof(ItemVertex),
			indices.data(),
			indices.size()
		);

		if (itemInfo.mesh_id == invalid_mesh_id)
		{
			LOG_WARNING_FMT("Failed to create mesh for item: %d", i);
		}
	}
}
