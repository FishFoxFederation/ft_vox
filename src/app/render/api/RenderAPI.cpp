#include "RenderAPI.hpp"
#include "logger.hpp"
#include "Block.hpp"
#include "Model.hpp"
#include "ObjLoader.hpp"
#include "Item.hpp"

#include "Tracy.hpp"

#include <stb_image.h>

#include <cstring>
#include <set>
#include <algorithm>
#include <map>

RenderAPI::RenderAPI(GLFWwindow * window):
	window(window),
	shadow_map_size(4096)
{
	ZoneScoped;

	_createInstance();
	_loadVulkanFunctions();
	_setupDebugMessenger();
	_createSurface(window);
	_pickPhysicalDevice();
	_createLogicalDevice();
	_createSwapChain(window);
	_createCommandPool();
	_createCommandBuffer();
	_createSyncObjects();

	_createColorAttachement();
	_createDepthAttachement();
	_createShadowMapRessources();
	_createUniformBuffers();
	_createTextureArray(BlockInfo::texture_names, 64);
	_createCubeMap({
		"assets/textures/skybox/right.jpg",
		"assets/textures/skybox/left.jpg",
		"assets/textures/skybox/top.jpg",
		"assets/textures/skybox/bottom.jpg",
		"assets/textures/skybox/front.jpg",
		"assets/textures/skybox/back.jpg"
	}, 512);
	_createTextureImage();
	_createInstanceData();
	_setupChunksRessources();
	_createDrawBuffer();

	_createDescriptors();
	_createGlobalDescriptor();
	_createRenderPass();
	_createPipelines();
	_createFramebuffers();

	_createMeshes();
	_createItemMeshes();

	_setupTextRenderer();

	_prerenderItemIconImages();

	_setupImgui();
	_setupTracy();

	_createRenderFrameRessources();

	LOG_INFO("RenderAPI initialized");
}

RenderAPI::~RenderAPI()
{
	ZoneScoped;

	vkDeviceWaitIdle(device);

	VulkanMemoryAllocator & vma = VulkanMemoryAllocator::getInstance();

	_destroyTracy();

	_destroyImGuiTextures();

	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	_destroyTextRenderer();

	_destroyInstanceData();
	_destroyDrawBuffer();

	{
		std::lock_guard lock(mesh_map_mutex);
		for (auto & [key, mesh] : mesh_map)
		{
			vkDestroyBuffer(device, mesh.buffer, nullptr);
			vma.freeMemory(device, mesh.buffer_memory, nullptr);
		}
	}

	m_chunks_in_scene.clear();
	m_chunks_indices_buffer.clear();

	for (int i = 0; i < m_max_frames_in_flight; i++)
	{
		vkUnmapMemory(device, camera_ubo.memory[i]);
		vma.freeMemory(device, camera_ubo.memory[i], nullptr);
		vkDestroyBuffer(device, camera_ubo.buffers[i], nullptr);

		vkUnmapMemory(device, light_mat_ubo.memory[i]);
		vma.freeMemory(device, light_mat_ubo.memory[i], nullptr);
		vkDestroyBuffer(device, light_mat_ubo.buffers[i], nullptr);

		vkUnmapMemory(device, atmosphere_ubo.memory[i]);
		vma.freeMemory(device, atmosphere_ubo.memory[i], nullptr);
		vkDestroyBuffer(device, atmosphere_ubo.buffers[i], nullptr);
	}

	_destroyFramebuffers();

	vkDestroyRenderPass(device, lighting_render_pass, nullptr);
	vkDestroyRenderPass(device, shadow_render_pass, nullptr);
	vkDestroyRenderPass(device, water_render_pass, nullptr);
	vkDestroyRenderPass(device, hud_render_pass, nullptr);
	vkDestroyRenderPass(device, prerender_item_icon_render_pass, nullptr);

	vkDestroyDescriptorPool(device, imgui_descriptor_pool, nullptr);

	for (int i = 0; i < m_max_frames_in_flight; i++)
	{
		vkDestroySemaphore(device, image_available_semaphores[i], nullptr);
		vkDestroySemaphore(device, shadow_pass_finished_semaphores[i], nullptr);
		vkDestroySemaphore(device, main_render_finished_semaphores[i], nullptr);
		vkDestroySemaphore(device, compute_finished_semaphores[i], nullptr);
		vkDestroySemaphore(device, copy_finished_semaphores[i], nullptr);
		vkDestroySemaphore(device, imgui_render_finished_semaphores[i], nullptr);
		vkDestroyFence(device, in_flight_fences[i], nullptr);
	}
	vkDestroyFence(device, single_time_command_fence, nullptr);

	vkFreeCommandBuffers(device, command_pool, static_cast<uint32_t>(draw_shadow_pass_command_buffers.size()), draw_shadow_pass_command_buffers.data());
	vkFreeCommandBuffers(device, command_pool, static_cast<uint32_t>(draw_command_buffers.size()), draw_command_buffers.data());
	vkFreeCommandBuffers(device, command_pool, static_cast<uint32_t>(copy_command_buffers.size()), copy_command_buffers.data());
	vkFreeCommandBuffers(device, command_pool, static_cast<uint32_t>(compute_command_buffers.size()), compute_command_buffers.data());
	vkFreeCommandBuffers(device, command_pool, static_cast<uint32_t>(imgui_command_buffers.size()), imgui_command_buffers.data());
	vkDestroyCommandPool(device, command_pool, nullptr);
	vkFreeCommandBuffers(device, transfer_command_pool, 1, &transfer_command_buffers);
	vkDestroyCommandPool(device, transfer_command_pool, nullptr);

	_destroyShadowMapRessources();

	// Bad design, but better than nothing until I find a better solution
	{
		output_attachement.clear();
		color_attachement.clear();
		depth_attachement.clear();
		shadow_map_depth_attachement.clear();
		block_textures.clear();
		skybox_cube_map.clear();
		crosshair_image.clear();
		toolbar_image.clear();
		toolbar_cursor_image.clear();
		player_skin_image.clear();
		debug_info_image.clear();
		item_icon_images.clear();

		debug_info_buffers.clear();

		global_descriptor.clear();
		crosshair_image_descriptor.clear();
		toolbar_image_descriptor.clear();
		toolbar_cursor_image_descriptor.clear();
		debug_info_image_descriptor.clear();

		chunk_pipeline.clear();
		water_pipeline.clear();
		line_pipeline.clear();
		skybox_pipeline.clear();
		sun_pipeline.clear();
		shadow_pipeline.clear();
		test_image_pipeline.clear();
		entity_pipeline.clear();
		player_pipeline.clear();
		hud_pipeline.clear();
		prerender_item_icon_pipeline.clear();
		item_icon_pipeline.clear();

		swapchain.clear();
	}

	vkDestroyDevice(device, nullptr);

	#ifndef NDEBUG
		vkDestroyDebugUtilsMessengerEXT(instance, debug_messenger, nullptr);
	#endif

	vkDestroySurfaceKHR(instance, surface, nullptr);

	vkDestroyInstance(instance, nullptr);
}

void RenderAPI::_createInstance()
{
	#ifndef NDEBUG
		if (!_checkValidationLayerSupport())
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

	std::vector<const char *> extensions = _getRequiredExtensions();
	create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	create_info.ppEnabledExtensionNames = extensions.data();

	VkDebugUtilsMessengerCreateInfoEXT debug_create_info;
	_populateDebugMessengerCreateInfo(debug_create_info);
	#ifndef NDEBUG
		create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
		create_info.ppEnabledLayerNames = validation_layers.data();

		// Shader printf is a feature of the validation layers that needs to be enabled
		// std::vector<VkValidationFeatureEnableEXT>  validation_feature_enables = { VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT };

		// VkValidationFeaturesEXT validation_features = { VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT };
		// validation_features.enabledValidationFeatureCount = static_cast<uint32_t>(validation_feature_enables.size());
		// validation_features.pEnabledValidationFeatures = validation_feature_enables.data();

		// debug_create_info.pNext = &validation_features;
		create_info.pNext = &debug_create_info;
	#else
		create_info.enabledLayerCount = 0;
	#endif

	VK_CHECK(
		vkCreateInstance(&create_info, nullptr, &instance),
		"Failed to create instance"
	);
}

bool RenderAPI::_checkValidationLayerSupport()
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

std::vector<const char *> RenderAPI::_getRequiredExtensions()
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

void RenderAPI::_loadVulkanFunctions()
{
#define VK_LOAD_FUNCTION(name) name = reinterpret_cast<PFN_##name>(vkGetInstanceProcAddr(instance, #name)); \
	if (name == nullptr) throw std::runtime_error("Failed to load Vulkan function: " #name);

	#ifndef NDEBUG
		VK_LOAD_FUNCTION(vkCreateDebugUtilsMessengerEXT)
		VK_LOAD_FUNCTION(vkDestroyDebugUtilsMessengerEXT)
	#endif

	VK_LOAD_FUNCTION(vkGetPhysicalDeviceCalibrateableTimeDomainsEXT)
	VK_LOAD_FUNCTION(vkGetCalibratedTimestampsEXT)

	VK_LOAD_FUNCTION(vkGetBufferDeviceAddress)

#undef VK_LOAD_FUNCTION
}

void RenderAPI::_setupDebugMessenger()
{
	#ifndef NDEBUG
		VkDebugUtilsMessengerCreateInfoEXT create_info;
		_populateDebugMessengerCreateInfo(create_info);

		VK_CHECK(
			vkCreateDebugUtilsMessengerEXT(instance, &create_info, nullptr, &debug_messenger),
			"Failed to set up debug messenger"
		);
	#endif
}

void RenderAPI::_populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT & create_info)
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

VKAPI_ATTR VkBool32 VKAPI_CALL RenderAPI::debugCallback(
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

void RenderAPI::_createSurface(GLFWwindow * window)
{
	if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create window surface");
	}
}

void RenderAPI::_pickPhysicalDevice()
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
		int score = _ratePhysicalDevice(device);
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

	queue_family_indices = _findQueueFamilies(physical_device);

	VkPhysicalDeviceProperties device_properties;
	vkGetPhysicalDeviceProperties(physical_device, &device_properties);

	LOG_INFO("device name: " << device_properties.deviceName);
}

bool RenderAPI::_isDeviceSuitable(VkPhysicalDevice device)
{
	QueueFamilyIndices indices = _findQueueFamilies(device);

	bool extensions_supported = _checkDeviceExtensionSupport(device);

	bool swapchain_adequate = false;
	if (extensions_supported)
	{
		Swapchain::SupportDetails swapchain_support = Swapchain::querySwapChainSupport(device, surface);
		swapchain_adequate = !swapchain_support.formats.empty() && !swapchain_support.present_modes.empty();
	}

	VkPhysicalDeviceFeatures supported_features;
	vkGetPhysicalDeviceFeatures(device, &supported_features);

	return indices.isComplete()
		&& extensions_supported
		&& swapchain_adequate
		&& supported_features.samplerAnisotropy;
}

int RenderAPI::_ratePhysicalDevice(VkPhysicalDevice device)
{
	if (!_isDeviceSuitable(device))
	{
		return 0;
	}

	int score = 1;

	VkPhysicalDeviceProperties device_properties;
	vkGetPhysicalDeviceProperties(device, &device_properties);

	// VkPhysicalDeviceFeatures device_features;
	// vkGetPhysicalDeviceFeatures(device, &device_features);

	if (device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
		score += 1000;
	}

	return score;
}

QueueFamilyIndices RenderAPI::_findQueueFamilies(VkPhysicalDevice device)
{
	QueueFamilyIndices indices;

	uint32_t queue_family_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);
	std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());

	std::vector<uint32_t> queue_families_use_count(queue_family_count, 0);

	int i = 0;
	for (const auto & queue_family : queue_families)
	{
		// Find a queue family that supports graphics
		if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT && queue_families_use_count[i] < queue_family.queueCount)
		{
			indices.graphics_family = i;
			queue_families_use_count[i]++;
		}

		// Find a queue family that supports presentation
		VkBool32 present_support = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &present_support);
		if (present_support && queue_families_use_count[i] < queue_family.queueCount)
		{
			indices.present_family = i;
			queue_families_use_count[i]++;
		}

		// Find a queue family that supports transfer
		if (queue_family.queueFlags & VK_QUEUE_TRANSFER_BIT && queue_families_use_count[i] < queue_family.queueCount)
		{
			indices.transfer_family = i;
			queue_families_use_count[i]++;
		}

		if (indices.isComplete())
		{
			break;
		}

		i++;
	}

	return indices;
}

bool RenderAPI::_checkDeviceExtensionSupport(VkPhysicalDevice device)
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

	for (const auto & extension : required_extensions)
	{
		LOG_ERROR("Missing extension: " << extension);
	}

	return required_extensions.empty();
}

void RenderAPI::_createLogicalDevice()
{
	std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
	std::set<uint32_t> unique_queue_families = {
		queue_family_indices.graphics_family.value(),
		queue_family_indices.present_family.value(),
		queue_family_indices.transfer_family.value()
	};
	std::map<uint32_t, std::vector<float>> queue_priorities;

	for (uint32_t queue_family : unique_queue_families)
	{
		VkDeviceQueueCreateInfo queue_create_info = {};

		if (queue_family == queue_family_indices.graphics_family.value())
		{
			queue_create_info.queueCount += 1;
		}
		if (queue_family == queue_family_indices.present_family.value())
		{
			queue_create_info.queueCount += 1;
		}
		if (queue_family == queue_family_indices.transfer_family.value())
		{
			queue_create_info.queueCount += 1;
		}
		queue_priorities.insert({queue_family, std::vector<float>(queue_create_info.queueCount, 1.0f)});

		queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queue_create_info.queueFamilyIndex = queue_family;
		queue_create_info.pQueuePriorities = queue_priorities[queue_family].data();
		queue_create_infos.push_back(queue_create_info);
	}




	VkPhysicalDeviceFeatures device_features = {};
	device_features.samplerAnisotropy = VK_TRUE;
	device_features.fillModeNonSolid = VK_TRUE;
	device_features.wideLines = VK_TRUE;
	device_features.shaderInt64 = VK_TRUE;
	device_features.geometryShader = VK_TRUE;
	device_features.multiDrawIndirect = VK_TRUE;

	VkPhysicalDeviceShaderDrawParametersFeatures shader_draw_parameters_features = {};
	shader_draw_parameters_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETERS_FEATURES;
	shader_draw_parameters_features.shaderDrawParameters = VK_TRUE;

	VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamic_rendering_features = {};
	dynamic_rendering_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR;
	dynamic_rendering_features.dynamicRendering = VK_TRUE;
	dynamic_rendering_features.pNext = &shader_draw_parameters_features;

	VkPhysicalDeviceVulkan12Features vulkan_12_features = {};
	vulkan_12_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
	vulkan_12_features.bufferDeviceAddress = VK_TRUE;
	vulkan_12_features.descriptorIndexing = VK_TRUE;
	vulkan_12_features.shaderInt8 = VK_TRUE;
	vulkan_12_features.storageBuffer8BitAccess = VK_TRUE;
	vulkan_12_features.pNext = &dynamic_rendering_features;


	VkDeviceCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	create_info.pQueueCreateInfos = queue_create_infos.data();
	create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
	create_info.pEnabledFeatures = &device_features;
	create_info.enabledExtensionCount = static_cast<uint32_t>(device_extensions.size());
	create_info.ppEnabledExtensionNames = device_extensions.data();

	create_info.pNext = &vulkan_12_features;

	#ifndef NDEBUG
		create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
		create_info.ppEnabledLayerNames = validation_layers.data();
	#else
		create_info.enabledLayerCount = 0;
	#endif


	VK_CHECK(
		vkCreateDevice(physical_device, &create_info, nullptr, &device),
		"Failed to create logical device"
	);

	std::map<uint32_t, uint32_t> queues_indices = {
		{queue_family_indices.graphics_family.value(), 0},
		{queue_family_indices.present_family.value(), 0},
		{queue_family_indices.transfer_family.value(), 0}
	};

	vkGetDeviceQueue(device, queue_family_indices.graphics_family.value(), queues_indices[queue_family_indices.graphics_family.value()]++, &graphics_queue);
	vkGetDeviceQueue(device, queue_family_indices.present_family.value(), queues_indices[queue_family_indices.present_family.value()]++, &present_queue);
	vkGetDeviceQueue(device, queue_family_indices.transfer_family.value(), queues_indices[queue_family_indices.transfer_family.value()]++, &transfer_queue);
}

void RenderAPI::_createSwapChain(GLFWwindow * window)
{
	Swapchain::SupportDetails swapchain_support = Swapchain::querySwapChainSupport(physical_device, surface);

	Swapchain::CreateInfo create_info = {};
	create_info.surface_format = _chooseSwapSurfaceFormat(swapchain_support.formats);
	create_info.present_mode = _chooseSwapPresentMode(swapchain_support.present_modes);
	create_info.extent = _chooseSwapExtent(swapchain_support.capabilities, window);
	create_info.old_swapchain = swapchain.swapchain;

	swapchain = Swapchain(device, physical_device, surface, create_info);
}

void RenderAPI::_recreateSwapChain(GLFWwindow * window)
{
	int width = 0, height = 0;
	glfwGetFramebufferSize(window, &width, &height);
	while (width == 0 || height == 0)
	{
		glfwGetFramebufferSize(window, &width, &height);
		glfwWaitEvents();
	}

	vkDeviceWaitIdle(device);

	_destroyFramebuffers();
	_destroyShadowMapRessources();

	_createSwapChain(window);
	_createColorAttachement();
	_createDepthAttachement();
	_createShadowMapRessources();
	_updateGlobalDescriptor();
	_createPipelines();
	_createFramebuffers();
}

VkSurfaceFormatKHR RenderAPI::_chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> & available_formats)
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

VkPresentModeKHR RenderAPI::_chooseSwapPresentMode(const std::vector<VkPresentModeKHR> & available_present_modes)
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

VkExtent2D RenderAPI::_chooseSwapExtent(const VkSurfaceCapabilitiesKHR & capabilities, GLFWwindow * window)
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

void RenderAPI::_createCommandPool()
{
	QueueFamilyIndices queue_family_indices = _findQueueFamilies(physical_device);

	VkCommandPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	pool_info.queueFamilyIndex = queue_family_indices.graphics_family.value();
	pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	VK_CHECK(
		vkCreateCommandPool(device, &pool_info, nullptr, &command_pool),
		"Failed to create command pool"
	);

	pool_info.queueFamilyIndex = queue_family_indices.transfer_family.value();

	VK_CHECK(
		vkCreateCommandPool(device, &pool_info, nullptr, &transfer_command_pool),
		"Failed to create command pool"
	);
}

void RenderAPI::_createCommandBuffer()
{
	draw_shadow_pass_command_buffers.resize(m_max_frames_in_flight);
	draw_command_buffers.resize(m_max_frames_in_flight);
	copy_command_buffers.resize(m_max_frames_in_flight);
	compute_command_buffers.resize(m_max_frames_in_flight);
	imgui_command_buffers.resize(m_max_frames_in_flight);

	VkCommandBufferAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	alloc_info.commandPool = command_pool;
	alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	alloc_info.commandBufferCount = static_cast<uint32_t>(m_max_frames_in_flight);

	VK_CHECK(
		vkAllocateCommandBuffers(device, &alloc_info, draw_shadow_pass_command_buffers.data()),
		"Failed to allocate command buffers"
	);
	VK_CHECK(
		vkAllocateCommandBuffers(device, &alloc_info, draw_command_buffers.data()),
		"Failed to allocate command buffers"
	);
	VK_CHECK(
		vkAllocateCommandBuffers(device, &alloc_info, copy_command_buffers.data()),
		"Failed to allocate command buffers"
	);
	VK_CHECK(
		vkAllocateCommandBuffers(device, &alloc_info, imgui_command_buffers.data()),
		"Failed to allocate command buffers"
	);
	VK_CHECK(
		vkAllocateCommandBuffers(device, &alloc_info, compute_command_buffers.data()),
		"Failed to allocate command buffers"
	);

	alloc_info.commandPool = transfer_command_pool;
	alloc_info.commandBufferCount = 1;

	VK_CHECK(
		vkAllocateCommandBuffers(device, &alloc_info, &transfer_command_buffers),
		"Failed to allocate command buffers"
	);
}

void RenderAPI::_createSyncObjects()
{
	image_available_semaphores.resize(m_max_frames_in_flight);
	shadow_pass_finished_semaphores.resize(m_max_frames_in_flight);
	main_render_finished_semaphores.resize(m_max_frames_in_flight);
	compute_finished_semaphores.resize(m_max_frames_in_flight);
	copy_finished_semaphores.resize(m_max_frames_in_flight);
	imgui_render_finished_semaphores.resize(m_max_frames_in_flight);
	in_flight_fences.resize(m_max_frames_in_flight);

	VkSemaphoreCreateInfo semaphore_info = {};
	semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fence_info = {};
	fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < static_cast<size_t>(m_max_frames_in_flight); i++)
	{
		VK_CHECK(
			vkCreateSemaphore(device, &semaphore_info, nullptr, &image_available_semaphores[i]),
			"Failed to create semaphores"
		);
		VK_CHECK(
			vkCreateSemaphore(device, &semaphore_info, nullptr, &shadow_pass_finished_semaphores[i]),
			"Failed to create semaphores"
		);
		VK_CHECK(
			vkCreateSemaphore(device, &semaphore_info, nullptr, &main_render_finished_semaphores[i]),
			"Failed to create semaphores"
		);
		VK_CHECK(
			vkCreateSemaphore(device, &semaphore_info, nullptr, &compute_finished_semaphores[i]),
			"Failed to create semaphores"
		);
		VK_CHECK(
			vkCreateSemaphore(device, &semaphore_info, nullptr, &copy_finished_semaphores[i]),
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

void RenderAPI::_createColorAttachement()
{
	SingleTimeCommand command_buffer(device, command_pool, graphics_queue);


	Image::CreateInfo color_attachement_info = {};
	color_attachement_info.extent = { swapchain.extent.width * 2, swapchain.extent.height * 2 };
	color_attachement_info.format = swapchain.image_format;
	color_attachement_info.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
								| VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT
								| VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	color_attachement_info.memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	color_attachement_info.final_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	color_attachement_info.create_view = true;

	color_attachement = Image(device, physical_device, command_buffer, color_attachement_info);


	color_attachement_info.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
								| VK_IMAGE_USAGE_TRANSFER_SRC_BIT
								| VK_IMAGE_USAGE_TRANSFER_DST_BIT;

	output_attachement = Image(device, physical_device, command_buffer, color_attachement_info);
}

void RenderAPI::_createDepthAttachement()
{
	SingleTimeCommand command_buffer(device, command_pool, graphics_queue);


	Image::CreateInfo depth_attachement_info = {};
	depth_attachement_info.extent = { swapchain.extent.width * 2, swapchain.extent.height * 2 };
	depth_attachement_info.aspect_mask = VK_IMAGE_ASPECT_DEPTH_BIT;
	depth_attachement_info.format = _findSupportedFormat(
		{VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);
	depth_attachement_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
	depth_attachement_info.memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	depth_attachement_info.final_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	depth_attachement_info.create_view = true;

	depth_attachement = Image(device, physical_device, command_buffer, depth_attachement_info);
}

void RenderAPI::_createShadowMapRessources()
{
	SingleTimeCommand command_buffer(device, command_pool, graphics_queue);

	Image::CreateInfo depth_attachement_info = {};
	depth_attachement_info.extent = { shadow_map_size, shadow_map_size };
	depth_attachement_info.aspect_mask = VK_IMAGE_ASPECT_DEPTH_BIT;
	depth_attachement_info.format = _findSupportedFormat(
		{VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);
	depth_attachement_info.memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	depth_attachement_info.final_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	depth_attachement_info.create_view = true;
	depth_attachement_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	depth_attachement_info.create_sampler = true;
	depth_attachement_info.sampler_address_mode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	depth_attachement_info.sampler_filter = VK_FILTER_LINEAR;
	depth_attachement_info.sampler_anisotropy_enable = VK_FALSE;
	depth_attachement_info.array_layers = shadow_maps_count;

	shadow_map_depth_attachement = Image(device, physical_device, command_buffer, depth_attachement_info);

	shadow_map_views.resize(shadow_maps_count);
	for (size_t i = 0; i < shadow_maps_count; i++)
	{
		VkImageViewCreateInfo view_info = {};
		view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		view_info.image = shadow_map_depth_attachement.image;
		view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		view_info.format = depth_attachement_info.format;
		view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		view_info.subresourceRange.baseMipLevel = 0;
		view_info.subresourceRange.levelCount = 1;
		view_info.subresourceRange.baseArrayLayer = static_cast<uint32_t>(i);
		view_info.subresourceRange.layerCount = 1;

		VK_CHECK(
			vkCreateImageView(device, &view_info, nullptr, &shadow_map_views[i]),
			"Failed to create image view"
		);
	}
}

void RenderAPI::_destroyShadowMapRessources()
{
	for (size_t i = 0; i < shadow_maps_count; i++)
	{
		vkDestroyImageView(device, shadow_map_views[i], nullptr);
	}
	shadow_map_depth_attachement.clear();
}

void RenderAPI::_createUBO(UBO & ubo, const VkDeviceSize size, const uint32_t count)
{
	ubo.buffers.resize(count);
	ubo.memory.resize(count);
	ubo.mapped_memory.resize(count);

	for (uint32_t i = 0; i < count; i++)
	{
		_createBuffer(
			size,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			ubo.buffers[i],
			ubo.memory[i]
		);

		VK_CHECK(
			vkMapMemory(device, ubo.memory[i], 0, size, 0, &ubo.mapped_memory[i]),
			"Failed to map memory for camera uniform buffer."
		);
	}
}

void RenderAPI::_createUniformBuffers()
{
	_createUBO(camera_ubo, sizeof(ViewProjMatrices), m_max_frames_in_flight);
	_createUBO(atmosphere_ubo, sizeof(AtmosphereParams), m_max_frames_in_flight);
	_createUBO(light_mat_ubo, sizeof(ShadowMapLight) * shadow_maps_count, m_max_frames_in_flight);
}

void RenderAPI::_createTextureArray(const std::vector<std::string> & file_paths, uint32_t size)
{
	Image::CreateInfo image_info = {};
	image_info.extent = {size, size};
	image_info.mip_levels = static_cast<uint32_t>(std::floor(std::log2(size))) + 1;
	image_info.format = VK_FORMAT_R8G8B8A8_SRGB;
	image_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	image_info.memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	image_info.final_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	image_info.create_view = true;
	image_info.create_sampler = true;
	image_info.sampler_filter = VK_FILTER_NEAREST;
	image_info.file_paths = file_paths;

	SingleTimeCommand command_buffer(device, command_pool, graphics_queue);

	block_textures = Image(device, physical_device, command_buffer, image_info);
}

void RenderAPI::_createCubeMap(const std::array<std::string, 6> & file_paths, uint32_t size)
{
	Image::CreateInfo _image_info = {};
	_image_info.extent = {size, size};
	_image_info.format = VK_FORMAT_R8G8B8A8_SRGB;
	_image_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	_image_info.memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	_image_info.final_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	_image_info.create_view = true;
	_image_info.create_sampler = true;
	_image_info.file_paths = {
		file_paths[0], file_paths[1], file_paths[2], file_paths[3], file_paths[4], file_paths[5]
	};
	_image_info.is_cube_map = true;

	SingleTimeCommand command_buffer(device, command_pool, graphics_queue);

	skybox_cube_map = Image(device, physical_device, command_buffer, _image_info);
}

void RenderAPI::_createHudImages(
	const std::string & file_path,
	Image & image
)
{
	Image::CreateInfo image_info = {};
	image_info.file_paths = {file_path};
	image_info.format = VK_FORMAT_R8G8B8A8_SRGB;
	image_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	image_info.memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	image_info.final_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	image_info.create_view = true;
	image_info.create_sampler = true;

	SingleTimeCommand command_buffer(device, command_pool, graphics_queue);

	image = Image(device, physical_device, command_buffer, image_info);
}

void RenderAPI::_createTextureImage()
{
	{ // player skin
		Image::CreateInfo image_info = {};
		image_info.file_paths = {"assets/textures/skin/player/steve.png"};
		image_info.extent = {64, 64};
		image_info.format = VK_FORMAT_R8G8B8A8_SRGB;
		image_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		image_info.memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		image_info.final_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		image_info.create_view = true;
		image_info.create_sampler = true;
		image_info.sampler_filter = VK_FILTER_NEAREST;

		SingleTimeCommand command_buffer(device, command_pool, graphics_queue);

		player_skin_image = Image(device, physical_device, command_buffer, image_info);
	}

	{ // Hud images
		_createHudImages("assets/textures/hud/crosshair.png", crosshair_image);
		_createHudImages("assets/textures/hud/toolbar.png", toolbar_image);
		_createHudImages("assets/textures/hud/toolbar_cursor.png", toolbar_cursor_image);
	}

	{ // Debug info
		Image::CreateInfo image_info = {};
		image_info.extent = {2048, 512};
		image_info.format = VK_FORMAT_R8G8B8A8_SRGB;
		image_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		image_info.memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		image_info.final_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		image_info.create_view = true;
		image_info.create_sampler = true;

		SingleTimeCommand command_buffer(device, command_pool, graphics_queue);

		debug_info_image = Image(device, physical_device, command_buffer, image_info);

		// create debug info buffer
		debug_info_buffers.resize(m_max_frames_in_flight);
		for (int i = 0; i < m_max_frames_in_flight; i++)
		{
			Buffer::CreateInfo buffer_info = {};
			buffer_info.size = debug_info_image.extent2D.width * debug_info_image.extent2D.height * 4;
			buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			buffer_info.memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

			debug_info_buffers[i] = Buffer(device, physical_device, buffer_info);
		}
	}

	{ // Item icons
		Image::CreateInfo image_info = {};
		image_info.extent = {256, 256};
		image_info.array_layers = g_items_info.count();
		image_info.format = VK_FORMAT_R8G8B8A8_SRGB;
		image_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		image_info.memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		image_info.final_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		image_info.create_view = true;
		image_info.create_sampler = true;
		image_info.sampler_filter = VK_FILTER_NEAREST;

		SingleTimeCommand command_buffer(device, command_pool, graphics_queue);

		item_icon_images = Image(device, physical_device, command_buffer, image_info);
	}
}


void RenderAPI::_createHudDescriptors(
	const Image & image,
	Descriptor & descriptor
)
{
	VkDescriptorSetLayoutBinding sampler_layout_binding = {};
	sampler_layout_binding.binding = 0;
	sampler_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	sampler_layout_binding.descriptorCount = 1;
	sampler_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	sampler_layout_binding.pImmutableSamplers = nullptr;

	Descriptor::CreateInfo descriptor_info = {};
	descriptor_info.bindings = { sampler_layout_binding };
	descriptor_info.descriptor_count = static_cast<uint32_t>(m_max_frames_in_flight);

	descriptor = Descriptor(device, descriptor_info);

	descriptor.update(
		device,
		image.view,
		image.sampler,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	);
}

void RenderAPI::_createDescriptors()
{
	{ // Hud image descriptor
		_createHudDescriptors(crosshair_image, crosshair_image_descriptor);
		_createHudDescriptors(debug_info_image, debug_info_image_descriptor);
		_createHudDescriptors(toolbar_image, toolbar_image_descriptor);
		_createHudDescriptors(toolbar_cursor_image, toolbar_cursor_image_descriptor);
	}
}

void RenderAPI::_createGlobalDescriptor()
{
	// Camera binding
	const VkDescriptorSetLayoutBinding camera_ubo_binding = {
		.binding = CAMERA_MATRICES_BINDING,
		.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_RAYGEN_BIT_KHR
	};

	// Block textures binding
	const VkDescriptorSetLayoutBinding block_texture_binding = {
		.binding = BLOCK_TEXTURES_BINDING,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR
	};

	// Skybox binding
	const VkDescriptorSetLayoutBinding skybox_binding = {
		.binding = SKYBOX_CUBE_MAP_BINDING,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
	};

	// Shadow map binding
	const VkDescriptorSetLayoutBinding shadow_map_binding = {
		.binding = SHADOW_MAP_BINDING,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
	};

	// Water subpass color input attachement
	const VkDescriptorSetLayoutBinding color_attachement_binding = {
		.binding = WATER_RENDERPASS_INPUT_COLOR_ATTACH_BINDING,
		.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
	};

	// Water subpass depth input attachement
	const VkDescriptorSetLayoutBinding depth_attachement_binding = {
		.binding = WATER_RENDERPASS_INPUT_DEPTH_ATTACH_BINDING,
		.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
	};

	// Test image binding
	const VkDescriptorSetLayoutBinding test_image_binding = {
		.binding = TEST_IMAGE_BINDING,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
	};

	// Player skin image binding
	const VkDescriptorSetLayoutBinding player_skin_binding = {
		.binding = PLAYER_SKIN_BINDING,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
	};

	// Sun matrices binding
	const VkDescriptorSetLayoutBinding light_mat_binding = {
		.binding = SUN_MATRICES_BINDING,
		.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT
	};

	// Atmosphere binding
	const VkDescriptorSetLayoutBinding atmosphere_binding = {
		.binding = ATMOSPHERE_PARAM_BINDING,
		.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
	};

	// Item icon binding
	const VkDescriptorSetLayoutBinding item_icon_binding = {
		.binding = ITEM_ICON_TEXTURE_BINDING,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
	};

	// Instance data buffer binding
	const VkDescriptorSetLayoutBinding instance_data_binding = {
		.binding = INSTANCE_DATA_BINDING,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT
	};

	const Descriptor::CreateInfo descriptor_info = {
		.bindings = {
			camera_ubo_binding,
			block_texture_binding,
			skybox_binding,
			shadow_map_binding,
			color_attachement_binding,
			depth_attachement_binding,
			test_image_binding,
			player_skin_binding,
			light_mat_binding,
			atmosphere_binding,
			item_icon_binding,
			instance_data_binding
		},
		.descriptor_count = static_cast<uint32_t>(m_max_frames_in_flight),
		.set_count = static_cast<uint32_t>(m_max_frames_in_flight)
	};

	global_descriptor = Descriptor(device, descriptor_info);

	_updateGlobalDescriptor();
}

void RenderAPI::_updateGlobalDescriptor()
{
	for (int i = 0; i < m_max_frames_in_flight; i++)
	{
		// Camera UBO
		const GlobalDescriptorWrite camera_ubo_write = {
			.dst_binding = CAMERA_MATRICES_BINDING,
			.descriptor_type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.buffer_info = { camera_ubo.buffers[i], 0, sizeof(ViewProjMatrices) }
		};

		// Block textures
		const GlobalDescriptorWrite block_texture_write = {
			.dst_binding = BLOCK_TEXTURES_BINDING,
			.descriptor_type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.image_info = { block_textures.sampler, block_textures.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }
		};

		// Skybox
		const GlobalDescriptorWrite skybox_write = {
			.dst_binding = SKYBOX_CUBE_MAP_BINDING,
			.descriptor_type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.image_info = { skybox_cube_map.sampler, skybox_cube_map.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }
		};

		// Shadow map
		const GlobalDescriptorWrite shadow_map_write = {
			.dst_binding = SHADOW_MAP_BINDING,
			.descriptor_type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.image_info = { shadow_map_depth_attachement.sampler, shadow_map_depth_attachement.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }
		};

		// Water subpass color input attachements
		const GlobalDescriptorWrite color_write = {
			.dst_binding = WATER_RENDERPASS_INPUT_COLOR_ATTACH_BINDING,
			.descriptor_type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
			.image_info = { VK_NULL_HANDLE , color_attachement.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }
		};

		// Water subpass depth input attachements
		const GlobalDescriptorWrite depth_write = {
			.dst_binding = WATER_RENDERPASS_INPUT_DEPTH_ATTACH_BINDING,
			.descriptor_type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
			.image_info = { VK_NULL_HANDLE , depth_attachement.view, VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL }
		};

		// Test image
		const GlobalDescriptorWrite test_image_write = {
			.dst_binding = TEST_IMAGE_BINDING,
			.descriptor_type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.image_info = { shadow_map_depth_attachement.sampler, shadow_map_depth_attachement.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }
		};

		// Player skin image
		const GlobalDescriptorWrite player_skin_write = {
			.dst_binding = PLAYER_SKIN_BINDING,
			.descriptor_type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.image_info = { player_skin_image.sampler, player_skin_image.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }
		};

		// Sun matrices
		const GlobalDescriptorWrite light_mat_write = {
			.dst_binding = SUN_MATRICES_BINDING,
			.descriptor_type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.buffer_info = { light_mat_ubo.buffers[i], 0, sizeof(ShadowMapLight) * shadow_maps_count }
		};

		// Atmosphere
		const GlobalDescriptorWrite atmosphere_write = {
			.dst_binding = ATMOSPHERE_PARAM_BINDING,
			.descriptor_type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.buffer_info = { atmosphere_ubo.buffers[i], 0, sizeof(AtmosphereParams) }
		};

		// Item icon
		const GlobalDescriptorWrite item_icon_write = {
			.dst_binding = ITEM_ICON_TEXTURE_BINDING,
			.descriptor_type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.image_info = { item_icon_images.sampler, item_icon_images.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }
		};

		// Instance data buffer
		const GlobalDescriptorWrite instance_data_write = {
			.dst_binding = INSTANCE_DATA_BINDING,
			.descriptor_type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.buffer_info = { instance_data_buffers[i].buffer, 0, VK_WHOLE_SIZE }
		};


		std::vector<VkWriteDescriptorSet> descriptor_writes;
		const std::vector<GlobalDescriptorWrite> writes = {
			camera_ubo_write,
			block_texture_write,
			skybox_write,
			shadow_map_write,
			color_write,
			depth_write,
			test_image_write,
			player_skin_write,
			light_mat_write,
			atmosphere_write,
			item_icon_write,
			instance_data_write
		};

		for (const auto & write : writes)
		{
			VkWriteDescriptorSet descriptor_write = {};
			descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptor_write.dstSet = global_descriptor.sets[i];
			descriptor_write.dstBinding = write.dst_binding;
			descriptor_write.dstArrayElement = write.dst_array_element;
			descriptor_write.descriptorType = write.descriptor_type;
			descriptor_write.descriptorCount = write.descriptor_count;
			descriptor_write.pImageInfo = &write.image_info;
			descriptor_write.pBufferInfo = &write.buffer_info;

			descriptor_writes.push_back(descriptor_write);
		}

		vkUpdateDescriptorSets(
			device,
			static_cast<uint32_t>(descriptor_writes.size()),
			descriptor_writes.data(),
			0, nullptr
		);
	}
}


void RenderAPI::_createRenderPass()
{
	{ // lighting render pass
		VkAttachmentDescription color_attachement_description = {};
		color_attachement_description.format = color_attachement.format;
		color_attachement_description.samples = VK_SAMPLE_COUNT_1_BIT;
		color_attachement_description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		color_attachement_description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		color_attachement_description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		color_attachement_description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		color_attachement_description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		color_attachement_description.finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

		VkAttachmentDescription depth_attachement_description = {};
		depth_attachement_description.format = depth_attachement.format;
		depth_attachement_description.samples = VK_SAMPLE_COUNT_1_BIT;
		depth_attachement_description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depth_attachement_description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		depth_attachement_description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depth_attachement_description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depth_attachement_description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depth_attachement_description.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription output_attachement_description = {};
		output_attachement_description.format = output_attachement.format;
		output_attachement_description.samples = VK_SAMPLE_COUNT_1_BIT;
		output_attachement_description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		output_attachement_description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		output_attachement_description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		output_attachement_description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		output_attachement_description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		output_attachement_description.finalLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

		std::vector<VkAttachmentDescription> attachments = {
			color_attachement_description,
			depth_attachement_description,
			output_attachement_description
		};


		VkAttachmentReference color_attachement_ref = {};
		color_attachement_ref.attachment = 0;
		color_attachement_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depth_attachement_ref = {};
		depth_attachement_ref.attachment = 1;
		depth_attachement_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;


		VkSubpassDescription opaque_subpass = {};
		opaque_subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		opaque_subpass.colorAttachmentCount = 1;
		opaque_subpass.pColorAttachments = &color_attachement_ref;
		opaque_subpass.pDepthStencilAttachment = &depth_attachement_ref;

		std::vector<VkSubpassDescription> subpasses = {
			opaque_subpass
		};


		VkRenderPassCreateInfo render_pass_info = {};
		render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		render_pass_info.attachmentCount = static_cast<uint32_t>(attachments.size());
		render_pass_info.pAttachments = attachments.data();
		render_pass_info.subpassCount = static_cast<uint32_t>(subpasses.size());
		render_pass_info.pSubpasses = subpasses.data();
		render_pass_info.dependencyCount = 0;
		render_pass_info.pDependencies = nullptr;

		VK_CHECK(
			vkCreateRenderPass(device, &render_pass_info, nullptr, &lighting_render_pass),
			"Failed to create render pass"
		);
	}

	{ // shadow render pass
		VkAttachmentDescription depth_attachement_description = {};
		depth_attachement_description.format = shadow_map_depth_attachement.format;
		depth_attachement_description.samples = VK_SAMPLE_COUNT_1_BIT;
		depth_attachement_description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depth_attachement_description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		depth_attachement_description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depth_attachement_description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depth_attachement_description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depth_attachement_description.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		VkAttachmentReference depth_attachement_ref = {};
		depth_attachement_ref.attachment = 0;
		depth_attachement_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.pDepthStencilAttachment = &depth_attachement_ref;

		std::vector<VkAttachmentDescription> attachments = {
			depth_attachement_description
		};

		VkRenderPassCreateInfo render_pass_info = {};
		render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		render_pass_info.attachmentCount = static_cast<uint32_t>(attachments.size());
		render_pass_info.pAttachments = attachments.data();
		render_pass_info.subpassCount = 1;
		render_pass_info.pSubpasses = &subpass;

		VK_CHECK(
			vkCreateRenderPass(device, &render_pass_info, nullptr, &shadow_render_pass),
			"Failed to create render pass"
		);
	}

	{ // water render pass
		VkAttachmentDescription color_attachement_description = {};
		color_attachement_description.format = color_attachement.format;
		color_attachement_description.samples = VK_SAMPLE_COUNT_1_BIT;
		color_attachement_description.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		color_attachement_description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		color_attachement_description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		color_attachement_description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		color_attachement_description.initialLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		color_attachement_description.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription depth_attachement_description = {};
		depth_attachement_description.format = depth_attachement.format;
		depth_attachement_description.samples = VK_SAMPLE_COUNT_1_BIT;
		depth_attachement_description.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		depth_attachement_description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		depth_attachement_description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depth_attachement_description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depth_attachement_description.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		depth_attachement_description.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription output_attachement_description = {};
		output_attachement_description.format = output_attachement.format;
		output_attachement_description.samples = VK_SAMPLE_COUNT_1_BIT;
		output_attachement_description.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		output_attachement_description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		output_attachement_description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		output_attachement_description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		output_attachement_description.initialLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		output_attachement_description.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		std::vector<VkAttachmentDescription> attachments = {
			color_attachement_description,
			depth_attachement_description,
			output_attachement_description
		};

		VkAttachmentReference input_color_attachement_ref = {};
		input_color_attachement_ref.attachment = 0;
		input_color_attachement_ref.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		VkAttachmentReference input_depth_attachement_ref = {};
		input_depth_attachement_ref.attachment = 1;
		input_depth_attachement_ref.layout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference output_attachement_ref = {};
		output_attachement_ref.attachment = 2;
		output_attachement_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;


		std::vector<VkAttachmentReference> water_subpass_input_attachements = {
			input_color_attachement_ref,
			input_depth_attachement_ref
		};
		VkSubpassDescription water_subpass = {};
		water_subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		water_subpass.colorAttachmentCount = 1;
		water_subpass.pColorAttachments = &output_attachement_ref;
		water_subpass.inputAttachmentCount = static_cast<uint32_t>(water_subpass_input_attachements.size());
		water_subpass.pInputAttachments = water_subpass_input_attachements.data();

		std::vector<VkSubpassDescription> subpasses = {
			water_subpass
		};


		VkRenderPassCreateInfo render_pass_info = {};
		render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		render_pass_info.attachmentCount = static_cast<uint32_t>(attachments.size());
		render_pass_info.pAttachments = attachments.data();
		render_pass_info.subpassCount = static_cast<uint32_t>(subpasses.size());
		render_pass_info.pSubpasses = subpasses.data();
		render_pass_info.dependencyCount = 0;
		render_pass_info.pDependencies = nullptr;

		VK_CHECK(
			vkCreateRenderPass(device, &render_pass_info, nullptr, &water_render_pass),
			"Failed to create render pass"
		);
	}

	{ // hud render pass
		VkAttachmentDescription output_attachement_description = {};
		output_attachement_description.format = output_attachement.format;
		output_attachement_description.samples = VK_SAMPLE_COUNT_1_BIT;
		output_attachement_description.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		output_attachement_description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		output_attachement_description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		output_attachement_description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		output_attachement_description.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		output_attachement_description.finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

		std::vector<VkAttachmentDescription> attachments = {
			output_attachement_description
		};

		VkAttachmentReference output_attachement_ref = {};
		output_attachement_ref.attachment = 0;
		output_attachement_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;


		VkSubpassDescription hud_subpass = {};
		hud_subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		hud_subpass.colorAttachmentCount = 1;
		hud_subpass.pColorAttachments = &output_attachement_ref;

		std::vector<VkSubpassDescription> subpasses = {
			hud_subpass
		};


		VkRenderPassCreateInfo render_pass_info = {};
		render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		render_pass_info.attachmentCount = static_cast<uint32_t>(attachments.size());
		render_pass_info.pAttachments = attachments.data();
		render_pass_info.subpassCount = static_cast<uint32_t>(subpasses.size());
		render_pass_info.pSubpasses = subpasses.data();
		render_pass_info.dependencyCount = 0;
		render_pass_info.pDependencies = nullptr;

		VK_CHECK(
			vkCreateRenderPass(device, &render_pass_info, nullptr, &hud_render_pass),
			"Failed to create render pass"
		);
	}

	{ // item icon render pass
		VkAttachmentDescription color_attachement_description = {};
		color_attachement_description.format = item_icon_images.format;
		color_attachement_description.samples = VK_SAMPLE_COUNT_1_BIT;
		color_attachement_description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		color_attachement_description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		color_attachement_description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		color_attachement_description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		color_attachement_description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		color_attachement_description.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		std::vector<VkAttachmentDescription> attachments = {
			color_attachement_description
		};


		VkAttachmentReference color_attachement_ref = {};
		color_attachement_ref.attachment = 0;
		color_attachement_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;


		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &color_attachement_ref;

		std::vector<VkSubpassDescription> subpasses = {
			subpass
		};


		VkRenderPassCreateInfo render_pass_info = {};
		render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		render_pass_info.attachmentCount = static_cast<uint32_t>(attachments.size());
		render_pass_info.pAttachments = attachments.data();
		render_pass_info.subpassCount = static_cast<uint32_t>(subpasses.size());
		render_pass_info.pSubpasses = subpasses.data();
		render_pass_info.dependencyCount = 0;
		render_pass_info.pDependencies = nullptr;

		VK_CHECK(
			vkCreateRenderPass(device, &render_pass_info, nullptr, &prerender_item_icon_render_pass),
			"Failed to create render pass"
		);
	}
}

void RenderAPI::_createPipelines()
{
	{ // chunk pipeline
		Pipeline::CreateInfo pipeline_info = {};
		pipeline_info.extent = color_attachement.extent2D;
		pipeline_info.vert_path = "shaders/rasterization/chunk_shader.vert.spv";
		pipeline_info.frag_path = "shaders/rasterization/chunk_shader.frag.spv";
		pipeline_info.color_formats = { color_attachement.format };
		pipeline_info.depth_format = depth_attachement.format;
		pipeline_info.descriptor_set_layouts = {
			global_descriptor.layout
		};
		pipeline_info.push_constant_ranges = {
			{ VK_SHADER_STAGE_ALL, 0, sizeof(GlobalPushConstant) }
		};
		pipeline_info.render_pass = lighting_render_pass;

		chunk_pipeline = Pipeline(device, pipeline_info);
	}

	{ // water pipeline
		Pipeline::CreateInfo pipeline_info = {};
		pipeline_info.extent = output_attachement.extent2D;
		pipeline_info.vert_path = "shaders/rasterization/water/water_shader.vert.spv";
		pipeline_info.frag_path = "shaders/rasterization/water/water_shader.frag.spv";
		pipeline_info.color_formats = { output_attachement.format };
		// pipeline_info.depth_format = depth_attachement.format;
		pipeline_info.enable_alpha_blending = true;
		pipeline_info.cull_mode = VK_CULL_MODE_NONE;
		pipeline_info.descriptor_set_layouts = {
			global_descriptor.layout
		};
		pipeline_info.push_constant_ranges = {
			{ VK_SHADER_STAGE_ALL, 0, sizeof(GlobalPushConstant) }
		};
		pipeline_info.render_pass = water_render_pass;

		water_pipeline = Pipeline(device, pipeline_info);
	}

	{ // line pipeline
		Pipeline::CreateInfo pipeline_info = {};
		pipeline_info.extent = color_attachement.extent2D;
		pipeline_info.vert_path = "shaders/rasterization/line_shader.vert.spv";
		pipeline_info.frag_path = "shaders/rasterization/line_shader.frag.spv";
		pipeline_info.binding_description = ObjVertex::getBindingDescription();
		const std::vector<VkVertexInputAttributeDescription> attribute_descriptions = ObjVertex::getAttributeDescriptions();
		const std::vector<VkVertexInputAttributeDescription> attribute_descriptions_2 = {
			attribute_descriptions[0]
		};
		pipeline_info.attribute_descriptions = attribute_descriptions_2;
		pipeline_info.polygon_mode = VK_POLYGON_MODE_LINE;
		pipeline_info.color_formats = { color_attachement.format };
		pipeline_info.depth_format = depth_attachement.format;
		pipeline_info.descriptor_set_layouts = {
			global_descriptor.layout
		};
		pipeline_info.push_constant_ranges = {
			{ VK_SHADER_STAGE_ALL, 0, sizeof(GlobalPushConstant) }
		};
		pipeline_info.render_pass = lighting_render_pass;
		pipeline_info.dynamic_states = { VK_DYNAMIC_STATE_LINE_WIDTH };

		line_pipeline = Pipeline(device, pipeline_info);
	}

	{ // skybox pipeline
		Pipeline::CreateInfo pipeline_info = {};
		pipeline_info.extent = color_attachement.extent2D;
		pipeline_info.vert_path = "shaders/rasterization/sky/box.vert.spv";
		pipeline_info.frag_path = "shaders/rasterization/sky/box.frag.spv";
		pipeline_info.color_formats = { color_attachement.format };
		pipeline_info.depth_format = depth_attachement.format;
		pipeline_info.descriptor_set_layouts = {
			global_descriptor.layout
		};
		pipeline_info.push_constant_ranges = {
			{ VK_SHADER_STAGE_ALL, 0, sizeof(GlobalPushConstant) }
		};
		pipeline_info.render_pass = lighting_render_pass;

		skybox_pipeline = Pipeline(device, pipeline_info);
	}

	{ // sun pipeline
		Pipeline::CreateInfo pipeline_info = {};
		pipeline_info.extent = color_attachement.extent2D;
		pipeline_info.vert_path = "shaders/rasterization/sky/sun.vert.spv";
		pipeline_info.frag_path = "shaders/rasterization/sky/sun.frag.spv";
		pipeline_info.binding_description = ObjVertex::getBindingDescription();
		pipeline_info.attribute_descriptions = ObjVertex::getAttributeDescriptions();
		pipeline_info.color_formats = { color_attachement.format };
		pipeline_info.depth_format = depth_attachement.format;
		pipeline_info.descriptor_set_layouts = {
			global_descriptor.layout
		};
		pipeline_info.push_constant_ranges = {
			{ VK_SHADER_STAGE_ALL, 0, sizeof(GlobalPushConstant) }
		};
		pipeline_info.render_pass = lighting_render_pass;
		pipeline_info.front_face = VK_FRONT_FACE_CLOCKWISE;

		sun_pipeline = Pipeline(device, pipeline_info);
	}

	{ // shadow map pipeline
		Pipeline::CreateInfo pipeline_info = {};
		pipeline_info.extent = shadow_map_depth_attachement.extent2D;
		pipeline_info.vert_path = "shaders/rasterization/shadow/shadow_shader.vert.spv";
		pipeline_info.geom_path = "shaders/rasterization/shadow/shadow_shader.geom.spv";
		pipeline_info.frag_path = "shaders/rasterization/shadow/shadow_shader.frag.spv";
		pipeline_info.cull_mode = VK_CULL_MODE_NONE;
		pipeline_info.depth_format = shadow_map_depth_attachement.format;
		pipeline_info.depth_bias_enable = VK_TRUE;
		pipeline_info.depth_bias_constant_factor = 0.005f;
		pipeline_info.depth_bias_slope_factor = 0.1f;
		pipeline_info.descriptor_set_layouts = {
			global_descriptor.layout
		};
		pipeline_info.push_constant_ranges = {
			{ VK_SHADER_STAGE_ALL, 0, sizeof(GlobalPushConstant) }
		};
		pipeline_info.render_pass = shadow_render_pass;

		shadow_pipeline = Pipeline(device, pipeline_info);
	}

	{ // test image pipeline
		Pipeline::CreateInfo pipeline_info = {};
		// pipeline_info.extent = { output_attachement.extent2D.width / 3, output_attachement.extent2D.height / 3 };
		pipeline_info.extent = { 800, 800 };
		pipeline_info.vert_path = "shaders/rasterization/hud/test_image_shader.vert.spv";
		pipeline_info.frag_path = "shaders/rasterization/hud/test_image_shader.frag.spv";
		pipeline_info.color_formats = { output_attachement.format };
		pipeline_info.depth_format = depth_attachement.format;
		pipeline_info.descriptor_set_layouts = {
			global_descriptor.layout
		};
		pipeline_info.push_constant_ranges = {
			{ VK_SHADER_STAGE_ALL, 0, sizeof(GlobalPushConstant) }
		};
		pipeline_info.render_pass = hud_render_pass;

		test_image_pipeline = Pipeline(device, pipeline_info);
	}

	{ // Entity pipeline
		Pipeline::CreateInfo pipeline_info = {};
		pipeline_info.extent = color_attachement.extent2D;
		pipeline_info.vert_path = "shaders/rasterization/entity_shader.vert.spv";
		pipeline_info.frag_path = "shaders/rasterization/entity_shader.frag.spv";
		pipeline_info.binding_description = EntityVertex::getBindingDescription();
		pipeline_info.attribute_descriptions = EntityVertex::getAttributeDescriptions();
		pipeline_info.color_formats = { color_attachement.format };
		pipeline_info.depth_format = depth_attachement.format;
		pipeline_info.descriptor_set_layouts = {
			global_descriptor.layout
		};
		pipeline_info.push_constant_ranges = {
			{ VK_SHADER_STAGE_ALL, 0, sizeof(GlobalPushConstant) }
		};
		pipeline_info.render_pass = lighting_render_pass;

		entity_pipeline = Pipeline(device, pipeline_info);
	}

	{ // Player pipeline
		Pipeline::CreateInfo pipeline_info = {};
		pipeline_info.extent = color_attachement.extent2D;
		pipeline_info.vert_path = "shaders/rasterization/player_shader.vert.spv";
		pipeline_info.frag_path = "shaders/rasterization/player_shader.frag.spv";
		pipeline_info.binding_description = ObjVertex::getBindingDescription();
		pipeline_info.attribute_descriptions = ObjVertex::getAttributeDescriptions();
		pipeline_info.color_formats = { color_attachement.format };
		pipeline_info.depth_format = depth_attachement.format;
		pipeline_info.descriptor_set_layouts = {
			global_descriptor.layout
		};
		pipeline_info.push_constant_ranges = {
			{ VK_SHADER_STAGE_ALL, 0, sizeof(GlobalPushConstant) }
		};
		pipeline_info.render_pass = lighting_render_pass;

		player_pipeline = Pipeline(device, pipeline_info);
	}

	{ // Hud pipeline
		Pipeline::CreateInfo pipeline_info = {};
		pipeline_info.extent = output_attachement.extent2D;
		pipeline_info.vert_path = "shaders/rasterization/hud/hud_shader.vert.spv";
		pipeline_info.frag_path = "shaders/rasterization/hud/hud_shader.frag.spv";
		pipeline_info.color_formats = { output_attachement.format };
		pipeline_info.descriptor_set_layouts = {
			global_descriptor.layout,
			crosshair_image_descriptor.layout
		};
		pipeline_info.push_constant_ranges = {
			{ VK_SHADER_STAGE_ALL, 0, sizeof(GlobalPushConstant) }
		};
		pipeline_info.render_pass = hud_render_pass;
		pipeline_info.dynamic_states = { VK_DYNAMIC_STATE_VIEWPORT };
		pipeline_info.enable_alpha_blending = true;

		hud_pipeline = Pipeline(device, pipeline_info);
	}

	{ // Prerender item icon pipeline
		Pipeline::CreateInfo pipeline_info = {};
		pipeline_info.extent = item_icon_images.extent2D;
		pipeline_info.vert_path = "shaders/rasterization/misc/prerender_item_icon_shader.vert.spv";
		pipeline_info.geom_path = "shaders/rasterization/misc/prerender_item_icon_shader.geom.spv";
		pipeline_info.frag_path = "shaders/rasterization/misc/prerender_item_icon_shader.frag.spv";
		pipeline_info.binding_description = ItemVertex::getBindingDescription();
		pipeline_info.attribute_descriptions = ItemVertex::getAttributeDescriptions();
		pipeline_info.color_formats = { item_icon_images.format };
		pipeline_info.descriptor_set_layouts = {
			global_descriptor.layout
		};
		pipeline_info.push_constant_ranges = {
			{ VK_SHADER_STAGE_ALL, 0, sizeof(GlobalPushConstant) }
		};
		pipeline_info.render_pass = prerender_item_icon_render_pass;
		pipeline_info.front_face = VK_FRONT_FACE_CLOCKWISE;

		prerender_item_icon_pipeline = Pipeline(device, pipeline_info);
	}

	{ // Item icon pipeline
		Pipeline::CreateInfo pipeline_info = {};
		pipeline_info.extent = output_attachement.extent2D;
		pipeline_info.vert_path = "shaders/rasterization/hud/item_icon_shader.vert.spv";
		pipeline_info.frag_path = "shaders/rasterization/hud/item_icon_shader.frag.spv";
		pipeline_info.color_formats = { output_attachement.format };
		pipeline_info.descriptor_set_layouts = {
			global_descriptor.layout
		};
		pipeline_info.push_constant_ranges = {
			{ VK_SHADER_STAGE_ALL, 0, sizeof(GlobalPushConstant) }
		};
		pipeline_info.render_pass = hud_render_pass;
		pipeline_info.dynamic_states = { VK_DYNAMIC_STATE_VIEWPORT };
		pipeline_info.enable_alpha_blending = true;

		item_icon_pipeline = Pipeline(device, pipeline_info);
	}
}

void RenderAPI::_createFramebuffers()
{
	lighting_framebuffers.resize(m_max_frames_in_flight);
	shadow_framebuffers.resize(m_max_frames_in_flight * shadow_maps_count);
	water_framebuffers.resize(m_max_frames_in_flight);
	hud_framebuffers.resize(m_max_frames_in_flight);

	for (int i = 0; i < m_max_frames_in_flight; i++)
	{
		std::vector<VkImageView> attachments = {
			color_attachement.view,
			depth_attachement.view,
			output_attachement.view
		};

		VkFramebufferCreateInfo framebuffer_info = {};
		framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebuffer_info.renderPass = lighting_render_pass;
		framebuffer_info.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebuffer_info.pAttachments = attachments.data();
		framebuffer_info.width = color_attachement.extent2D.width;
		framebuffer_info.height = color_attachement.extent2D.height;
		framebuffer_info.layers = 1;

		VK_CHECK(
			vkCreateFramebuffer(device, &framebuffer_info, nullptr, &lighting_framebuffers[i]),
			"Failed to create framebuffer"
		);
	}

	for (int i = 0; i < m_max_frames_in_flight; i++)
	{
		for (size_t j = 0; j < shadow_maps_count; j++)
		{
			std::vector<VkImageView> attachments = {
				shadow_map_views[j]
			};

			VkFramebufferCreateInfo framebuffer_info = {};
			framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebuffer_info.renderPass = shadow_render_pass;
			framebuffer_info.attachmentCount = static_cast<uint32_t>(attachments.size());
			framebuffer_info.pAttachments = attachments.data();
			framebuffer_info.width = shadow_map_depth_attachement.extent2D.width;
			framebuffer_info.height = shadow_map_depth_attachement.extent2D.height;
			framebuffer_info.layers = 1;

			VK_CHECK(
				vkCreateFramebuffer(device, &framebuffer_info, nullptr, &shadow_framebuffers[i * shadow_maps_count + j]),
				"Failed to create framebuffer"
			);
		}
	}

	for (int i = 0; i < m_max_frames_in_flight; i++)
	{
		std::vector<VkImageView> attachments = {
			color_attachement.view,
			depth_attachement.view,
			output_attachement.view
		};

		VkFramebufferCreateInfo framebuffer_info = {};
		framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebuffer_info.renderPass = water_render_pass;
		framebuffer_info.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebuffer_info.pAttachments = attachments.data();
		framebuffer_info.width = output_attachement.extent2D.width;
		framebuffer_info.height = output_attachement.extent2D.height;
		framebuffer_info.layers = 1;

		VK_CHECK(
			vkCreateFramebuffer(device, &framebuffer_info, nullptr, &water_framebuffers[i]),
			"Failed to create framebuffer"
		);
	}

	for (int i = 0; i < m_max_frames_in_flight; i++)
	{
		std::vector<VkImageView> attachments = {
			output_attachement.view
		};

		VkFramebufferCreateInfo framebuffer_info = {};
		framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebuffer_info.renderPass = hud_render_pass;
		framebuffer_info.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebuffer_info.pAttachments = attachments.data();
		framebuffer_info.width = output_attachement.extent2D.width;
		framebuffer_info.height = output_attachement.extent2D.height;
		framebuffer_info.layers = 1;

		VK_CHECK(
			vkCreateFramebuffer(device, &framebuffer_info, nullptr, &hud_framebuffers[i]),
			"Failed to create framebuffer"
		);
	}

	{ // item icon framebuffer
		std::vector<VkImageView> attachments = {
			item_icon_images.view
		};

		VkFramebufferCreateInfo framebuffer_info = {};
		framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebuffer_info.renderPass = prerender_item_icon_render_pass;
		framebuffer_info.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebuffer_info.pAttachments = attachments.data();
		framebuffer_info.width = item_icon_images.extent2D.width;
		framebuffer_info.height = item_icon_images.extent2D.height;
		framebuffer_info.layers = item_icon_images.array_layers;

		VK_CHECK(
			vkCreateFramebuffer(device, &framebuffer_info, nullptr, &prerender_item_icon_framebuffer),
			"Failed to create framebuffer"
		);
	}
}

void RenderAPI::_destroyFramebuffers()
{
	for (int i = 0; i < m_max_frames_in_flight; i++)
	{
		vkDestroyFramebuffer(device, lighting_framebuffers[i], nullptr);
		vkDestroyFramebuffer(device, water_framebuffers[i], nullptr);
		vkDestroyFramebuffer(device, hud_framebuffers[i], nullptr);
	}
	for (size_t i = 0; i < shadow_framebuffers.size(); i++)
	{
		vkDestroyFramebuffer(device, shadow_framebuffers[i], nullptr);
	}
	vkDestroyFramebuffer(device, prerender_item_icon_framebuffer, nullptr);
}

void RenderAPI::_createMeshes()
{
	{ // load cube mesh
		ObjLoader obj_loader("assets/models/cube.obj");

		cube_mesh_id = storeMesh(
			obj_loader.vertices().data(),
			obj_loader.vertices().size(),
			sizeof(ObjVertex),
			obj_loader.indices().data(),
			obj_loader.indices().size()
		);
	}

	{ // load icosphere mesh
		ObjLoader obj_loader("assets/models/icosphere.obj");
		icosphere_mesh_id = storeMesh(
			obj_loader.vertices().data(),
			obj_loader.vertices().size(),
			sizeof(ObjVertex),
			obj_loader.indices().data(),
			obj_loader.indices().size()
		);
	}

	{ // load player mesh
		ObjLoader chest_geometry("assets/models/entity/player/chest.obj");
		player_chest_mesh_id = storeMesh(
			chest_geometry.vertices().data(),
			chest_geometry.vertices().size(),
			sizeof(ObjVertex),
			chest_geometry.indices().data(),
			chest_geometry.indices().size()
		);

		ObjLoader head_geometry("assets/models/entity/player/head.obj");
		player_head_mesh_id = storeMesh(
			head_geometry.vertices().data(),
			head_geometry.vertices().size(),
			sizeof(ObjVertex),
			head_geometry.indices().data(),
			head_geometry.indices().size()
		);

		ObjLoader right_leg_geometry("assets/models/entity/player/right_leg.obj");
		player_right_leg_mesh_id = storeMesh(
			right_leg_geometry.vertices().data(),
			right_leg_geometry.vertices().size(),
			sizeof(ObjVertex),
			right_leg_geometry.indices().data(),
			right_leg_geometry.indices().size()
		);

		ObjLoader left_leg_geometry("assets/models/entity/player/left_leg.obj");
		player_left_leg_mesh_id = storeMesh(
			left_leg_geometry.vertices().data(),
			left_leg_geometry.vertices().size(),
			sizeof(ObjVertex),
			left_leg_geometry.indices().data(),
			left_leg_geometry.indices().size()
		);

		ObjLoader right_arm_geometry("assets/models/entity/player/right_arm.obj");
		player_right_arm_mesh_id = storeMesh(
			right_arm_geometry.vertices().data(),
			right_arm_geometry.vertices().size(),
			sizeof(ObjVertex),
			right_arm_geometry.indices().data(),
			right_arm_geometry.indices().size()
		);

		ObjLoader left_arm_geometry("assets/models/entity/player/left_arm.obj");
		player_left_arm_mesh_id = storeMesh(
			left_arm_geometry.vertices().data(),
			left_arm_geometry.vertices().size(),
			sizeof(ObjVertex),
			left_arm_geometry.indices().data(),
			left_arm_geometry.indices().size()
		);
	}
}


void RenderAPI::_setupTextRenderer()
{
	text_renderer.initialize();
}

void RenderAPI::_destroyTextRenderer()
{
	text_renderer.destroy();
}

void RenderAPI::_writeTextToDebugImage(
	VkCommandBuffer command_buffer,
	const std::string & text,
	const uint32_t x,
	const uint32_t y,
	const uint32_t font_size
)
{
	const VkDeviceSize image_size = debug_info_image.extent2D.width * debug_info_image.extent2D.height * 4;

	Buffer & staging_buffer = debug_info_buffers[m_current_frame];

	memset(staging_buffer.mappedMemory(), 0, image_size);

	text_renderer.renderText(
		text,
		x, y,
		font_size,
		staging_buffer.mappedMemory(),
		debug_info_image.extent2D.width,
		debug_info_image.extent2D.height
	);

	VkBufferImageCopy region = {};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = { debug_info_image.extent2D.width, debug_info_image.extent2D.height, 1 };

	debug_info_image.transitionLayout(
		command_buffer,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
	);

	vkCmdCopyBufferToImage(
		command_buffer,
		staging_buffer.buffer,
		debug_info_image.image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1,
		&region
	);

	debug_info_image.transitionLayout(
		command_buffer,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	);
}


uint64_t RenderAPI::createImGuiTexture(const uint32_t width, const uint32_t height)
{
	ImGuiTexture imgui_texture;

	imgui_texture.extent = { width, height };
	imgui_texture.format = VK_FORMAT_R8G8B8A8_SRGB;

	_createImage(
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

	_createImageView(
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

	_transitionImageLayout(
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

	const uint64_t id = next_imgui_texture_id++;
	{
		std::lock_guard lock(imgui_textures_mutex);
		imgui_textures[id] = imgui_texture;
	}

	return id;
}

void RenderAPI::_destroyImGuiTextures()
{
	std::lock_guard lock(imgui_textures_mutex);
	for (auto & [id, texture] : imgui_textures)
	{
		vkUnmapMemory(device, texture.memory);
		ImGui_ImplVulkan_RemoveTexture(texture.descriptor_set);
		vkDestroySampler(device, texture.sampler, nullptr);
		vkDestroyImageView(device, texture.view, nullptr);
		VulkanMemoryAllocator & vma = VulkanMemoryAllocator::getInstance();
		vma.freeMemory(device, texture.memory, nullptr);
		vkDestroyImage(device, texture.image, nullptr);
	}
}

void RenderAPI::ImGuiTexturePutPixel(
	const uint64_t texture_id,
	const uint32_t x,
	const uint32_t y,
	const uint8_t r,
	const uint8_t g,
	const uint8_t b,
	const uint8_t a
)
{
	std::lock_guard lock(imgui_textures_mutex);
	ImGuiTexture & imgui_texture = imgui_textures.at(texture_id);
	imgui_texture.putPixel(x, y, r, g, b, a);
}

void RenderAPI::ImGuiTextureClear(const uint64_t texture_id)
{
	std::lock_guard lock(imgui_textures_mutex);
	ImGuiTexture & imgui_texture = imgui_textures.at(texture_id);
	imgui_texture.clear();
}

void RenderAPI::ImGuiTextureDraw(const uint64_t texture_id)
{
	std::lock_guard lock(imgui_textures_mutex);
	ImGuiTexture & imgui_texture = imgui_textures.at(texture_id);
	ImGui::Image(
		(ImTextureID)imgui_texture.descriptor_set,
		ImVec2(imgui_texture.width(), imgui_texture.height())
	);
}


void RenderAPI::_setupImgui()
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
	init_info.ImageCount = static_cast<uint32_t>(swapchain.images.size());
	init_info.UseDynamicRendering = VK_TRUE;
	init_info.PipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
	init_info.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
	init_info.PipelineRenderingCreateInfo.pColorAttachmentFormats = &swapchain.image_format;

	ImGui_ImplVulkan_Init(&init_info);
}

void RenderAPI::_setupTracy()
{
	{
		const char * const ctx_name = "Draw context";
		(void)ctx_name;
		draw_ctx = TracyVkContextCalibrated(
			physical_device,
			device,
			graphics_queue,
			draw_command_buffers[0],
			vkGetPhysicalDeviceCalibrateableTimeDomainsEXT,
			vkGetCalibratedTimestampsEXT
		);
		TracyVkContextName(draw_ctx, ctx_name, strlen(ctx_name));
	}
}

void RenderAPI::_destroyTracy()
{
	TracyVkDestroy(draw_ctx);
}


void RenderAPI::_prerenderItemIconImages()
{
	SingleTimeCommand command_buffer(device, command_pool, graphics_queue);

	VkRenderPassBeginInfo render_pass_info = {};
	render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	render_pass_info.renderPass = prerender_item_icon_render_pass;
	render_pass_info.framebuffer = prerender_item_icon_framebuffer;
	render_pass_info.renderArea.offset = { 0, 0 };
	render_pass_info.renderArea.extent = item_icon_images.extent2D;
	VkClearValue clear_color = { 0.0f, 0.0f, 0.0f, 0.0f };
	render_pass_info.clearValueCount = 1;
	render_pass_info.pClearValues = &clear_color;

	vkCmdBeginRenderPass(command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, prerender_item_icon_pipeline.pipeline);

	std::vector<VkDescriptorSet> descriptor_sets = {
		global_descriptor.sets[m_current_frame]
	};
	vkCmdBindDescriptorSets(
		command_buffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		prerender_item_icon_pipeline.layout,
		0,
		static_cast<uint32_t>(descriptor_sets.size()),
		descriptor_sets.data(),
		0,
		nullptr
	);

	// should probably fit the model bounding box or something like that
	const glm::mat4 clip = {
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, -1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.5f, 0.0f,
		0.0f, 0.0f, 0.5f, 1.0f
	};
	glm::mat4 proj = glm::ortho(
		-1.0f, 1.0f,
		-1.0f, 1.0f,
		0.0f, 5.0f
	);
	glm::mat4 view = glm::lookAt(
		glm::vec3(2.0f, 2.0f, 2.0f),
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f)
	);
	glm::mat4 model = glm::mat4(1.0f);

	GlobalPushConstant push_constants = {};
	push_constants.matrice = clip * proj * view * model;

	for (size_t i = 0; i < g_items_info.count(); i++)
	{
		push_constants.layer = i;

		_drawMesh(
			command_buffer,
			prerender_item_icon_pipeline,
			g_items_info.get(i).mesh_id,
			&push_constants,
			sizeof(GlobalPushConstant),
			VK_SHADER_STAGE_ALL
		);
	}

	vkCmdEndRenderPass(command_buffer);

	command_buffer.end();
}



void RenderAPI::_startFrame()
{
	// unlock in endFrame()
	global_mutex.lock();

	DebugGui::chunk_mesh_count = mesh_map.size();

	{
		ZoneScopedN("Wait for gpu");

		const std::chrono::nanoseconds start_cpu_wait_time = std::chrono::steady_clock::now().time_since_epoch();

		VK_CHECK(
			vkWaitForFences(device, 1, &in_flight_fences[m_current_frame], VK_TRUE, std::numeric_limits<uint64_t>::max()),
			"Failed to wait for in flight fence"
		);

		const std::chrono::nanoseconds end_cpu_wait_time = std::chrono::steady_clock::now().time_since_epoch();
		DebugGui::cpu_wait_time_history.push((end_cpu_wait_time - start_cpu_wait_time).count() / 1e6);
	}
	vkResetFences(device, 1, &in_flight_fences[m_current_frame]);

	{ // reset mesh usage by frame info
		std::lock_guard lock(mesh_map_mutex);
		for (auto & [id, mesh] : mesh_map)
		{
			mesh.used_by_frame[m_current_frame] = false;
		}
	}

	for (auto & [id, chunk] : m_chunks_in_scene)
	{
		chunk.used_by_frame[m_current_frame] = false;
	}

	_updateInstancesData();
}

void RenderAPI::_endFrame()
{
	m_current_frame = (m_current_frame + 1) % m_max_frames_in_flight;
	global_mutex.unlock();
}



std::pair<bool, Mesh> RenderAPI::_getMesh(const uint64_t id)
{
	std::lock_guard lock(mesh_map_mutex);
	if (!mesh_map.contains(id))
	{
		if (id != 0)
			LOG_WARNING("Mesh " << id << " not found in the mesh map.");
		return std::make_pair(false, Mesh());
	}

	Mesh mesh = mesh_map.at(id);

	if (mesh.buffer == VK_NULL_HANDLE)
	{
		LOG_WARNING("Mesh " << id << " has a null buffer.");
		return std::make_pair(false, Mesh());
	}

	mesh_map.at(id).used_by_frame[m_current_frame] = true;

	return std::make_pair(true, mesh);
}

void RenderAPI::_drawMesh(
	VkCommandBuffer command_buffer,
	const Pipeline & pipeline,
	const uint64_t mesh_id,
	const void * push_constants,
	const uint32_t push_constants_size,
	VkShaderStageFlags push_constants_stage,
	const uint32_t instance_id
)
{
	const std::pair<bool, Mesh> ret = _getMesh(mesh_id);
	if (ret.first == false)
	{
		return;
	}
	const Mesh & mesh = ret.second;

	const VkBuffer vertex_buffers[] = { mesh.buffer };
	const VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(
		command_buffer,
		0, 1,
		vertex_buffers,
		offsets
	);

	vkCmdBindIndexBuffer(
		command_buffer,
		mesh.buffer,
		mesh.index_offset,
		VK_INDEX_TYPE_UINT32
	);

	if (push_constants_size > 0)
	{
		vkCmdPushConstants(
			command_buffer,
			pipeline.layout,
			push_constants_stage,
			0,
			push_constants_size,
			push_constants
		);
	}

	vkCmdDrawIndexed(
		command_buffer,
		static_cast<uint32_t>(mesh.index_count),
		1, 0, 0,
		instance_id
	);
}

void RenderAPI::_bindChunkIndexBuffer(VkCommandBuffer command_buffer)
{
	vkCmdBindIndexBuffer(
		command_buffer,
		m_chunks_indices_buffer.buffer,
		0,
		VK_INDEX_TYPE_UINT32
	);
}

void RenderAPI::_drawChunkBlock(VkCommandBuffer command_buffer, const InstanceId & id)
{
	if (id == m_null_instance_id)
		return;

	ChunkMeshesInfo & chunk_info = m_chunks_in_scene[id];

	chunk_info.used_by_frame[m_current_frame] = true;

	vkCmdDrawIndexed(
		command_buffer,
		chunk_info.block_index_count,
		1,
		chunk_info.block_index_offset,
		0,
		id
	);
}

void RenderAPI::_drawChunksBlock(
	VkCommandBuffer command_buffer,
	Buffer & indirect_buffer,
	const std::vector<InstanceId> & ids
)
{
	vkCmdBindIndexBuffer(
		command_buffer,
		m_chunks_indices_buffer.buffer,
		0,
		VK_INDEX_TYPE_UINT32
	);

	VkDrawIndexedIndirectCommand * draw_commands = static_cast<VkDrawIndexedIndirectCommand *>(indirect_buffer.mappedMemory());
	uint32_t draw_count = 0;

	for (const InstanceId & id : ids)
	{
		if (id == m_null_instance_id)
			return;

		ChunkMeshesInfo & chunk_info = m_chunks_in_scene[id];

		chunk_info.used_by_frame[m_current_frame] = true;

		draw_commands[draw_count] = {
			.indexCount = chunk_info.block_index_count,
			.instanceCount = 1,
			.firstIndex = chunk_info.block_index_offset,
			.vertexOffset = 0,
			.firstInstance = id
		};
		draw_count++;
	}

	vkCmdDrawIndexedIndirect(
		command_buffer,
		indirect_buffer.buffer,
		0,
		draw_count,
		sizeof(VkDrawIndexedIndirectCommand)
	);
}

void RenderAPI::_drawChunkWater(VkCommandBuffer command_buffer, const InstanceId & id)
{
	if (id == m_null_instance_id)
		return;

	ChunkMeshesInfo & chunk_info = m_chunks_in_scene[id];

	chunk_info.used_by_frame[m_current_frame] = true;

	vkCmdDrawIndexed(
		command_buffer,
		chunk_info.water_index_count,
		1,
		chunk_info.water_index_offset,
		0,
		id
	);
}

void RenderAPI::_drawHudImage(
	const Descriptor & descriptor,
	const VkViewport & viewport
)
{
	const std::vector<VkDescriptorSet> descriptor_sets = {
		descriptor.set
	};

	vkCmdBindDescriptorSets(
		draw_command_buffers[m_current_frame],
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		hud_pipeline.layout,
		1,
		static_cast<uint32_t>(descriptor_sets.size()),
		descriptor_sets.data(),
		0,
		nullptr
	);

	vkCmdSetViewport(draw_command_buffers[m_current_frame], 0, 1, &viewport);

	vkCmdDraw(
		draw_command_buffers[m_current_frame],
		6,
		1,
		0,
		0
	);
}

void RenderAPI::_drawItemIcon(
	const VkViewport & viewport,
	const uint32_t layer
)
{
	GlobalPushConstant push_constants = {};
	push_constants.layer = layer;

	vkCmdPushConstants(
		draw_command_buffers[m_current_frame],
		item_icon_pipeline.layout,
		VK_SHADER_STAGE_ALL,
		0,
		sizeof(GlobalPushConstant),
		&push_constants
	);

	vkCmdSetViewport(draw_command_buffers[m_current_frame], 0, 1, &viewport);

	vkCmdDraw(
		draw_command_buffers[m_current_frame],
		6,
		1,
		0,
		0
	);
}


VkFormat RenderAPI::_findSupportedFormat(
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

void RenderAPI::_createImage(
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
	alloc_info.memoryTypeIndex = vk_helper::findMemoryType(physical_device, mem_requirements.memoryTypeBits, properties);

	VulkanMemoryAllocator & vma = VulkanMemoryAllocator::getInstance();
	VK_CHECK(
		vma.allocateMemory(device, &alloc_info, nullptr, &image_memory),
		"Failed to allocate image memory"
	);

	vkBindImageMemory(device, image, image_memory, 0);
}

void RenderAPI::_createImageView(
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

void RenderAPI::_transitionImageLayout(
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

void RenderAPI::_setImageLayout(
	VkCommandBuffer command_buffer,
	VkImage image,
	VkImageLayout old_layout,
	VkImageLayout new_layout,
	VkImageSubresourceRange subresource_range,
	VkAccessFlags srcAccessMask,
	VkAccessFlags dstAccessMask,
	VkPipelineStageFlags srcStageMask,
	VkPipelineStageFlags dstStageMask
)
{
	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = old_layout;
	barrier.newLayout = new_layout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange = subresource_range;
	barrier.srcAccessMask = srcAccessMask;
	barrier.dstAccessMask = dstAccessMask;

	vkCmdPipelineBarrier(
		command_buffer,
		srcStageMask,
		dstStageMask,
		0,
		0,
		nullptr,
		0,
		nullptr,
		1,
		&barrier
	);
}

void RenderAPI::_createBuffer(
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

	VkMemoryAllocateFlagsInfo memory_allocate_flags_info = {};
	memory_allocate_flags_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;

	if (usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
	{
		memory_allocate_flags_info.flags |= VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;
	}

	VkMemoryAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.allocationSize = mem_requirements.size;
	alloc_info.memoryTypeIndex = vk_helper::findMemoryType(physical_device, mem_requirements.memoryTypeBits, properties);
	alloc_info.pNext = &memory_allocate_flags_info;

	VulkanMemoryAllocator & vma = VulkanMemoryAllocator::getInstance();
	VK_CHECK(
		vma.allocateMemory(device, &alloc_info, nullptr, &buffer_memory),
		"Failed to allocate buffer memory"
	);

	vkBindBufferMemory(device, buffer, buffer_memory, 0);
}

void RenderAPI::_copyBuffer(
	VkBuffer src_buffer,
	VkBuffer dst_buffer,
	const VkBufferCopy & copy_region
)
{
	std::unique_lock lock(transfer_operation_mutex);

	vkResetCommandBuffer(transfer_command_buffers, 0);

	VkCommandBufferBeginInfo begin_info = {};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	VK_CHECK(
		vkBeginCommandBuffer(transfer_command_buffers, &begin_info),
		"Failed to begin recording command buffer"
	);

	vkCmdCopyBuffer(transfer_command_buffers, src_buffer, dst_buffer, 1, &copy_region);

	VK_CHECK(
		vkEndCommandBuffer(transfer_command_buffers),
		"Failed to end recording command buffer"
	);

	VkSubmitInfo submit_info = {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &transfer_command_buffers;

	VK_CHECK(
		vkQueueSubmit(transfer_queue, 1, &submit_info, VK_NULL_HANDLE),
		"Failed to submit queue 2"
	);

	VK_CHECK(
		vkQueueWaitIdle(transfer_queue),
		"Failed to wait for queue"
	);
}

void RenderAPI::_copyBufferToImage(
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

