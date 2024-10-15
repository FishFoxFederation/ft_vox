#include "BindlessDescriptor.hpp"

#include <array>

BindlessDescriptor::BindlessDescriptor():
	m_device(VK_NULL_HANDLE),
	m_max_descriptor_count(0),
	m_descriptor_set_layout(VK_NULL_HANDLE),
	m_descriptor_pool(VK_NULL_HANDLE),
	m_descriptor_set(VK_NULL_HANDLE)
{
}

BindlessDescriptor::BindlessDescriptor(
	VkDevice device,
	uint32_t max_descriptor_count
):
	m_device(device),
	m_max_descriptor_count(max_descriptor_count)
{
	const std::array<VkDescriptorPoolSize, 3> pool_sizes = {
		VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, m_max_descriptor_count},
		VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, m_max_descriptor_count},
		VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, m_max_descriptor_count}
	};

	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
	pool_info.pPoolSizes = pool_sizes.data();
	pool_info.maxSets = 1;
	// pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT_EXT;

	VK_CHECK(
		vkCreateDescriptorPool(m_device, &pool_info, nullptr, &m_descriptor_pool),
		"Failed to create bindless descriptor pool"
	);

	const std::array<VkDescriptorSetLayoutBinding, 3> bindings = {
		VkDescriptorSetLayoutBinding{m_uniform_buffer_binding, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_ALL, nullptr},
		VkDescriptorSetLayoutBinding{m_storage_buffer_binding, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_ALL, nullptr},
		VkDescriptorSetLayoutBinding{m_combined_image_sampler_binding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_ALL, nullptr}
	};

	VkDescriptorSetLayoutCreateInfo layout_info = {};
	layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layout_info.bindingCount = static_cast<uint32_t>(bindings.size());
	layout_info.pBindings = bindings.data();

	VK_CHECK(
		vkCreateDescriptorSetLayout(m_device, &layout_info, nullptr, &m_descriptor_set_layout),
		"Failed to create bindless descriptor set layout"
	);

	VkDescriptorSetAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	alloc_info.descriptorPool = m_descriptor_pool;

	alloc_info.descriptorSetCount = 1;
	alloc_info.pSetLayouts = &m_descriptor_set_layout;

	VK_CHECK(
		vkAllocateDescriptorSets(m_device, &alloc_info, &m_descriptor_set),
		"Failed to allocate bindless descriptor set"
	);

	for (uint32_t i = 0; i < m_max_descriptor_count; ++i)
	{
		m_free_descriptor_indices[m_uniform_buffer_binding].push_back(i);
		m_free_descriptor_indices[m_storage_buffer_binding].push_back(i);
		m_free_descriptor_indices[m_combined_image_sampler_binding].push_back(i);
	}
}

BindlessDescriptor::~BindlessDescriptor()
{
	clear();
}

BindlessDescriptor::BindlessDescriptor(BindlessDescriptor && other) noexcept:
	m_device(other.m_device),
	m_max_descriptor_count(other.m_max_descriptor_count),
	m_descriptor_set_layout(other.m_descriptor_set_layout),
	m_descriptor_pool(other.m_descriptor_pool),
	m_descriptor_set(other.m_descriptor_set),
	m_free_descriptor_indices(std::move(other.m_free_descriptor_indices))
{
	other.m_device = VK_NULL_HANDLE;
	other.m_descriptor_set_layout = VK_NULL_HANDLE;
	other.m_descriptor_pool = VK_NULL_HANDLE;
	other.m_descriptor_set = VK_NULL_HANDLE;
}

BindlessDescriptor& BindlessDescriptor::operator=(BindlessDescriptor && other) noexcept
{
	if (this != &other)
	{
		clear();

		m_device = other.m_device;
		m_max_descriptor_count = other.m_max_descriptor_count;
		m_descriptor_set_layout = other.m_descriptor_set_layout;
		m_descriptor_pool = other.m_descriptor_pool;
		m_descriptor_set = other.m_descriptor_set;
		m_free_descriptor_indices = std::move(other.m_free_descriptor_indices);

		other.m_device = VK_NULL_HANDLE;
		other.m_descriptor_set_layout = VK_NULL_HANDLE;
		other.m_descriptor_pool = VK_NULL_HANDLE;
		other.m_descriptor_set = VK_NULL_HANDLE;
	}

	return *this;
}

void BindlessDescriptor::clear()
{
	if (m_device == VK_NULL_HANDLE)
		return;

	vkDestroyDescriptorSetLayout(m_device, m_descriptor_set_layout, nullptr);
	vkDestroyDescriptorPool(m_device, m_descriptor_pool, nullptr);

	m_device = VK_NULL_HANDLE;
}

uint32_t BindlessDescriptor::storeUniformBuffer(VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range)
{
	if (m_free_descriptor_indices[m_uniform_buffer_binding].empty())
	{
		throw std::runtime_error("Failed to store uniform buffer: max descriptor count reached");
	}

	uint32_t descriptor_index = m_free_descriptor_indices[m_uniform_buffer_binding].front();
	m_free_descriptor_indices[m_uniform_buffer_binding].pop_front();

	VkDescriptorBufferInfo buffer_info = {};
	buffer_info.buffer = buffer;
	buffer_info.offset = offset;
	buffer_info.range = range;

	VkWriteDescriptorSet write = {};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.dstSet = m_descriptor_set;
	write.dstBinding = m_uniform_buffer_binding;
	write.dstArrayElement = descriptor_index;
	write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	write.descriptorCount = 1;
	write.pBufferInfo = &buffer_info;

	vkUpdateDescriptorSets(m_device, 1, &write, 0, nullptr);

	return descriptor_index;
}

uint32_t BindlessDescriptor::storeStorageBuffer(VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range)
{
	if (m_free_descriptor_indices[m_storage_buffer_binding].empty())
	{
		throw std::runtime_error("Failed to store storage buffer: max descriptor count reached");
	}

	uint32_t descriptor_index = m_free_descriptor_indices[m_storage_buffer_binding].front();
	m_free_descriptor_indices[m_storage_buffer_binding].pop_front();

	VkDescriptorBufferInfo buffer_info = {};
	buffer_info.buffer = buffer;
	buffer_info.offset = offset;
	buffer_info.range = range;

	VkWriteDescriptorSet write = {};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.dstSet = m_descriptor_set;
	write.dstBinding = m_storage_buffer_binding;
	write.dstArrayElement = descriptor_index;
	write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	write.descriptorCount = 1;
	write.pBufferInfo = &buffer_info;

	vkUpdateDescriptorSets(m_device, 1, &write, 0, nullptr);

	return descriptor_index;
}

uint32_t BindlessDescriptor::storeCombinedImageSampler(VkImageView imageView, VkSampler sampler)
{
	if (m_free_descriptor_indices[m_combined_image_sampler_binding].empty())
	{
		throw std::runtime_error("Failed to store combined image sampler: max descriptor count reached");
	}

	uint32_t descriptor_index = m_free_descriptor_indices[m_combined_image_sampler_binding].front();
	m_free_descriptor_indices[m_combined_image_sampler_binding].pop_front();

	VkDescriptorImageInfo image_info = {};
	image_info.imageView = imageView;
	image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	image_info.sampler = sampler;

	VkWriteDescriptorSet write = {};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.dstSet = m_descriptor_set;
	write.dstBinding = m_combined_image_sampler_binding;
	write.dstArrayElement = descriptor_index;
	write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	write.descriptorCount = 1;
	write.pImageInfo = &image_info;

	vkUpdateDescriptorSets(m_device, 1, &write, 0, nullptr);

	return descriptor_index;
}
