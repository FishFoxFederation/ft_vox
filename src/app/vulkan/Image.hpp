#pragma once

#include "vk_define.hpp"
#include "Command.hpp"

#include <string>
#include <vector>

class Image
{

public:

	struct CreateInfo
	{
		VkExtent2D extent;
		VkImageAspectFlags aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT;
		uint32_t mip_levels = 1;
		uint32_t array_layers = 1;

		VkFormat format;
		VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;

		VkImageUsageFlags usage;
		VkMemoryPropertyFlags memory_properties;

		std::vector<std::string> file_paths = {};

		VkImageLayout final_layout;

		bool create_view = false;

		bool create_sampler = false;
		VkFilter sampler_filter = VK_FILTER_LINEAR;
		VkSamplerAddressMode sampler_address_mode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		VkBool32 sampler_anisotropy_enable = VK_TRUE;

		bool is_cube_map = false;
	};

	Image();

	Image(const Image &) = delete;
	Image & operator=(const Image &) = delete;

	Image(Image && other) noexcept;
	Image & operator=(Image && other) noexcept;

	~Image();

	Image(
		VkDevice device,
		VkPhysicalDevice physical_device,
		SingleTimeCommand & command_buffer,
		const CreateInfo & create_info
	);

	void transitionLayout(
		VkCommandBuffer command_buffer,
		VkImageLayout old_layout,
		VkImageLayout new_layout,
		VkPipelineStageFlags src_stage = VK_PIPELINE_STAGE_NONE,
		VkPipelineStageFlags dst_stage = VK_PIPELINE_STAGE_NONE
	);

	void clear();

	VkImage image;
	VkDeviceMemory memory;
	VkImageView view;
	VkSampler sampler;

	VkFormat format;
	VkExtent2D extent2D;
	VkExtent3D extent3D;
	VkImageAspectFlags aspect_mask;
	uint32_t mip_levels;
	uint32_t array_layers;

private:

	VkDevice m_device;
	VkPhysicalDevice m_physical_device;

};
