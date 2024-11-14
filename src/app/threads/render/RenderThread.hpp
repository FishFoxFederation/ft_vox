#pragma once

#include "define.hpp"
#include "Settings.hpp"
#include "VulkanAPI.hpp"
#include "Chunk.hpp"
#include "WorldGenerator.hpp"
#include "DebugGui.hpp"

#include <chrono>
#include <map>

/**
 * @brief The push constant for the model matrix.
 */
struct ModelMatrix_push_constant
{
	glm::mat4 model;
};


/**
 * @brief A wrapper for the thread that handles rendering
 *
 * This thread will run at max framerate possible
 * Interacts with the vk::RenderAPI to draw the Scene
 * Every function called from this class MUST be thread safe
 */
class RenderThread
{

public:

	/**
	 * @brief Construct a new RenderThread object
	*/
	RenderThread(
		const Settings & settings,
		VulkanAPI & vulkanAPI,
		std::chrono::nanoseconds start_time
	);

	/**
	 * @brief Destroy the RenderThread object
	*/
	~RenderThread();

	RenderThread(RenderThread & renderer) = delete;
	RenderThread(RenderThread && renderer) = delete;
	RenderThread & operator=(RenderThread & renderer) = delete;
	RenderThread & operator=(RenderThread && renderer) = delete;

private:

	const Settings & m_settings;
	VulkanAPI & vk;

	DebugGui m_debug_gui;

	struct ChunkData
	{
		glm::vec3 position;
		uint64_t mesh_id;
	};

	std::chrono::nanoseconds m_start_time;
	std::chrono::nanoseconds m_current_time;
	std::chrono::nanoseconds m_last_frame_time;
	std::chrono::nanoseconds m_delta_time;


	// For DebugGui
	int m_frame_count;
	std::chrono::nanoseconds m_start_time_counting_fps;

	// Scene data
	int window_width, window_height;
	double aspect_ratio;

	const glm::mat4 clip = glm::mat4(
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f,-1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.5f, 0.0f,
		0.0f, 0.0f, 0.5f, 1.0f
	);
	Camera::RenderInfo camera;
	ViewProjMatrices camera_matrices = {};
	ViewProjMatrices camera_matrices_fc = {};

	std::map<VulkanAPI::InstanceId, ChunkRenderData> chunk_meshes;
	std::vector<MeshRenderData> entity_meshes;
	std::vector<PlayerRenderData> players;

	std::map<VulkanAPI::InstanceId, ChunkRenderData> visible_chunks;
	std::vector<std::map<VulkanAPI::InstanceId, ChunkRenderData>> shadow_visible_chunks;

	ViewProjMatrices sun = {};
	glm::dvec3 sun_position;

	std::optional<glm::vec3> target_block;

	AtmosphereParams atmosphere_params = {};

	std::vector<glm::mat4> light_view_proj_matrices;
	ShadowMapLight shadow_map_light = {};

	std::string debug_text;

	std::array<ItemInfo::Type, 9> toolbar_items;
	int toolbar_cursor_index = 0;


	// Should be the last member
	std::jthread m_thread;

	/**
	 * @brief function used as the entry point for the thread
	 *
	 */
	void launch();

	void init();
	void loop();

	void prepareFrame();
	void updateTime();
	void updateDebugText();
	void updateVisibleChunks();

	void shadowPass();
	void lightingPass();

	void drawPlayerBodyPart(
		const uint64_t mesh_id,
		const glm::mat4 & model
	);

	void copyToSwapchain();

	void drawDebugGui();

	std::vector<glm::mat4> getCSMLightViewProjMatrices(
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

	bool isInsideFrustum_ndcSpace(const glm::mat4 & model, const glm::vec3 & size) const;
	bool isInsideFrustum_planes(
		const glm::mat4 & view_proj,
		const glm::mat4 & model,
		const glm::vec3 & size
	) const;

};
