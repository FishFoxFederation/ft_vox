#pragma once

#include "vk_define.hpp"
#include "ShaderCommon.hpp"
#include "Buffer.hpp"

#include <vector>
#include <list>

class BindlessDescriptor
{

public:

	BindlessDescriptor();
	BindlessDescriptor(
		VkDevice device,
		VkPhysicalDevice physical_device,
		uint32_t max_frames_in_flight
	);
	~BindlessDescriptor();

	BindlessDescriptor(const BindlessDescriptor &) = delete;
	BindlessDescriptor& operator=(const BindlessDescriptor &) = delete;

	BindlessDescriptor(BindlessDescriptor && other) noexcept;
	BindlessDescriptor& operator=(BindlessDescriptor && other) noexcept;

	void clear();

	VkDescriptorSetLayout layout() const { return m_descriptor_set_layout; }
	VkDescriptorSet set() const { return m_descriptor_set; }

	uint32_t storeUniformBuffer(VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range, uint32_t index = invalid_descriptor_index);
	uint32_t storeStorageBuffer(VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range, uint32_t index = invalid_descriptor_index);
	uint32_t storeCombinedImageSampler(VkImageView imageView, VkSampler sampler, uint32_t index = invalid_descriptor_index);

	void setParams(BindlessDescriptorParams params, uint32_t current_frame);
	uint32_t getParamsOffset(uint32_t current_frame) const;

private:

	static const uint32_t invalid_descriptor_index = std::numeric_limits<uint32_t>::max();

	VkDevice m_device;

	uint32_t m_max_frames_in_flight;

	VkDescriptorSetLayout m_descriptor_set_layout;
	VkDescriptorPool m_descriptor_pool;
	VkDescriptorSet m_descriptor_set;

	std::vector<std::list<uint32_t>> m_free_descriptor_indices;

	Buffer m_parameter_buffer;
	VkDeviceSize m_parameter_object_size;

	uint32_t padSizeToMinAlignment(uint32_t originalSize, uint32_t minAlignment)
	{
		return (originalSize + minAlignment - 1) & ~(minAlignment - 1);
	}

	uint32_t checkAndGetFreeDescriptorIndex(uint32_t index, uint32_t descriptor_binding);
};
