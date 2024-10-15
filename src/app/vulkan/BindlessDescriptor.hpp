#pragma once

#include "vk_define.hpp"

#include <array>
#include <list>

class BindlessDescriptor
{

public:

	BindlessDescriptor();
	BindlessDescriptor(
		VkDevice device,
		uint32_t max_descriptor_count
	);
	~BindlessDescriptor();

	BindlessDescriptor(const BindlessDescriptor &) = delete;
	BindlessDescriptor& operator=(const BindlessDescriptor &) = delete;

	BindlessDescriptor(BindlessDescriptor && other) noexcept;
	BindlessDescriptor& operator=(BindlessDescriptor && other) noexcept;

	void clear();

	VkDescriptorSetLayout getSetLayout() const { return m_descriptor_set_layout; }
	VkDescriptorSet getSet() const { return m_descriptor_set; }

	uint32_t storeUniformBuffer(VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range);
	uint32_t storeStorageBuffer(VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range);
	uint32_t storeCombinedImageSampler(VkImageView imageView, VkSampler sampler);

private:

	VkDevice m_device;

	uint32_t m_max_descriptor_count;

	VkDescriptorSetLayout m_descriptor_set_layout;
	VkDescriptorPool m_descriptor_pool;
	VkDescriptorSet m_descriptor_set;

	std::array<std::list<uint32_t>, 3> m_free_descriptor_indices;

	static constexpr uint32_t m_uniform_buffer_binding = 0;
	static constexpr uint32_t m_storage_buffer_binding = 1;
	static constexpr uint32_t m_combined_image_sampler_binding = 2;

};
