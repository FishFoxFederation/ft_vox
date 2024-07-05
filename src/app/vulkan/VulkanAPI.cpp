#include "VulkanAPI.hpp"
#include "logger.hpp"
#include "Block.hpp"
#include "Model.hpp"
#include "ObjLoader.hpp"

#include "Tracy.hpp"

#include <stb_image.h>

#include <cstring>
#include <set>
#include <algorithm>
#include <map>

VulkanAPI::VulkanAPI(GLFWwindow * window):
	window(window)
{
	ZoneScoped;

	createInstance();
	loadVulkanFunctions();
	setupDebugMessenger();
	createSurface(window);
	pickPhysicalDevice();
	createLogicalDevice();
	createSwapChain(window);
	createCommandPool();
	createCommandBuffer();
	createSyncObjects();

	createColorAttachement();
	createDepthAttachement();
	createUniformBuffers();
	createTextureArray(Block::texture_names, 64);
	createCubeMap({
		"assets/textures/skybox/right.jpg",
		"assets/textures/skybox/left.jpg",
		"assets/textures/skybox/top.jpg",
		"assets/textures/skybox/bottom.jpg",
		"assets/textures/skybox/front.jpg",
		"assets/textures/skybox/back.jpg"
	}, 512);
	createFrustumLineBuffers();
	createTextureImage();

	createDescriptors();
	createRenderPass();
	createPipelines();
	createFramebuffers();

	createMeshes();

	setupImgui();
	createImGuiTexture(100, 100);

	setupTracy();

	setupRayTracing();

	LOG_INFO("VulkanAPI initialized");
}

VulkanAPI::~VulkanAPI()
{
	ZoneScoped;

	vkDeviceWaitIdle(device);

	destroyTracy();

	destroyImGuiTexture(imgui_texture);

	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();


	vkDestroyAccelerationStructureKHR(device, icospere_blas, nullptr);
	vkDestroyBuffer(device, icospere_blas_buffer, nullptr);
	vma.freeMemory(device, icospere_blas_buffer_memory, nullptr);

	vkDestroyBuffer(device, blas_transform_buffer, nullptr);
	vma.freeMemory(device, blas_transform_buffer_memory, nullptr);

	vkDestroyAccelerationStructureKHR(device, tlas, nullptr);
	vkDestroyBuffer(device, tlas_buffer, nullptr);
	vma.freeMemory(device, tlas_buffer_memory, nullptr);

	rt_descriptor.clear();

	vkDestroyImage(device, rt_output_image, nullptr);
	vma.freeMemory(device, rt_output_image_memory, nullptr);
	vkDestroyImageView(device, rt_output_image_view, nullptr);

	vkDestroyPipeline(device, rt_pipeline, nullptr);
	vkDestroyPipelineLayout(device, rt_pipeline_layout, nullptr);

	vkDestroyBuffer(device, rt_sbt_buffer, nullptr);
	vma.freeMemory(device, rt_sbt_buffer_memory, nullptr);



	{
		std::lock_guard lock(mesh_map_mutex);
		for (auto & [key, mesh] : mesh_map)
		{
			vkDestroyBuffer(device, mesh.buffer, nullptr);
			vma.freeMemory(device, mesh.buffer_memory, nullptr);
		}
	}

	for (int i = 0; i < max_frames_in_flight; i++)
	{
		vkUnmapMemory(device, frustum_line_buffers_memory[i]);
		vma.freeMemory(device, frustum_line_buffers_memory[i], nullptr);
		vkDestroyBuffer(device, frustum_line_buffers[i], nullptr);

		vkUnmapMemory(device, camera_ubo.memory[i]);
		vma.freeMemory(device, camera_ubo.memory[i], nullptr);
		vkDestroyBuffer(device, camera_ubo.buffers[i], nullptr);

		vkUnmapMemory(device, sun_ubo.memory[i]);
		vma.freeMemory(device, sun_ubo.memory[i], nullptr);
		vkDestroyBuffer(device, sun_ubo.buffers[i], nullptr);

		vkUnmapMemory(device, atmosphere_ubo.memory[i]);
		vma.freeMemory(device, atmosphere_ubo.memory[i], nullptr);
		vkDestroyBuffer(device, atmosphere_ubo.buffers[i], nullptr);
	}

	for (int i = 0; i < max_frames_in_flight; i++)
	{
		vkDestroyFramebuffer(device, lighting_framebuffers[i], nullptr);
		vkDestroyFramebuffer(device, shadow_framebuffers[i], nullptr);
	}

	vkDestroyRenderPass(device, lighting_render_pass, nullptr);
	vkDestroyRenderPass(device, shadow_render_pass, nullptr);

	vkDestroyDescriptorPool(device, imgui_descriptor_pool, nullptr);

	for (int i = 0; i < max_frames_in_flight; i++)
	{
		vkDestroySemaphore(device, image_available_semaphores[i], nullptr);
		vkDestroySemaphore(device, main_render_finished_semaphores[i], nullptr);
		vkDestroySemaphore(device, copy_finished_semaphores[i], nullptr);
		vkDestroySemaphore(device, imgui_render_finished_semaphores[i], nullptr);
		vkDestroyFence(device, in_flight_fences[i], nullptr);
	}
	vkDestroyFence(device, single_time_command_fence, nullptr);

	vkFreeCommandBuffers(device, command_pool, static_cast<uint32_t>(draw_command_buffers.size()), draw_command_buffers.data());
	vkFreeCommandBuffers(device, command_pool, static_cast<uint32_t>(copy_command_buffers.size()), copy_command_buffers.data());
	vkFreeCommandBuffers(device, command_pool, static_cast<uint32_t>(imgui_command_buffers.size()), imgui_command_buffers.data());
	vkDestroyCommandPool(device, command_pool, nullptr);
	vkFreeCommandBuffers(device, transfer_command_pool, 1, &transfer_command_buffers);
	vkDestroyCommandPool(device, transfer_command_pool, nullptr);

	// Bad design, but better than nothing until I find a better solution
	{
		color_attachement.clear();
		depth_attachement.clear();
		shadow_map_depth_attachement.clear();
		block_textures.clear();
		skybox_cube_map.clear();
		crosshair_image.clear();
		player_skin_image.clear();

		camera_descriptor.clear();
		block_textures_descriptor.clear();
		cube_map_descriptor.clear();
		shadow_map_descriptor.clear();
		test_image_descriptor.clear();
		sun_descriptor.clear();
		crosshair_image_descriptor.clear();
		player_skin_image_descriptor.clear();
		atmosphere_descriptor.clear();

		chunk_pipeline.clear();
		line_pipeline.clear();
		skybox_pipeline.clear();
		sun_pipeline.clear();
		shadow_pipeline.clear();
		test_image_pipeline.clear();
		entity_pipeline.clear();
		player_pipeline.clear();
		gui_pipeline.clear();

		swapchain.clear();
	}

	vkDestroyDevice(device, nullptr);

	#ifndef NDEBUG
		vkDestroyDebugUtilsMessengerEXT(instance, debug_messenger, nullptr);
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

void VulkanAPI::loadVulkanFunctions()
{
#define VK_LOAD_FUNCTION(name) name = reinterpret_cast<PFN_##name>(vkGetInstanceProcAddr(instance, #name)); \
	if (name == nullptr) { \
		throw std::runtime_error("Failed to load Vulkan function: " #name); \
	}

	VK_LOAD_FUNCTION(vkCreateDebugUtilsMessengerEXT)
	VK_LOAD_FUNCTION(vkDestroyDebugUtilsMessengerEXT)

	VK_LOAD_FUNCTION(vkGetPhysicalDeviceCalibrateableTimeDomainsEXT)
	VK_LOAD_FUNCTION(vkGetCalibratedTimestampsEXT)

	VK_LOAD_FUNCTION(vkCreateAccelerationStructureKHR)
	VK_LOAD_FUNCTION(vkDestroyAccelerationStructureKHR)
	VK_LOAD_FUNCTION(vkGetAccelerationStructureBuildSizesKHR)
	VK_LOAD_FUNCTION(vkCmdBuildAccelerationStructuresKHR)
	VK_LOAD_FUNCTION(vkGetAccelerationStructureDeviceAddressKHR)
	VK_LOAD_FUNCTION(vkCreateRayTracingPipelinesKHR)
	VK_LOAD_FUNCTION(vkGetRayTracingShaderGroupHandlesKHR)
	VK_LOAD_FUNCTION(vkGetBufferDeviceAddressKHR)
	VK_LOAD_FUNCTION(vkCmdTraceRaysKHR)

#undef VK_LOAD_FUNCTION
}

void VulkanAPI::setupDebugMessenger()
{
	#ifndef NDEBUG
		VkDebugUtilsMessengerCreateInfoEXT create_info;
		populateDebugMessengerCreateInfo(create_info);

		VK_CHECK(
			vkCreateDebugUtilsMessengerEXT(instance, &create_info, nullptr, &debug_messenger),
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

int VulkanAPI::ratePhysicalDevice(VkPhysicalDevice device)
{
	if (!isDeviceSuitable(device))
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

QueueFamilyIndices VulkanAPI::findQueueFamilies(VkPhysicalDevice device)
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

void VulkanAPI::createLogicalDevice()
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

	// VkPhysicalDeviceBufferDeviceAddressFeaturesEXT buffer_device_address_features_ext = {};
	// buffer_device_address_features_ext.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES_EXT;
	// buffer_device_address_features_ext.bufferDeviceAddress = VK_TRUE;

	VkPhysicalDeviceBufferDeviceAddressFeatures buffer_device_address_features = {};
	buffer_device_address_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
	buffer_device_address_features.bufferDeviceAddress = VK_TRUE;
	// buffer_device_address_features.pNext = &buffer_device_address_features_ext;

	VkPhysicalDeviceRayTracingPipelineFeaturesKHR ray_tracing_pipeline_features = {};
	ray_tracing_pipeline_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
	ray_tracing_pipeline_features.rayTracingPipeline = VK_TRUE;
	ray_tracing_pipeline_features.pNext = &buffer_device_address_features;

	VkPhysicalDeviceAccelerationStructureFeaturesKHR acceleration_structure_features = {};
	acceleration_structure_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
	acceleration_structure_features.accelerationStructure = VK_TRUE;
	acceleration_structure_features.pNext = &ray_tracing_pipeline_features;

	VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamic_rendering_features = {};
	dynamic_rendering_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR;
	dynamic_rendering_features.dynamicRendering = VK_TRUE;
	dynamic_rendering_features.pNext = &acceleration_structure_features;

	// VkPhysicalDeviceVulkan12Features vulkan_12_features = {};
	// vulkan_12_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
	// vulkan_12_features.bufferDeviceAddress = VK_TRUE;
	// vulkan_12_features.pNext = &dynamic_rendering_features;

	// VkPhysicalDeviceFeatures2 device_features_2 = {};
	// device_features_2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	// device_features_2.features = device_features;
	// device_features_2.pNext = &vulkan_12_features;

	VkDeviceCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	create_info.pQueueCreateInfos = queue_create_infos.data();
	create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
	create_info.pEnabledFeatures = &device_features;
	create_info.enabledExtensionCount = static_cast<uint32_t>(device_extensions.size());
	create_info.ppEnabledExtensionNames = device_extensions.data();

	create_info.pNext = &dynamic_rendering_features;

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

void VulkanAPI::createSwapChain(GLFWwindow * window)
{
	Swapchain::SupportDetails swapchain_support = Swapchain::querySwapChainSupport(physical_device, surface);

	Swapchain::CreateInfo create_info = {};
	create_info.surface_format = chooseSwapSurfaceFormat(swapchain_support.formats);
	create_info.present_mode = chooseSwapPresentMode(swapchain_support.present_modes);
	create_info.extent = chooseSwapExtent(swapchain_support.capabilities, window);
	create_info.old_swapchain = swapchain.swapchain;

	swapchain = Swapchain(device, physical_device, surface, create_info);
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
	for (int i = 0; i < max_frames_in_flight; i++)
	{
		vkDestroyFramebuffer(device, lighting_framebuffers[i], nullptr);
		vkDestroyFramebuffer(device, shadow_framebuffers[i], nullptr);
	}

	createSwapChain(window);
	createColorAttachement();
	createDepthAttachement();
	createPipelines();
	createFramebuffers();
	setupImgui();
	createImGuiTexture(100, 100);

	shadow_map_descriptor.update(
		device,
		shadow_map_depth_attachement.view,
		shadow_map_depth_attachement.sampler,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	);

	test_image_descriptor.update(
		device,
		shadow_map_depth_attachement.view,
		shadow_map_depth_attachement.sampler,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	);
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

	pool_info.queueFamilyIndex = queue_family_indices.transfer_family.value();

	VK_CHECK(
		vkCreateCommandPool(device, &pool_info, nullptr, &transfer_command_pool),
		"Failed to create command pool"
	);
}

void VulkanAPI::createCommandBuffer()
{
	draw_command_buffers.resize(max_frames_in_flight);
	copy_command_buffers.resize(max_frames_in_flight);
	imgui_command_buffers.resize(max_frames_in_flight);

	VkCommandBufferAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	alloc_info.commandPool = command_pool;
	alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	alloc_info.commandBufferCount = static_cast<uint32_t>(draw_command_buffers.size());

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

	alloc_info.commandPool = transfer_command_pool;
	alloc_info.commandBufferCount = 1;

	VK_CHECK(
		vkAllocateCommandBuffers(device, &alloc_info, &transfer_command_buffers),
		"Failed to allocate command buffers"
	);
}

void VulkanAPI::createSyncObjects()
{
	image_available_semaphores.resize(max_frames_in_flight);
	main_render_finished_semaphores.resize(max_frames_in_flight);
	copy_finished_semaphores.resize(max_frames_in_flight);
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
			vkCreateSemaphore(device, &semaphore_info, nullptr, &main_render_finished_semaphores[i]),
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

void VulkanAPI::createColorAttachement()
{
	Image::CreateInfo color_attachement_info = {};
	color_attachement_info.extent = { swapchain.extent.width * 2, swapchain.extent.height * 2 };
	color_attachement_info.format = swapchain.image_format;
	color_attachement_info.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	color_attachement_info.memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	color_attachement_info.final_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	color_attachement_info.create_view = true;

	SingleTimeCommand command_buffer(device, command_pool, graphics_queue);

	color_attachement = Image(device, physical_device, command_buffer, color_attachement_info);
}

void VulkanAPI::createDepthAttachement()
{
	SingleTimeCommand command_buffer(device, command_pool, graphics_queue);


	Image::CreateInfo depth_attachement_info = {};
	depth_attachement_info.extent = { swapchain.extent.width * 2, swapchain.extent.height * 2 };
	depth_attachement_info.aspect_mask = VK_IMAGE_ASPECT_DEPTH_BIT;
	depth_attachement_info.format = findSupportedFormat(
		{VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);
	depth_attachement_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	depth_attachement_info.memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	depth_attachement_info.final_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	depth_attachement_info.create_view = true;

	depth_attachement = Image(device, physical_device, command_buffer, depth_attachement_info);


	depth_attachement_info.extent = { 10000, 10000 };
	depth_attachement_info.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
	depth_attachement_info.create_sampler = true;
	depth_attachement_info.sampler_address_mode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

	shadow_map_depth_attachement = Image(device, physical_device, command_buffer, depth_attachement_info);
}

void VulkanAPI::createUBO(UBO & ubo, const VkDeviceSize size, const uint32_t count)
{
	ubo.buffers.resize(count);
	ubo.memory.resize(count);
	ubo.mapped_memory.resize(count);

	for (uint32_t i = 0; i < count; i++)
	{
		createBuffer(
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

void VulkanAPI::createUniformBuffers()
{
	createUBO(camera_ubo, sizeof(ViewProjMatrices), max_frames_in_flight);
	createUBO(sun_ubo, sizeof(ViewProjMatrices), max_frames_in_flight);
	createUBO(atmosphere_ubo, sizeof(AtmosphereParams), max_frames_in_flight);
}

void VulkanAPI::createTextureArray(const std::vector<std::string> & file_paths, uint32_t size)
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

void VulkanAPI::createCubeMap(const std::array<std::string, 6> & file_paths, uint32_t size)
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

	SingleTimeCommand command_buffer_2(device, command_pool, graphics_queue);

	skybox_cube_map = Image(device, physical_device, command_buffer_2, _image_info);
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

void VulkanAPI::createTextureImage()
{
	{ // crosshair
		Image::CreateInfo image_info = {};
		image_info.file_paths = {"assets/textures/gui/crosshair.png"};
		image_info.extent = {32, 32};
		image_info.format = VK_FORMAT_R8G8B8A8_SRGB;
		image_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		image_info.memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		image_info.final_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		image_info.create_view = true;
		image_info.create_sampler = true;

		SingleTimeCommand command_buffer(device, command_pool, graphics_queue);

		crosshair_image = Image(device, physical_device, command_buffer, image_info);
	}

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
}

void VulkanAPI::createDescriptors()
{
	{ // Camera descriptor
		VkDescriptorSetLayoutBinding ubo_layout_binding = {};
		ubo_layout_binding.binding = 0;
		ubo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		ubo_layout_binding.descriptorCount = 1;
		ubo_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_RAYGEN_BIT_KHR;
		ubo_layout_binding.pImmutableSamplers = nullptr;

		Descriptor::CreateInfo descriptor_info = {};
		descriptor_info.bindings = { ubo_layout_binding };
		descriptor_info.descriptor_count = static_cast<uint32_t>(max_frames_in_flight);
		descriptor_info.set_count = static_cast<uint32_t>(max_frames_in_flight);

		camera_descriptor = Descriptor(device, descriptor_info);

		for (int i = 0; i < max_frames_in_flight; i++)
		{
			VkDescriptorBufferInfo buffer_info = {};
			buffer_info.buffer = camera_ubo.buffers[i];
			buffer_info.offset = 0;
			buffer_info.range = sizeof(ViewProjMatrices);

			VkWriteDescriptorSet descriptor_write = {};
			descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptor_write.dstSet = camera_descriptor.sets[i];
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

	{ // Block textures descriptor
		VkDescriptorSetLayoutBinding sampler_layout_binding = {};
		sampler_layout_binding.binding = 0;
		sampler_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		sampler_layout_binding.descriptorCount = 1;
		sampler_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		sampler_layout_binding.pImmutableSamplers = nullptr;

		Descriptor::CreateInfo descriptor_info = {};
		descriptor_info.bindings = { sampler_layout_binding };
		descriptor_info.descriptor_count = static_cast<uint32_t>(max_frames_in_flight);

		block_textures_descriptor = Descriptor(device, descriptor_info);

		block_textures_descriptor.update(
			device,
			block_textures.view,
			block_textures.sampler,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		);
	}

	{ // Cube map descriptor
		VkDescriptorSetLayoutBinding sampler_layout_binding = {};
		sampler_layout_binding.binding = 0;
		sampler_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		sampler_layout_binding.descriptorCount = 1;
		sampler_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		sampler_layout_binding.pImmutableSamplers = nullptr;

		Descriptor::CreateInfo descriptor_info = {};
		descriptor_info.bindings = { sampler_layout_binding };
		descriptor_info.descriptor_count = static_cast<uint32_t>(max_frames_in_flight);

		cube_map_descriptor = Descriptor(device, descriptor_info);

		cube_map_descriptor.update(
			device,
			skybox_cube_map.view,
			skybox_cube_map.sampler,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		);
	}

	{ // Shadow map descriptor
		VkDescriptorSetLayoutBinding sampler_layout_binding = {};
		sampler_layout_binding.binding = 0;
		sampler_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		sampler_layout_binding.descriptorCount = 1;
		sampler_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		sampler_layout_binding.pImmutableSamplers = nullptr;

		Descriptor::CreateInfo descriptor_info = {};
		descriptor_info.bindings = { sampler_layout_binding };
		descriptor_info.descriptor_count = static_cast<uint32_t>(max_frames_in_flight);

		shadow_map_descriptor = Descriptor(device, descriptor_info);

		shadow_map_descriptor.update(
			device,
			shadow_map_depth_attachement.view,
			shadow_map_depth_attachement.sampler,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		);
	}

	{ // Test image descriptor
		VkDescriptorSetLayoutBinding sampler_layout_binding = {};
		sampler_layout_binding.binding = 0;
		sampler_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		sampler_layout_binding.descriptorCount = 1;
		sampler_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		sampler_layout_binding.pImmutableSamplers = nullptr;

		Descriptor::CreateInfo descriptor_info = {};
		descriptor_info.bindings = { sampler_layout_binding };
		descriptor_info.descriptor_count = static_cast<uint32_t>(max_frames_in_flight);

		test_image_descriptor = Descriptor(device, descriptor_info);

		test_image_descriptor.update(
			device,
			shadow_map_depth_attachement.view,
			shadow_map_depth_attachement.sampler,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		);
	}

	{ // CrossHair image descriptor
		VkDescriptorSetLayoutBinding sampler_layout_binding = {};
		sampler_layout_binding.binding = 0;
		sampler_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		sampler_layout_binding.descriptorCount = 1;
		sampler_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		sampler_layout_binding.pImmutableSamplers = nullptr;

		Descriptor::CreateInfo descriptor_info = {};
		descriptor_info.bindings = { sampler_layout_binding };
		descriptor_info.descriptor_count = static_cast<uint32_t>(max_frames_in_flight);

		crosshair_image_descriptor = Descriptor(device, descriptor_info);

		crosshair_image_descriptor.update(
			device,
			crosshair_image.view,
			crosshair_image.sampler,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		);
	}

	{ // Player skin image descriptor
		VkDescriptorSetLayoutBinding sampler_layout_binding = {};
		sampler_layout_binding.binding = 0;
		sampler_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		sampler_layout_binding.descriptorCount = 1;
		sampler_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		sampler_layout_binding.pImmutableSamplers = nullptr;

		Descriptor::CreateInfo descriptor_info = {};
		descriptor_info.bindings = { sampler_layout_binding };
		descriptor_info.descriptor_count = static_cast<uint32_t>(max_frames_in_flight);

		player_skin_image_descriptor = Descriptor(device, descriptor_info);

		player_skin_image_descriptor.update(
			device,
			player_skin_image.view,
			player_skin_image.sampler,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		);
	}

	{ // Sun matrices descriptor
		VkDescriptorSetLayoutBinding ubo_layout_binding = {};
		ubo_layout_binding.binding = 0;
		ubo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		ubo_layout_binding.descriptorCount = 1;
		ubo_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		ubo_layout_binding.pImmutableSamplers = nullptr;

		Descriptor::CreateInfo descriptor_info = {};
		descriptor_info.bindings = { ubo_layout_binding };
		descriptor_info.descriptor_count = static_cast<uint32_t>(max_frames_in_flight);
		descriptor_info.set_count = static_cast<uint32_t>(max_frames_in_flight);

		sun_descriptor = Descriptor(device, descriptor_info);

		for (int i = 0; i < max_frames_in_flight; i++)
		{
			VkDescriptorBufferInfo buffer_info = {};
			buffer_info.buffer = sun_ubo.buffers[i];
			buffer_info.offset = 0;
			buffer_info.range = sizeof(ViewProjMatrices);

			VkWriteDescriptorSet descriptor_write = {};
			descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptor_write.dstSet = sun_descriptor.sets[i];
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

	{ // Atmosphere descriptor
		VkDescriptorSetLayoutBinding ubo_layout_binding = {};
		ubo_layout_binding.binding = 0;
		ubo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		ubo_layout_binding.descriptorCount = 1;
		ubo_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		ubo_layout_binding.pImmutableSamplers = nullptr;

		Descriptor::CreateInfo descriptor_info = {};
		descriptor_info.bindings = { ubo_layout_binding };
		descriptor_info.descriptor_count = static_cast<uint32_t>(max_frames_in_flight);
		descriptor_info.set_count = static_cast<uint32_t>(max_frames_in_flight);

		atmosphere_descriptor = Descriptor(device, descriptor_info);

		for (int i = 0; i < max_frames_in_flight; i++)
		{
			VkDescriptorBufferInfo buffer_info = {};
			buffer_info.buffer = atmosphere_ubo.buffers[i];
			buffer_info.offset = 0;
			buffer_info.range = sizeof(AtmosphereParams);

			VkWriteDescriptorSet descriptor_write = {};
			descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptor_write.dstSet = atmosphere_descriptor.sets[i];
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
}

void VulkanAPI::createRenderPass()
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

		VkAttachmentReference color_attachement_ref = {};
		color_attachement_ref.attachment = 0;
		color_attachement_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depth_attachement_ref = {};
		depth_attachement_ref.attachment = 1;
		depth_attachement_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &color_attachement_ref;
		subpass.pDepthStencilAttachment = &depth_attachement_ref;

		std::array<VkAttachmentDescription, 2> attachments = {
			color_attachement_description,
			depth_attachement_description
		};

		VkRenderPassCreateInfo render_pass_info = {};
		render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		render_pass_info.attachmentCount = static_cast<uint32_t>(attachments.size());
		render_pass_info.pAttachments = attachments.data();
		render_pass_info.subpassCount = 1;
		render_pass_info.pSubpasses = &subpass;

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
}

void VulkanAPI::createPipelines()
{
	{ // chunk pipeline
		Pipeline::CreateInfo pipeline_info = {};
		pipeline_info.extent = color_attachement.extent2D;
		pipeline_info.vert_path = "shaders/chunk_shader.vert.spv";
		pipeline_info.frag_path = "shaders/chunk_shader.frag.spv";
		pipeline_info.binding_description = BlockVertex::getBindingDescription();
		pipeline_info.attribute_descriptions = BlockVertex::getAttributeDescriptions();
		pipeline_info.color_formats = { color_attachement.format };
		pipeline_info.depth_format = depth_attachement.format;
		pipeline_info.descriptor_set_layouts = {
			camera_descriptor.layout,
			sun_descriptor.layout,
			block_textures_descriptor.layout,
			shadow_map_descriptor.layout
		};
		pipeline_info.push_constant_ranges = {
			{ VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ModelMatrice) }
		};
		pipeline_info.render_pass = lighting_render_pass;

		chunk_pipeline = Pipeline(device, pipeline_info);
	}

	{ // line pipeline
		Pipeline::CreateInfo pipeline_info = {};
		pipeline_info.extent = color_attachement.extent2D;
		pipeline_info.vert_path = "shaders/line_shader.vert.spv";
		pipeline_info.frag_path = "shaders/line_shader.frag.spv";
		pipeline_info.binding_description = LineVertex::getBindingDescription();
		pipeline_info.attribute_descriptions = LineVertex::getAttributeDescriptions();
		pipeline_info.polygon_mode = VK_POLYGON_MODE_LINE;
		pipeline_info.color_formats = { color_attachement.format };
		pipeline_info.depth_format = depth_attachement.format;
		pipeline_info.descriptor_set_layouts = {
			camera_descriptor.layout
		};
		pipeline_info.push_constant_ranges = {
			{ VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ModelMatrice) }
		};
		pipeline_info.render_pass = lighting_render_pass;
		pipeline_info.dynamic_states = { VK_DYNAMIC_STATE_LINE_WIDTH };

		line_pipeline = Pipeline(device, pipeline_info);
	}

	{ // skybox pipeline
		Pipeline::CreateInfo pipeline_info = {};
		pipeline_info.extent = color_attachement.extent2D;
		pipeline_info.vert_path = "shaders/sky/box.vert.spv";
		pipeline_info.frag_path = "shaders/sky/box.frag.spv";
		pipeline_info.color_formats = { color_attachement.format };
		pipeline_info.depth_format = depth_attachement.format;
		pipeline_info.descriptor_set_layouts = {
			camera_descriptor.layout,
			cube_map_descriptor.layout
		};
		pipeline_info.push_constant_ranges = {
			{ VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ModelMatrice) }
		};
		pipeline_info.render_pass = lighting_render_pass;

		skybox_pipeline = Pipeline(device, pipeline_info);
	}

	{ // sun pipeline
		Pipeline::CreateInfo pipeline_info = {};
		pipeline_info.extent = color_attachement.extent2D;
		pipeline_info.vert_path = "shaders/sky/sun.vert.spv";
		pipeline_info.frag_path = "shaders/sky/sun.frag.spv";
		pipeline_info.binding_description = ObjVertex::getBindingDescription();
		pipeline_info.attribute_descriptions = ObjVertex::getAttributeDescriptions();
		pipeline_info.color_formats = { color_attachement.format };
		pipeline_info.depth_format = depth_attachement.format;
		pipeline_info.descriptor_set_layouts = {
			camera_descriptor.layout,
			atmosphere_descriptor.layout
		};
		pipeline_info.push_constant_ranges = {
			{ VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SkyShaderData) }
		};
		pipeline_info.render_pass = lighting_render_pass;
		pipeline_info.front_face = VK_FRONT_FACE_CLOCKWISE;

		sun_pipeline = Pipeline(device, pipeline_info);
	}

	{ // shadow map pipeline
		Pipeline::CreateInfo pipeline_info = {};
		pipeline_info.extent = shadow_map_depth_attachement.extent2D;
		pipeline_info.vert_path = "shaders/shadow_shader.vert.spv";
		pipeline_info.binding_description = BlockVertex::getBindingDescription();
		std::vector<VkVertexInputAttributeDescription> attribute_descriptions = {
			BlockVertex::getAttributeDescriptions()[0]
		};
		pipeline_info.attribute_descriptions = attribute_descriptions;
		pipeline_info.depth_format = shadow_map_depth_attachement.format;
		pipeline_info.depth_bias_enable = VK_TRUE;
		pipeline_info.depth_bias_constant_factor = 0.0f;
		pipeline_info.depth_bias_slope_factor = 0.1f;
		pipeline_info.descriptor_set_layouts = {
			sun_descriptor.layout
		};
		pipeline_info.push_constant_ranges = {
			{ VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ModelMatrice) }
		};
		pipeline_info.render_pass = shadow_render_pass;

		shadow_pipeline = Pipeline(device, pipeline_info);
	}

	{ // test image pipeline
		Pipeline::CreateInfo pipeline_info = {};
		pipeline_info.extent = { color_attachement.extent2D.width / 3, color_attachement.extent2D.height / 3 };
		pipeline_info.vert_path = "shaders/test_image_shader.vert.spv";
		pipeline_info.frag_path = "shaders/test_image_shader.frag.spv";
		pipeline_info.color_formats = { color_attachement.format };
		pipeline_info.depth_format = depth_attachement.format;
		pipeline_info.descriptor_set_layouts = {
			test_image_descriptor.layout
		};
		pipeline_info.push_constant_ranges = {
			{ VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ModelMatrice) }
		};
		pipeline_info.render_pass = lighting_render_pass;

		test_image_pipeline = Pipeline(device, pipeline_info);
	}

	{ // Entity pipeline
		Pipeline::CreateInfo pipeline_info = {};
		pipeline_info.extent = color_attachement.extent2D;
		pipeline_info.vert_path = "shaders/entity_shader.vert.spv";
		pipeline_info.frag_path = "shaders/entity_shader.frag.spv";
		pipeline_info.binding_description = EntityVertex::getBindingDescription();
		pipeline_info.attribute_descriptions = EntityVertex::getAttributeDescriptions();
		pipeline_info.color_formats = { color_attachement.format };
		pipeline_info.depth_format = depth_attachement.format;
		pipeline_info.descriptor_set_layouts = {
			camera_descriptor.layout
		};
		pipeline_info.push_constant_ranges = {
			{ VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(EntityMatrices) }
		};
		pipeline_info.render_pass = lighting_render_pass;

		entity_pipeline = Pipeline(device, pipeline_info);
	}

	{ // Player pipeline
		Pipeline::CreateInfo pipeline_info = {};
		pipeline_info.extent = color_attachement.extent2D;
		pipeline_info.vert_path = "shaders/player_shader.vert.spv";
		pipeline_info.frag_path = "shaders/player_shader.frag.spv";
		pipeline_info.binding_description = ObjVertex::getBindingDescription();
		pipeline_info.attribute_descriptions = ObjVertex::getAttributeDescriptions();
		pipeline_info.color_formats = { color_attachement.format };
		pipeline_info.depth_format = depth_attachement.format;
		pipeline_info.descriptor_set_layouts = {
			camera_descriptor.layout,
			player_skin_image_descriptor.layout
		};
		pipeline_info.push_constant_ranges = {
			{ VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ModelMatrice) }
		};
		pipeline_info.render_pass = lighting_render_pass;

		player_pipeline = Pipeline(device, pipeline_info);
	}

	{ // Gui pipeline
		Pipeline::CreateInfo pipeline_info = {};
		pipeline_info.extent = color_attachement.extent2D;
		pipeline_info.vert_path = "shaders/gui_shader.vert.spv";
		pipeline_info.frag_path = "shaders/gui_shader.frag.spv";
		pipeline_info.color_formats = { color_attachement.format };
		pipeline_info.depth_format = depth_attachement.format;
		pipeline_info.descriptor_set_layouts = {
			crosshair_image_descriptor.layout
		};
		pipeline_info.push_constant_ranges = {
			{ VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(GuiTextureData) }
		};
		pipeline_info.render_pass = lighting_render_pass;
		pipeline_info.dynamic_states = { VK_DYNAMIC_STATE_VIEWPORT };
		pipeline_info.enable_alpha_blending = true;

		gui_pipeline = Pipeline(device, pipeline_info);
	}

}

void VulkanAPI::createFramebuffers()
{
	lighting_framebuffers.resize(max_frames_in_flight);
	shadow_framebuffers.resize(max_frames_in_flight);

	for (int i = 0; i < max_frames_in_flight; i++)
	{
		std::vector<VkImageView> attachments = {
			color_attachement.view,
			depth_attachement.view
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

	for (int i = 0; i < max_frames_in_flight; i++)
	{
		std::vector<VkImageView> attachments = {
			shadow_map_depth_attachement.view
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
			vkCreateFramebuffer(device, &framebuffer_info, nullptr, &shadow_framebuffers[i]),
			"Failed to create framebuffer"
		);
	}

}

void VulkanAPI::createMeshes()
{
	{ // load cube mesh
		ObjLoader obj_loader("assets/models/cube.obj");

		std::vector<EntityVertex> vertices;
		std::transform(
			obj_loader.vertices().begin(),
			obj_loader.vertices().end(),
			std::back_inserter(vertices),
			[](const ObjVertex & obj_vertex) {
				return EntityVertex{obj_vertex.pos, obj_vertex.normal};
			}
		);

		cube_mesh_id = storeMesh(
			vertices.data(),
			vertices.size(),
			sizeof(EntityVertex),
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
			obj_loader.indices().size(),
			VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR
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


void VulkanAPI::setupRayTracing()
{
	getRayTracingProperties();
	createBottomLevelAS();
	createTopLevelAS();
	createRayTracingOutputImage();
	createRayTracingDescriptor();
	updateRayTracingDescriptor();
	createRayTracingPipeline();
	createRayTracingShaderBindingTable();

	LOG_INFO("Ray tracing setup complete");
}

void VulkanAPI::getRayTracingProperties()
{
	VkPhysicalDeviceProperties2 properties = {};
	properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;

	// ray_tracing_features = {};
	// ray_tracing_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
	// properties.pNext = &ray_tracing_features;
	// vkGetPhysicalDeviceProperties2(physical_device, &properties);

	ray_tracing_properties = {};
	ray_tracing_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
	properties.pNext = &ray_tracing_properties;
	vkGetPhysicalDeviceProperties2(physical_device, &properties);

	// acceleration_structure_features = {};
	// acceleration_structure_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
	// properties.pNext = &acceleration_structure_features;
	// vkGetPhysicalDeviceProperties2(physical_device, &properties);

	acceleration_structure_properties = {};
	acceleration_structure_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR;
	properties.pNext = &acceleration_structure_properties;
	vkGetPhysicalDeviceProperties2(physical_device, &properties);
}

void VulkanAPI::createBottomLevelAS()
{
	VkDeviceAddress address = getBufferDeviceAddress(mesh_map[icosphere_mesh_id].buffer);

	VkTransformMatrixKHR transform_matrix = {
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f
	};

	createBuffer(
		sizeof(VkTransformMatrixKHR),
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		blas_transform_buffer,
		blas_transform_buffer_memory
	);

	void * data;
	vkMapMemory(device, blas_transform_buffer_memory, 0, sizeof(VkTransformMatrixKHR), 0, &data);
	memcpy(data, &transform_matrix, sizeof(VkTransformMatrixKHR));
	vkUnmapMemory(device, blas_transform_buffer_memory);

	VkDeviceOrHostAddressConstKHR transformBufferDeviceAddress{};
	transformBufferDeviceAddress.deviceAddress = getBufferDeviceAddress(blas_transform_buffer);

	VkAccelerationStructureGeometryKHR as_geometry = {};
	as_geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
	as_geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
	as_geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
	as_geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
	as_geometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
	as_geometry.geometry.triangles.vertexData.deviceAddress = address;
	as_geometry.geometry.triangles.vertexStride = sizeof(ObjVertex);
	as_geometry.geometry.triangles.maxVertex = mesh_map[icosphere_mesh_id].vertex_count - 1;
	as_geometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
	as_geometry.geometry.triangles.indexData.deviceAddress = address + mesh_map[icosphere_mesh_id].index_offset;
	as_geometry.geometry.triangles.transformData.hostAddress = nullptr;
	as_geometry.geometry.triangles.transformData.deviceAddress = 0;
	as_geometry.geometry.triangles.transformData = transformBufferDeviceAddress;

	VkAccelerationStructureBuildGeometryInfoKHR build_info = {};
	build_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	build_info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
	build_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	build_info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
	build_info.geometryCount = 1;
	build_info.pGeometries = &as_geometry;


	uint32_t primitive_count = mesh_map[icosphere_mesh_id].index_count / 3;

	VkAccelerationStructureBuildSizesInfoKHR build_sizes_info = {};
	build_sizes_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
	vkGetAccelerationStructureBuildSizesKHR(
		device,
		VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
		&build_info,
		&primitive_count,
		&build_sizes_info
	);

	VkBuffer scratch_buffer;
	VkDeviceMemory scratch_buffer_memory;
	createBuffer(
		build_sizes_info.buildScratchSize,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		scratch_buffer,
		scratch_buffer_memory
	);

	createBuffer(
		build_sizes_info.accelerationStructureSize,
		VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		icospere_blas_buffer,
		icospere_blas_buffer_memory
	);

	VkAccelerationStructureCreateInfoKHR acceleration_structure_info = {};
	acceleration_structure_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
	acceleration_structure_info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
	acceleration_structure_info.buffer = icospere_blas_buffer;
	acceleration_structure_info.size = build_sizes_info.accelerationStructureSize;

	VK_CHECK(
		vkCreateAccelerationStructureKHR(device, &acceleration_structure_info, nullptr, &icospere_blas),
		"Failed to create bottom level acceleration structure"
	);

	build_info.scratchData.deviceAddress = getBufferDeviceAddress(scratch_buffer);
	build_info.dstAccelerationStructure = icospere_blas;

	VkAccelerationStructureBuildRangeInfoKHR build_range_info = {};
	build_range_info.primitiveCount = primitive_count;
	build_range_info.primitiveOffset = 0;
	build_range_info.firstVertex = 0;
	build_range_info.transformOffset = 0;

	std::vector<VkAccelerationStructureBuildRangeInfoKHR *> build_range_infos = { &build_range_info };

	VkCommandBuffer command_buffer = beginSingleTimeCommands();

	vkCmdBuildAccelerationStructuresKHR(
		command_buffer,
		1,
		&build_info,
		build_range_infos.data()
	);

	endSingleTimeCommands(command_buffer);

	vkDestroyBuffer(device, scratch_buffer, nullptr);
	vma.freeMemory(device, scratch_buffer_memory, nullptr);
}

void VulkanAPI::createTopLevelAS()
{
	VkAccelerationStructureDeviceAddressInfoKHR device_address_info = {};
	device_address_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
	device_address_info.accelerationStructure = icospere_blas;

	VkDeviceAddress blas_device_address = vkGetAccelerationStructureDeviceAddressKHR(device, &device_address_info);

	VkAccelerationStructureInstanceKHR instance = {};
	instance.transform = {
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f
	};
	instance.instanceCustomIndex = 0;
	instance.mask = 0xFF;
	instance.instanceShaderBindingTableRecordOffset = 0;
	instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
	instance.accelerationStructureReference = blas_device_address;

	VkBuffer as_instance_buffer;
	VkDeviceMemory as_instance_buffer_memory;
	createBuffer(
		sizeof(VkAccelerationStructureInstanceKHR),
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		as_instance_buffer,
		as_instance_buffer_memory
	);

	void * data;
	vkMapMemory(device, as_instance_buffer_memory, 0, sizeof(VkAccelerationStructureInstanceKHR), 0, &data);
	memcpy(data, &instance, sizeof(VkAccelerationStructureInstanceKHR));
	vkUnmapMemory(device, as_instance_buffer_memory);

	VkAccelerationStructureGeometryKHR as_geometry = {};
	as_geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
	as_geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
	as_geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
	as_geometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
	as_geometry.geometry.instances.arrayOfPointers = VK_FALSE;
	as_geometry.geometry.instances.data.deviceAddress = getBufferDeviceAddress(as_instance_buffer);

	VkAccelerationStructureBuildGeometryInfoKHR build_info = {};
	build_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	build_info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	build_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	build_info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
	build_info.geometryCount = 1;
	build_info.pGeometries = &as_geometry;


	std::vector<uint32_t> primitive_counts = {
		1
	};

	VkAccelerationStructureBuildSizesInfoKHR build_sizes_info = {};
	build_sizes_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
	vkGetAccelerationStructureBuildSizesKHR(
		device,
		VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
		&build_info,
		primitive_counts.data(),
		&build_sizes_info
	);

	VkBuffer scratch_buffer;
	VkDeviceMemory scratch_buffer_memory;
	createBuffer(
		build_sizes_info.buildScratchSize,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		scratch_buffer,
		scratch_buffer_memory
	);

	createBuffer(
		build_sizes_info.accelerationStructureSize,
		VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		tlas_buffer,
		tlas_buffer_memory
	);

	VkAccelerationStructureCreateInfoKHR acceleration_structure_info = {};
	acceleration_structure_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
	acceleration_structure_info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	acceleration_structure_info.buffer = tlas_buffer;
	acceleration_structure_info.size = build_sizes_info.accelerationStructureSize;

	VK_CHECK(
		vkCreateAccelerationStructureKHR(device, &acceleration_structure_info, nullptr, &tlas),
		"Failed to create bottom level acceleration structure"
	);

	build_info.scratchData.deviceAddress = getBufferDeviceAddress(scratch_buffer);
	build_info.dstAccelerationStructure = tlas;

	VkAccelerationStructureBuildRangeInfoKHR build_range_info = {};
	build_range_info.primitiveCount = primitive_counts[0];
	build_range_info.primitiveOffset = 0;
	build_range_info.firstVertex = 0;
	build_range_info.transformOffset = 0;

	std::vector<VkAccelerationStructureBuildRangeInfoKHR *> build_range_infos = { &build_range_info };


	VkCommandBuffer command_buffer = beginSingleTimeCommands();

	vkCmdBuildAccelerationStructuresKHR(
		command_buffer,
		1,
		&build_info,
		build_range_infos.data()
	);

	endSingleTimeCommands(command_buffer);

	vkDestroyBuffer(device, scratch_buffer, nullptr);
	vma.freeMemory(device, scratch_buffer_memory, nullptr);

	vkDestroyBuffer(device, as_instance_buffer, nullptr);
	vma.freeMemory(device, as_instance_buffer_memory, nullptr);
}

void VulkanAPI::createRayTracingOutputImage()
{
	rt_output_image_width = swapchain.extent.width;
	rt_output_image_height = swapchain.extent.height;

	createImage(
		rt_output_image_width,
		rt_output_image_height,
		1,
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		rt_output_image,
		rt_output_image_memory
	);

	createImageView(
		rt_output_image,
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_ASPECT_COLOR_BIT,
		rt_output_image_view
	);

	transitionImageLayout(
		rt_output_image,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_GENERAL,
		VK_IMAGE_ASPECT_COLOR_BIT,
		1,
		0,
		VK_ACCESS_SHADER_WRITE_BIT,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR
	);
}

void VulkanAPI::createRayTracingDescriptor()
{
	VkDescriptorSetLayoutBinding acceleration_structure_binding = {};
	acceleration_structure_binding.binding = 0;
	acceleration_structure_binding.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
	acceleration_structure_binding.descriptorCount = 1;
	acceleration_structure_binding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
	acceleration_structure_binding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutBinding result_image_binding = {};
	result_image_binding.binding = 1;
	result_image_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	result_image_binding.descriptorCount = 1;
	result_image_binding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
	result_image_binding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutBinding camera_binding = {};
	camera_binding.binding = 2;
	camera_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	camera_binding.descriptorCount = 1;
	camera_binding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
	camera_binding.pImmutableSamplers = nullptr;

	Descriptor::CreateInfo descriptor_info = {};
	descriptor_info.bindings = {
		acceleration_structure_binding,
		result_image_binding,
		camera_binding
	};
	descriptor_info.descriptor_count = static_cast<uint32_t>(max_frames_in_flight);
	descriptor_info.set_count = static_cast<uint32_t>(max_frames_in_flight);

	rt_descriptor = Descriptor(device, descriptor_info);
}

void VulkanAPI::updateRayTracingDescriptor()
{
	for (int i = 0; i < max_frames_in_flight; i++)
	{
		VkWriteDescriptorSetAccelerationStructureKHR acceleration_structure_info = {};
		acceleration_structure_info.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
		acceleration_structure_info.accelerationStructureCount = 1;
		acceleration_structure_info.pAccelerationStructures = &tlas;

		VkWriteDescriptorSet acceleration_structure_write = {};
		acceleration_structure_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		acceleration_structure_write.dstSet = rt_descriptor.sets[i];
		acceleration_structure_write.dstBinding = 0;
		acceleration_structure_write.descriptorCount = 1;
		acceleration_structure_write.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
		acceleration_structure_write.pNext = &acceleration_structure_info;


		VkDescriptorImageInfo result_image_info = {};
		result_image_info.imageView = rt_output_image_view;
		result_image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

		VkWriteDescriptorSet result_image_write = {};
		result_image_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		result_image_write.dstSet = rt_descriptor.sets[i];
		result_image_write.dstBinding = 1;
		result_image_write.descriptorCount = 1;
		result_image_write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		result_image_write.pImageInfo = &result_image_info;


		VkDescriptorBufferInfo camera_info = {};
		camera_info.buffer = camera_ubo.buffers[i];
		camera_info.offset = 0;
		camera_info.range = sizeof(ViewProjMatrices);

		VkWriteDescriptorSet camera_write = {};
		camera_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		camera_write.dstSet = rt_descriptor.sets[i];
		camera_write.dstBinding = 2;
		camera_write.descriptorCount = 1;
		camera_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		camera_write.pBufferInfo = &camera_info;

		std::vector<VkWriteDescriptorSet> writes = {
			acceleration_structure_write,
			result_image_write,
			camera_write
		};

		vkUpdateDescriptorSets(device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
	}
}

void VulkanAPI::createRayTracingPipeline()
{
	std::vector<VkPipelineShaderStageCreateInfo> shader_stages;

	#define LOAD_SHADER_STAGE(stage_name, path) \
	{ \
		auto code = readFile(path); \
		VkShaderModule module = createShaderModule(code); \
		VkPipelineShaderStageCreateInfo stage_info = {}; \
		stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO; \
		stage_info.stage = stage_name; \
		stage_info.module = module; \
		stage_info.pName = "main"; \
		shader_stages.push_back(stage_info); \
	}

	LOAD_SHADER_STAGE(VK_SHADER_STAGE_RAYGEN_BIT_KHR, "shaders/ray_tracing/raytrace.rgen.spv");
	LOAD_SHADER_STAGE(VK_SHADER_STAGE_MISS_BIT_KHR, "shaders/ray_tracing/raytrace.rmiss.spv");
	LOAD_SHADER_STAGE(VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, "shaders/ray_tracing/raytrace.rchit.spv");

	#undef LOAD_SHADER_STAGE

	VkRayTracingShaderGroupCreateInfoKHR raygen_group = {};
	raygen_group.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
	raygen_group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
	raygen_group.generalShader = static_cast<uint32_t>(shader_stages.size() - 3);
	raygen_group.closestHitShader = VK_SHADER_UNUSED_KHR;
	raygen_group.anyHitShader = VK_SHADER_UNUSED_KHR;
	raygen_group.intersectionShader = VK_SHADER_UNUSED_KHR;

	VkRayTracingShaderGroupCreateInfoKHR miss_group = {};
	miss_group.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
	miss_group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
	miss_group.generalShader = static_cast<uint32_t>(shader_stages.size() - 2);
	miss_group.closestHitShader = VK_SHADER_UNUSED_KHR;
	miss_group.anyHitShader = VK_SHADER_UNUSED_KHR;
	miss_group.intersectionShader = VK_SHADER_UNUSED_KHR;

	VkRayTracingShaderGroupCreateInfoKHR hit_group = {};
	hit_group.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
	hit_group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
	hit_group.generalShader = VK_SHADER_UNUSED_KHR;
	hit_group.closestHitShader = static_cast<uint32_t>(shader_stages.size() - 1);
	hit_group.anyHitShader = VK_SHADER_UNUSED_KHR;
	hit_group.intersectionShader = VK_SHADER_UNUSED_KHR;

	std::vector<VkRayTracingShaderGroupCreateInfoKHR> groups = {
		raygen_group,
		miss_group,
		hit_group
	};

	VkPipelineLayoutCreateInfo pipeline_layout_info = {};
	pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_info.setLayoutCount = 1;
	pipeline_layout_info.pSetLayouts = &rt_descriptor.layout;
	pipeline_layout_info.pushConstantRangeCount = 0;
	pipeline_layout_info.pPushConstantRanges = nullptr;

	VK_CHECK(
		vkCreatePipelineLayout(device, &pipeline_layout_info, nullptr, &rt_pipeline_layout),
		"Failed to create ray tracing pipeline layout"
	);

	VkRayTracingPipelineCreateInfoKHR pipeline_info = {};
	pipeline_info.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
	pipeline_info.stageCount = static_cast<uint32_t>(shader_stages.size());
	pipeline_info.pStages = shader_stages.data();
	pipeline_info.groupCount = static_cast<uint32_t>(groups.size());
	pipeline_info.pGroups = groups.data();
	pipeline_info.maxPipelineRayRecursionDepth = 1;
	pipeline_info.layout = rt_pipeline_layout;

	VK_CHECK(
		vkCreateRayTracingPipelinesKHR(device, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &rt_pipeline),
		"Failed to create ray tracing pipeline"
	);

	for (auto & stage : shader_stages)
	{
		vkDestroyShaderModule(device, stage.module, nullptr);
	}
}

uint32_t alignedSize(uint32_t value, uint32_t alignment)
{
	return (value + alignment - 1) & ~(alignment - 1);
}

void VulkanAPI::createRayTracingShaderBindingTable()
{
	uint32_t miss_count = 1;
	uint32_t hit_count = 1;
	uint32_t handle_count = 1 + miss_count + hit_count;
	uint32_t handle_size = ray_tracing_properties.shaderGroupHandleSize;
	uint32_t handle_size_aligned = alignedSize(handle_size, ray_tracing_properties.shaderGroupHandleAlignment);

	rt_sbt_rgen_region.stride = alignedSize(handle_size_aligned, ray_tracing_properties.shaderGroupBaseAlignment);
	rt_sbt_rgen_region.size = rt_sbt_rgen_region.stride;
	rt_sbt_miss_region.stride = handle_size_aligned;
	rt_sbt_miss_region.size = alignedSize(handle_size_aligned * miss_count, ray_tracing_properties.shaderGroupBaseAlignment);
	rt_sbt_hit_region.stride = handle_size_aligned;
	rt_sbt_hit_region.size = alignedSize(handle_size_aligned * hit_count, ray_tracing_properties.shaderGroupBaseAlignment);

	uint32_t data_size = handle_count * handle_size;
	std::vector<uint8_t> handles(data_size);
	VK_CHECK(
		vkGetRayTracingShaderGroupHandlesKHR(device, rt_pipeline, 0, handle_count, data_size, handles.data()),
		"Failed to get ray tracing shader group handles"
	);

	uint32_t sbt_size = rt_sbt_rgen_region.size + rt_sbt_miss_region.size + rt_sbt_hit_region.size;
	createBuffer(
		sbt_size,
		VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		rt_sbt_buffer,
		rt_sbt_buffer_memory
	);

	VkDeviceAddress sbt_address = getBufferDeviceAddress(rt_sbt_buffer);
	rt_sbt_rgen_region.deviceAddress = sbt_address;
	rt_sbt_miss_region.deviceAddress = sbt_address + rt_sbt_rgen_region.size;
	rt_sbt_hit_region.deviceAddress = sbt_address + rt_sbt_rgen_region.size + rt_sbt_miss_region.size;

	auto getHandle = [&] (int i) { return handles.data() + i * handle_size; };

	uint32_t handle_index = 0;
	void * mapped_memory_tmp;
	vkMapMemory(device, rt_sbt_buffer_memory, 0, sbt_size, 0, &mapped_memory_tmp);

	uint8_t * mapped_memory = static_cast<uint8_t *>(mapped_memory_tmp);

	// raygen
	uint8_t * data = mapped_memory;
	memcpy(data, getHandle(handle_index++), handle_size);

	// miss
	data = mapped_memory + rt_sbt_rgen_region.size;
	for (uint32_t i = 0; i < miss_count; i++)
	{
		memcpy(data, getHandle(handle_index++), handle_size);
		data += rt_sbt_rgen_region.stride;
	}

	// hit
	data = mapped_memory + rt_sbt_rgen_region.size + rt_sbt_miss_region.size;
	for (uint32_t i = 0; i < hit_count; i++)
	{
		memcpy(data, getHandle(handle_index++), handle_size);
		data += rt_sbt_rgen_region.stride;
	}

	vkUnmapMemory(device, rt_sbt_buffer_memory);
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
	init_info.ImageCount = static_cast<uint32_t>(swapchain.images.size());
	init_info.UseDynamicRendering = VK_TRUE;
	init_info.PipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
	init_info.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
	init_info.PipelineRenderingCreateInfo.pColorAttachmentFormats = &swapchain.image_format;

	ImGui_ImplVulkan_Init(&init_info);
}

void VulkanAPI::setupTracy()
{
	const char * const ctx_name = "Gpu rendering";
	(void)ctx_name;
	ctx = TracyVkContextCalibrated(
		physical_device,
		device,
		graphics_queue,
		draw_command_buffers[0],
		vkGetPhysicalDeviceCalibrateableTimeDomainsEXT,
		vkGetCalibratedTimestampsEXT
	);
	TracyVkContextName(ctx, ctx_name, strlen(ctx_name));
}

void VulkanAPI::destroyTracy()
{
	TracyVkDestroy(ctx);
}

VkCommandBuffer VulkanAPI::beginSingleTimeCommands()
{
	VK_CHECK(
		vkResetFences(device, 1, &single_time_command_fence),
		"Failed to reset single time command buffer fence"
	);

	VkCommandBufferAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	alloc_info.commandPool = command_pool;
	alloc_info.commandBufferCount = 1;

	VkCommandBuffer command_buffer;
	VK_CHECK(
		vkAllocateCommandBuffers(device, &alloc_info, &command_buffer),
		"Failed to allocate single time command buffer"
	);

	VkCommandBufferBeginInfo begin_info = {};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	VK_CHECK(
		vkBeginCommandBuffer(command_buffer, &begin_info),
		"Failed to begin single time command buffer"
	);

	return command_buffer;
}

void VulkanAPI::endSingleTimeCommands(VkCommandBuffer command_buffer)
{
	VK_CHECK(
		vkEndCommandBuffer(command_buffer),
		"Failed to end single time command buffer"
	);


	VkSubmitInfo submit_info = {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &command_buffer;

	VK_CHECK(
		vkQueueSubmit(graphics_queue, 1, &submit_info, single_time_command_fence),
		"Failed to submit single time command buffer"
	);
	VK_CHECK(
		vkWaitForFences(device, 1, &single_time_command_fence, VK_TRUE, UINT64_MAX),
		"Failed to wait for single time command buffer"
	);

	vkFreeCommandBuffers(device, command_pool, 1, &command_buffer);
}


void VulkanAPI::drawMesh(
	const Pipeline & pipeline,
	const uint64_t mesh_id,
	const void * push_constants,
	const uint32_t push_constants_size,
	VkShaderStageFlags push_constants_stage
)
{
	Mesh mesh;
	{
		std::lock_guard lock(mesh_map_mutex);
		if (!mesh_map.contains(mesh_id))
		{
			LOG_WARNING("Mesh " << mesh_id << " not found in the mesh map.");
			return;
		}

		mesh = mesh_map.at(mesh_id);

		if (mesh.buffer == VK_NULL_HANDLE)
		{
			LOG_WARNING("Mesh " << mesh_id << " has a null buffer.");
			return;
		}

		mesh_map.at(mesh_id).used_by_frame[current_frame] = true;
	}

	const VkBuffer vertex_buffers[] = { mesh.buffer };
	const VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(
		draw_command_buffers[current_frame],
		0, 1,
		vertex_buffers,
		offsets
	);

	vkCmdBindIndexBuffer(
		draw_command_buffers[current_frame],
		mesh.buffer,
		mesh.index_offset,
		VK_INDEX_TYPE_UINT32
	);

	if (push_constants_size > 0)
	{
		vkCmdPushConstants(
			draw_command_buffers[current_frame],
			pipeline.layout,
			push_constants_stage,
			0,
			push_constants_size,
			push_constants
		);
	}

	vkCmdDrawIndexed(
		draw_command_buffers[current_frame],
		static_cast<uint32_t>(mesh.index_count),
		1, 0, 0, 0
	);
}



VkDeviceAddress VulkanAPI::getBufferDeviceAddress(VkBuffer buffer)
{
	VkBufferDeviceAddressInfo buffer_device_address_info = {};
	buffer_device_address_info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
	buffer_device_address_info.buffer = buffer;

	return vkGetBufferDeviceAddressKHR(device, &buffer_device_address_info);
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
	alloc_info.memoryTypeIndex = vk_helper::findMemoryType(physical_device, mem_requirements.memoryTypeBits, properties);

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

void VulkanAPI::setImageLayout(
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
	// VkCommandBufferAllocateInfo alloc_info = {};
	// alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	// alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	// alloc_info.commandPool = command_pool;
	// alloc_info.commandBufferCount = 1;

	// VkCommandBuffer command_buffer;
	// VK_CHECK(
	// 	vkAllocateCommandBuffers(device, &alloc_info, &command_buffer),
	// 	"Failed to allocate command buffers"
	// );

	std::unique_lock lock(transfer_operation_mutex);

	vkResetCommandBuffer(transfer_command_buffers, 0);

	VkCommandBufferBeginInfo begin_info = {};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	VK_CHECK(
		vkBeginCommandBuffer(transfer_command_buffers, &begin_info),
		"Failed to begin recording command buffer"
	);

	VkBufferCopy copy_region = {};
	copy_region.size = size;

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

	// vkFreeCommandBuffers(device, command_pool, 1, &command_buffer);
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

