#include "Descriptor.hpp"

Descriptor::Descriptor():
	layout(VK_NULL_HANDLE),
	pool(VK_NULL_HANDLE),
	set(VK_NULL_HANDLE),
	m_device(VK_NULL_HANDLE)
{
}

Descriptor::Descriptor(VkDevice device, const CreateInfo & info)
	: m_device(device)
{
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = info.bindings.size();
	layoutInfo.pBindings = info.bindings.data();

	if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &layout) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create descriptor set layout!");
	}

	std::vector<VkDescriptorPoolSize> poolSizes = {};
	for (const auto & binding : info.bindings)
	{
		poolSizes.push_back({ binding.descriptorType, info.descriptor_count });
	}

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = poolSizes.size();
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = info.set_count;

	if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &pool) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create descriptor pool!");
	}

	std::vector<VkDescriptorSetLayout> layouts(info.set_count, layout);

	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = pool;
	allocInfo.descriptorSetCount = info.set_count;
	allocInfo.pSetLayouts = layouts.data();

	sets.resize(info.set_count);

	if (vkAllocateDescriptorSets(device, &allocInfo, sets.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate descriptor set!");
	}

	set = sets[0];
}

Descriptor::Descriptor(Descriptor && other) noexcept:
	layout(other.layout),
	pool(other.pool),
	sets(std::move(other.sets)),
	set(other.set),
	m_device(other.m_device)
{
	other.layout = VK_NULL_HANDLE;
	other.pool = VK_NULL_HANDLE;
	other.set = VK_NULL_HANDLE;
}

Descriptor & Descriptor::operator=(Descriptor && other) noexcept
{
	if (this != &other)
	{
		clear();

		layout = other.layout;
		pool = other.pool;
		sets = std::move(other.sets);
		set = other.set;
		m_device = other.m_device;

		other.layout = VK_NULL_HANDLE;
		other.pool = VK_NULL_HANDLE;
		other.set = VK_NULL_HANDLE;
	}

	return *this;
}

Descriptor::~Descriptor()
{
	clear();
}

void Descriptor::clear()
{
	if (set != VK_NULL_HANDLE)
	{
		vkFreeDescriptorSets(m_device, pool, 1, &set);
		set = VK_NULL_HANDLE;
	}

	if (pool != VK_NULL_HANDLE)
	{
		vkDestroyDescriptorPool(m_device, pool, nullptr);
		pool = VK_NULL_HANDLE;
	}

	if (layout != VK_NULL_HANDLE)
	{
		vkDestroyDescriptorSetLayout(m_device, layout, nullptr);
		layout = VK_NULL_HANDLE;
	}
}
