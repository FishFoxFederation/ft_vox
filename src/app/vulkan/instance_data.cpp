#include "VulkanAPI.hpp"

void VulkanAPI::_createInstanceData()
{
	instance_data_size = sizeof(InstanceData);
	instance_data_max_count = 10000;

	instance_data_buffers.resize(max_frames_in_flight);
	for (int i = 0; i < max_frames_in_flight; ++i)
	{
		const Buffer::CreateInfo buffer_info = {
			.size = instance_data_size * instance_data_max_count,
			.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			.memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
		};

		instance_data_buffers[i] = Buffer(device, physical_device, buffer_info);
	}
}

void VulkanAPI::_destroyInstanceData()
{
	for (int i = 0; i < max_frames_in_flight; ++i)
	{
		instance_data_buffers[i].clear();
	}
}

void VulkanAPI::_updateInstancesData()
{
	{
		InstanceData * data = static_cast<InstanceData *>(instance_data_buffers[current_frame].mappedMemory());

		for (auto & [id, chunk_data]: m_chunks_in_scene)
		{
			const VkDeviceAddress block_vertex_buffer = chunk_data.block_mesh_id != 0 ? mesh_map[chunk_data.block_mesh_id].buffer_address : 0;
			const VkDeviceAddress water_vertex_buffer = chunk_data.water_mesh_id != 0 ? mesh_map[chunk_data.water_mesh_id].buffer_address : 0;
			const InstanceData instance_data = {
				.matrice = chunk_data.model,
				.block_vertex_buffer = block_vertex_buffer,
				.water_vertex_buffer = water_vertex_buffer
			};
			data[id] = instance_data;
		}
	}
}
