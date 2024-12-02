#include "RenderAPI.hpp"

void RenderAPI::_createInstanceData()
{
	instance_data_size = sizeof(InstanceData);
	instance_data_max_count = 10000;

	instance_data_buffers.resize(m_max_frames_in_flight);
	for (int i = 0; i < m_max_frames_in_flight; ++i)
	{
		const Buffer::CreateInfo buffer_info = {
			.size = instance_data_size * instance_data_max_count,
			.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			.memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
		};

		instance_data_buffers[i] = Buffer(device, physical_device, buffer_info);
	}
}

void RenderAPI::_destroyInstanceData()
{
	for (int i = 0; i < m_max_frames_in_flight; ++i)
	{
		instance_data_buffers[i].clear();
	}
}

void RenderAPI::_updateInstancesData()
{
	{
		InstanceData * data = static_cast<InstanceData *>(instance_data_buffers[m_current_frame].mappedMemory());

		for (auto & [id, chunk_data]: m_chunks_in_scene)
		{
			const InstanceData instance_data = {
				.matrice = chunk_data.model,
				.block_vertex_buffer = chunk_data.block_vertex_address,
				.water_vertex_buffer = chunk_data.water_vertex_address
			};
			data[id] = instance_data;
		}
	}
}
