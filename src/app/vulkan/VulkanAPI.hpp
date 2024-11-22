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
#include "TextRenderer.hpp"
#include "ShaderCommon.hpp"
#include "Buffer.hpp"
#include "Transform.hpp"
#include "Camera.hpp"
#include "Model.hpp"
#include "Item.hpp"
#include "MemoryRange.hpp"
#include "Settings.hpp"
#include "DebugGui.hpp"

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
#include <list>

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
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions(1);

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(LineVertex, pos);

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

struct ItemVertex
{
	glm::vec3 pos;
	glm::vec3 normal;
	glm::vec2 tex_coord;
	uint32_t texture_index;

	static VkVertexInputBindingDescription getBindingDescription()
	{
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(ItemVertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions()
	{
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions(4);

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(ItemVertex, pos);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(ItemVertex, normal);

		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(ItemVertex, tex_coord);

		attributeDescriptions[3].binding = 0;
		attributeDescriptions[3].location = 3;
		attributeDescriptions[3].format = VK_FORMAT_R32_UINT;
		attributeDescriptions[3].offset = offsetof(ItemVertex, texture_index);

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

	VkDeviceAddress buffer_address;

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

struct GlobalDescriptorWrite
{
	uint32_t dst_binding = 0;
    uint32_t dst_array_element = 0;
    uint32_t descriptor_count = 1;
	VkDescriptorType descriptor_type = VK_DESCRIPTOR_TYPE_MAX_ENUM;
	VkDescriptorImageInfo image_info = {};
	VkDescriptorBufferInfo buffer_info = {};
};

struct ChunkRenderData
{
	uint64_t block_mesh_id;
	uint64_t water_mesh_id;
	glm::dmat4 model;
};

struct MeshRenderData
{
	uint64_t id;
	glm::dmat4 model;
};

struct PlayerRenderData
{
	glm::dvec3 position;
	double yaw = 0;
	double pitch = 0;

	PlayerModel::WalkAnimation walk_animation;
	PlayerModel::AttackAnimation attack_animation;

	bool visible = true;
};

struct InstanceData
{
	alignas(16) mat4 matrice;
	VkDeviceAddress block_vertex_buffer;
	VkDeviceAddress water_vertex_buffer;
};

// forward declaration
struct BlockVertex;

class VulkanAPI
{

public:

	typedef uint32_t InstanceId;

	VulkanAPI(GLFWwindow * window);
	~VulkanAPI();

	VulkanAPI(const VulkanAPI &) = delete;
	VulkanAPI(VulkanAPI &&) = delete;
	VulkanAPI & operator=(const VulkanAPI &) = delete;
	VulkanAPI & operator=(VulkanAPI &&) = delete;

	void renderFrame();


	void setCamera(const Camera & camera);
	void toggleDebugText();
	void setTargetBlock(const std::optional<glm::vec3> & target_block);

	IdList<uint64_t, MeshRenderData> entity_mesh_list;

	std::map<uint64_t, PlayerRenderData> players;
	mutable TracyLockableN(std::mutex, m_player_mutex, "Player Render Data");

	// hud
	std::array<ItemInfo::Type, 9> toolbar_items;
	mutable std::mutex toolbar_items_mutex;
	std::atomic<int> toolbar_cursor_index = 0;


	struct ChunkMeshCreateInfo
	{
		std::vector<BlockVertex> block_vertex;
		std::vector<uint32_t> block_index;

		std::vector<BlockVertex> water_vertex;
		std::vector<uint32_t> water_index;

		glm::dmat4 model;
	};
	InstanceId addChunkToScene(
		const ChunkMeshCreateInfo & mesh_info
	);
	void removeChunkFromScene(const uint64_t chunk_id);

	uint64_t storeMesh(
		const void * vertices,
		const uint32_t vertex_count,
		const uint32_t vertex_size,
		const void * indices,
		const uint32_t index_count
	);
	void destroyMesh(const uint64_t & mesh_id);
	uint64_t getCubeMeshId() const { return cube_mesh_id; }


	uint64_t createImGuiTexture(const uint32_t width, const uint32_t height);
	void ImGuiTexturePutPixel(
		const uint64_t texture_id,
		const uint32_t x,
		const uint32_t y,
		const uint8_t r,
		const uint8_t g,
		const uint8_t b,
		const uint8_t a = 255
	);
	void ImGuiTextureClear(const uint64_t texture_id);
	void ImGuiTextureDraw(const uint64_t texture_id);

private:

	//###########################################################################################################
	//																											#
	//													Generics												#
	//																											#
	//###########################################################################################################

	const std::vector<const char *> validation_layers = {
		"VK_LAYER_KHRONOS_validation"
	};

	const std::vector<const char *> device_extensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
		VK_EXT_CALIBRATED_TIMESTAMPS_EXTENSION_NAME,
		VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
		VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
		VK_KHR_8BIT_STORAGE_EXTENSION_NAME,
		VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME
	};

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

	mutable TracyLockableN (std::mutex, global_mutex, "Vulkan Global Mutex");

	// function pointers
	PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT;
	PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT;

	PFN_vkGetPhysicalDeviceCalibrateableTimeDomainsEXT vkGetPhysicalDeviceCalibrateableTimeDomainsEXT;
	PFN_vkGetCalibratedTimestampsEXT vkGetCalibratedTimestampsEXT;

	PFN_vkGetBufferDeviceAddress vkGetBufferDeviceAddress;


	void _createInstance();
	bool _checkValidationLayerSupport();
	std::vector<const char *> _getRequiredExtensions();

	void _loadVulkanFunctions();

	void _createSurface(GLFWwindow * window);

	void _setupDebugMessenger();
	void _populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT & create_info);
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
		VkDebugUtilsMessageTypeFlagsEXT message_type,
		const VkDebugUtilsMessengerCallbackDataEXT * callback_data,
		void * user_data
	);

	void _pickPhysicalDevice();
	bool _isDeviceSuitable(VkPhysicalDevice device);
	int _ratePhysicalDevice(VkPhysicalDevice device);
	QueueFamilyIndices _findQueueFamilies(VkPhysicalDevice device);
	bool _checkDeviceExtensionSupport(VkPhysicalDevice device);

	void _createLogicalDevice();

	//###########################################################################################################
	//																											#
	//											Swapchain and framebuffers										#
	//																											#
	//###########################################################################################################

	Swapchain swapchain;

	std::vector<VkFramebuffer> lighting_framebuffers;
	std::vector<VkFramebuffer> shadow_framebuffers;
	std::vector<VkFramebuffer> water_framebuffers;
	std::vector<VkFramebuffer> hud_framebuffers;
	VkFramebuffer prerender_item_icon_framebuffer;

	void _createSwapChain(GLFWwindow * window);
	void _recreateSwapChain(GLFWwindow * window);
	VkSurfaceFormatKHR _chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> & available_formats);
	VkPresentModeKHR _chooseSwapPresentMode(const std::vector<VkPresentModeKHR> & available_present_modes);
	VkExtent2D _chooseSwapExtent(const VkSurfaceCapabilitiesKHR & capabilities, GLFWwindow * window);

	void _createFramebuffers();
	void _destroyFramebuffers();

	//###########################################################################################################
	//																											#
	//												Command buffer												#
	//																											#
	//###########################################################################################################

	VkCommandPool command_pool;
	std::vector<VkCommandBuffer> draw_shadow_pass_command_buffers;
	std::vector<VkCommandBuffer> draw_command_buffers;
	std::vector<VkCommandBuffer> copy_command_buffers;
	std::vector<VkCommandBuffer> compute_command_buffers;
	std::vector<VkCommandBuffer> imgui_command_buffers;

	VkCommandPool transfer_command_pool;
	VkCommandBuffer transfer_command_buffers;
	TracyLockableN (std::mutex, transfer_operation_mutex, "Vulkan Transfer Operation");

	void _createCommandPool();
	void _createCommandBuffer();

	//###########################################################################################################
	//																											#
	//												Synchronisation												#
	//																											#
	//###########################################################################################################

	std::vector<VkSemaphore> image_available_semaphores;
	std::vector<VkSemaphore> shadow_pass_finished_semaphores;
	std::vector<VkSemaphore> main_render_finished_semaphores;
	std::vector<VkSemaphore> compute_finished_semaphores;
	std::vector<VkSemaphore> copy_finished_semaphores;
	std::vector<VkSemaphore> imgui_render_finished_semaphores;
	std::vector<VkFence> in_flight_fences;
	VkFence single_time_command_fence;

	void _createSyncObjects();

	//###########################################################################################################
	//																											#
	//													Shadow maps												#
	//																											#
	//###########################################################################################################

	const uint shadow_maps_count = SHADOW_MAP_MAX_COUNT;
	const uint shadow_map_size;

	Image shadow_map_depth_attachement;

	std::vector<VkImageView> shadow_map_views;

	void _createShadowMapRessources();
	void _destroyShadowMapRessources();

	//###########################################################################################################
	//																											#
	//											Images and buffers												#
	//																											#
	//###########################################################################################################

	Image output_attachement;
	Image color_attachement;
	Image depth_attachement;
	Image block_textures;
	Image skybox_cube_map;

	Image crosshair_image;
	Image toolbar_image;
	Image toolbar_cursor_image;
	Image player_skin_image;
	Image item_icon_images;

	Image debug_info_image;
	std::vector<Buffer> debug_info_buffers;

	UBO camera_ubo;
	UBO light_mat_ubo;
	UBO atmosphere_ubo;

	void _createColorAttachement();
	void _createDepthAttachement();

	void _createUBO(UBO & ubo, const VkDeviceSize size, const uint32_t count);
	void _createUniformBuffers();
	void _createTextureArray(const std::vector<std::string> & file_paths, uint32_t size);
	void _createCubeMap(const std::array<std::string, 6> & file_paths, uint32_t size);
	void _createHudImages(
		const std::string & file_path,
		Image & image
	);
	void _createTextureImage();

	//###########################################################################################################
	//																											#
	//												Instance data												#
	//																											#
	//###########################################################################################################

	uint32_t instance_data_size;
	uint32_t instance_data_max_count;
	std::vector<Buffer> instance_data_buffers;

	void _createInstanceData();
	void _destroyInstanceData();

	void _updateInstancesData();

	//###########################################################################################################
	//																											#
	//													Descriptors												#
	//																											#
	//###########################################################################################################

	Descriptor crosshair_image_descriptor;
	Descriptor toolbar_image_descriptor;
	Descriptor toolbar_cursor_image_descriptor;
	Descriptor debug_info_image_descriptor;
	Descriptor global_descriptor;

	void _createHudDescriptors(
		const Image & image,
		Descriptor & descriptor
	);
	void _createDescriptors();
	void _createGlobalDescriptor();
	void _updateGlobalDescriptor();

	//###########################################################################################################
	//																											#
	//													Render pass												#
	//																											#
	//###########################################################################################################

	VkRenderPass lighting_render_pass;
	VkRenderPass shadow_render_pass;
	VkRenderPass water_render_pass;
	VkRenderPass hud_render_pass;
	VkRenderPass prerender_item_icon_render_pass;

	void _createRenderPass();

	//###########################################################################################################
	//																											#
	//													Pipelines												#
	//																											#
	//###########################################################################################################

	Pipeline chunk_pipeline;
	Pipeline water_pipeline;
	Pipeline line_pipeline;
	Pipeline skybox_pipeline;
	Pipeline sun_pipeline;
	Pipeline shadow_pipeline;
	Pipeline test_image_pipeline;
	Pipeline entity_pipeline;
	Pipeline player_pipeline;
	Pipeline hud_pipeline;
	Pipeline prerender_item_icon_pipeline;
	Pipeline item_icon_pipeline;

	void _createPipelines();

	//###########################################################################################################
	//																											#
	//												ImGui and Tracy												#
	//																											#
	//###########################################################################################################

	VkDescriptorPool imgui_descriptor_pool;
	std::unordered_map<uint64_t, ImGuiTexture> imgui_textures;
	uint64_t next_imgui_texture_id = 1;
	TracyLockableN (std::mutex, imgui_textures_mutex, "Vulkan Imgui Texture Mutex");

	TracyVkCtx draw_ctx;

	void _setupImgui();
	void _destroyImGuiTextures();

	void _setupTracy();
	void _destroyTracy();

	//###########################################################################################################
	//																											#
	//													Meshes													#
	//																											#
	//###########################################################################################################

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

	void _createMeshes();
	void _createItemMeshes();
	void _destroyMesh(const uint64_t & mesh_id);
	void _destroyMeshes();

	//###########################################################################################################
	//																											#
	//													Chunks													#
	//																											#
	//###########################################################################################################

	struct ChunkMeshesInfo
	{
		Buffer vertex_buffer;

		VkDeviceAddress block_vertex_address;
		uint32_t block_index_offset;
		uint32_t block_index_count;

		VkDeviceAddress water_vertex_address;
		uint32_t water_index_offset;
		uint32_t water_index_count;

		glm::dmat4 model;

		union
		{
			uint64_t is_used = 0;
			uint8_t used_by_frame[8];
		};
	};

	Buffer m_chunks_indices_buffer;
	MemoryRange m_chunks_indices_buffer_memory_range;

	std::map<InstanceId, ChunkMeshesInfo> m_chunks_in_scene;
	std::map<InstanceId, glm::dmat4> m_chunks_in_scene_rendered;
	std::list<InstanceId> m_free_chunk_ids;
	const InstanceId m_null_instance_id = 0;
	std::vector<InstanceId> m_chunk_instance_to_destroy;
	mutable TracyLockableN(std::mutex, m_chunks_in_scene_mutex, "Chunk Render Data");

	void _setupChunksRessources();
	void _resizeChunksIndicesBuffer(const VkDeviceSize & size);
	void _deleteUnusedChunks();

	std::map<InstanceId, glm::dmat4> _getChunksInScene() const;

	void _bindChunkIndexBuffer(VkCommandBuffer command_buffer);
	void _drawChunksBlock(
		VkCommandBuffer command_buffer,
		Buffer & indirect_buffer,
		const std::vector<InstanceId> & ids
	);
	void _drawChunkWater(VkCommandBuffer command_buffer, const InstanceId & id);

	std::vector<PlayerRenderData> _getPlayers() const;


	//###########################################################################################################
	//																											#
	//												Indirect Draw												#
	//																											#
	//###########################################################################################################

	uint32_t m_max_draw_count;
	std::vector<std::vector<Buffer>> m_draw_chunk_block_shadow_pass_buffer;
	std::vector<Buffer> m_draw_chunk_block_light_pass_buffer;

	void _createDrawBuffer();
	void _destroyDrawBuffer();

	void _drawChunkBlock(VkCommandBuffer command_buffer, const InstanceId & id);

	//###########################################################################################################
	//																											#
	//													Text													#
	//																											#
	//###########################################################################################################

	TextRenderer text_renderer;

	void _setupTextRenderer();
	void _destroyTextRenderer();

	//###########################################################################################################
	//																											#
	//												Prerender													#
	//																											#
	//###########################################################################################################

	void _prerenderItemIconImages();

	//###########################################################################################################
	//																											#
	//													Helper													#
	//																											#
	//###########################################################################################################

	void _transitionImageLayout(
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
	void _setImageLayout(
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
	VkFormat _findSupportedFormat(
		const std::vector<VkFormat> & candidates,
		VkImageTiling tiling,
		VkFormatFeatureFlags features
	);
	void _createImage(
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
	void _createImageView(
		VkImage image,
		VkFormat format,
		VkImageAspectFlags aspect_flags,
		VkImageView & image_view
	);
	void _createBuffer(
		VkDeviceSize size,
		VkBufferUsageFlags usage,
		VkMemoryPropertyFlags properties,
		VkBuffer & buffer,
		VkDeviceMemory & buffer_memory
	);
	void _copyBuffer(
		VkBuffer src_buffer,
		VkBuffer dst_buffer,
		const VkBufferCopy & copy_region
	);
	void _copyBufferToImage(
		VkBuffer buffer,
		VkImage image,
		uint32_t width,
		uint32_t height
	);

	//###########################################################################################################
	//																											#
	//												Render update												#
	//																											#
	//###########################################################################################################

	Camera m_camera_update;

	bool m_show_debug_text_update = false;

	std::optional<glm::vec3> m_target_block_update;

	std::map<InstanceId, ChunkMeshCreateInfo> m_chunk_to_create;

	// IdList<uint64_t, MeshRenderData> entity_mesh_list;

	// std::map<uint64_t, PlayerRenderData> players;
	// mutable TracyLockableN(std::mutex, m_player_mutex, "Player Render Data");

	// // hud
	// std::array<ItemInfo::Type, 9> toolbar_items;
	// mutable std::mutex toolbar_items_mutex;
	// std::atomic<int> toolbar_cursor_index = 0;

	mutable TracyLockableN(std::mutex, m_render_data_update_mutex, "Render Data Update");

	void _updateRenderData();

	void _updateChunksData();

	//###########################################################################################################
	//																											#
	//												Render loop													#
	//																											#
	//###########################################################################################################

	const int m_max_frames_in_flight = 2;
	int m_current_frame = 0;
	uint32_t m_current_image_index = 0;

	std::chrono::nanoseconds m_start_time;
	std::chrono::nanoseconds m_current_time;
	std::chrono::nanoseconds m_last_frame_time;
	std::chrono::nanoseconds m_delta_time;

	// For DebugGui
	int m_frame_count;
	std::chrono::nanoseconds m_start_time_counting_fps;

	// Scene data
	int m_window_width, m_window_height;
	double m_aspect_ratio;

	const glm::mat4 m_clip = glm::mat4(
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f,-1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.5f, 0.0f,
		0.0f, 0.0f, 0.5f, 1.0f
	);
	Camera::RenderInfo m_camera_render_info;
	ViewProjMatrices m_camera_matrices = {};
	ViewProjMatrices m_camera_matrices_fc = {};

	std::map<VulkanAPI::InstanceId, glm::dmat4> m_chunk_meshes;
	std::vector<MeshRenderData> m_entity_meshes;
	std::vector<PlayerRenderData> m_players;

	std::vector<VulkanAPI::InstanceId> m_visible_chunks;
	std::vector<std::vector<VulkanAPI::InstanceId>> m_shadow_visible_chunks;

	ViewProjMatrices m_sun_matrices = {};
	glm::dvec3 m_sun_position;

	std::optional<glm::vec3> m_target_block;
	mutable TracyLockableN(std::mutex, m_target_block_mutex, "Target Block");


	AtmosphereParams m_atmosphere_params = {};

	std::vector<glm::mat4> m_light_view_proj_matrices;
	ShadowMapLight m_shadow_map_light = {};

	bool m_show_debug_text = false;
	std::string m_debug_text;

	std::array<ItemInfo::Type, 9> m_toolbar_items;
	int m_toolbar_cursor_index = 0;

	void _createRenderFrameRessources();

	void _prepareFrame();
	void _updateTime();
	void _updateDebugText();
	void _updateVisibleChunks();

	void _startFrame();
	void _endFrame();

	void _shadowPass();
	void _lightingPass();

	void _drawPlayerBodyPart(
		const uint64_t mesh_id,
		const glm::mat4 & model
	);

	void _copyToSwapchain();

	void _drawDebugGui();
	void _updateImGui();

	std::vector<glm::mat4> _getCSMLightViewProjMatrices(
		const glm::vec3 & light_dir,
		const std::vector<float> & split,
		const float blend_distance,
		const glm::mat4 & camera_view,
		const float cam_fov,
		const float cam_ratio,
		const float cam_near_plane,
		const float cam_far_plane,
		std::vector<float> & far_plane_distances
	);

	bool _isInsideFrustum_ndcSpace(const glm::mat4 & model, const glm::vec3 & size) const;
	bool _isInsideFrustum_planes(
		const glm::mat4 & view_proj,
		const glm::mat4 & model,
		const glm::vec3 & size
	) const;

	std::pair<bool, Mesh> _getMesh(const uint64_t id);
	void _drawMesh(
		VkCommandBuffer command_buffer,
		const Pipeline & pipeline,
		const uint64_t mesh_id,
		const void * push_constants,
		const uint32_t push_constants_size,
		const VkShaderStageFlags push_constants_stage,
		const uint32_t instance_id = 0
	);

	void _writeTextToDebugImage(
		VkCommandBuffer command_buffer,
		const std::string & text,
		const uint32_t x,
		const uint32_t y,
		const uint32_t font_size
	);

	void _drawHudImage(
		const Descriptor & descriptor,
		const VkViewport & viewport
	);
	void _drawItemIcon(
		const VkViewport & viewport,
		const uint32_t layer
	);

};
