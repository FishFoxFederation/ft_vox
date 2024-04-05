#pragma once

#include "vk_define.hpp"

#include <vector>

class Swapchain
{

public:

	struct SupportDetails
	{
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> present_modes;
	};

	struct CreateInfo
	{
		VkSurfaceFormatKHR surface_format;
		VkPresentModeKHR present_mode;
		VkExtent2D extent;
	};

	Swapchain();
	Swapchain(
		VkDevice device,
		VkPhysicalDevice physical_device,
		VkSurfaceKHR surface,
		CreateInfo create_info
	);

	Swapchain(const Swapchain &) = delete;
	Swapchain & operator=(const Swapchain &) = delete;

	Swapchain(Swapchain && other) noexcept;
	Swapchain & operator=(Swapchain && other) noexcept;

	~Swapchain();

	static SupportDetails querySwapChainSupport(
		VkPhysicalDevice physical_device,
		VkSurfaceKHR surface
	);

	VkSwapchainKHR swapchain;
	VkExtent2D extent;
	VkFormat image_format;
	std::vector<VkImage> images;
	std::vector<VkImageView> image_views;

private:

	VkDevice m_device;

	void clear();

};