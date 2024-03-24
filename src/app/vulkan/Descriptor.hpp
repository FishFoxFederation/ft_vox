#pragma once

#include "vk_define.hpp"

#include <vector>

class Descriptor
{

public:

	struct CreateInfo
	{
		std::vector<VkDescriptorSetLayoutBinding> bindings;
		uint32_t descriptor_count;
		uint32_t set_count = 1;
	};

	Descriptor();
	Descriptor(VkDevice device, const CreateInfo & info);

	Descriptor(const Descriptor &) = delete;
	Descriptor & operator=(const Descriptor &) = delete;

	Descriptor(Descriptor && other) noexcept;
	Descriptor & operator=(Descriptor && other) noexcept;

	~Descriptor();

	VkDescriptorSetLayout layout;
	VkDescriptorPool pool;
	std::vector<VkDescriptorSet> sets;
	VkDescriptorSet set;

private:

	VkDevice m_device;

	void clear();

};