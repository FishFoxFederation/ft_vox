#include "BindlessDescriptor.hpp"
#include "logger.hpp"

#include <array>
#include <stdexcept>
#include <cstring>

BindlessDescriptor::BindlessDescriptor():
	m_device(VK_NULL_HANDLE),
	m_max_frames_in_flight(0),
	m_descriptor_set_layout(VK_NULL_HANDLE),
	m_descriptor_pool(VK_NULL_HANDLE),
	m_descriptor_set(VK_NULL_HANDLE),
	m_free_descriptor_indices(4),
	m_parameter_buffer(),
	m_parameter_object_size(0),
	m_compatible_pipeline_layout(VK_NULL_HANDLE)
{
}

BindlessDescriptor::BindlessDescriptor(
	VkDevice device,
	VkPhysicalDevice physical_device,
	uint32_t max_frames_in_flight
):
	m_device(device),
	m_max_frames_in_flight(max_frames_in_flight)
{
	// create descriptor pool

	const std::vector<VkDescriptorPoolSize> pool_sizes = {
		VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1}, // for bindless params
		VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, BINDLESS_DESCRIPTOR_MAX_COUNT},
		VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, BINDLESS_DESCRIPTOR_MAX_COUNT},
		VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, BINDLESS_DESCRIPTOR_MAX_COUNT}
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

	// create descriptor set layout

	const std::vector<VkDescriptorSetLayoutBinding> bindings = {
		VkDescriptorSetLayoutBinding{BINDLESS_PARAMS_BINDING, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1, VK_SHADER_STAGE_ALL, nullptr},
		VkDescriptorSetLayoutBinding{BINDLESS_UNIFORM_BUFFER_BINDING, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, BINDLESS_DESCRIPTOR_MAX_COUNT, VK_SHADER_STAGE_ALL, nullptr},
		VkDescriptorSetLayoutBinding{BINDLESS_STORAGE_BUFFER_BINDING, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, BINDLESS_DESCRIPTOR_MAX_COUNT, VK_SHADER_STAGE_ALL, nullptr},
		VkDescriptorSetLayoutBinding{BINDLESS_COMBINED_IMAGE_SAMPLER_BINDING, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, BINDLESS_DESCRIPTOR_MAX_COUNT, VK_SHADER_STAGE_ALL, nullptr}
	};
	const std::vector<VkDescriptorBindingFlags> binding_flags = {
		0,
		VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT,
		VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT,
		VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT
	};

	VkDescriptorSetLayoutBindingFlagsCreateInfo binding_flags_info = {};
	binding_flags_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
	binding_flags_info.bindingCount = static_cast<uint32_t>(binding_flags.size());
	binding_flags_info.pBindingFlags = binding_flags.data();


	VkDescriptorSetLayoutCreateInfo layout_info = {};
	layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layout_info.bindingCount = static_cast<uint32_t>(bindings.size());
	layout_info.pBindings = bindings.data();
	layout_info.pNext = &binding_flags_info;

	VK_CHECK(
		vkCreateDescriptorSetLayout(m_device, &layout_info, nullptr, &m_descriptor_set_layout),
		"Failed to create bindless descriptor set layout"
	);

	// allocate descriptor set

	VkDescriptorSetAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	alloc_info.descriptorPool = m_descriptor_pool;

	alloc_info.descriptorSetCount = 1;
	alloc_info.pSetLayouts = &m_descriptor_set_layout;

	VK_CHECK(
		vkAllocateDescriptorSets(m_device, &alloc_info, &m_descriptor_set),
		"Failed to allocate bindless descriptor set"
	);

	// initialize free descriptor indices

	m_free_descriptor_indices.resize(4);
	for (uint32_t i = 0; i < BINDLESS_DESCRIPTOR_MAX_COUNT; ++i)
	{
		m_free_descriptor_indices[BINDLESS_UNIFORM_BUFFER_BINDING].push_back(i);
		m_free_descriptor_indices[BINDLESS_STORAGE_BUFFER_BINDING].push_back(i);
		m_free_descriptor_indices[BINDLESS_COMBINED_IMAGE_SAMPLER_BINDING].push_back(i);
	}

	// create parameter buffer

	VkPhysicalDeviceProperties properties;
	vkGetPhysicalDeviceProperties(physical_device, &properties);
	const uint32_t minUniformBufferOffsetAlignment = static_cast<uint32_t>(properties.limits.minUniformBufferOffsetAlignment);
	m_parameter_object_size = padSizeToMinAlignment(sizeof(BindlessDescriptorParams), minUniformBufferOffsetAlignment);

	Buffer::CreateInfo create_buffer_info = {};
	create_buffer_info.size = m_parameter_object_size * m_max_frames_in_flight;
	create_buffer_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	create_buffer_info.memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

	m_parameter_buffer = Buffer(m_device, physical_device, create_buffer_info);

	VkDescriptorBufferInfo buffer_info = {};
	buffer_info.buffer = m_parameter_buffer.buffer;
	buffer_info.offset = 0;
	buffer_info.range = sizeof(BindlessDescriptorParams);

	VkWriteDescriptorSet write = {};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.dstSet = m_descriptor_set;
	write.dstBinding = BINDLESS_PARAMS_BINDING;
	write.dstArrayElement = 0;
	write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	write.descriptorCount = 1;
	write.pBufferInfo = &buffer_info;

	vkUpdateDescriptorSets(m_device, 1, &write, 0, nullptr);

	// create compatible pipeline layout

	VkPipelineLayoutCreateInfo pipeline_layout_info = {};
	pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_info.setLayoutCount = 1;
	pipeline_layout_info.pSetLayouts = &m_descriptor_set_layout;
	VkPushConstantRange push_constant_range = { VK_SHADER_STAGE_ALL, 0, sizeof(ObjectData) };
	pipeline_layout_info.pushConstantRangeCount = 1;
	pipeline_layout_info.pPushConstantRanges = &push_constant_range;

	VK_CHECK(
		vkCreatePipelineLayout(m_device, &pipeline_layout_info, nullptr, &m_compatible_pipeline_layout),
		"Failed to create bindless descriptor compatible pipeline layout"
	);
}

BindlessDescriptor::~BindlessDescriptor()
{
	clear();
}

BindlessDescriptor::BindlessDescriptor(BindlessDescriptor && other) noexcept:
	m_device(other.m_device),
	m_max_frames_in_flight(other.m_max_frames_in_flight),
	m_descriptor_set_layout(other.m_descriptor_set_layout),
	m_descriptor_pool(other.m_descriptor_pool),
	m_descriptor_set(other.m_descriptor_set),
	m_free_descriptor_indices(std::move(other.m_free_descriptor_indices)),
	m_parameter_buffer(std::move(other.m_parameter_buffer)),
	m_parameter_object_size(other.m_parameter_object_size),
	m_compatible_pipeline_layout(other.m_compatible_pipeline_layout)
{
	other.m_device = VK_NULL_HANDLE;
	other.m_descriptor_set_layout = VK_NULL_HANDLE;
	other.m_descriptor_pool = VK_NULL_HANDLE;
	other.m_descriptor_set = VK_NULL_HANDLE;
	other.m_compatible_pipeline_layout = VK_NULL_HANDLE;
}

BindlessDescriptor& BindlessDescriptor::operator=(BindlessDescriptor && other) noexcept
{
	if (this != &other)
	{
		clear();

		m_device = other.m_device;
		m_max_frames_in_flight = other.m_max_frames_in_flight;
		m_descriptor_set_layout = other.m_descriptor_set_layout;
		m_descriptor_pool = other.m_descriptor_pool;
		m_descriptor_set = other.m_descriptor_set;
		m_free_descriptor_indices = std::move(other.m_free_descriptor_indices);
		m_parameter_buffer = std::move(other.m_parameter_buffer);
		m_parameter_object_size = other.m_parameter_object_size;
		m_compatible_pipeline_layout = other.m_compatible_pipeline_layout;

		other.m_device = VK_NULL_HANDLE;
		other.m_descriptor_set_layout = VK_NULL_HANDLE;
		other.m_descriptor_pool = VK_NULL_HANDLE;
		other.m_descriptor_set = VK_NULL_HANDLE;
		other.m_compatible_pipeline_layout = VK_NULL_HANDLE;
	}

	return *this;
}

void BindlessDescriptor::clear()
{
	if (m_device == VK_NULL_HANDLE)
		return;

	m_parameter_buffer.clear();

	vkDestroyPipelineLayout(m_device, m_compatible_pipeline_layout, nullptr);
	vkDestroyDescriptorSetLayout(m_device, m_descriptor_set_layout, nullptr);
	vkDestroyDescriptorPool(m_device, m_descriptor_pool, nullptr);

	m_device = VK_NULL_HANDLE;
}

uint32_t BindlessDescriptor::storeUniformBuffer(VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range, uint32_t index)
{
	uint32_t descriptor_index = checkAndGetFreeDescriptorIndex(index, BINDLESS_UNIFORM_BUFFER_BINDING);

	VkDescriptorBufferInfo buffer_info = {};
	buffer_info.buffer = buffer;
	buffer_info.offset = offset;
	buffer_info.range = range;

	VkWriteDescriptorSet write = {};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.dstSet = m_descriptor_set;
	write.dstBinding = BINDLESS_UNIFORM_BUFFER_BINDING;
	write.dstArrayElement = descriptor_index;
	write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	write.descriptorCount = 1;
	write.pBufferInfo = &buffer_info;

	vkUpdateDescriptorSets(m_device, 1, &write, 0, nullptr);

	return descriptor_index;
}

uint32_t BindlessDescriptor::storeStorageBuffer(VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range, uint32_t index)
{
	uint32_t descriptor_index = checkAndGetFreeDescriptorIndex(index, BINDLESS_STORAGE_BUFFER_BINDING);

	VkDescriptorBufferInfo buffer_info = {};
	buffer_info.buffer = buffer;
	buffer_info.offset = offset;
	buffer_info.range = range;

	VkWriteDescriptorSet write = {};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.dstSet = m_descriptor_set;
	write.dstBinding = BINDLESS_STORAGE_BUFFER_BINDING;
	write.dstArrayElement = descriptor_index;
	write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	write.descriptorCount = 1;
	write.pBufferInfo = &buffer_info;

	vkUpdateDescriptorSets(m_device, 1, &write, 0, nullptr);

	return descriptor_index;
}

uint32_t BindlessDescriptor::storeCombinedImageSampler(VkImageView imageView, VkSampler sampler, uint32_t index)
{
	uint32_t descriptor_index = checkAndGetFreeDescriptorIndex(index, BINDLESS_COMBINED_IMAGE_SAMPLER_BINDING);

	VkDescriptorImageInfo image_info = {};
	image_info.imageView = imageView;
	image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	image_info.sampler = sampler;

	VkWriteDescriptorSet write = {};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.dstSet = m_descriptor_set;
	write.dstBinding = BINDLESS_COMBINED_IMAGE_SAMPLER_BINDING;
	write.dstArrayElement = descriptor_index;
	write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	write.descriptorCount = 1;
	write.pImageInfo = &image_info;

	vkUpdateDescriptorSets(m_device, 1, &write, 0, nullptr);

	return descriptor_index;
}

void BindlessDescriptor::setParams(BindlessDescriptorParams params, uint32_t current_frame)
{
	// void *data;
	// VK_CHECK(
	// 	vkMapMemory(m_device, m_parameter_buffer.memory, getParamsOffset(current_frame), sizeof(BindlessDescriptorParams), 0, &data),
	// 	"Failed to map bindless descriptor parameter buffer"
	// );
	// memcpy(data, &params, sizeof(BindlessDescriptorParams));
	// vkUnmapMemory(m_device, m_parameter_buffer.memory);

	memcpy(
		static_cast<uint8_t*>(m_parameter_buffer.mappedMemory()) + getParamsOffset(current_frame),
		&params,
		sizeof(BindlessDescriptorParams)
	);
}

uint32_t BindlessDescriptor::getParamsOffset(uint32_t current_frame) const
{
	return m_parameter_object_size * current_frame;
}

uint32_t BindlessDescriptor::checkAndGetFreeDescriptorIndex(uint32_t index, uint32_t descriptor_binding)
{
	if (index != invalid_descriptor_index)
	{
		if (index >= BINDLESS_DESCRIPTOR_MAX_COUNT)
		{
			throw std::runtime_error("Failed to store descriptor: invalid index >= max descriptor count");
		}

		m_free_descriptor_indices[descriptor_binding].remove(index);
	}
	else
	{
		if (m_free_descriptor_indices[descriptor_binding].empty())
		{
			throw std::runtime_error("Failed to store descriptor: max descriptor count reached");
		}

		index = m_free_descriptor_indices[descriptor_binding].front();
		m_free_descriptor_indices[descriptor_binding].pop_front();
	}

	return index;
}
