#include "VulkanAPI.hpp"
#include "DebugGui.hpp"

#include "Tracy.hpp"

#include <cstring>

uint64_t VulkanAPI::storeMesh(
	const void * vertices,
	const uint32_t vertex_count,
	const uint32_t vertex_size,
	const void * indices,
	const uint32_t index_count
)
{
	ZoneScoped;

	if (vertex_count == 0 || index_count == 0)
	{
		return invalid_mesh_id;
	}


	Mesh mesh;
	VkDeviceSize vertex_buffer_size = vertex_count * vertex_size;
	VkDeviceSize index_buffer_size = index_count * sizeof(uint32_t);
	VkDeviceSize buffer_size = vertex_buffer_size + index_buffer_size;

	// std::lock_guard lock(global_mutex);

	VkBuffer staging_buffer;
	VkDeviceMemory staging_buffer_memory;
	createBuffer(
		buffer_size,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		staging_buffer,
		staging_buffer_memory
	);

	void * data;
	VK_CHECK(
		vkMapMemory(device, staging_buffer_memory, 0, buffer_size, 0, &data),
		"Failed to map memory for vertex/index staging buffer."
	);
	std::memcpy(data, vertices, static_cast<size_t>(vertex_buffer_size));
	std::memcpy(static_cast<char *>(data) + vertex_buffer_size, indices, static_cast<size_t>(index_buffer_size));
	vkUnmapMemory(device, staging_buffer_memory);

	createBuffer(
		buffer_size,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT
		| VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
		| VK_BUFFER_USAGE_INDEX_BUFFER_BIT
		| VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		// | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		mesh.buffer,
		mesh.buffer_memory
	);

	copyBuffer(staging_buffer, mesh.buffer, buffer_size);

	vkDestroyBuffer(device, staging_buffer, nullptr);
	VulkanMemoryAllocator & vma = VulkanMemoryAllocator::getInstance();
	vma.freeMemory(device, staging_buffer_memory, nullptr);

	mesh.vertex_count = vertex_count;
	mesh.vertex_size = vertex_size;
	mesh.index_offset = vertex_buffer_size;
	mesh.index_count = index_count;
	mesh.memory_size = buffer_size;

	VkBufferDeviceAddressInfoKHR address_info = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
		.pNext = nullptr,
		.buffer = mesh.buffer
	};
	mesh.buffer_address = vkGetBufferDeviceAddress(device, &address_info);

	uint64_t mesh_id;
	{
		std::lock_guard lock(mesh_map_mutex);
		mesh_id = next_mesh_id++;
		mesh_map.emplace(mesh_id, mesh);
	}

	return mesh_id;
}

void VulkanAPI::destroyMesh(const uint64_t & mesh_id)
{
	std::lock_guard lock(global_mutex);
	_destroyMesh(mesh_id);
}

void VulkanAPI::_destroyMesh(const uint64_t & mesh_id)
{
	mesh_ids_to_destroy.push_back(mesh_id);
	destroyMeshes();
}

void VulkanAPI::destroyMeshes(const std::vector<uint64_t> & mesh_ids)
{
	std::lock_guard lock(global_mutex);
	mesh_ids_to_destroy.insert(mesh_ids_to_destroy.end(), mesh_ids.begin(), mesh_ids.end());
	destroyMeshes();
}

void VulkanAPI::destroyMeshes()
{
	ZoneScoped;
	std::lock_guard lock(mesh_map_mutex);

	std::vector<uint64_t> meshes_still_in_use;
	meshes_still_in_use.reserve(mesh_ids_to_destroy.size());
	for (auto & id: mesh_ids_to_destroy)
	{
		if (mesh_map.contains(id))
		{
			Mesh mesh = mesh_map.at(id);
			if (mesh.is_used == false)
			{
				vkDestroyBuffer(device, mesh.buffer, nullptr);
				VulkanMemoryAllocator & vma = VulkanMemoryAllocator::getInstance();
				vma.freeMemory(device, mesh.buffer_memory, nullptr);
				mesh_map.erase(id);
				// removeMeshFromTopLevelAS(id);
			}
			else
			{
				meshes_still_in_use.push_back(id);
			}
		}
	}
	mesh_ids_to_destroy = std::move(meshes_still_in_use);
}
