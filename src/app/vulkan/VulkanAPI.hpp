#pragma once

#include "vk_define.hpp"
#include "vk_helper.hpp"
#include "VulkanMemoryAllocator.hpp"
#include "Command.hpp"
#include "Image.hpp"
#include "Descriptor.hpp"
#include "Swapchain.hpp"
#include "Pipeline.hpp"
#include "Chunk.hpp"
#include "CreateMeshData.hpp"
#include "List.hpp"

#include "Tracy.hpp"
#include "tracy_globals.hpp"
#include "TracyVulkan.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

#include "Tracy.hpp"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#include <stdexcept>
#include <vector>
#include <optional>
#include <unordered_map>
#include <mutex>
#include <map>
#include <queue>
#include <memory>

#define VK_CHECK_RESULT(f)																				\
{																										\
	VkResult res = (f);																					\
	if (res != VK_SUCCESS)																				\
	{																									\
		throw ("fatal error");																	\
	}																									\
}

struct QueueFamilyIndices
{
	std::optional<uint32_t> graphics_family;
	std::optional<uint32_t> present_family;
	std::optional<uint32_t> transfer_family;

	bool isComplete()
	{
		return graphics_family.has_value() && present_family.has_value() && transfer_family.has_value();
	}
};

struct LineVertex
{
	glm::vec3 pos;
	glm::vec3 color;

	static VkVertexInputBindingDescription getBindingDescription()
	{
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(LineVertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions()
	{
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions(2);

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(LineVertex, pos);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(LineVertex, color);

		return attributeDescriptions;
	}
};

struct EntityVertex
{
	glm::vec3 pos;
	glm::vec3 normal;

	static VkVertexInputBindingDescription getBindingDescription()
	{
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(EntityVertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions()
	{
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions(2);

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(EntityVertex, pos);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(EntityVertex, normal);

		return attributeDescriptions;
	}
};

struct Mesh
{
	VkBuffer buffer;
	VkDeviceMemory buffer_memory;
	uint32_t vertex_count;
	uint32_t vertex_size;
	VkDeviceSize index_offset;
	uint32_t index_count;

	uint32_t memory_size;

	union
	{
		uint64_t is_used = 0;
		uint8_t used_by_frame[8];
	};
};

struct UBO
{
	std::vector<VkBuffer> buffers;
	std::vector<VkDeviceMemory> memory;
	std::vector<void *> mapped_memory;
};

struct ViewProjMatrices
{
	glm::mat4 view;
	glm::mat4 proj;
};

struct ModelMatrice
{
	glm::mat4 model;
};

struct EntityMatrices
{
	glm::mat4 model;
	glm::vec4 color;
};

struct ShadowMapLight
{
	glm::mat4 view;
	glm::mat4 proj;
	glm::mat4 model;
};

struct GuiTextureData
{
	glm::vec2 position;
	glm::vec2 size;
};

struct AtmosphereParams
{
	glm::vec3 sun_direction;
	float earth_radius;
	float atmosphere_radius;
	float player_height;
	float h_rayleigh;
	float h_mie;
	float g;
	float sun_intensity;
	float n_samples;
	float n_light_samples;
	alignas(16) glm::vec3 beta_rayleigh;
	alignas(16) glm::vec3 beta_mie;
};

struct ImGuiTexture
{
	VkImage image;
	VkDeviceMemory memory;
	VkImageView view;
	VkSampler sampler;

	void * mapped_memory;

	VkDescriptorSet descriptor_set;

	VkFormat format;
	VkExtent2D extent;

	uint32_t width() const { return extent.width; }
	uint32_t height() const { return extent.height; }

	void clear()
	{
		memset(mapped_memory, 0, extent.width * extent.height * 4);
	}

	void putPixel(uint32_t x, uint32_t y, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
	{
		uint8_t * pixel = (uint8_t *)mapped_memory + (y * extent.width + x) * 4;
		pixel[0] = r;
		pixel[1] = g;
		pixel[2] = b;
		pixel[3] = a;
	}
};

struct RTPushConstant
{
	float time;
};

struct RTCameraMatrices
{
	glm::mat4 view;
	glm::mat4 proj;
	glm::mat4 last_view;
	glm::mat4 last_proj;
};

class VulkanAPI
{

public:

	VulkanAPI(GLFWwindow * window);
	~VulkanAPI();

	VulkanAPI(const VulkanAPI &) = delete;
	VulkanAPI(VulkanAPI &&) = delete;
	VulkanAPI & operator=(const VulkanAPI &) = delete;
	VulkanAPI & operator=(VulkanAPI &&) = delete;

	void transitionImageLayout(
		VkImage image,
		VkImageLayout oldLayout,
		VkImageLayout newLayout,
		VkImageAspectFlags aspectMask,
		uint32_t mipLevels,
		VkAccessFlags srcAccessMask,
		VkAccessFlags dstAccessMask,
		VkPipelineStageFlags srcStageMask,
		VkPipelineStageFlags dstStageMask
	);

	void setImageLayout(
		VkCommandBuffer command_buffer,
		VkImage image,
		VkImageLayout old_layout,
		VkImageLayout new_layout,
		VkImageSubresourceRange subresource_range,
		VkAccessFlags srcAccessMask,
		VkAccessFlags dstAccessMask,
		VkPipelineStageFlags srcStageMask,
		VkPipelineStageFlags dstStageMask
	);

	void recreateSwapChain(GLFWwindow * window);

	uint64_t storeMesh(
		const void * vertices,
		const uint32_t vertex_count,
		const uint32_t vertex_size,
		const void * indices,
		const uint32_t index_count
	);
	void destroyMeshes(const std::vector<uint64_t> & mesh_ids);
	void destroyMesh(const uint64_t & mesh_id);

	void addMeshToScene(const uint64_t mesh_id, const glm::mat4 & transform = glm::mat4(1.0f));
	void removeMeshFromScene(const uint64_t mesh_id);

	void drawMesh(
		const Pipeline & pipeline,
		const uint64_t mesh_id,
		const void * push_constants,
		const uint32_t push_constants_size,
		const VkShaderStageFlags push_constants_stage
	);

	uint64_t createImGuiTexture(const uint32_t width, const uint32_t height);


	VulaknMemoryAllocator vma;

	GLFWwindow * window;

	VkInstance instance;
	VkDebugUtilsMessengerEXT debug_messenger;
	VkPhysicalDevice physical_device = VK_NULL_HANDLE;

	VkSurfaceKHR surface;

	VkDevice device;
	VkQueue graphics_queue;
	VkQueue present_queue;
	VkQueue transfer_queue;
	QueueFamilyIndices queue_family_indices;

	Swapchain swapchain;
	std::vector<VkFramebuffer> lighting_framebuffers;
	std::vector<VkFramebuffer> shadow_framebuffers;

	VkCommandPool command_pool;
	std::vector<VkCommandBuffer> draw_command_buffers;
	std::vector<VkCommandBuffer> copy_command_buffers;
	std::vector<VkCommandBuffer> compute_command_buffers;
	std::vector<VkCommandBuffer> imgui_command_buffers;

	VkCommandPool transfer_command_pool;
	VkCommandBuffer transfer_command_buffers;
	TracyLockableN (std::mutex, transfer_operation_mutex, "Vulkan Transfer Operation");

	std::vector<VkSemaphore> image_available_semaphores;
	std::vector<VkSemaphore> main_render_finished_semaphores;
	std::vector<VkSemaphore> compute_finished_semaphores;
	std::vector<VkSemaphore> copy_finished_semaphores;
	std::vector<VkSemaphore> imgui_render_finished_semaphores;
	std::vector<VkFence> in_flight_fences;
	VkFence single_time_command_fence;

	const int max_frames_in_flight = 2;
	int current_frame = 0;
	uint32_t current_image_index = 0;


	Image color_attachement;
	Image depth_attachement;
	Image block_textures;
	Image skybox_cube_map;
	Image shadow_map_depth_attachement;

	Image crosshair_image;
	Image player_skin_image;

	UBO camera_ubo;
	UBO sun_ubo;
	UBO atmosphere_ubo;

	// Buffers for the line vertices and indices for the frustum
	std::vector<VkBuffer> frustum_line_buffers;
	std::vector<VkDeviceMemory> frustum_line_buffers_memory;
	std::vector<void *> frustum_line_buffers_mapped_memory;
	VkDeviceSize frustum_line_index_offset;
	uint32_t frustum_line_vertex_count;
	uint32_t frustum_line_index_count;

	Descriptor swapchain_image_descriptor;
	Descriptor camera_descriptor;
	Descriptor block_textures_descriptor;
	Descriptor cube_map_descriptor;
	Descriptor shadow_map_descriptor;
	Descriptor test_image_descriptor;
	Descriptor sun_descriptor;
	Descriptor crosshair_image_descriptor;
	Descriptor player_skin_image_descriptor;
	Descriptor atmosphere_descriptor;

	VkRenderPass lighting_render_pass;
	VkRenderPass shadow_render_pass;

	Pipeline chunk_pipeline;
	Pipeline line_pipeline;
	Pipeline skybox_pipeline;
	Pipeline sun_pipeline;
	Pipeline shadow_pipeline;
	Pipeline test_image_pipeline;
	Pipeline entity_pipeline;
	Pipeline player_pipeline;
	Pipeline gui_pipeline;

	// Dear ImGui resources
	VkDescriptorPool imgui_descriptor_pool;
	ImGuiTexture imgui_texture;

	// Meshes
	std::unordered_map<uint64_t, Mesh> mesh_map;
	uint64_t next_mesh_id = 1;
	const uint64_t invalid_mesh_id = 0;
	TracyLockableN (std::mutex, mesh_map_mutex, "Vulkan Mesh Map Mutex");
	std::vector<uint64_t> mesh_ids_to_destroy;

	uint64_t cube_mesh_id;
	uint64_t icosphere_mesh_id;

	uint64_t player_chest_mesh_id;
	uint64_t player_head_mesh_id;
	uint64_t player_right_leg_mesh_id;
	uint64_t player_left_leg_mesh_id;
	uint64_t player_right_arm_mesh_id;
	uint64_t player_left_arm_mesh_id;

	// Ray tracing
	VkPhysicalDeviceRayTracingPipelinePropertiesKHR ray_tracing_properties;
	VkPhysicalDeviceAccelerationStructurePropertiesKHR acceleration_structure_properties;

	struct BottomLevelAS
	{
		bool is_used = false;
		VkAccelerationStructureKHR handle = VK_NULL_HANDLE;
		VkBuffer buffer = VK_NULL_HANDLE;
		VkDeviceMemory memory = VK_NULL_HANDLE;
		VkDeviceAddress address = 0;

		VkBuffer transform_buffer = VK_NULL_HANDLE;
		VkDeviceMemory transform_buffer_memory = VK_NULL_HANDLE;
		VkDeviceAddress transform_address = 0;
		void * transform_mapped_memory = nullptr;

		uint64_t mesh_id = 0;
		uint32_t instance_custom_index = 0;
	};

	std::vector<BottomLevelAS> blas_list;

	struct InstanceData
	{
		bool is_used = false;
		glm::mat4 transform = glm::mat4(1.0f);
		uint64_t blas_address = 0;
		uint32_t custom_index = 0;

		VkBuffer buffer = VK_NULL_HANDLE;
		VkDeviceMemory memory = VK_NULL_HANDLE;
		VkDeviceAddress address = 0;
		void * mapped_memory = nullptr;
	};

	std::vector<InstanceData> instance_list;

	VkAccelerationStructureKHR tlas = VK_NULL_HANDLE;
	VkBuffer tlas_buffer = VK_NULL_HANDLE;
	VkDeviceMemory tlas_buffer_memory = VK_NULL_HANDLE;

	struct RTMeshData
	{
		uint64_t vertex_buffer_address = 0;
		uint64_t index_buffer_address = 0;
	};

	VkBuffer rt_mesh_data_buffer = VK_NULL_HANDLE;
	VkDeviceMemory rt_mesh_data_buffer_memory = VK_NULL_HANDLE;
	VkDeviceSize rt_mesh_data_buffer_size = 0;
	uint32_t rt_mesh_data_buffer_count = 0;

	UBO rt_camera_ubo;

	VkImage rt_lighting_image;
	VkDeviceMemory rt_lighting_image_memory;
	VkImageView rt_lighting_image_view;
	uint32_t rt_lighting_image_width;
	uint32_t rt_lighting_image_height;

	VkImage rt_shadow_image;
	VkDeviceMemory rt_shadow_image_memory;
	VkImageView rt_shadow_image_view;
	uint32_t rt_shadow_image_width;
	uint32_t rt_shadow_image_height;

	VkImage rt_output_image;
	VkDeviceMemory rt_output_image_memory;
	VkImageView rt_output_image_view;
	uint32_t rt_output_image_width;
	uint32_t rt_output_image_height;

	Descriptor rt_lighting_shadow_image_descriptor;
	Descriptor rt_output_image_descriptor;
	Descriptor rt_objects_descriptor;
	Descriptor rt_camera_descriptor;

	VkPipelineLayout rt_pipeline_layout;
	VkPipeline rt_pipeline;

	VkBuffer rt_sbt_buffer;
	VkDeviceMemory rt_sbt_buffer_memory;
	VkStridedDeviceAddressRegionKHR rt_sbt_rgen_region = {};
	VkStridedDeviceAddressRegionKHR rt_sbt_miss_region = {};
	VkStridedDeviceAddressRegionKHR rt_sbt_hit_region = {};
	VkStridedDeviceAddressRegionKHR rt_sbt_call_region = {};

	VkPipeline compute_pipeline;
	VkPipelineLayout compute_pipeline_layout;


	TracyLockableN (std::mutex, global_mutex, "Vulkan Global Mutex");


	TracyVkCtx ctx;

	// function pointers
	PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT;
	PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT;

	PFN_vkGetPhysicalDeviceCalibrateableTimeDomainsEXT vkGetPhysicalDeviceCalibrateableTimeDomainsEXT;
	PFN_vkGetCalibratedTimestampsEXT vkGetCalibratedTimestampsEXT;

	PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR;
	PFN_vkDestroyAccelerationStructureKHR vkDestroyAccelerationStructureKHR;
	PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHR;
	PFN_vkCmdBuildAccelerationStructuresKHR vkCmdBuildAccelerationStructuresKHR;
	PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddressKHR;
	PFN_vkCreateRayTracingPipelinesKHR vkCreateRayTracingPipelinesKHR;
	PFN_vkGetRayTracingShaderGroupHandlesKHR vkGetRayTracingShaderGroupHandlesKHR;
	PFN_vkGetBufferDeviceAddressKHR vkGetBufferDeviceAddressKHR;
	PFN_vkCmdTraceRaysKHR vkCmdTraceRaysKHR;


private:

	const std::vector<const char *> validation_layers = {
		"VK_LAYER_KHRONOS_validation"
	};

	std::vector<const char *> device_extensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
		VK_EXT_CALIBRATED_TIMESTAMPS_EXTENSION_NAME,
		VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
		VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
		VK_KHR_RAY_QUERY_EXTENSION_NAME,
		VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
		VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
		VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
		VK_KHR_8BIT_STORAGE_EXTENSION_NAME
	};



	void createInstance();
	bool checkValidationLayerSupport();
	std::vector<const char *> getRequiredExtensions();

	void loadVulkanFunctions();

	void createSurface(GLFWwindow * window);

	void setupDebugMessenger();
	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT & create_info);
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
		VkDebugUtilsMessageTypeFlagsEXT message_type,
		const VkDebugUtilsMessengerCallbackDataEXT * callback_data,
		void * user_data
	);

	void pickPhysicalDevice();
	bool isDeviceSuitable(VkPhysicalDevice device);
	int ratePhysicalDevice(VkPhysicalDevice device);
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
	bool checkDeviceExtensionSupport(VkPhysicalDevice device);

	void createLogicalDevice();

	void createSwapChain(GLFWwindow * window);
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> & available_formats);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> & available_present_modes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR & capabilities, GLFWwindow * window);

	void createCommandPool();
	void createCommandBuffer();

	void createSyncObjects();

	void createColorAttachement();
	void createDepthAttachement();

	void createUBO(UBO & ubo, const VkDeviceSize size, const uint32_t count);
	void createUniformBuffers();
	void createTextureArray(const std::vector<std::string> & file_paths, uint32_t size);
	void createCubeMap(const std::array<std::string, 6> & file_paths, uint32_t size);
	void createFrustumLineBuffers();
	void createTextureImage();

	void createDescriptors();
	void createRenderPass();
	void createPipelines();
	void createFramebuffers();

	void createMeshes();
	void destroyMeshes();

	void setupRayTracing();
	void destroyRayTracing();
	void handleResizeRT();
	void getRayTracingProperties();
	int createBottomLevelAS(uint64_t mesh_id);
	void destroyBottomLevelAS(BottomLevelAS & blas);
	int createInstance(const BottomLevelAS & blas, const glm::mat4 & transform);
	void destroyInstance(InstanceData & instance);
	void createMeshDataBuffer();
	void createTopLevelAS(const std::vector<InstanceData> & instance_list);
	void createRayTracingUBO();
	void createRayTracingImages();
	void createRayTracingDescriptor();
	void updateRTImagesDescriptor();
	void updateRTObjectsDescriptor();
	void createRayTracingPipeline();
	void createComputePipeline();
	void createRayTracingShaderBindingTable();
	std::vector<char> readFile(const std::string & filename);
	VkShaderModule createShaderModule(const std::vector<char> & code);
	void addMeshToTopLevelAS(
		const uint64_t mesh_id,
		const glm::mat4 & transform = glm::mat4(1.0f)
	);
	void removeMeshFromTopLevelAS(const uint64_t mesh_id);
	VkTransformMatrixKHR glmToVkTransformMatrix(const glm::mat4 & matrix);
	void updateInstanceTransform(const uint32_t instance_id, const glm::mat4 & transform);



	void setupImgui();
	void destroyImGuiTexture(ImGuiTexture & imgui_texture);

	void setupTracy();
	void destroyTracy();

	VkCommandBuffer beginSingleTimeCommands();
	void endSingleTimeCommands(VkCommandBuffer command_buffer);

	VkDeviceAddress getBufferDeviceAddress(VkBuffer buffer);
	VkFormat findSupportedFormat(
		const std::vector<VkFormat> & candidates,
		VkImageTiling tiling,
		VkFormatFeatureFlags features
	);
	void createImage(
		uint32_t width,
		uint32_t height,
		uint32_t mip_levels,
		VkFormat format,
		VkImageTiling tiling,
		VkImageUsageFlags usage,
		VkMemoryPropertyFlags properties,
		VkImage & image,
		VkDeviceMemory & image_memory
	);
	void createImageView(
		VkImage image,
		VkFormat format,
		VkImageAspectFlags aspect_flags,
		VkImageView & image_view
	);
	void createBuffer(
		VkDeviceSize size,
		VkBufferUsageFlags usage,
		VkMemoryPropertyFlags properties,
		VkBuffer & buffer,
		VkDeviceMemory & buffer_memory
	);
	void copyBuffer(
		VkBuffer src_buffer,
		VkBuffer dst_buffer,
		VkDeviceSize size
	);
	void copyBufferToImage(
		VkBuffer buffer,
		VkImage image,
		uint32_t width,
		uint32_t height
	);

};
