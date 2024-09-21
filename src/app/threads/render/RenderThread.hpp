#pragma once

#include "define.hpp"
#include "WorldScene.hpp"
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
		const WorldScene & worldScene,
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
	const WorldScene & m_world_scene;

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


	std::jthread m_thread;

	/**
	 * @brief function used as the entry point for the thread
	 *
	 */
	void launch();

	void init();
	void loop();

	void updateTime();

	void shadowPass(
		const std::vector<WorldScene::ChunkMeshRenderData> & chunk_meshes
	);

	void lightingPass(
		const Camera::RenderInfo & camera,
		const std::vector<WorldScene::ChunkMeshRenderData> & chunk_meshes,
		const std::vector<WorldScene::MeshRenderData> & entity_meshes,
		const std::vector<WorldScene::PlayerRenderData> & players,
		const std::optional<glm::vec3> & target_block,
		const std::vector<WorldScene::DebugBlock> & debug_blocks,
		const glm::dvec3 & sun_position
	);

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

};
