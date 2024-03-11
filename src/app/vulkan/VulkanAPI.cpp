#include "VulkanAPI.hpp"
#include "logger.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <cstring>
#include <set>
#include <algorithm>
#include <map>

VulkanAPI::VulkanAPI(GLFWwindow * window):
	window(window)
{
	createInstance();
	setupDebugMessenger();
	createSurface(window);
	pickPhysicalDevice();
	createLogicalDevice();
	createSwapChain(window);
	createCommandPool();
	createCommandBuffer();
	createSyncObjects();

	createColorResources();
	createDepthResources();
	createUniformBuffers();
	createTextureArray({
		"assets/textures/stone.jpg",
		"assets/textures/stone.jpg"
	}, 64);
	createCubeMap({
		"assets/textures/grass.jpg",
		"assets/textures/grass.jpg",
		"assets/textures/grass.jpg",
		"assets/textures/grass.jpg",
		"assets/textures/grass.jpg",
		"assets/textures/grass.jpg"
	}, 512);
	createFrustumLineBuffers();
	
	createCameraDescriptors();
	createTextureArrayDescriptors();
	createPipeline();
	createLinePipeline();

	setupImgui();
	createImGuiTexture(100, 100);

	LOG_INFO("VulkanAPI initialized");
}

VulkanAPI::~VulkanAPI()
{
	vkDeviceWaitIdle(device);

	destroyImGuiTexture(imgui_texture);

	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	for (auto & [key, mesh] : meshes)
	{
		vkDestroyBuffer(device, mesh.buffer, nullptr);
		vma.freeMemory(device, mesh.buffer_memory, nullptr);
	}

	vkDestroyImage(device, textures_image, nullptr);
	vma.freeMemory(device, textures_image_memory, nullptr);
	vkDestroyImageView(device, textures_image_view, nullptr);
	vkDestroySampler(device, textures_sampler, nullptr);

	vkDestroyImage(device, cube_map_image, nullptr);
	vma.freeMemory(device, cube_map_image_memory, nullptr);
	vkDestroyImageView(device, cube_map_image_view, nullptr);
	vkDestroySampler(device, cube_map_sampler, nullptr);

	for (int i = 0; i < max_frames_in_flight; i++)
	{
		vkUnmapMemory(device, camera_uniform_buffers_memory[i]);
		vma.freeMemory(device, camera_uniform_buffers_memory[i], nullptr);
		vkDestroyBuffer(device, camera_uniform_buffers[i], nullptr);

		vkUnmapMemory(device, frustum_line_buffers_memory[i]);
		vma.freeMemory(device, frustum_line_buffers_memory[i], nullptr);
		vkDestroyBuffer(device, frustum_line_buffers[i], nullptr);
	}

	vkDestroyDescriptorPool(device, imgui_descriptor_pool, nullptr);

	vkDestroyDescriptorSetLayout(device, camera_descriptor_set_layout, nullptr);
	vkDestroyDescriptorPool(device, camera_descriptor_pool, nullptr);

	vkDestroyDescriptorSetLayout(device, texture_array_descriptor_set_layout, nullptr);
	vkDestroyDescriptorPool(device, texture_array_descriptor_pool, nullptr);

	for (int i = 0; i < max_frames_in_flight; i++)
	{
		vkDestroySemaphore(device, image_available_semaphores[i], nullptr);
		vkDestroySemaphore(device, render_finished_semaphores[i], nullptr);
		vkDestroySemaphore(device, swap_chain_updated_semaphores[i], nullptr);
		vkDestroySemaphore(device, imgui_render_finished_semaphores[i], nullptr);
		vkDestroyFence(device, in_flight_fences[i], nullptr);
	}
	vkDestroyFence(device, single_time_command_fence, nullptr);

	vkDestroyImageView(device, color_attachement_view, nullptr);
	vma.freeMemory(device, color_attachement_memory, nullptr);
	vkDestroyImage(device, color_attachement_image, nullptr);

	vkDestroyImageView(device, depth_attachement_view, nullptr);
	vma.freeMemory(device, depth_attachement_memory, nullptr);
	vkDestroyImage(device, depth_attachement_image, nullptr);

	vkFreeCommandBuffers(device, command_pool, static_cast<uint32_t>(render_command_buffers.size()), render_command_buffers.data());
	vkFreeCommandBuffers(device, command_pool, static_cast<uint32_t>(copy_command_buffers.size()), copy_command_buffers.data());
	vkDestroyCommandPool(device, command_pool, nullptr);

	vkDestroyPipeline(device, graphics_pipeline, nullptr);
	vkDestroyPipelineLayout(device, pipeline_layout, nullptr);

	vkDestroyPipeline(device, line_graphics_pipeline, nullptr);
	vkDestroyPipelineLayout(device, line_pipeline_layout, nullptr);

	for (size_t i = 0; i < swap_chain_image_views.size(); i++)
	{
		vkDestroyImageView(device, swap_chain_image_views[i], nullptr);
	}
	vkDestroySwapchainKHR(device, swap_chain, nullptr);

	vkDestroyDevice(device, nullptr);

	#ifndef NDEBUG
		DestroyDebugUtilsMessengerEXT(instance, debug_messenger, nullptr);
	#endif

	vkDestroySurfaceKHR(instance, surface, nullptr);

	vkDestroyInstance(instance, nullptr);
}

void VulkanAPI::createInstance()
{
	#ifndef NDEBUG
		if (!checkValidationLayerSupport())
		{
			throw std::runtime_error("Validation layers requested, but not available!");
		}
	#endif

	VkApplicationInfo app_info = {};
	app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pApplicationName = "Vulkan Tutorial";
	app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.pEngineName = "No Engine";
	app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.apiVersion = VK_API_VERSION_1_3;

	VkInstanceCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	create_info.pApplicationInfo = &app_info;

	std::vector<const char *> extensions = getRequiredExtensions();

	create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	create_info.ppEnabledExtensionNames = extensions.data();

	VkDebugUtilsMessengerCreateInfoEXT debug_create_info;
	populateDebugMessengerCreateInfo(debug_create_info);
	#ifndef NDEBUG
		create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
		create_info.ppEnabledLayerNames = validation_layers.data();

		create_info.pNext = &debug_create_info;
	#else
		create_info.enabledLayerCount = 0;
	#endif

	VK_CHECK(
		vkCreateInstance(&create_info, nullptr, &instance),
		"Failed to create instance"
	);
}

bool VulkanAPI::checkValidationLayerSupport()
{
	uint32_t layer_count;
	vkEnumerateInstanceLayerProperties(&layer_count, nullptr);
	std::vector<VkLayerProperties> available_layers(layer_count);
	vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

	for (const char * layer_name : validation_layers)
	{
		bool layer_found = false;

		for (const auto & layer_properties : available_layers)
		{
			if (strcmp(layer_name, layer_properties.layerName) == 0)
			{
				layer_found = true;
				break;
			}
		}

		if (!layer_found)
		{
			return false;
		}
	}

	return true;
}

std::vector<const char *> VulkanAPI::getRequiredExtensions()
{
	uint32_t glfw_extension_count = 0;
	const char** glfw_extensions;
	glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

	std::vector<const char *> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);

	#ifndef NDEBUG
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	#endif

	return extensions;
}

void VulkanAPI::setupDebugMessenger()
{
	#ifndef NDEBUG
		VkDebugUtilsMessengerCreateInfoEXT create_info;
		populateDebugMessengerCreateInfo(create_info);

		VK_CHECK(
			CreateDebugUtilsMessengerEXT(instance, &create_info, nullptr, &debug_messenger),
			"Failed to set up debug messenger"
		);
	#endif
}

void VulkanAPI::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT & create_info)
{
	create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	create_info.pfnUserCallback = debugCallback;
}

VkResult VulkanAPI::CreateDebugUtilsMessengerEXT(
	VkInstance instance,
	const VkDebugUtilsMessengerCreateInfoEXT * create_info,
	const VkAllocationCallbacks * allocator,
	VkDebugUtilsMessengerEXT * debug_messenger
)
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		return func(instance, create_info, allocator, debug_messenger);
	}
	else
	{
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void VulkanAPI::DestroyDebugUtilsMessengerEXT(
	VkInstance instance,
	VkDebugUtilsMessengerEXT debug_messenger,
	const VkAllocationCallbacks * allocator
)
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		func(instance, debug_messenger, allocator);
	}
}

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanAPI::debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
	VkDebugUtilsMessageTypeFlagsEXT message_type,
	const VkDebugUtilsMessengerCallbackDataEXT * callback_data,
	void *
)
{
	(void)message_type;

	// if (message_severity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
	// {
	// 	LOG_TRACE(callback_data->pMessage);
	// }
	// if (message_severity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
	// {
	// 	LOG_INFO(callback_data->pMessage);
	// }
	if (message_severity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
	{
		LOG_WARNING(callback_data->pMessage);
	}
	if (message_severity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
	{
		LOG_ERROR(callback_data->pMessage);
	}

	return VK_FALSE;
}

void VulkanAPI::createSurface(GLFWwindow * window)
{
	if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create window surface");
	}
}

void VulkanAPI::pickPhysicalDevice()
{
	uint32_t device_count = 0;
	vkEnumeratePhysicalDevices(instance, &device_count, nullptr);

	if (device_count == 0)
	{
		throw std::runtime_error("Failed to find GPUs with Vulkan support");
	}

	std::vector<VkPhysicalDevice> devices(device_count);
	vkEnumeratePhysicalDevices(instance, &device_count, devices.data());

	std::multimap<int, VkPhysicalDevice, std::greater<int>> candidates;

	for (const auto & device : devices)
	{
		int score = ratePhysicalDevice(device);
		candidates.insert(std::make_pair(score, device));
	}

	if (candidates.begin()->first > 0)
	{
		physical_device = candidates.begin()->second;
	}
	else
	{
		throw std::runtime_error("Failed to find a suitable GPU");
	}

	queue_family_indices = findQueueFamilies(physical_device);

	VkPhysicalDeviceProperties device_properties;
	vkGetPhysicalDeviceProperties(physical_device, &device_properties);

	LOG_INFO("device name: " << device_properties.deviceName);
}

bool VulkanAPI::isDeviceSuitable(VkPhysicalDevice device)
{
	QueueFamilyIndices indices = findQueueFamilies(device);

	bool extensions_supported = checkDeviceExtensionSupport(device);

	bool swap_chain_adequate = false;
	if (extensions_supported)
	{
		SwapChainSupportDetails swap_chain_support = querySwapChainSupport(device);
		swap_chain_adequate = !swap_chain_support.formats.empty() && !swap_chain_support.present_modes.empty();
	}

	VkPhysicalDeviceFeatures supported_features;
	vkGetPhysicalDeviceFeatures(device, &supported_features);

	return indices.isComplete()
		&& extensions_supported
		&& swap_chain_adequate
		&& supported_features.samplerAnisotropy;
}

int VulkanAPI::ratePhysicalDevice(VkPhysicalDevice device)
{
	if (!isDeviceSuitable(device))
	{
		return 0;
	}

	int score = 0;

	VkPhysicalDeviceProperties device_properties;
	vkGetPhysicalDeviceProperties(device, &device_properties);

	// VkPhysicalDeviceFeatures device_features;
	// vkGetPhysicalDeviceFeatures(device, &device_features);

	if (device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
		score += 1000;
	}

	return score;
}

QueueFamilyIndices VulkanAPI::findQueueFamilies(VkPhysicalDevice device)
{
	QueueFamilyIndices indices;

	uint32_t queue_family_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);
	std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());

	int i = 0;
	for (const auto & queue_family : queue_families)
	{
		if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			indices.graphics_family = i;
		}

		VkBool32 present_support = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &present_support);
		if (present_support)
		{
			indices.present_family = i;
		}

		if (indices.isComplete())
		{
			break;
		}

		i++;
	}

	return indices;
}

bool VulkanAPI::checkDeviceExtensionSupport(VkPhysicalDevice device)
{
	uint32_t extension_count;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);
	std::vector<VkExtensionProperties> available_extensions(extension_count);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, available_extensions.data());

	std::set<std::string> required_extensions(device_extensions.begin(), device_extensions.end());

	for (const auto & extension : available_extensions)
	{
		required_extensions.erase(extension.extensionName);
	}

	return required_extensions.empty();
}

SwapChainSupportDetails VulkanAPI::querySwapChainSupport(VkPhysicalDevice device)
{
	SwapChainSupportDetails details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

	uint32_t format_count;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, nullptr);
	if (format_count != 0)
	{
		details.formats.resize(format_count);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, details.formats.data());
	}

	uint32_t present_mode_count;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, nullptr);
	if (present_mode_count != 0)
	{
		details.present_modes.resize(present_mode_count);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, details.present_modes.data());
	}

	return details;
}

void VulkanAPI::createLogicalDevice()
{
	std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
	std::set<uint32_t> unique_queue_families = {
		queue_family_indices.graphics_family.value(),
		queue_family_indices.present_family.value()
	};

	float queue_priority = 1.0f;
	for (uint32_t queue_family : unique_queue_families)
	{
		VkDeviceQueueCreateInfo queue_create_info = {};
		queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queue_create_info.queueFamilyIndex = queue_family;
		queue_create_info.queueCount = 1;
		queue_create_info.pQueuePriorities = &queue_priority;
		queue_create_infos.push_back(queue_create_info);
	}

	VkPhysicalDeviceFeatures device_features = {};
	device_features.samplerAnisotropy = VK_TRUE;
	device_features.fillModeNonSolid = VK_TRUE;

	VkDeviceCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	create_info.pQueueCreateInfos = queue_create_infos.data();
	create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
	create_info.pEnabledFeatures = &device_features;
	create_info.enabledExtensionCount = static_cast<uint32_t>(device_extensions.size());
	create_info.ppEnabledExtensionNames = device_extensions.data();

	#ifndef NDEBUG
		create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
		create_info.ppEnabledLayerNames = validation_layers.data();
	#else
		create_info.enabledLayerCount = 0;
	#endif

	VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamic_rendering_features = {};
	dynamic_rendering_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR;
	dynamic_rendering_features.dynamicRendering = VK_TRUE;

	create_info.pNext = &dynamic_rendering_features;

	VK_CHECK(
		vkCreateDevice(physical_device, &create_info, nullptr, &device),
		"Failed to create logical device"
	);

	vkGetDeviceQueue(device, queue_family_indices.graphics_family.value(), 0, &graphics_queue);
	vkGetDeviceQueue(device, queue_family_indices.present_family.value(), 0, &present_queue);
}

void VulkanAPI::createSwapChain(GLFWwindow * window)
{
	SwapChainSupportDetails swap_chain_support = querySwapChainSupport(physical_device);

	VkSurfaceFormatKHR surface_format = chooseSwapSurfaceFormat(swap_chain_support.formats);
	VkPresentModeKHR present_mode = chooseSwapPresentMode(swap_chain_support.present_modes);
	VkExtent2D extent = chooseSwapExtent(swap_chain_support.capabilities, window);

	uint32_t image_count = swap_chain_support.capabilities.minImageCount + 1;
	if (swap_chain_support.capabilities.maxImageCount > 0 && image_count > swap_chain_support.capabilities.maxImageCount)
	{
		image_count = swap_chain_support.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	create_info.surface = surface;
	create_info.minImageCount = image_count;
	create_info.imageFormat = surface_format.format;
	create_info.imageColorSpace = surface_format.colorSpace;
	create_info.imageExtent = extent;
	create_info.imageArrayLayers = 1;
	create_info.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	uint32_t indices[] = {
		queue_family_indices.graphics_family.value(),
		queue_family_indices.present_family.value()
	};

	if (queue_family_indices.graphics_family != queue_family_indices.present_family)
	{
		create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		create_info.queueFamilyIndexCount = 2;
		create_info.pQueueFamilyIndices = indices;
	}
	else
	{
		create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		create_info.queueFamilyIndexCount = 0;
		create_info.pQueueFamilyIndices = nullptr;
	}

	create_info.preTransform = swap_chain_support.capabilities.currentTransform;
	create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	create_info.presentMode = present_mode;
	create_info.clipped = VK_TRUE;
	create_info.oldSwapchain = VK_NULL_HANDLE;

	VK_CHECK(
		vkCreateSwapchainKHR(device, &create_info, nullptr, &swap_chain),
		"Failed to create swap chain"
	);

	vkGetSwapchainImagesKHR(device, swap_chain, &image_count, nullptr);
	swap_chain_images.resize(image_count);
	vkGetSwapchainImagesKHR(device, swap_chain, &image_count, swap_chain_images.data());

	swap_chain_image_format = surface_format.format;
	swap_chain_extent = extent;

	swap_chain_image_views.resize(image_count);
	for (size_t i = 0; i < image_count; i++)
	{
		createImageView(
			swap_chain_images[i],
			swap_chain_image_format,
			VK_IMAGE_ASPECT_COLOR_BIT,
			swap_chain_image_views[i]
		);
	}
}

void VulkanAPI::recreateSwapChain(GLFWwindow * window)
{
	int width = 0, height = 0;
	glfwGetFramebufferSize(window, &width, &height);
	while (width == 0 || height == 0)
	{
		glfwGetFramebufferSize(window, &width, &height);
		glfwWaitEvents();
	}

	vkDeviceWaitIdle(device);

	destroyImGuiTexture(imgui_texture);

	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	vkDestroyDescriptorPool(device, imgui_descriptor_pool, nullptr);

	vkDestroyImageView(device, color_attachement_view, nullptr);
	vma.freeMemory(device, color_attachement_memory, nullptr);
	vkDestroyImage(device, color_attachement_image, nullptr);

	vkDestroyImageView(device, depth_attachement_view, nullptr);
	vma.freeMemory(device, depth_attachement_memory, nullptr);
	vkDestroyImage(device, depth_attachement_image, nullptr);

	vkDestroyPipeline(device, graphics_pipeline, nullptr);
	vkDestroyPipelineLayout(device, pipeline_layout, nullptr);

	vkDestroyPipeline(device, line_graphics_pipeline, nullptr);
	vkDestroyPipelineLayout(device, line_pipeline_layout, nullptr);

	for (size_t i = 0; i < swap_chain_image_views.size(); i++)
	{
		vkDestroyImageView(device, swap_chain_image_views[i], nullptr);
	}
	vkDestroySwapchainKHR(device, swap_chain, nullptr);


	createSwapChain(window);
	createColorResources();
	createDepthResources();
	createPipeline();
	createLinePipeline();
	setupImgui();
	createImGuiTexture(100, 100);
}

VkSurfaceFormatKHR VulkanAPI::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> & available_formats)
{
	for (const auto & available_format : available_formats)
	{
		if (available_format.format == VK_FORMAT_B8G8R8A8_SRGB && available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			return available_format;
		}
	}

	return available_formats[0];
}

VkPresentModeKHR VulkanAPI::chooseSwapPresentMode(const std::vector<VkPresentModeKHR> & available_present_modes)
{
	for (const auto & available_present_mode : available_present_modes)
	{
		if (available_present_mode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			return available_present_mode;
		}
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanAPI::chooseSwapExtent(const VkSurfaceCapabilitiesKHR & capabilities, GLFWwindow * window)
{
	if (capabilities.currentExtent.width != UINT32_MAX)
	{
		return capabilities.currentExtent;
	}
	else
	{
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);

		VkExtent2D actual_extent = {
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height)
		};

		actual_extent.width = std::clamp(actual_extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		actual_extent.height = std::clamp(actual_extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

		return actual_extent;
	}
}

void VulkanAPI::createCommandPool()
{
	QueueFamilyIndices queue_family_indices = findQueueFamilies(physical_device);

	VkCommandPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	pool_info.queueFamilyIndex = queue_family_indices.graphics_family.value();
	pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	VK_CHECK(
		vkCreateCommandPool(device, &pool_info, nullptr, &command_pool),
		"Failed to create command pool"
	);
}

void VulkanAPI::createCommandBuffer()
{
	render_command_buffers.resize(max_frames_in_flight);
	copy_command_buffers.resize(max_frames_in_flight);

	VkCommandBufferAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	alloc_info.commandPool = command_pool;
	alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	alloc_info.commandBufferCount = static_cast<uint32_t>(render_command_buffers.size());

	VK_CHECK(
		vkAllocateCommandBuffers(device, &alloc_info, render_command_buffers.data()),
		"Failed to allocate command buffers"
	);
	VK_CHECK(
		vkAllocateCommandBuffers(device, &alloc_info, copy_command_buffers.data()),
		"Failed to allocate command buffers"
	);
}

void VulkanAPI::createSyncObjects()
{
	image_available_semaphores.resize(max_frames_in_flight);
	render_finished_semaphores.resize(max_frames_in_flight);
	swap_chain_updated_semaphores.resize(max_frames_in_flight);
	imgui_render_finished_semaphores.resize(max_frames_in_flight);
	in_flight_fences.resize(max_frames_in_flight);

	VkSemaphoreCreateInfo semaphore_info = {};
	semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fence_info = {};
	fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < static_cast<size_t>(max_frames_in_flight); i++)
	{
		VK_CHECK(
			vkCreateSemaphore(device, &semaphore_info, nullptr, &image_available_semaphores[i]),
			"Failed to create semaphores"
		);
		VK_CHECK(
			vkCreateSemaphore(device, &semaphore_info, nullptr, &render_finished_semaphores[i]),
			"Failed to create semaphores"
		);
		VK_CHECK(
			vkCreateSemaphore(device, &semaphore_info, nullptr, &swap_chain_updated_semaphores[i]),
			"Failed to create semaphores"
		);
		VK_CHECK(
			vkCreateSemaphore(device, &semaphore_info, nullptr, &imgui_render_finished_semaphores[i]),
			"Failed to create semaphores"
		);
		VK_CHECK(
			vkCreateFence(device, &fence_info, nullptr, &in_flight_fences[i]),
			"Failed to create fences"
		);
	}

	VK_CHECK(
		vkCreateFence(device, &fence_info, nullptr, &single_time_command_fence),
		"Failed to create fences"
	);
}

void VulkanAPI::createColorResources()
{
	color_attachement_format = swap_chain_image_format;
	color_attachement_extent = swap_chain_extent;

	createImage(
		color_attachement_extent.width,
		color_attachement_extent.height,
		1,
		color_attachement_format,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		color_attachement_image,
		color_attachement_memory
	);
	createImageView(
		color_attachement_image,
		color_attachement_format,
		VK_IMAGE_ASPECT_COLOR_BIT,
		color_attachement_view
	);

	transitionImageLayout(
		color_attachement_image,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_IMAGE_ASPECT_COLOR_BIT,
		1,
		0,
		0,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
	);
}

void VulkanAPI::createDepthResources()
{
	depth_attachement_format = findSupportedFormat(
		{VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);
	depth_attachement_extent = swap_chain_extent;

	createImage(
		depth_attachement_extent.width,
		depth_attachement_extent.height,
		1,
		depth_attachement_format,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		depth_attachement_image,
		depth_attachement_memory
	);
	createImageView(
		depth_attachement_image,
		depth_attachement_format,
		VK_IMAGE_ASPECT_DEPTH_BIT,
		depth_attachement_view
	);

	transitionImageLayout(
		depth_attachement_image,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		VK_IMAGE_ASPECT_DEPTH_BIT,
		1,
		0,
		0,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT
	);
}

void VulkanAPI::createUniformBuffers()
{
	VkDeviceSize buffer_size = sizeof(CameraMatrices);

	camera_uniform_buffers.resize(max_frames_in_flight);
	camera_uniform_buffers_memory.resize(max_frames_in_flight);
	camera_uniform_buffers_mapped_memory.resize(max_frames_in_flight);

	for (int i = 0; i < max_frames_in_flight; i++)
	{
		createBuffer(
			buffer_size,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			camera_uniform_buffers[i],
			camera_uniform_buffers_memory[i]
		);

		VK_CHECK(
			vkMapMemory(device, camera_uniform_buffers_memory[i], 0, buffer_size, 0, &camera_uniform_buffers_mapped_memory[i]),
			"Failed to map memory for uniform buffer."
		);
	}

}

void VulkanAPI::createTextureArray(const std::vector<std::string> & file_paths, uint32_t size)
{
	uint32_t layers_count = static_cast<uint32_t>(file_paths.size());
	std::vector<stbi_uc *> pixels;
	std::vector<VkDeviceSize> image_sizes;
	std::vector<VkImage> staging_images;
	std::vector<VkDeviceMemory> staging_images_memory;
	std::vector<VkExtent2D> staging_images_extents;


	for (const auto & file_path : file_paths)
	{
		int tex_width, tex_height, tex_channels;
		stbi_uc * pixel = stbi_load(
			file_path.c_str(),
			&tex_width,
			&tex_height,
			&tex_channels,
			STBI_rgb_alpha
		);
		VkDeviceSize image_size = tex_width * tex_height * 4;

		if (!pixel)
		{
			throw std::runtime_error("Failed to load texture image" + file_path);
		}

		pixels.push_back(pixel);
		image_sizes.push_back(image_size);

		VkImage staging_image;
		VkDeviceMemory staging_image_memory;

		createImage(
			tex_width,
			tex_height,
			1,
			VK_FORMAT_R8G8B8A8_SRGB,
			VK_IMAGE_TILING_LINEAR,
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			staging_image,
			staging_image_memory
		);

		transitionImageLayout(
			staging_image,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			VK_IMAGE_ASPECT_COLOR_BIT,
			1,
			0,
			VK_ACCESS_TRANSFER_WRITE_BIT,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT
		);

		void * data;
		VK_CHECK(
			vkMapMemory(device, staging_image_memory, 0, image_size, 0, &data),
			"Failed to map memory for texture array staging image."
		);
		memcpy(data, pixel, static_cast<size_t>(image_size));
		vkUnmapMemory(device, staging_image_memory);

		staging_images.push_back(staging_image);
		staging_images_memory.push_back(staging_image_memory);
		staging_images_extents.push_back({
			static_cast<uint32_t>(tex_width),
			static_cast<uint32_t>(tex_height)
		});
	}

	textures_mip_levels = 1;

	VkImageCreateInfo image_info = {};
	image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_info.imageType = VK_IMAGE_TYPE_2D;
	image_info.extent = {size, size, 1};
	image_info.mipLevels = textures_mip_levels;
	image_info.arrayLayers = layers_count;
	image_info.format = VK_FORMAT_R8G8B8A8_SRGB;
	image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
	image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	image_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	image_info.samples = VK_SAMPLE_COUNT_1_BIT;

	VK_CHECK(
		vkCreateImage(device, &image_info, nullptr, &textures_image),
		"Failed to create image"
	);

	VkMemoryRequirements memory_requirements;
	vkGetImageMemoryRequirements(device, textures_image, &memory_requirements);

	VkMemoryAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.allocationSize = memory_requirements.size;
	alloc_info.memoryTypeIndex = findMemoryType(
		memory_requirements.memoryTypeBits,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);

	VK_CHECK(
		vma.allocateMemory(device, &alloc_info, nullptr, &textures_image_memory),
		"Failed to allocate image memory"
	);

	vkBindImageMemory(device, textures_image, textures_image_memory, 0);

	VkCommandBuffer command_buffer = beginSingleTimeCommands();

	// Transition image layout
	VkImageMemoryBarrier first_barrier = {};
	first_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	first_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	first_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	first_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	first_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	first_barrier.image = textures_image;
	first_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	first_barrier.subresourceRange.baseMipLevel = 0;
	first_barrier.subresourceRange.levelCount = textures_mip_levels;
	first_barrier.subresourceRange.baseArrayLayer = 0;
	first_barrier.subresourceRange.layerCount = layers_count;
	first_barrier.srcAccessMask = 0;
	first_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

	vkCmdPipelineBarrier(
		command_buffer,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		0,
		0, nullptr,
		0, nullptr,
		1, &first_barrier
	);

	// Blit images to texture array
	VkImageBlit blit = {};
	blit.srcOffsets[0] = { 0, 0, 0 };
	blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	blit.srcSubresource.mipLevel = 0;
	blit.srcSubresource.baseArrayLayer = 0;
	blit.srcSubresource.layerCount = 1;

	blit.dstOffsets[0] = { 0, 0, 0 };
	blit.dstOffsets[1] = {
		static_cast<int32_t>(size),
		static_cast<int32_t>(size),
		1
	};
	blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	blit.dstSubresource.mipLevel = 0;
	blit.dstSubresource.baseArrayLayer = 0;
	blit.dstSubresource.layerCount = 1;

	for (uint32_t i = 0; i < layers_count; i++)
	{
		blit.srcOffsets[1] = {
			static_cast<int32_t>(staging_images_extents[i].width),
			static_cast<int32_t>(staging_images_extents[i].height),
			1
		};
		blit.dstSubresource.baseArrayLayer = i;

		LOG_DEBUG("Copy texture '" << file_paths[i] << "' from ("
			<< staging_images_extents[i].width << ", " << staging_images_extents[i].height
			<< ") to (" << size << ", " << size << ")");

		vkCmdBlitImage(
			command_buffer,
			staging_images[i],
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			textures_image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&blit,
			VK_FILTER_LINEAR
		);
	}

	// Transition image layout
	VkImageMemoryBarrier second_barrier = {};
	second_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	second_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	second_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	second_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	second_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	second_barrier.image = textures_image;
	second_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	second_barrier.subresourceRange.baseMipLevel = 0;
	second_barrier.subresourceRange.levelCount = textures_mip_levels;
	second_barrier.subresourceRange.baseArrayLayer = 0;
	second_barrier.subresourceRange.layerCount = layers_count;
	second_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	second_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier(
		command_buffer,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		0,
		0, nullptr,
		0, nullptr,
		1, &second_barrier
	);

	endSingleTimeCommands(command_buffer);

	for (size_t i = 0; i < layers_count; i++)
	{
		vkDestroyImage(device, staging_images[i], nullptr);
		vma.freeMemory(device, staging_images_memory[i], nullptr);
		stbi_image_free(pixels[i]);
	}

	VkImageViewCreateInfo view_info = {};
	view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	view_info.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
	view_info.format = VK_FORMAT_R8G8B8A8_SRGB;
	view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	view_info.subresourceRange.baseMipLevel = 0;
	view_info.subresourceRange.levelCount = textures_mip_levels;
	view_info.subresourceRange.baseArrayLayer = 0;
	view_info.subresourceRange.layerCount = layers_count;
	view_info.image = textures_image;

	VK_CHECK(
		vkCreateImageView(device, &view_info, nullptr, &textures_image_view),
		"Failed to create image view"
	);


	VkPhysicalDeviceProperties properties{};
	vkGetPhysicalDeviceProperties(physical_device, &properties);

	VkSamplerCreateInfo sampler_info = {};
	sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sampler_info.magFilter = VK_FILTER_NEAREST;
	sampler_info.minFilter = VK_FILTER_NEAREST;
	sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_info.anisotropyEnable = VK_FALSE;
	sampler_info.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
	sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	sampler_info.unnormalizedCoordinates = VK_FALSE;
	sampler_info.compareEnable = VK_FALSE;
	sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
	sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	sampler_info.mipLodBias = 0.0f;
	sampler_info.minLod = 0.0f;
	sampler_info.maxLod = 0.0f;

	VK_CHECK(
		vkCreateSampler(device, &sampler_info, nullptr, &textures_sampler),
		"Failed to create sampler"
	);
}

void VulkanAPI::createCubeMap(const std::array<std::string, 6> & file_paths, uint32_t size)
{
	uint32_t layers_count = static_cast<uint32_t>(file_paths.size());
	std::vector<stbi_uc *> pixels;
	std::vector<VkDeviceSize> image_sizes;
	std::vector<VkImage> staging_images;
	std::vector<VkDeviceMemory> staging_images_memory;
	std::vector<VkExtent2D> staging_images_extents;


	for (const auto & file_path : file_paths)
	{
		int tex_width, tex_height, tex_channels;
		stbi_uc * pixel = stbi_load(
			file_path.c_str(),
			&tex_width,
			&tex_height,
			&tex_channels,
			STBI_rgb_alpha
		);
		VkDeviceSize image_size = tex_width * tex_height * 4;

		if (!pixel)
		{
			throw std::runtime_error("Failed to load texture image" + file_path);
		}

		pixels.push_back(pixel);
		image_sizes.push_back(image_size);

		VkImage staging_image;
		VkDeviceMemory staging_image_memory;

		createImage(
			tex_width,
			tex_height,
			1,
			VK_FORMAT_R8G8B8A8_SRGB,
			VK_IMAGE_TILING_LINEAR,
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			staging_image,
			staging_image_memory
		);

		transitionImageLayout(
			staging_image,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			VK_IMAGE_ASPECT_COLOR_BIT,
			1,
			0,
			VK_ACCESS_TRANSFER_WRITE_BIT,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT
		);

		void * data;
		VK_CHECK(
			vkMapMemory(device, staging_image_memory, 0, image_size, 0, &data),
			"Failed to map memory for texture array staging image."
		);
		memcpy(data, pixel, static_cast<size_t>(image_size));
		vkUnmapMemory(device, staging_image_memory);

		staging_images.push_back(staging_image);
		staging_images_memory.push_back(staging_image_memory);
		staging_images_extents.push_back({
			static_cast<uint32_t>(tex_width),
			static_cast<uint32_t>(tex_height)
		});
	}

	textures_mip_levels = 1;

	VkImageCreateInfo image_info = {};
	image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_info.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
	image_info.imageType = VK_IMAGE_TYPE_2D;
	image_info.extent = {size, size, 1};
	image_info.mipLevels = textures_mip_levels;
	image_info.arrayLayers = layers_count;
	image_info.format = VK_FORMAT_R8G8B8A8_SRGB;
	image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
	image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	image_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	image_info.samples = VK_SAMPLE_COUNT_1_BIT;

	VK_CHECK(
		vkCreateImage(device, &image_info, nullptr, &textures_image),
		"Failed to create image"
	);

	VkMemoryRequirements memory_requirements;
	vkGetImageMemoryRequirements(device, textures_image, &memory_requirements);

	VkMemoryAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.allocationSize = memory_requirements.size;
	alloc_info.memoryTypeIndex = findMemoryType(
		memory_requirements.memoryTypeBits,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);

	VK_CHECK(
		vma.allocateMemory(device, &alloc_info, nullptr, &textures_image_memory),
		"Failed to allocate image memory"
	);

	vkBindImageMemory(device, textures_image, textures_image_memory, 0);

	VkCommandBuffer command_buffer = beginSingleTimeCommands();

	// Transition image layout
	VkImageMemoryBarrier first_barrier = {};
	first_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	first_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	first_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	first_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	first_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	first_barrier.image = textures_image;
	first_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	first_barrier.subresourceRange.baseMipLevel = 0;
	first_barrier.subresourceRange.levelCount = textures_mip_levels;
	first_barrier.subresourceRange.baseArrayLayer = 0;
	first_barrier.subresourceRange.layerCount = layers_count;
	first_barrier.srcAccessMask = 0;
	first_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

	vkCmdPipelineBarrier(
		command_buffer,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		0,
		0, nullptr,
		0, nullptr,
		1, &first_barrier
	);

	// Blit images to texture array
	VkImageBlit blit = {};
	blit.srcOffsets[0] = { 0, 0, 0 };
	blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	blit.srcSubresource.mipLevel = 0;
	blit.srcSubresource.baseArrayLayer = 0;
	blit.srcSubresource.layerCount = 1;

	blit.dstOffsets[0] = { 0, 0, 0 };
	blit.dstOffsets[1] = {
		static_cast<int32_t>(size),
		static_cast<int32_t>(size),
		1
	};
	blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	blit.dstSubresource.mipLevel = 0;
	blit.dstSubresource.baseArrayLayer = 0;
	blit.dstSubresource.layerCount = 1;

	for (uint32_t i = 0; i < layers_count; i++)
	{
		blit.srcOffsets[1] = {
			static_cast<int32_t>(staging_images_extents[i].width),
			static_cast<int32_t>(staging_images_extents[i].height),
			1
		};
		blit.dstSubresource.baseArrayLayer = i;

		LOG_DEBUG("Copy texture '" << file_paths[i] << "' from ("
			<< staging_images_extents[i].width << ", " << staging_images_extents[i].height
			<< ") to (" << size << ", " << size << ")");

		vkCmdBlitImage(
			command_buffer,
			staging_images[i],
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			textures_image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&blit,
			VK_FILTER_LINEAR
		);
	}

	// Transition image layout
	VkImageMemoryBarrier second_barrier = {};
	second_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	second_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	second_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	second_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	second_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	second_barrier.image = textures_image;
	second_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	second_barrier.subresourceRange.baseMipLevel = 0;
	second_barrier.subresourceRange.levelCount = textures_mip_levels;
	second_barrier.subresourceRange.baseArrayLayer = 0;
	second_barrier.subresourceRange.layerCount = layers_count;
	second_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	second_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier(
		command_buffer,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		0,
		0, nullptr,
		0, nullptr,
		1, &second_barrier
	);

	endSingleTimeCommands(command_buffer);

	for (size_t i = 0; i < layers_count; i++)
	{
		vkDestroyImage(device, staging_images[i], nullptr);
		vma.freeMemory(device, staging_images_memory[i], nullptr);
		stbi_image_free(pixels[i]);
	}

	VkImageViewCreateInfo view_info = {};
	view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	view_info.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
	view_info.format = VK_FORMAT_R8G8B8A8_SRGB;
	view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	view_info.subresourceRange.baseMipLevel = 0;
	view_info.subresourceRange.levelCount = textures_mip_levels;
	view_info.subresourceRange.baseArrayLayer = 0;
	view_info.subresourceRange.layerCount = layers_count;
	view_info.image = textures_image;

	VK_CHECK(
		vkCreateImageView(device, &view_info, nullptr, &textures_image_view),
		"Failed to create image view"
	);


	VkPhysicalDeviceProperties properties{};
	vkGetPhysicalDeviceProperties(physical_device, &properties);

	VkSamplerCreateInfo sampler_info = {};
	sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sampler_info.magFilter = VK_FILTER_NEAREST;
	sampler_info.minFilter = VK_FILTER_NEAREST;
	sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_info.anisotropyEnable = VK_FALSE;
	sampler_info.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
	sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	sampler_info.unnormalizedCoordinates = VK_FALSE;
	sampler_info.compareEnable = VK_FALSE;
	sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
	sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	sampler_info.mipLodBias = 0.0f;
	sampler_info.minLod = 0.0f;
	sampler_info.maxLod = 0.0f;

	VK_CHECK(
		vkCreateSampler(device, &sampler_info, nullptr, &textures_sampler),
		"Failed to create sampler"
	);
}

void VulkanAPI::createFrustumLineBuffers()
{
	frustum_line_vertex_count = 8;
	frustum_line_index_count = 24;

	VkDeviceSize vertex_buffer_size = sizeof(LineVertex) * frustum_line_vertex_count;
	VkDeviceSize index_buffer_size = sizeof(uint32_t) * frustum_line_index_count;
	VkDeviceSize buffer_size = vertex_buffer_size + index_buffer_size;

	frustum_line_index_offset = vertex_buffer_size;

	frustum_line_buffers.resize(max_frames_in_flight);
	frustum_line_buffers_memory.resize(max_frames_in_flight);
	frustum_line_buffers_mapped_memory.resize(max_frames_in_flight);

	std::vector<uint32_t> indices = {
		0, 1, 1, 2, 2, 3, 3, 0,
		4, 5, 5, 6, 6, 7, 7, 4,
		0, 4, 1, 5, 2, 6, 3, 7
	};

	for (int i = 0; i < max_frames_in_flight; i++)
	{
		createBuffer(
			buffer_size,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			frustum_line_buffers[i],
			frustum_line_buffers_memory[i]
		);

		VK_CHECK(
			vkMapMemory(device, frustum_line_buffers_memory[i], 0, buffer_size, 0, &frustum_line_buffers_mapped_memory[i]),
			"Failed to map memory for frustum line buffer."
		);

		// Copy the indices to the buffer because they are static
		memcpy(
			static_cast<char *>(frustum_line_buffers_mapped_memory[i]) + vertex_buffer_size,
			indices.data(),
			static_cast<size_t>(index_buffer_size)
		);
	}
}

void VulkanAPI::createCameraDescriptors()
{
	VkDescriptorSetLayoutBinding ubo_layout_binding = {};
	ubo_layout_binding.binding = 0;
	ubo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	ubo_layout_binding.descriptorCount = 1;
	ubo_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	ubo_layout_binding.pImmutableSamplers = nullptr;

	std::array<VkDescriptorSetLayoutBinding, 1> bindings = { ubo_layout_binding };

	VkDescriptorSetLayoutCreateInfo layout_info = {};
	layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layout_info.bindingCount = static_cast<uint32_t>(bindings.size());
	layout_info.pBindings = bindings.data();

	VK_CHECK(
		vkCreateDescriptorSetLayout(device, &layout_info, nullptr, &camera_descriptor_set_layout),
		"Failed to create descriptor set layout"
	);

	VkDescriptorPoolSize pool_size = {};
	pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	pool_size.descriptorCount = static_cast<uint32_t>(max_frames_in_flight);

	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.poolSizeCount = 1;
	pool_info.pPoolSizes = &pool_size;
	pool_info.maxSets = static_cast<uint32_t>(max_frames_in_flight);

	VK_CHECK(
		vkCreateDescriptorPool(device, &pool_info, nullptr, &camera_descriptor_pool),
		"Failed to create descriptor pool"
	);

	VkDescriptorSetAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	alloc_info.descriptorPool = camera_descriptor_pool;
	alloc_info.descriptorSetCount = 1;
	alloc_info.pSetLayouts = &camera_descriptor_set_layout;

	VK_CHECK(
		vkAllocateDescriptorSets(device, &alloc_info, &camera_descriptor_set),
		"Failed to allocate descriptor sets"
	);

	for (int i = 0; i < max_frames_in_flight; i++)
	{
		VkDescriptorBufferInfo buffer_info = {};
		buffer_info.buffer = camera_uniform_buffers[i];
		buffer_info.offset = 0;
		buffer_info.range = sizeof(CameraMatrices);

		VkWriteDescriptorSet descriptor_write = {};
		descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptor_write.dstSet = camera_descriptor_set;
		descriptor_write.dstBinding = 0;
		descriptor_write.dstArrayElement = 0;
		descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptor_write.descriptorCount = 1;
		descriptor_write.pBufferInfo = &buffer_info;

		vkUpdateDescriptorSets(
			device,
			1,
			&descriptor_write,
			0, nullptr
		);
	}
}

void VulkanAPI::createTextureArrayDescriptors()
{
	VkDescriptorSetLayoutBinding sampler_layout_binding = {};
	sampler_layout_binding.binding = 0;
	sampler_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	sampler_layout_binding.descriptorCount = 1;
	sampler_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	sampler_layout_binding.pImmutableSamplers = nullptr;

	std::array<VkDescriptorSetLayoutBinding, 1> bindings = { sampler_layout_binding };

	VkDescriptorSetLayoutCreateInfo layout_info = {};
	layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layout_info.bindingCount = static_cast<uint32_t>(bindings.size());
	layout_info.pBindings = bindings.data();

	VK_CHECK(
		vkCreateDescriptorSetLayout(device, &layout_info, nullptr, &texture_array_descriptor_set_layout),
		"Failed to create descriptor set layout"
	);

	VkDescriptorPoolSize pool_size = {};
	pool_size.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	pool_size.descriptorCount = static_cast<uint32_t>(max_frames_in_flight);

	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.poolSizeCount = 1;
	pool_info.pPoolSizes = &pool_size;
	pool_info.maxSets = static_cast<uint32_t>(max_frames_in_flight);

	VK_CHECK(
		vkCreateDescriptorPool(device, &pool_info, nullptr, &texture_array_descriptor_pool),
		"Failed to create descriptor pool"
	);

	VkDescriptorSetAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	alloc_info.descriptorPool = texture_array_descriptor_pool;
	alloc_info.descriptorSetCount = 1;
	alloc_info.pSetLayouts = &texture_array_descriptor_set_layout;

	VK_CHECK(
		vkAllocateDescriptorSets(device, &alloc_info, &texture_array_descriptor_set),
		"Failed to allocate descriptor sets"
	);

	for (int i = 0; i < max_frames_in_flight; i++)
	{
		VkDescriptorImageInfo image_info = {};
		image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		image_info.imageView = textures_image_view;
		image_info.sampler = textures_sampler;

		VkWriteDescriptorSet descriptor_write = {};
		descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptor_write.dstSet = texture_array_descriptor_set;
		descriptor_write.dstBinding = 0;
		descriptor_write.dstArrayElement = 0;
		descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptor_write.descriptorCount = 1;
		descriptor_write.pImageInfo = &image_info;

		vkUpdateDescriptorSets(
			device,
			1,
			&descriptor_write,
			0, nullptr
		);
	}
}

void VulkanAPI::createPipeline()
{
	auto vert_shader_code = readFile("shaders/simple_shader.vert.spv");
	auto frag_shader_code = readFile("shaders/simple_shader.frag.spv");

	VkShaderModule vert_shader_module = createShaderModule(vert_shader_code);
	VkShaderModule frag_shader_module = createShaderModule(frag_shader_code);

	VkPipelineShaderStageCreateInfo vert_shader_stage_info = {};
	vert_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vert_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vert_shader_stage_info.module = vert_shader_module;
	vert_shader_stage_info.pName = "main";

	VkPipelineShaderStageCreateInfo frag_shader_stage_info = {};
	frag_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	frag_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	frag_shader_stage_info.module = frag_shader_module;
	frag_shader_stage_info.pName = "main";

	VkPipelineShaderStageCreateInfo shader_stages[] = { vert_shader_stage_info, frag_shader_stage_info };


	VkVertexInputBindingDescription binding_description = BlockVertex::getBindingDescription();
	std::array<VkVertexInputAttributeDescription, 4> attribute_descriptions = BlockVertex::getAttributeDescriptions();

	VkPipelineVertexInputStateCreateInfo vertex_input_info = {};
	vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input_info.vertexBindingDescriptionCount = 1;
	vertex_input_info.pVertexBindingDescriptions = &binding_description;
	vertex_input_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribute_descriptions.size());
	vertex_input_info.pVertexAttributeDescriptions = attribute_descriptions.data();


	VkPipelineInputAssemblyStateCreateInfo input_assembly = {};
	input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	input_assembly.primitiveRestartEnable = VK_FALSE;


	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(swap_chain_extent.width);
	viewport.height = static_cast<float>(swap_chain_extent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = {0, 0};
	scissor.extent = swap_chain_extent;

	VkPipelineViewportStateCreateInfo viewport_state = {};
	viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_state.viewportCount = 1;
	viewport_state.pViewports = &viewport;
	viewport_state.scissorCount = 1;
	viewport_state.pScissors = &scissor;


	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f;
	rasterizer.depthBiasClamp = 0.0f;
	rasterizer.depthBiasSlopeFactor = 0.0f;


	VkPipelineMultisampleStateCreateInfo multisampling = {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;


	VkPipelineColorBlendAttachmentState color_blend_attachment = {};
	color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	color_blend_attachment.blendEnable = VK_FALSE;


	VkPipelineColorBlendStateCreateInfo color_blending = {};
	color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	color_blending.logicOpEnable = VK_FALSE;
	color_blending.attachmentCount = 1;
	color_blending.pAttachments = &color_blend_attachment;


	VkPipelineDepthStencilStateCreateInfo depth_stencil = {};
	depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depth_stencil.depthTestEnable = VK_TRUE;
	depth_stencil.depthWriteEnable = VK_TRUE;
	depth_stencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depth_stencil.depthBoundsTestEnable = VK_FALSE;
	depth_stencil.stencilTestEnable = VK_FALSE;


	std::vector<VkFormat> color_formats = {color_attachement_format};

	VkPipelineRenderingCreateInfo rendering_info = {};
	rendering_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
	rendering_info.colorAttachmentCount = static_cast<uint32_t>(color_formats.size());
	rendering_info.pColorAttachmentFormats = color_formats.data();
	rendering_info.depthAttachmentFormat = depth_attachement_format;


	std::array<VkDescriptorSetLayout, 2> descriptor_set_layouts = {
		camera_descriptor_set_layout,
		texture_array_descriptor_set_layout
	};

	VkPushConstantRange push_constant_range = {};
	push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	push_constant_range.offset = 0;
	push_constant_range.size = sizeof(ModelMatrice);

	VkPipelineLayoutCreateInfo pipeline_layout_info = {};
	pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_info.setLayoutCount = static_cast<uint32_t>(descriptor_set_layouts.size());
	pipeline_layout_info.pSetLayouts = descriptor_set_layouts.data();
	pipeline_layout_info.pushConstantRangeCount = 1;
	pipeline_layout_info.pPushConstantRanges = &push_constant_range;

	VK_CHECK(
		vkCreatePipelineLayout(device, &pipeline_layout_info, nullptr, &pipeline_layout),
		"Failed to create pipeline layout"
	);


	VkGraphicsPipelineCreateInfo pipeline_info = {};
	pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipeline_info.stageCount = 2;
	pipeline_info.pStages = shader_stages;
	pipeline_info.pVertexInputState = &vertex_input_info;
	pipeline_info.pInputAssemblyState = &input_assembly;
	pipeline_info.pViewportState = &viewport_state;
	pipeline_info.pRasterizationState = &rasterizer;
	pipeline_info.pMultisampleState = &multisampling;
	pipeline_info.pColorBlendState = &color_blending;
	pipeline_info.pDepthStencilState = &depth_stencil;
	pipeline_info.layout = pipeline_layout;
	pipeline_info.pNext = &rendering_info;

	VK_CHECK(
		vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &graphics_pipeline),
		"Failed to create graphics pipeline"
	);

	vkDestroyShaderModule(device, frag_shader_module, nullptr);
	vkDestroyShaderModule(device, vert_shader_module, nullptr);
}

void VulkanAPI::createLinePipeline()
{
	auto vert_shader_code = readFile("shaders/line_shader.vert.spv");
	auto frag_shader_code = readFile("shaders/line_shader.frag.spv");

	VkShaderModule vert_shader_module = createShaderModule(vert_shader_code);
	VkShaderModule frag_shader_module = createShaderModule(frag_shader_code);

	VkPipelineShaderStageCreateInfo vert_shader_stage_info = {};
	vert_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vert_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vert_shader_stage_info.module = vert_shader_module;
	vert_shader_stage_info.pName = "main";

	VkPipelineShaderStageCreateInfo frag_shader_stage_info = {};
	frag_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	frag_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	frag_shader_stage_info.module = frag_shader_module;
	frag_shader_stage_info.pName = "main";

	VkPipelineShaderStageCreateInfo shader_stages[] = { vert_shader_stage_info, frag_shader_stage_info };


	VkVertexInputBindingDescription binding_description = LineVertex::getBindingDescription();
	std::array<VkVertexInputAttributeDescription, 2> attribute_descriptions = LineVertex::getAttributeDescriptions();

	VkPipelineVertexInputStateCreateInfo vertex_input_info = {};
	vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input_info.vertexBindingDescriptionCount = 1;
	vertex_input_info.pVertexBindingDescriptions = &binding_description;
	vertex_input_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribute_descriptions.size());
	vertex_input_info.pVertexAttributeDescriptions = attribute_descriptions.data();


	VkPipelineInputAssemblyStateCreateInfo input_assembly = {};
	input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
	input_assembly.primitiveRestartEnable = VK_FALSE;


	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(swap_chain_extent.width);
	viewport.height = static_cast<float>(swap_chain_extent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = {0, 0};
	scissor.extent = swap_chain_extent;

	VkPipelineViewportStateCreateInfo viewport_state = {};
	viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_state.viewportCount = 1;
	viewport_state.pViewports = &viewport;
	viewport_state.scissorCount = 1;
	viewport_state.pScissors = &scissor;


	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT; // VK_CULL_MODE_NONE;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f;
	rasterizer.depthBiasClamp = 0.0f;
	rasterizer.depthBiasSlopeFactor = 0.0f;


	VkPipelineMultisampleStateCreateInfo multisampling = {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;


	VkPipelineColorBlendAttachmentState color_blend_attachment = {};
	color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	color_blend_attachment.blendEnable = VK_FALSE;


	VkPipelineColorBlendStateCreateInfo color_blending = {};
	color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	color_blending.logicOpEnable = VK_FALSE;
	color_blending.attachmentCount = 1;
	color_blending.pAttachments = &color_blend_attachment;


	VkPipelineDepthStencilStateCreateInfo depth_stencil = {};
	depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depth_stencil.depthTestEnable = VK_TRUE;
	depth_stencil.depthWriteEnable = VK_TRUE;
	depth_stencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depth_stencil.depthBoundsTestEnable = VK_FALSE;
	depth_stencil.stencilTestEnable = VK_FALSE;


	std::vector<VkFormat> color_formats = { color_attachement_format };

	VkPipelineRenderingCreateInfo rendering_info = {};
	rendering_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
	rendering_info.colorAttachmentCount = static_cast<uint32_t>(color_formats.size());
	rendering_info.pColorAttachmentFormats = color_formats.data();
	rendering_info.depthAttachmentFormat = depth_attachement_format;


	std::array<VkDescriptorSetLayout, 1> descriptor_set_layouts = {
		camera_descriptor_set_layout
	};

	VkPipelineLayoutCreateInfo pipeline_layout_info = {};
	pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_info.setLayoutCount = static_cast<uint32_t>(descriptor_set_layouts.size());
	pipeline_layout_info.pSetLayouts = descriptor_set_layouts.data();

	VK_CHECK(
		vkCreatePipelineLayout(device, &pipeline_layout_info, nullptr, &line_pipeline_layout),
		"Failed to create pipeline layout"
	);


	VkGraphicsPipelineCreateInfo pipeline_info = {};
	pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipeline_info.stageCount = 2;
	pipeline_info.pStages = shader_stages;
	pipeline_info.pVertexInputState = &vertex_input_info;
	pipeline_info.pInputAssemblyState = &input_assembly;
	pipeline_info.pViewportState = &viewport_state;
	pipeline_info.pRasterizationState = &rasterizer;
	pipeline_info.pMultisampleState = &multisampling;
	pipeline_info.pColorBlendState = &color_blending;
	pipeline_info.pDepthStencilState = &depth_stencil;
	pipeline_info.layout = line_pipeline_layout;
	pipeline_info.pNext = &rendering_info;

	VK_CHECK(
		vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &line_graphics_pipeline),
		"Failed to create graphics pipeline"
	);

	vkDestroyShaderModule(device, frag_shader_module, nullptr);
	vkDestroyShaderModule(device, vert_shader_module, nullptr);
}

std::vector<char> VulkanAPI::readFile(const std::string & filename)
{
	std::ifstream file
	{
		filename,
		std::ios::ate | std::ios::binary
	};

	if (!file.is_open())
	{
		throw std::runtime_error("Failed to open file: " + filename);
	}

	size_t file_size = static_cast<size_t>(file.tellg());
	std::vector<char> buffer(file_size);

	file.seekg(0);
	file.read(buffer.data(), file_size);
	file.close();

	return buffer;
}

VkShaderModule VulkanAPI::createShaderModule(const std::vector<char> & code)
{
	VkShaderModuleCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	create_info.codeSize = code.size();
	create_info.pCode = reinterpret_cast<const uint32_t *>(code.data());

	VkShaderModule shader_module;
	VK_CHECK(
		vkCreateShaderModule(device, &create_info, nullptr, &shader_module),
		"Failed to create shader module"
	);

	return shader_module;
}

uint64_t VulkanAPI::createImGuiTexture(const uint32_t width, const uint32_t height)
{
	imgui_texture.extent = { width, height };
	imgui_texture.format = VK_FORMAT_R8G8B8A8_SRGB;

	createImage(
		imgui_texture.extent.width,
		imgui_texture.extent.height,
		1,
		imgui_texture.format,
		VK_IMAGE_TILING_LINEAR,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		imgui_texture.image,
		imgui_texture.memory
	);

	createImageView(
		imgui_texture.image,
		imgui_texture.format,
		VK_IMAGE_ASPECT_COLOR_BIT,
		imgui_texture.view
	);

	VkSamplerCreateInfo sampler_info = {};
	sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sampler_info.magFilter = VK_FILTER_LINEAR;
	sampler_info.minFilter = VK_FILTER_LINEAR;
	sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler_info.anisotropyEnable = VK_FALSE;
	sampler_info.maxAnisotropy = 1.0f;
	sampler_info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	sampler_info.unnormalizedCoordinates = VK_FALSE;
	sampler_info.compareEnable = VK_FALSE;
	sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
	sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	sampler_info.mipLodBias = 0.0f;
	sampler_info.minLod = 0.0f;
	sampler_info.maxLod = 0.0f;

	VK_CHECK(
		vkCreateSampler(device, &sampler_info, nullptr, &imgui_texture.sampler),
		"Failed to create imgui texture sampler"
	);

	imgui_texture.descriptor_set = ImGui_ImplVulkan_AddTexture(
		imgui_texture.sampler,
		imgui_texture.view,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	);

	transitionImageLayout(
		imgui_texture.image,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_GENERAL,
		VK_IMAGE_ASPECT_COLOR_BIT,
		1,
		0,
		0,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT
	);

	VK_CHECK(
		vkMapMemory(device, imgui_texture.memory, 0, VK_WHOLE_SIZE, 0, &imgui_texture.mapped_memory),
		"Failed to map memory for ImGui texture."
	);

	return 0;
}

void VulkanAPI::destroyImGuiTexture(ImGuiTexture & imgui_texture)
{
	vkUnmapMemory(device, imgui_texture.memory);
	ImGui_ImplVulkan_RemoveTexture(imgui_texture.descriptor_set);
	vkDestroySampler(device, imgui_texture.sampler, nullptr);
	vkDestroyImageView(device, imgui_texture.view, nullptr);
	vma.freeMemory(device, imgui_texture.memory, nullptr);
	vkDestroyImage(device, imgui_texture.image, nullptr);
}


void VulkanAPI::setupImgui()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO & io = ImGui::GetIO();
	(void)io;

	ImGui::StyleColorsDark();

	std::vector<VkDescriptorPoolSize> pool_sizes =
	{
		{VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
		{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
		{VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
		{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
		{VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
		{VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
		{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
		{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
		{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
		{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
		{VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}
	};

	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	pool_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
	pool_info.pPoolSizes = pool_sizes.data();
	pool_info.maxSets = 1000;

	VK_CHECK(
		vkCreateDescriptorPool(device, &pool_info, nullptr, &imgui_descriptor_pool),
		"Failed to create imgui descriptor pool"
	);

	ImGui_ImplGlfw_InitForVulkan(window, true);
	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = instance;
	init_info.PhysicalDevice = physical_device;
	init_info.Device = device;
	init_info.QueueFamily = queue_family_indices.graphics_family.value();
	init_info.Queue = graphics_queue;
	init_info.PipelineCache = VK_NULL_HANDLE;
	init_info.DescriptorPool = imgui_descriptor_pool;
	init_info.Allocator = nullptr;
	init_info.MinImageCount = 2;
	init_info.ImageCount = static_cast<uint32_t>(swap_chain_images.size());
	init_info.UseDynamicRendering = VK_TRUE;
	init_info.PipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
	init_info.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
	init_info.PipelineRenderingCreateInfo.pColorAttachmentFormats = &swap_chain_image_format;

	ImGui_ImplVulkan_Init(&init_info);
}


VkCommandBuffer VulkanAPI::beginSingleTimeCommands()
{
	vkResetFences(device, 1, &single_time_command_fence);

	VkCommandBufferAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	alloc_info.commandPool = command_pool;
	alloc_info.commandBufferCount = 1;

	VkCommandBuffer command_buffer;
	vkAllocateCommandBuffers(device, &alloc_info, &command_buffer);

	VkCommandBufferBeginInfo begin_info = {};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(command_buffer, &begin_info);

	return command_buffer;
}

void VulkanAPI::endSingleTimeCommands(VkCommandBuffer command_buffer)
{
	vkEndCommandBuffer(command_buffer);

	VkSubmitInfo submit_info = {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &command_buffer;

	vkQueueSubmit(graphics_queue, 1, &submit_info, single_time_command_fence);
	vkWaitForFences(device, 1, &single_time_command_fence, VK_TRUE, UINT64_MAX);

	vkFreeCommandBuffers(device, command_pool, 1, &command_buffer);
}

uint32_t VulkanAPI::findMemoryType(
	uint32_t type_filter,
	VkMemoryPropertyFlags properties
)
{
	VkPhysicalDeviceMemoryProperties mem_properties;
	vkGetPhysicalDeviceMemoryProperties(physical_device, &mem_properties);

	for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++)
	{
		if ((type_filter & (1 << i)) && (mem_properties.memoryTypes[i].propertyFlags & properties) == properties)
		{
			return i;
		}
	}

	throw std::runtime_error("Failed to find suitable memory type");
}

VkFormat VulkanAPI::findSupportedFormat(
	const std::vector<VkFormat> & candidates,
	VkImageTiling tiling,
	VkFormatFeatureFlags features
)
{
	for (VkFormat format : candidates)
	{
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(physical_device, format, &props);

		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
		{
			return format;
		}
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
		{
			return format;
		}
	}

	throw std::runtime_error("Failed to find supported format");
}

void VulkanAPI::createImage(
	uint32_t width,
	uint32_t height,
	uint32_t mip_levels,
	VkFormat format,
	VkImageTiling tiling,
	VkImageUsageFlags usage,
	VkMemoryPropertyFlags properties,
	VkImage & image,
	VkDeviceMemory & image_memory
)
{
	VkImageCreateInfo image_info = {};
	image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_info.imageType = VK_IMAGE_TYPE_2D;
	image_info.extent.width = width;
	image_info.extent.height = height;
	image_info.extent.depth = 1;
	image_info.mipLevels = mip_levels;
	image_info.arrayLayers = 1;
	image_info.format = format;
	image_info.tiling = tiling;
	image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	image_info.usage = usage;
	image_info.samples = VK_SAMPLE_COUNT_1_BIT;
	image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VK_CHECK(
		vkCreateImage(device, &image_info, nullptr, &image),
		"Failed to create image"
	);

	VkMemoryRequirements mem_requirements;
	vkGetImageMemoryRequirements(device, image, &mem_requirements);

	VkMemoryAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.allocationSize = mem_requirements.size;
	alloc_info.memoryTypeIndex = findMemoryType(mem_requirements.memoryTypeBits, properties);

	VK_CHECK(
		vma.allocateMemory(device, &alloc_info, nullptr, &image_memory),
		"Failed to allocate image memory"
	);

	vkBindImageMemory(device, image, image_memory, 0);
}

void VulkanAPI::createImageView(
	VkImage image,
	VkFormat format,
	VkImageAspectFlags aspect_flags,
	VkImageView & image_view
)
{
	VkImageViewCreateInfo view_info = {};
	view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	view_info.image = image;
	view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	view_info.format = format;
	view_info.subresourceRange.aspectMask = aspect_flags;
	view_info.subresourceRange.baseMipLevel = 0;
	view_info.subresourceRange.levelCount = 1;
	view_info.subresourceRange.baseArrayLayer = 0;
	view_info.subresourceRange.layerCount = 1;

	VK_CHECK(
		vkCreateImageView(device, &view_info, nullptr, &image_view),
		"Failed to create image view"
	);
}

void VulkanAPI::transitionImageLayout(
	VkImage image,
	VkImageLayout old_layout,
	VkImageLayout new_layout,
	VkImageAspectFlags aspect_mask,
	uint32_t mip_levels,
	VkAccessFlags src_access_mask,
	VkAccessFlags dst_access_mask,
	VkPipelineStageFlags src_stage_mask,
	VkPipelineStageFlags dst_stage_mask
)
{
	VkCommandBufferAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	alloc_info.commandPool = command_pool;
	alloc_info.commandBufferCount = 1;

	VkCommandBuffer command_buffer;
	VK_CHECK(
		vkAllocateCommandBuffers(device, &alloc_info, &command_buffer),
		"Failed to allocate command buffers"
	);

	VkCommandBufferBeginInfo begin_info = {};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	VK_CHECK(
		vkBeginCommandBuffer(command_buffer, &begin_info),
		"Failed to begin recording command buffer"
	);

	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = old_layout;
	barrier.newLayout = new_layout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = aspect_mask;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = mip_levels;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.srcAccessMask = src_access_mask;
	barrier.dstAccessMask = dst_access_mask;

	vkCmdPipelineBarrier(
		command_buffer,
		src_stage_mask,
		dst_stage_mask,
		0,
		0,
		nullptr,
		0,
		nullptr,
		1,
		&barrier
	);

	VK_CHECK(
		vkEndCommandBuffer(command_buffer),
		"Failed to end recording command buffer"
	);

	VkSubmitInfo submit_info = {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &command_buffer;

	VK_CHECK(
		vkQueueSubmit(graphics_queue, 1, &submit_info, VK_NULL_HANDLE),
		"Failed to submit queue 1"
	);

	VK_CHECK(
		vkQueueWaitIdle(graphics_queue),
		"Failed to wait for queue"
	);

	vkFreeCommandBuffers(device, command_pool, 1, &command_buffer);
}

void VulkanAPI::createBuffer(
	VkDeviceSize size,
	VkBufferUsageFlags usage,
	VkMemoryPropertyFlags properties,
	VkBuffer & buffer,
	VkDeviceMemory & buffer_memory
)
{
	VkBufferCreateInfo buffer_info = {};
	buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_info.size = size;
	buffer_info.usage = usage;
	buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VK_CHECK(
		vkCreateBuffer(device, &buffer_info, nullptr, &buffer),
		"Failed to create buffer"
	);

	VkMemoryRequirements mem_requirements;
	vkGetBufferMemoryRequirements(device, buffer, &mem_requirements);

	VkMemoryAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.allocationSize = mem_requirements.size;
	alloc_info.memoryTypeIndex = findMemoryType(mem_requirements.memoryTypeBits, properties);

	VK_CHECK(
		vma.allocateMemory(device, &alloc_info, nullptr, &buffer_memory),
		"Failed to allocate buffer memory"
	);

	vkBindBufferMemory(device, buffer, buffer_memory, 0);
}

void VulkanAPI::copyBuffer(
	VkBuffer src_buffer,
	VkBuffer dst_buffer,
	VkDeviceSize size
)
{
	VkCommandBufferAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	alloc_info.commandPool = command_pool;
	alloc_info.commandBufferCount = 1;

	VkCommandBuffer command_buffer;
	VK_CHECK(
		vkAllocateCommandBuffers(device, &alloc_info, &command_buffer),
		"Failed to allocate command buffers"
	);

	VkCommandBufferBeginInfo begin_info = {};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	VK_CHECK(
		vkBeginCommandBuffer(command_buffer, &begin_info),
		"Failed to begin recording command buffer"
	);

	VkBufferCopy copy_region = {};
	copy_region.size = size;

	vkCmdCopyBuffer(command_buffer, src_buffer, dst_buffer, 1, &copy_region);

	VK_CHECK(
		vkEndCommandBuffer(command_buffer),
		"Failed to end recording command buffer"
	);

	VkSubmitInfo submit_info = {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &command_buffer;

	VK_CHECK(
		vkQueueSubmit(graphics_queue, 1, &submit_info, VK_NULL_HANDLE),
		"Failed to submit queue 2"
	);

	VK_CHECK(
		vkQueueWaitIdle(graphics_queue),
		"Failed to wait for queue"
	);

	vkFreeCommandBuffers(device, command_pool, 1, &command_buffer);
}

void VulkanAPI::copyBufferToImage(
	VkBuffer buffer,
	VkImage image,
	uint32_t width,
	uint32_t height
)
{
	VkCommandBufferAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	alloc_info.commandPool = command_pool;
	alloc_info.commandBufferCount = 1;

	VkCommandBuffer command_buffer;
	VK_CHECK(
		vkAllocateCommandBuffers(device, &alloc_info, &command_buffer),
		"Failed to allocate command buffers"
	);

	VkCommandBufferBeginInfo begin_info = {};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	VK_CHECK(
		vkBeginCommandBuffer(command_buffer, &begin_info),
		"Failed to begin recording command buffer"
	);

	VkBufferImageCopy region = {};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageOffset = {0, 0, 0};
	region.imageExtent = {width, height, 1};

	vkCmdCopyBufferToImage(
		command_buffer,
		buffer,
		image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1,
		&region
	);

	VK_CHECK(
		vkEndCommandBuffer(command_buffer),
		"Failed to end recording command buffer"
	);

	VkSubmitInfo submit_info = {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &command_buffer;

	VK_CHECK(
		vkQueueSubmit(graphics_queue, 1, &submit_info, VK_NULL_HANDLE),
		"Failed to submit queue 3"
	);

	VK_CHECK(
		vkQueueWaitIdle(graphics_queue),
		"Failed to wait for queue"
	);

	vkFreeCommandBuffers(device, command_pool, 1, &command_buffer);
}

