#include "Swapchain.hpp"

Swapchain::Swapchain():
	swapchain(VK_NULL_HANDLE),
	extent({0, 0}),
	image_format(VK_FORMAT_UNDEFINED),
	m_device(VK_NULL_HANDLE)
{
}

Swapchain::Swapchain(
	VkDevice device,
	VkPhysicalDevice physical_device,
	VkSurfaceKHR surface,
	CreateInfo create_info
):
	m_device(device)
{
	SupportDetails swapchain_support = querySwapChainSupport(physical_device, surface);

	uint32_t image_count = swapchain_support.capabilities.minImageCount + 1;
	if (swapchain_support.capabilities.maxImageCount > 0 && image_count > swapchain_support.capabilities.maxImageCount)
	{
		image_count = swapchain_support.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR swapchain_info = {};
	swapchain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchain_info.surface = surface;
	swapchain_info.minImageCount = image_count;
	swapchain_info.imageFormat = create_info.surface_format.format;
	swapchain_info.imageColorSpace = create_info.surface_format.colorSpace;
	swapchain_info.imageExtent = create_info.extent;
	swapchain_info.imageArrayLayers = 1;
	swapchain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

	uint32_t queue_family_indices[] = { 0, 1 };
	if (queue_family_indices[0] != queue_family_indices[1])
	{
		swapchain_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapchain_info.queueFamilyIndexCount = 2;
		swapchain_info.pQueueFamilyIndices = queue_family_indices;
	}
	else
	{
		swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapchain_info.queueFamilyIndexCount = 0;
		swapchain_info.pQueueFamilyIndices = nullptr;
	}

	swapchain_info.preTransform = swapchain_support.capabilities.currentTransform;
	swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchain_info.presentMode = create_info.present_mode;
	swapchain_info.clipped = VK_TRUE;
	swapchain_info.oldSwapchain = create_info.old_swapchain;

	VK_CHECK(
		vkCreateSwapchainKHR(device, &swapchain_info, nullptr, &swapchain),
		"Failed to create swapchain"
	);

	vkGetSwapchainImagesKHR(device, swapchain, &image_count, nullptr);
	images.resize(image_count);
	vkGetSwapchainImagesKHR(device, swapchain, &image_count, images.data());

	image_format = create_info.surface_format.format;
	extent = create_info.extent;

	image_views.resize(images.size());

	for (size_t i = 0; i < images.size(); i++)
	{
		VkImageViewCreateInfo image_view_info = {};
		image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		image_view_info.image = images[i];
		image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		image_view_info.format = image_format;
		image_view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		image_view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		image_view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		image_view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		image_view_info.subresourceRange.baseMipLevel = 0;
		image_view_info.subresourceRange.levelCount = 1;
		image_view_info.subresourceRange.baseArrayLayer = 0;
		image_view_info.subresourceRange.layerCount = 1;

		if (vkCreateImageView(device, &image_view_info, nullptr, &image_views[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create image views!");
		}
	}
}

Swapchain::Swapchain(Swapchain && other) noexcept:
	swapchain(other.swapchain),
	extent(other.extent),
	image_format(other.image_format),
	images(std::move(other.images)),
	image_views(std::move(other.image_views)),
	m_device(other.m_device)
{
	other.swapchain = VK_NULL_HANDLE;
	other.extent = {};
	other.image_format = VK_FORMAT_UNDEFINED;
	other.images.clear();
	other.image_views.clear();
	other.m_device = VK_NULL_HANDLE;
}

Swapchain & Swapchain::operator=(Swapchain && other) noexcept
{
	if (this != &other)
	{
		clear();

		swapchain = other.swapchain;
		extent = other.extent;
		image_format = other.image_format;
		images = std::move(other.images);
		image_views = std::move(other.image_views);
		m_device = other.m_device;

		other.swapchain = VK_NULL_HANDLE;
		other.extent = {};
		other.image_format = VK_FORMAT_UNDEFINED;
		other.images.clear();
		other.image_views.clear();
		other.m_device = VK_NULL_HANDLE;
	}

	return *this;
}

Swapchain::~Swapchain()
{
	clear();
}

void Swapchain::clear()
{
	if (m_device != VK_NULL_HANDLE)
	{
		for (VkImageView image_view : image_views)
		{
			vkDestroyImageView(m_device, image_view, nullptr);
			image_view = VK_NULL_HANDLE;
		}

		if (swapchain != VK_NULL_HANDLE)
		{
			vkDestroySwapchainKHR(m_device, swapchain, nullptr);
			swapchain = VK_NULL_HANDLE;
		}
		m_device = VK_NULL_HANDLE;
	}
}

Swapchain::SupportDetails Swapchain::querySwapChainSupport(
	VkPhysicalDevice physical_device,
	VkSurfaceKHR surface
)
{
	SupportDetails details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &details.capabilities);

	uint32_t format_count;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, nullptr);

	if (format_count != 0)
	{
		details.formats.resize(format_count);
		vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, details.formats.data());
	}

	uint32_t present_mode_count;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count, nullptr);

	if (present_mode_count != 0)
	{
		details.present_modes.resize(present_mode_count);
		vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count, details.present_modes.data());
	}

	return details;
}