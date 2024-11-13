#include "VulkanAPI.hpp"

void VulkanAPI::_createInstanceData()
{
	instance_data_size = sizeof(GlobalPushConstant);
	instance_data_max_count = 1000;

	instance_id_to_instance_data_offset.resize(instance_data_max_count);

	instance_id_to_instance_data_offset_buffers.resize(max_frames_in_flight);
	for (int i = 0; i < max_frames_in_flight; ++i)
	{
		const Buffer::CreateInfo buffer_info = {
			.size = sizeof(VkDeviceAddress) * instance_data_max_count,
			.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			.memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
		};

		instance_id_to_instance_data_offset_buffers[i] = Buffer(device, physical_device, buffer_info);
	}

	instance_data_buffers.resize(max_frames_in_flight);
	for (int i = 0; i < max_frames_in_flight; ++i)
	{
		const Buffer::CreateInfo buffer_info = {
			.size = instance_data_size * instance_data_max_count,
			.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			.memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
		};

		instance_data_buffers[i] = Buffer(device, physical_device, buffer_info);
	}

	free_instance_data_ranges.push_back({
		.offset = 0,
		.size = instance_data_size * instance_data_max_count
	});
}

void VulkanAPI::_destroyInstanceData()
{
	for (int i = 0; i < max_frames_in_flight; ++i)
	{
		instance_id_to_instance_data_offset_buffers[i].clear();
		instance_data_buffers[i].clear();
	}
}

VkDeviceSize VulkanAPI::_reserveInstanceDataRange(const VkDeviceSize size)
{
	for (auto free_range = free_instance_data_ranges.begin(); free_range != free_instance_data_ranges.end(); ++free_range)
	{
		if (free_range->size < size) continue;

		const BufferRange used_range = { free_range->offset, size };
		used_instance_data_ranges[free_range->offset] = size;

		free_range->offset += size;
		free_range->size -= size;

		if (free_range->size == 0)
		{
			free_instance_data_ranges.erase(free_range);
		}

		return used_range.offset;
	}

	throw std::runtime_error("No free push constant range available");
}

void VulkanAPI::_releaseInstanceDataRange(const VkDeviceSize offset)
{
	const VkDeviceSize size = used_instance_data_ranges[offset];
	used_instance_data_ranges.erase(offset);

	const BufferRange free_range = { offset, size };
	for (auto it = free_instance_data_ranges.begin(); it != free_instance_data_ranges.end(); ++it)
	{
		if (it->offset < offset)
		{
			if (it->offset + it->size == offset)
			{
				it->size += size;
			}
			else
			{
				free_instance_data_ranges.insert(it, free_range);
			}
			return;
		}
	}

	free_instance_data_ranges.push_back(free_range);
}

void VulkanAPI::_writeInstanceData(
	const VkDeviceSize offset,
	const void * data,
	const VkDeviceSize size
)
{
	for (int i = 0; i < max_frames_in_flight; i++)
	{
		void * mapped_memory = instance_data_buffers[i].mappedMemory();
		std::memcpy(static_cast<uint8_t *>(mapped_memory) + offset, data, size);
	}
}
