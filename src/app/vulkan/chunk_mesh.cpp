#include "VulkanAPI.hpp"

#include <cstring>

uint64_t VulkanAPI::storeMesh(const std::vector<BlockVertex> & vertices, const std::vector<uint32_t> & indices)
{

	if (vertices.empty())
	{
		return no_mesh_id;
	}


	Mesh mesh;
	VkDeviceSize vertex_size = sizeof(vertices[0]) * vertices.size();
	VkDeviceSize index_size = sizeof(indices[0]) * indices.size();
	VkDeviceSize buffer_size = vertex_size + index_size;

	// std::lock_guard<std::mutex> lock(global_mutex);

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
	std::memcpy(data, vertices.data(), static_cast<size_t>(vertex_size));
	std::memcpy(static_cast<char *>(data) + vertex_size, indices.data(), static_cast<size_t>(index_size));
	vkUnmapMemory(device, staging_buffer_memory);

	createBuffer(
		buffer_size,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		mesh.buffer,
		mesh.buffer_memory
	);

	copyBuffer(staging_buffer, mesh.buffer, buffer_size);

	vkDestroyBuffer(device, staging_buffer, nullptr);
	vma.freeMemory(device, staging_buffer_memory, nullptr);

	mesh.vertex_count = static_cast<uint32_t>(vertices.size());
	mesh.index_offset = vertex_size;
	mesh.index_count = static_cast<uint32_t>(indices.size());
	mesh.memory_size = buffer_size;

	uint64_t mesh_id;
	{
		std::lock_guard<std::mutex> lock(mesh_mutex);
		mesh_id = next_mesh_id++;
		meshes.emplace(mesh_id, mesh);
	}

	return mesh_id;
}

void VulkanAPI::destroyMesh(const uint64_t & mesh_id)
{
	std::lock_guard<std::mutex> lock(global_mutex);
	mesh_ids_to_destroy.push_back(mesh_id);
	destroyMeshes();
}

void VulkanAPI::destroyMeshes(const std::vector<uint64_t> & mesh_ids)
{
	std::lock_guard<std::mutex> lock(global_mutex);
	mesh_ids_to_destroy.insert(mesh_ids_to_destroy.end(), mesh_ids.begin(), mesh_ids.end());
	destroyMeshes();
}

void VulkanAPI::destroyMeshes()
{
	std::vector<uint64_t> meshes_still_in_use;
	meshes_still_in_use.reserve(mesh_ids_to_destroy.size());
	for (auto & id: mesh_ids_to_destroy)
	{
		std::lock_guard<std::mutex> lock(mesh_mutex);
		auto mesh = meshes.find(id);
		if (mesh != meshes.end() && mesh->second.is_used == false)
		{
			// LOG_INFO("Destroying mesh: " << id);
			vkDestroyBuffer(device, mesh->second.buffer, nullptr);
			vma.freeMemory(device, mesh->second.buffer_memory, nullptr);
			meshes.erase(mesh);
		}
		else
		{
			meshes_still_in_use.push_back(id);
		}
	}
	mesh_ids_to_destroy = std::move(meshes_still_in_use);
}
