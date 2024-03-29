#pragma once

#include "vk_define.hpp"
#include "vk_helper.hpp"
#include "VulkanMemoryAllocator.hpp"
#include "Command.hpp"
#include "Image.hpp"
#include "Descriptor.hpp"
#include "Pipeline.hpp"
#include "Chunk.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

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

struct QueueFamilyIndices
{
	std::optional<uint32_t> graphics_family;
	std::optional<uint32_t> present_family;

	bool isComplete()
	{
		return graphics_family.has_value() && present_family.has_value();
	}
};

struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> present_modes;
};

struct BlockVertex
{
	glm::ivec3 pos;
	glm::vec3 normal;
	glm::vec2 texCoord;
	uint32_t texLayer;

	static VkVertexInputBindingDescription getBindingDescription()
	{
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(BlockVertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions()
	{
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions(4);

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SINT;
		attributeDescriptions[0].offset = offsetof(BlockVertex, pos);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(BlockVertex, normal);

		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(BlockVertex, texCoord);

		attributeDescriptions[3].binding = 0;
		attributeDescriptions[3].location = 3;
		attributeDescriptions[3].format = VK_FORMAT_R32_UINT;
		attributeDescriptions[3].offset = offsetof(BlockVertex, texLayer);

		return attributeDescriptions;
	}

	bool operator==(const BlockVertex& other) const
	{
		return pos == other.pos && normal == other.normal && texCoord == other.texCoord;
	}
};

namespace std
{
	template<> struct hash<BlockVertex>
	{
		size_t operator()(const BlockVertex & vertex) const
		{
			return ((hash<glm::vec3>()(vertex.pos) ^
				(hash<glm::vec3>()(vertex.normal) << 1)) >> 1) ^
				(hash<glm::vec2>()(vertex.texCoord) << 1);
		}
	};
}

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

struct Mesh
{
	VkBuffer buffer;
	VkDeviceMemory buffer_memory;
	uint32_t vertex_count;
	VkDeviceSize index_offset;
	uint32_t index_count;

	uint32_t memory_size;

	union
	{
		uint64_t is_used = 0;
		uint8_t used_by_frame[8];
	};
};

struct CameraMatrices
{
	glm::mat4 view;
	glm::mat4 proj;
};

struct ModelMatrice
{
	glm::mat4 model;
};

struct ShadowMapLight
{
	glm::mat4 view;
	glm::mat4 proj;
	glm::mat4 model;
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

	uint64_t createMesh(
		const Chunk & chunk,
		const Chunk * x_pos_chunk,
		const Chunk * x_neg_chunk,
		const Chunk * y_pos_chunk,
		const Chunk * y_neg_chunk,
		const Chunk * z_pos_chunk,
		const Chunk * z_neg_chunk
	);
	uint64_t storeMesh(const std::vector<BlockVertex> & vertices, const std::vector<uint32_t> & indices);
	void	 destroyMeshes(const std::vector<uint64_t> & mesh_ids);
	void	 destroyMesh(const uint64_t & mesh_id);

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
	QueueFamilyIndices queue_family_indices;

	VkSwapchainKHR swap_chain;
	std::vector<VkImage> swap_chain_images;
	std::vector<VkImageView> swap_chain_image_views;
	VkFormat swap_chain_image_format;
	VkExtent2D swap_chain_extent;

	VkCommandPool command_pool;
	std::vector<VkCommandBuffer> draw_command_buffers;
	std::vector<VkCommandBuffer> copy_command_buffers;
	std::vector<VkCommandBuffer> imgui_command_buffers;

	std::vector<VkSemaphore> image_available_semaphores;
	std::vector<VkSemaphore> main_render_finished_semaphores;
	std::vector<VkSemaphore> copy_finished_semaphores;
	std::vector<VkSemaphore> imgui_render_finished_semaphores;
	std::vector<VkFence> in_flight_fences;
	VkFence single_time_command_fence;

	const int max_frames_in_flight = 2;
	int current_frame = 0;


	Image color_attachement;
	Image depth_attachement;
	Image block_textures;
	Image skybox_cube_map;

	// Uniform buffers for the camera matrices
	std::vector<VkBuffer> camera_uniform_buffers;
	std::vector<VkDeviceMemory> camera_uniform_buffers_memory;
	std::vector<void *> camera_uniform_buffers_mapped_memory;


	// Buffers for the line vertices and indices for the frustum
	std::vector<VkBuffer> frustum_line_buffers;
	std::vector<VkDeviceMemory> frustum_line_buffers_memory;
	std::vector<void *> frustum_line_buffers_mapped_memory;
	VkDeviceSize frustum_line_index_offset;
	uint32_t frustum_line_vertex_count;
	uint32_t frustum_line_index_count;

	Descriptor camera_descriptor;
	Descriptor block_textures_descriptor;
	Descriptor cube_map_descriptor;
	void createDescriptors();

	Pipeline chunk_pipeline;
	Pipeline line_pipeline;
	Pipeline skybox_pipeline;


	// Dear ImGui resources
	VkDescriptorPool imgui_descriptor_pool;
	ImGuiTexture imgui_texture;

	// Meshes
	uint64_t next_mesh_id = 1;
	static const uint64_t no_mesh_id = 0;
	std::unordered_map<uint64_t, Mesh> meshes;
	std::vector<uint64_t> mesh_ids_to_destroy;

	std::mutex global_mutex;


private:

	const std::vector<const char *> validation_layers = {
		"VK_LAYER_KHRONOS_validation"
	};

	std::vector<const char *> device_extensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME
	};



	void createInstance();
	bool checkValidationLayerSupport();
	std::vector<const char *> getRequiredExtensions();

	void createSurface(GLFWwindow * window);

	void setupDebugMessenger();
	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT & create_info);
	VkResult CreateDebugUtilsMessengerEXT(
		VkInstance instance,
		const VkDebugUtilsMessengerCreateInfoEXT * create_info,
		const VkAllocationCallbacks * allocator,
		VkDebugUtilsMessengerEXT * debug_messenger
	);
	void DestroyDebugUtilsMessengerEXT(
		VkInstance instance,
		VkDebugUtilsMessengerEXT debug_messenger,
		const VkAllocationCallbacks * allocator
	);
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
	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

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

	void createUniformBuffers();
	void createTextureArray(const std::vector<std::string> & file_paths, uint32_t size);
	void createCubeMap(const std::array<std::string, 6> & file_paths, uint32_t size);
	void createFrustumLineBuffers();

	void createChunkPipeline();
	void createLinePipeline();
	void createSkyboxPipeline();

	void destroyMeshes();


	void setupImgui();
	void destroyImGuiTexture(ImGuiTexture & imgui_texture);

	VkCommandBuffer beginSingleTimeCommands();
	void endSingleTimeCommands(VkCommandBuffer command_buffer);

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
