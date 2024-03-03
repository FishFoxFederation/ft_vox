#pragma once

#include "Chunk.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

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

// #define NDEBUG

#define VK_CHECK(function, message) \
	{ \
		VkResult result = function; \
		if (result != VK_SUCCESS) \
		{ \
			throw std::runtime_error(std::string(message) + " (" + std::string(string_VkResult(result)) + ")"); \
		} \
	}


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

struct BlockVertex {
	glm::ivec3 pos;
	glm::vec3 normal;
	glm::vec2 texCoord;

	static VkVertexInputBindingDescription getBindingDescription()
	{
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(BlockVertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions()
	{
		std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

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


struct Mesh
{
	VkBuffer buffer;
	VkDeviceMemory buffer_memory;
	uint32_t vertex_count;
	VkDeviceSize index_offset;
	uint32_t index_count;
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

	void recreateSwapChain(GLFWwindow * window);

	uint64_t createMesh(const Chunk & chunk);
	uint64_t storeMesh(const std::vector<BlockVertex> & vertices, const std::vector<uint32_t> & indices);

	uint64_t createImGuiTexture(const uint32_t width, const uint32_t height);


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
	std::vector<VkCommandBuffer> render_command_buffers;
	std::vector<VkCommandBuffer> copy_command_buffers;

	std::vector<VkSemaphore> image_available_semaphores;
	std::vector<VkSemaphore> render_finished_semaphores;
	std::vector<VkSemaphore> swap_chain_updated_semaphores;
	std::vector<VkSemaphore> imgui_render_finished_semaphores;
	std::vector<VkFence> in_flight_fences;
	VkFence single_time_command_fence;

	const int max_frames_in_flight = 2;
	int current_frame = 0;

	VkImage color_attachement_image;
	VkDeviceMemory color_attachement_memory;
	VkImageView color_attachement_view;
	VkFormat color_attachement_format;
	VkExtent2D color_attachement_extent;

	VkImage depth_attachement_image;
	VkDeviceMemory depth_attachement_memory;
	VkImageView depth_attachement_view;
	VkFormat depth_attachement_format;
	VkExtent2D depth_attachement_extent;

	std::vector<VkBuffer> uniform_buffers;
	std::vector<VkDeviceMemory> uniform_buffers_memory;
	std::vector<void *> uniform_buffers_mapped_memory;

	VkImage texture_image;
	VkDeviceMemory texture_image_memory;
	VkImageView texture_image_view;
	VkSampler texture_sampler;
	uint32_t texture_width;
	uint32_t texture_height;
	uint32_t mip_levels;

	VkImage textures_image;
	VkDeviceMemory textures_image_memory;
	VkImageView textures_image_view;
	VkSampler textures_sampler;
	uint32_t textures_size;
	uint32_t textures_mip_levels;

	VkDescriptorSetLayout descriptor_set_layout;
	VkDescriptorPool descriptor_pool;
	VkDescriptorSet descriptor_set;

	VkPipelineLayout pipeline_layout;
	VkPipeline graphics_pipeline;

	// Dear ImGui resources
	VkDescriptorPool imgui_descriptor_pool;
	ImGuiTexture imgui_texture;


	uint64_t next_mesh_id = 1;
	static const uint64_t no_mesh_id = 0;
	std::unordered_map<uint64_t, Mesh> meshes;

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

	void createColorResources();
	void createDepthResources();

	void createUniformBuffers();
	void createImageTexture(const std::string & file_path);
	void createTextureArray(const std::vector<std::string> & file_paths, uint32_t size);

	void createDescriptors();

	void createPipeline();
	static std::vector<char> readFile(const std::string & filename);
	VkShaderModule createShaderModule(const std::vector<char> & code);


	void setupImgui();
	void destroyImGuiTexture(ImGuiTexture & imgui_texture);

	VkCommandBuffer beginSingleTimeCommands();
	void endSingleTimeCommands(VkCommandBuffer command_buffer);

	uint32_t findMemoryType(
		uint32_t type_filter,
		VkMemoryPropertyFlags properties
	);
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
